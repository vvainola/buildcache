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

#include "file_utils.hpp"
#include "hasher.hpp"

#include <map>
#include <string>
#include <vector>

namespace bcache {

class cache_t {
public:
  struct entry_t {
    /// @returns true if this object represents a valid cache entry. For a cache miss, the return
    /// value is false.
    operator bool() const {
      return (!files.empty()) || (!std_out.empty()) || (!std_err.empty());
    }

    std::map<std::string, std::string> files;
    std::string std_out;
    std::string std_err;
    int return_code = 0;
  };

  /// @brief Initialize the cache object.
  /// @throws runtime_error if the cache could not be initialized.
  cache_t();

  /// @brief De-initialzie the cache object.
  ~cache_t();

  /// @brief Clear all entries in the cache.
  void clear();

  /// @brief Show cache statistics (print to standard out).
  void show_stats();

  /// @brief Adds a set of files to the cache
  /// @param hash The cache entry identifier.
  /// @param entry The cache entry data (files, stdout, etc).
  /// @param allow_hard_links Whether or not to allow hard links to be used when caching files.
  void add(const hasher_t::hash_t& hash, const entry_t& entry, const bool allow_hard_links);

  /// @brief Check if an entry exists in the cache.
  /// @returns A cache hit struct.
  entry_t lookup(const hasher_t::hash_t& hash);

  /// @brief Get a temporary file.
  /// @param extension File extension (including the period).
  /// @returns a temporary file object with a unique path.
  file::tmp_file_t get_temp_file(const std::string& extension) const;

private:
  const std::string hash_to_cache_entry_path(const hasher_t::hash_t& hash) const;
  const std::string get_tmp_folder() const;
  const std::string get_cache_files_folder() const;

  void perform_housekeeping();
};

}  // namespace bcache

#endif  // BUILDCACHE_CACHE_HPP_
