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

  // TODO(m): Implement me!
  std::cout << "*** Clearing the cache has not yet been implemented\n";
}

void cache_t::show_stats() {
  if (!m_is_valid) {
    return;
  }

  // TODO(m): Implement me!
  std::cout << "*** Stats have not yet been implemented\n";
}

void cache_t::add(const hasher_t::hash_t& hash, const std::string& object_file) {
  if (!m_is_valid) {
    return;
  }

  // TODO(m): Implement me!
  (void)hash;
  (void)object_file;
}

std::string cache_t::lookup(const hasher_t::hash_t& hash) {
  if (!m_is_valid) {
    return std::string();
  }

  // TODO(m): Implement me!
  (void)hash;
  return std::string();
}

}  // namespace bcache
