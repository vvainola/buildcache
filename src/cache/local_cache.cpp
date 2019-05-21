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

#include <cache/local_cache.hpp>

#include <base/compressor.hpp>
#include <base/debug_utils.hpp>
#include <base/file_utils.hpp>
#include <base/serializer_utils.hpp>
#include <config/configuration.hpp>

#include <chrono>
#include <cstdlib>
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <stdexcept>

namespace bcache {
namespace {
const std::string CACHE_FILES_FOLDER_NAME = "c";
const std::string CACHE_ENTRY_FILE_NAME = ".entry";
const std::string LOCK_FILE_SUFFIX = ".lock";

// The version of the entry file serialization data format.
const int32_t ENTRY_DATA_FORMAT_VERSION = 2;

std::string cache_entry_lock_file_path(const std::string& cache_entry_path) {
  return cache_entry_path + LOCK_FILE_SUFFIX;
}

bool is_cache_entry_dir_path(const std::string& path) {
  const auto entry_dir_name = file::get_file_part(path);
  const auto hash_prefix_dir_name = file::get_file_part(file::get_dir_part(path));

  // Is the hash prefix dir name 2 characters long and the endtry dir name 30 characters long?
  if ((hash_prefix_dir_name.length() != 2) || (entry_dir_name.length() != 30)) {
    return false;
  }

  // Is the hash prefix dir + entry dir name a valid hash (i.a. all characters are hex)?
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
    const auto cache_files_dir = file::append_path(root_folder, CACHE_FILES_FOLDER_NAME);
    if (file::dir_exists(cache_files_dir)) {
      // Get all the files in the cache dir.
      const auto files = file::walk_directory(cache_files_dir);

      // Return only the directories that are valid cache entries.
      for (const auto& file : files) {
        if (file.is_dir() && is_cache_entry_dir_path(file.path())) {
          cache_dirs.push_back(file);
        }
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
}  // namespace

local_cache_t::local_cache_t() {
  file::create_dir_with_parents(config::dir());
}

local_cache_t::~local_cache_t() {
}

const std::string local_cache_t::get_cache_files_folder() const {
  const auto cache_files_path = file::append_path(config::dir(), CACHE_FILES_FOLDER_NAME);
  file::create_dir_with_parents(cache_files_path);
  return cache_files_path;
}

const std::string local_cache_t::hash_to_cache_entry_path(const hasher_t::hash_t& hash) const {
  const std::string str = hash.as_string();
  const auto parent_dir_path = file::append_path(get_cache_files_folder(), str.substr(0, 2));
  return file::append_path(parent_dir_path, str.substr(2));
}

void local_cache_t::clear() {
  const auto start_t = std::chrono::high_resolution_clock::now();

  // Remove all cache entries.
  auto dirs = get_cache_entry_dirs(config::dir());
  for (const auto& dir : dirs) {
    try {
      // We acquire an exclusive lock for the cache entry before deleting it.
      file::lock_file_t lock(cache_entry_lock_file_path(dir.path()));
      if (lock.has_lock()) {
        file::remove_dir(dir.path());
      }
    } catch (const std::exception& e) {
      debug::log(debug::DEBUG) << "Failed: " << e.what();
    }
  }

  const auto stop_t = std::chrono::high_resolution_clock::now();
  const auto dt = std::chrono::duration_cast<std::chrono::milliseconds>(stop_t - start_t).count();
  std::cout << "Cleared the cache in " << dt << " ms\n";
}

void local_cache_t::show_stats() {
  // Calculate the total cache size.
  const auto dirs = get_cache_entry_dirs(config::dir());
  int num_entries = 0;
  int64_t total_size = 0;
  for (const auto& dir : dirs) {
    num_entries++;
    total_size += dir.size();
  }
  const auto full_percentage =
      100.0 * static_cast<double>(total_size) / static_cast<double>(config::max_cache_size());

  // Print stats.
  std::ios old_fmt(nullptr);
  old_fmt.copyfmt(std::cout);
  std::cout << std::setiosflags(std::ios::fixed) << std::setprecision(1);
  std::cout << "  Entries in cache:  " << num_entries << "\n";
  std::cout << "  Cache size:        " << file::human_readable_size(total_size) << " ("
            << full_percentage << "%)\n";
  std::cout.copyfmt(old_fmt);
}

void local_cache_t::add(const hasher_t::hash_t& hash,
                        const cache_entry_t& entry,
                        const std::map<std::string, std::string>& file_paths,
                        const bool allow_hard_links) {
  // Create the cache entry parent directory if necessary.
  const auto cache_entry_path = hash_to_cache_entry_path(hash);
  const auto cache_entry_parent_path = file::get_dir_part(cache_entry_path);
  file::create_dir_with_parents(cache_entry_parent_path);

  {
    // Acquire a scoped exclusive lock for the cache entry.
    file::lock_file_t lock(cache_entry_lock_file_path(cache_entry_path));
    if (!lock.has_lock()) {
      throw std::runtime_error("Unable to acquire a cache entry lock for writing.");
    }

    // Create the cache entry directory.
    file::create_dir_with_parents(cache_entry_path);

    // Copy (and optinally compress) the files into the cache.
    for (const auto& file_id : entry.file_ids()) {
      const auto& source_path = file_paths.at(file_id);
      const auto target_path = file::append_path(cache_entry_path, file_id);
      if (entry.compression_mode() == cache_entry_t::comp_mode_t::ALL) {
        debug::log(debug::DEBUG) << "Compressing " << source_path << " => " << target_path;
        comp::compress_file(source_path, target_path);
      } else if (allow_hard_links) {
        file::link_or_copy(source_path, target_path);
      } else {
        file::copy(source_path, target_path);
      }
    }

    // Create a cache entry file.
    const auto cache_entry_file_name = file::append_path(cache_entry_path, CACHE_ENTRY_FILE_NAME);
    file::write(entry.serialize(), cache_entry_file_name);
  }

  // Occassionally perform housekeeping. We do it here, since:
  //  1) This is the only place where the cache should ever grow.
  //  2) Cache misses are slow anyway.
  if (is_time_for_housekeeping()) {
    perform_housekeeping();
  }
}

std::pair<cache_entry_t, file::lock_file_t> local_cache_t::lookup(const hasher_t::hash_t& hash) {
  // Get the path to the cache entry.
  const auto cache_entry_path = hash_to_cache_entry_path(hash);

  try {
    // If the cache parent dir does not exist yet, we can't possibly have a cache hit.
    // Note: This is mostly a trick to avoid irrelevant error log printouts from the lock_file_t
    // constructor later on.
    const auto cache_entry_parent_path = file::get_dir_part(cache_entry_path);
    if (!file::dir_exists(cache_entry_parent_path)) {
      throw std::runtime_error("Cache entry parent dir does not exist.");
    }

    // Acquire a scoped lock for the cache entry.
    file::lock_file_t lock(cache_entry_lock_file_path(cache_entry_path));
    if (!lock.has_lock()) {
      throw std::runtime_error("Unable to acquire a cache entry lock for reading.");
    }

    // Read the cache entry file (this will throw if the file does not exist - i.e. if we have a
    // cache miss).
    const auto cache_entry_file_name = file::append_path(cache_entry_path, CACHE_ENTRY_FILE_NAME);
    const auto entry_data = file::read(cache_entry_file_name);
    return std::make_pair(cache_entry_t::deserialize(entry_data), std::move(lock));
  } catch (...) {
    return std::make_pair(cache_entry_t(), file::lock_file_t());
  }
}

void local_cache_t::get_file(const hasher_t::hash_t& hash,
                             const std::string& source_id,
                             const std::string& target_path,
                             const bool is_compressed,
                             const bool allow_hard_links) {
  const auto cache_entry_path = hash_to_cache_entry_path(hash);
  const auto source_path = file::append_path(cache_entry_path, source_id);
  if (is_compressed) {
    debug::log(debug::DEBUG) << "Decompressing file from cache";
    comp::decompress_file(source_path, target_path);
  } else if (allow_hard_links) {
    file::link_or_copy(source_path, target_path);
  } else {
    file::copy(source_path, target_path);
  }
}

void local_cache_t::perform_housekeeping() {
  const auto start_t = std::chrono::high_resolution_clock::now();

  debug::log(debug::INFO) << "Performing housekeeping.";

  // Get all the cache entry directories.
  auto dirs = get_cache_entry_dirs(config::dir());

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
    if (total_size > config::max_cache_size()) {
      try {
        debug::log(debug::DEBUG) << "Purging " << dir.path() << " (last accessed "
                                 << dir.access_time() << ", " << dir.size() << " bytes)";

        // We acquire a scoped lock for the cache entry before deleting it.
        file::lock_file_t lock(cache_entry_lock_file_path(dir.path()));
        if (lock.has_lock()) {
          file::remove_dir(dir.path());
          total_size -= dir.size();
          num_entries--;
        }
      } catch (const std::exception& e) {
        debug::log(debug::DEBUG) << "Failed: " << e.what();
      }
    }
  }

  const auto stop_t = std::chrono::high_resolution_clock::now();
  const auto dt = std::chrono::duration_cast<std::chrono::milliseconds>(stop_t - start_t).count();
  debug::log(debug::INFO) << "Finished housekeeping in " << dt << " ms";
}

}  // namespace bcache
