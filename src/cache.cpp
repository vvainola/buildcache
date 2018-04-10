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

#include "cache.hpp"

#include "debug_utils.hpp"
#include "file_utils.hpp"

#include <cstdlib>
#include <iostream>

namespace bcache {
namespace {
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
      return file::append_path(home, ".buildcache");
    }
  }

  // We failed.
  return std::string();
}

bool init_root_folder(const std::string& path) {
  auto success = true;
  if (!file::dir_exists(path)) {
    success = file::create_dir(path);
  }
  return success;
}

struct cache_entry_path_t {
  cache_entry_path_t(const std::string& _dir, const std::string& _file) : dir(_dir), file(_file) {
  }
  const std::string dir;
  const std::string file;
};

cache_entry_path_t hash_to_cache_entry_path(const hasher_t::hash_t& hash, const cache_t& cache) {
  const std::string str = hash.as_string();
  const auto full_dir_path = file::append_path(cache.root_folder(), str.substr(0, 2));
  const auto full_file_path = file::append_path(full_dir_path, str.substr(2));
  return cache_entry_path_t(full_dir_path, full_file_path);
}

bool is_cache_file(const std::string& path) {
  const auto dir_name = file::get_file_part(file::get_dir_part(path));
  const auto file_name = file::get_file_part(path, false);

  // Is the dir name 2 characters long and the file name 30 characters long?
  if ((dir_name.length() != 2) || (file_name.length() != 30)) {
    return false;
  }

  // Is the dir + file name a valid hash (i.a. all characters are hex)?
  const auto hash_str = dir_name + file_name;
  for (const auto c : hash_str) {
    if ((c < '0') || (c > 'f') || ((c > '9') && (c < 'a'))) {
      return false;
    }
  }

  return true;
}

std::vector<file::file_info_t> get_cache_files(const std::string& root_folder) {
  // Get all the files in the cache dir.
  const auto files = file::walk_directory(root_folder);

  // Return only the files that are valid cache files.
  std::vector<file::file_info_t> cache_files;
  for (const auto& file : files) {
    if ((!file.is_dir()) && is_cache_file(file.path())) {
      cache_files.push_back(file);
    }
  }

  return cache_files;
}
}  // namespace

cache_t::cache_t() {
  // Find the cache root folder.
  m_root_folder = find_root_folder();

  // Can we use the cache?
  m_is_valid = init_root_folder(m_root_folder);
}

cache_t::~cache_t() {
}

void cache_t::clear() {
  if (!m_is_valid) {
    return;
  }

  // Remove all cached files.
  const auto files = get_cache_files(m_root_folder);
  int num_files = 0;
  int64_t total_size = 0;
  for (const auto& file : files) {
    if (!file.is_dir()) {
      try {
        file::remove_file(file.path());
        num_files++;
        total_size += file.size();
      } catch (const std::exception& e) {
        debug::log(debug::ERROR) << e.what();
      }
    }
  }
  const double total_size_mb = static_cast<double>(total_size) / (1024.0 * 1024.0);

  std::cout << "Removed " << num_files << " cached files (" << total_size_mb << " MB)\n";
}

void cache_t::show_stats() {
  if (!m_is_valid) {
    return;
  }

  // Calculate the total cache size.
  const auto files = get_cache_files(m_root_folder);
  int num_files = 0;
  int64_t total_size = 0;
  for (const auto& file : files) {
    if (!file.is_dir()) {
      num_files++;
      total_size += file.size();
    }
  }
  const double total_size_mb = static_cast<double>(total_size) / (1024.0 * 1024.0);

  std::cout << "cache directory:   " << m_root_folder << "\n";
  std::cout << "files in cache:    " << num_files << "\n";
  std::cout << "cache size:        " << total_size_mb << " MB\n";

  // TODO(m): Implement more stats.
}

void cache_t::add(const hasher_t::hash_t& hash, const std::string& object_file) {
  if (!m_is_valid) {
    return;
  }

  // Create an entry in the cache.
  const auto cache_entry_path = hash_to_cache_entry_path(hash, *this);
  if (!file::dir_exists(cache_entry_path.dir)) {
    file::create_dir(cache_entry_path.dir);
  }
  file::link_or_copy(object_file, cache_entry_path.file);
}

std::string cache_t::lookup(const hasher_t::hash_t& hash) {
  if (!m_is_valid) {
    return std::string();
  }

  const auto cache_entry_path = hash_to_cache_entry_path(hash, *this);
  return file::file_exists(cache_entry_path.file) ? cache_entry_path.file : std::string();
}

}  // namespace bcache
