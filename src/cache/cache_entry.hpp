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

#ifndef BUILDCACHE_CACHE_ENTRY_HPP_
#define BUILDCACHE_CACHE_ENTRY_HPP_

#include <map>
#include <string>

namespace bcache {
/// @brief Meta data for a single cache entry.
class cache_entry_t {
public:
  /// @brief Compression mode.
  enum class comp_mode_t {
    NONE = 0,  ///< Compress nothing (i.e. the entry is uncompressed in the cache).
    ALL = 1    ///< Compress all files and stdout + stderr.
  };

  /// @brief Construct an empty/invalid cache entry.
  cache_entry_t();

  /// @brief Construct a valid cache entry.
  /// @param files ID:s and paths of the cached files.
  /// @param compression_mode Compression mode.
  /// @param std_out stdout from the program run.
  /// @param std_err stderr from the program run.
  /// @param return_code Program return code (0 = success).
  cache_entry_t(const std::map<std::string, std::string>& files,
                const comp_mode_t compression_mode,
                const std::string& std_out,
                const std::string& std_err,
                const int return_code);

  /// @returns true if this object represents a valid cache entry. E.g. for a cache miss, the
  /// return value is false.
  operator bool() const {
    return (!m_files.empty()) || (!m_std_out.empty()) || (!m_std_err.empty());
  }

  /// @brief Serialize a cache entry.
  /// @returns a serialized data as a string object.
  std::string serialize() const;

  /// @brief Deserialize a cache entry.
  /// @param data The serialized data.
  /// @returns the deserialized cache entry.
  static cache_entry_t deserialize(const std::string& data);

  /// @returns the ID:s and paths of the cached files.
  const std::map<std::string, std::string>& files() const {
    return m_files;
  }

  /// @returns the compression mode.
  comp_mode_t compression_mode() const {
    return m_compression_mode;
  }

  /// @returns the stdout from the program run.
  const std::string& std_out() const {
    return m_std_out;
  }

  /// @returns the stderr from the program run.
  const std::string& std_err() const {
    return m_std_err;
  }

  /// @returns the program return code (0 = success).
  int return_code() const {
    return m_return_code;
  }

private:
  // TODO(m): The source file paths are only used during addition to the cache, so they can be
  // kept in a separate map and do not have to be stored in the cache entry. I.e. we only need to
  // store the file ID:s (e.g. "object") in the cache entry.
  std::map<std::string, std::string> m_files;
  comp_mode_t m_compression_mode = comp_mode_t::NONE;
  std::string m_std_out;
  std::string m_std_err;
  int m_return_code = 0;
};
}  // namespace bcache

#endif  // BUILDCACHE_CACHE_ENTRY_HPP_
