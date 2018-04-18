//--------------------------------------------------------------------------------------------------
// Copyright (c) 2018 Marcus Geelnard
//
// This software is provided 'as-is', without any express or implied warranty. In no event will the
// authors be held liable for any damages arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose, including commercial
// applications, and to alter it and redistribute it freely, subject to the following restrictions:
//
//  1. The origin of this software must not be misrepresented; you must not claim that you wrote
//     the original software. If you use this software in a product, an acknowledgment in the
//     product documentation would be appreciated but is not required.
//
//  2. Altered source versions must be plainly marked as such, and must not be misrepresented as
//     being the original software.
//
//  3. This notice may not be removed or altered from any source distribution.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// The cache directory layout is as follows:
//
//  [root_folder]                             (default: $HOME/.buildcache)
//  |
//  +- buildcache.conf                        (BuildCache configuration file)
//  |
//  +- tmp                                    (temporary files)
//  |  |
//  |  +- ...
//  |
//  +- c                                      (cache files)
//     |
//     +- 9e                                  (first 2 chars of hash)
//     |  |
//     |  +- 8967a0708e7876df765864531bcd3f   (last 30 chars of hash)
//     |  |  |
//     |  |  +- .entry                        (information about this cache entry)
//     |  |  +- somefile                      (a cached file)
//     |  |  +- yetanotherfile                (a cached file)
//     |  |  +- ...
//     |  |
//     |  +- ...
//     |
//     +- ...
//--------------------------------------------------------------------------------------------------

#include "cache.hpp"

#include "debug_utils.hpp"
#include "file_utils.hpp"
#include "serializer_utils.hpp"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <stdexcept>

namespace bcache {
namespace {
const int64_t DEFAULT_MAX_CACHE_SIZE_IN_BYTES = 5368709120L;  // 5 GB

const std::string ROOT_FOLDER_NAME = ".buildcache";
const std::string TEMP_FOLDER_NAME = "tmp";
const std::string CACHE_FILES_FOLDER_NAME = "c";

const std::string CONFIGURATION_FILE_NAME = "buildcache.conf";
const std::string CACHE_ENTRY_FILE_NAME = ".entry";

// The version of the entry file serialization data format.
const int32_t ENTRY_DATA_FORMAT_VERSION = 1;

std::string find_root_folder() {
  // Is the environment variable BUILDCACHE_DIR defined?
  {
    const auto* buildcache_dir_env = std::getenv("BUILDCACHE_DIR");
    if (buildcache_dir_env != nullptr) {
      return std::string(buildcache_dir_env);
    }
  }

  // Use the user home directory if possible.
  {
    auto home = file::get_user_home_dir();
    if (!home.empty()) {
      // TODO(m): Should use ".cache/buildcache".
      return file::append_path(home, ROOT_FOLDER_NAME);
    }
  }

  // We failed.
  throw std::runtime_error("Unable to determine a home directory for BuildCache.");
}

bool is_cache_entry_dir_path(const std::string& path) {
  const auto entry_dir_name = file::get_file_part(path);
  const auto hash_prefix_dir_name = file::get_file_part(file::get_dir_part(path));

  // Is the hash prefix dir name 2 characters long and the endtry dir name 30 characters long?
  if ((hash_prefix_dir_name.length() != 2) || (entry_dir_name.length() != 30)) {
    return false;
  }

  // Is the hash prefix dir + endtry dir name a valid hash (i.a. all characters are hex)?
  const auto hash_str = hash_prefix_dir_name + entry_dir_name;
  for (const auto c : hash_str) {
    if ((c < '0') || (c > 'f') || ((c > '9') && (c < 'a'))) {
      return false;
    }
  }

  return true;
}

std::vector<file::file_info_t> get_cache_entry_dirs(const std::string& root_folder) {
  std::vector<file::file_info_t> cache_dirs;

  try {
    // Get all the files in the cache dir.
    const auto files =
        file::walk_directory(file::append_path(root_folder, CACHE_FILES_FOLDER_NAME));

    // Return only the directories that are valid cache entries.
    for (const auto& file : files) {
      if (file.is_dir() && is_cache_entry_dir_path(file.path())) {
        cache_dirs.push_back(file);
      }
    }
  } catch (const std::exception& e) {
    debug::log(debug::ERROR) << e.what();
  }

  return cache_dirs;
}

bool is_time_for_housekeeping() {
  // Get the time since the epoch, in microseconds.
  const auto t =
      std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::system_clock::now())
          .time_since_epoch()
          .count();

