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

#ifndef BUILDCACHE_CACHE_HPP_
#define BUILDCACHE_CACHE_HPP_

#include "hasher.hpp"

#include <string>

namespace bcache {

class cache_t {
public:
  cache_t();
  ~cache_t();

  /// @brief Check if the cache is valid to use at all.
  /// @returns true if it is safe to use the cache.
  bool is_valid() const {
    return m_is_valid;
  }

  /// @brief Clear all entries in the cache.
  void clear();

  /// @brief Show cache statistics (print to standard out).
  void show_stats();

  /// @brief Adds a file to the cache
  /// @param hash The cache entry identifier.
  /// @param source_file The file to copy into the cache.
  void add(const hasher_t::hash_t& hash, const std::string& source_file);

  /// @brief Check if an entry exists in the cache.
  /// @returns The file name base (without extension) to the cache entry, or en empty string if the
  /// entry did not exist in the cache.
  std::string lookup(const hasher_t::hash_t& hash);

private:
  std::string m_root_folder;
  bool m_is_valid;
};

}  // namespace bcache

#endif  // BUILDCACHE_CACHE_HPP_
