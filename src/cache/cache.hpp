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

#include <map>
#include <string>

namespace bcache {

class cache_t {
public:
  struct entry_t {
    enum class comp_mode_t {
      NONE = 0,
      ALL = 1
    };

    /// @returns true if this object represents a valid cache entry. For a cache miss, the return
    /// value is false.
    operator bool() const {
      return (!files.empty()) || (!std_out.empty()) || (!std_err.empty());
    }

    // TODO(m): The source file paths are only used during addition to the cache, so they can be
    // kept in a separate map and do not have to be stored in the cache entry. I.e. we only need to
    // store the file ID:s (e.g. "object") in the cache entry.
    std::map<std::string, std::string> files;          ///< ID:s and paths of the cached files.
    comp_mode_t compression_mode = comp_mode_t::NONE;  ///< Compression mode.
    std::string std_out;                               ///< stdout from the program run.
    std::string std_err;                               ///< stderr from the program run.
    int return_code = 0;                               ///< Program return code (0 = success).
  };

  /// @brief Serialize a cache entry.
  /// @param entry The cache entry.
  /// @returns a serialized data as a string object.
  static std::string serialize_entry(const entry_t& entry);

  /// @brief Deserialize a cache entry.
  /// @param data The serialized data.
  /// @returns the deserialized cache entry.
  static entry_t deserialize_entry(const std::string& data);
};

}  // namespace bcache

#endif  // BUILDCACHE_CACHE_HPP_