  // Scramble the timestamp (to make up for possible problems with timer accuracy).
  const auto rnd = (t ^ (t >> 7)) ^ ((t >> 14) ^ (t >> 20));

  // Only perform housekeeping 1% of the times that we are called.
  return (rnd % 100L) == 0L;
}

std::string serialize_entry(const cache_t::entry_t& entry) {
  std::string data = serialize::from_int(ENTRY_DATA_FORMAT_VERSION);
  data += serialize::from_map(entry.files);
  data += serialize::from_string(entry.std_out);
  data += serialize::from_string(entry.std_err);
  data += serialize::from_int(static_cast<int32_t>(entry.return_code));
  return data;
}

cache_t::entry_t deserialize_entry(const std::string& data) {
  std::string::size_type pos = 0;

  // Read and check the format version.
  int32_t format_version = serialize::to_int(data, pos);
  if (format_version != ENTRY_DATA_FORMAT_VERSION) {
    throw std::runtime_error("Unsupported serialization format version.");
  }

  // De-serialize the entry.
  cache_t::entry_t entry;
  entry.files = serialize::to_map(data, pos);
  entry.std_out = serialize::to_string(data, pos);
  entry.std_err = serialize::to_string(data, pos);
  entry.return_code = static_cast<int>(serialize::to_int(data, pos));

  return entry;
}
}  // namespace

cache_t::cache_t() {
  // Find the cache root folder.
  m_root_folder = find_root_folder();

  // Can we use the cache?
  if (!file::dir_exists(m_root_folder)) {
    file::create_dir(m_root_folder);
  }

  load_config();
}

cache_t::~cache_t() {
}

const std::string cache_t::get_tmp_folder() const {
  const auto tmp_path = file::append_path(m_root_folder, TEMP_FOLDER_NAME);
  if (!file::dir_exists(tmp_path)) {
    file::create_dir(tmp_path);
  }
  return tmp_path;
}

const std::string cache_t::get_cache_files_folder() const {
  const auto cache_files_path = file::append_path(m_root_folder, CACHE_FILES_FOLDER_NAME);
  if (!file::dir_exists(cache_files_path)) {
    file::create_dir(cache_files_path);
  }
  return cache_files_path;
}

const std::string cache_t::hash_to_cache_entry_path(const hasher_t::hash_t& hash) const {
  const std::string str = hash.as_string();
  const auto parent_dir_path = file::append_path(get_cache_files_folder(), str.substr(0, 2));
  return file::append_path(parent_dir_path, str.substr(2));
}

void cache_t::clear() {
  // Remove all cached files.
  const auto cache_files_path = file::append_path(m_root_folder, CACHE_FILES_FOLDER_NAME);
  try {
    file::remove_dir(cache_files_path);
  } catch (const std::exception& e) {
    debug::log(debug::ERROR) << e.what();
  }
  std::cout << "Cleared the cache.\n";
}

void cache_t::show_stats() {
  // Calculate the total cache size.
  const auto dirs = get_cache_entry_dirs(m_root_folder);
  int num_entries = 0;
  int64_t total_size = 0;
  for (const auto& dir : dirs) {
    num_entries++;
    total_size += dir.size();
  }
  const double total_size_mb = static_cast<double>(total_size) / (1024.0 * 1024.0);
  const double max_size_mb = static_cast<double>(m_max_size) / (1024.0 * 1024.0);

  std::cout << "cache directory:   " << m_root_folder << "\n";
  // std::cout << "primary config:    " << get_configuration_file_name() << "\n";
  std::cout << "entries in cache:  " << num_entries << "\n";
  std::cout << "cache size:        " << total_size_mb << " MB\n";
  std::cout << "max cache size:    " << max_size_mb << " MB\n";

  // TODO(m): Implement more stats.
}

void cache_t::add(const hasher_t::hash_t& hash, const cache_t::entry_t& entry) {
  // Create the required directories in the cache.
  const auto cache_entry_path = hash_to_cache_entry_path(hash);
  const auto cache_entry_parent_path = file::get_dir_part(cache_entry_path);
  if (!file::dir_exists(cache_entry_parent_path)) {
    file::create_dir(cache_entry_parent_path);
  }
  if (!file::dir_exists(cache_entry_path)) {
    file::create_dir(cache_entry_path);
  }

  // Copy the files into the cache.
  for (const auto& file : entry.files) {
    const auto target_path = file::append_path(cache_entry_path, file.first);
    file::link_or_copy(file.second, target_path);
  }

  // Create a cache entry file.
  const auto cache_entry_file_name = file::append_path(cache_entry_path, CACHE_ENTRY_FILE_NAME);
  file::write(serialize_entry(entry), cache_entry_file_name);

  // Occassionally perform housekeeping. We do it here, since:
  //  1) This is the only place where the cache should ever grow.
  //  2) Cache misses are slow anyway.
  if (is_time_for_housekeeping()) {
    perform_housekeeping();
  }
}

cache_t::entry_t cache_t::lookup(const hasher_t::hash_t& hash) {
  // Get the path to the cache entry.
  const auto cache_entry_path = hash_to_cache_entry_path(hash);
  const auto cache_entry_file_name = file::append_path(cache_entry_path, CACHE_ENTRY_FILE_NAME);

  try {
    // Read the cache entry file (this will throw if the file does not exist - i.e. if we have a
    // cache miss).
    const auto entry_data = file::read(cache_entry_file_name);
    auto entry = deserialize_entry(entry_data);

    // Update the file names member of the entry, and check that the files exist.
    for (auto& file : entry.files) {
      const auto file_name = file.first;
      const auto file_path = file::append_path(cache_entry_path, file_name);
      if (!file::file_exists(file_path)) {
        throw std::runtime_error("Missing file in cache entry.");
      }
      entry.files[file_name] = file_path;
    }

    return entry;
  } catch (...) {
    return entry_t();
  }
}

void cache_t::load_config() {
  // Set default values.
  m_max_size = DEFAULT_MAX_CACHE_SIZE_IN_BYTES;

  // TODO(m): Load settings from a config file!
}

void cache_t::save_config() {
  // TODO(m): Save settings to a config file!
}

void cache_t::perform_housekeeping() {
  const auto start_t = std::chrono::high_resolution_clock::now();

  debug::log(debug::INFO) << "Performing housekeeping.";

  // Get all the cache entry directories.
  auto dirs = get_cache_entry_dirs(m_root_folder);

  // Sort the entries according to their access time (newest first).
  std::sort(
      dirs.begin(), dirs.end(), [](const file::file_info_t& a, const file::file_info_t& b) -> bool {
        return a.access_time() > b.access_time();
      });

  // Remove old cache files and directories in order to keep the cache size under the configured
  // limit.
  int num_entries = 0;
  int64_t total_size = 0;
  for (const auto& dir : dirs) {
    num_entries++;
    total_size += dir.size();
    if (total_size > m_max_size) {
      try {
        debug::log(debug::DEBUG) << "Purging " << dir.path() << " (last accessed "
                                 << dir.access_time() << ", " << dir.size() << " bytes)";
        file::remove_dir(dir.path());
        total_size -= dir.size();
        num_entries--;
      } catch (const std::exception& e) {
        debug::log(debug::DEBUG) << "Failed: " << e.what();
      }
    }
  }

  const auto stop_t = std::chrono::high_resolution_clock::now();
  const auto dt = std::chrono::duration_cast<std::chrono::milliseconds>(stop_t - start_t).count();
  debug::log(debug::INFO) << "Finished housekeeping in " << dt << " ms";
}

file::tmp_file_t cache_t::get_temp_file(const std::string& extension) const {
  return file::tmp_file_t(get_tmp_folder(), extension);
}

}  // namespace bcache
