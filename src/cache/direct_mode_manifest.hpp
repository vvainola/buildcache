//--------------------------------------------------------------------------------------------------
// Copyright (c) 2021 Marcus Geelnard
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

#ifndef BUILDCACHE_DIRECT_MODE_MANIFEST_HPP_
#define BUILDCACHE_DIRECT_MODE_MANIFEST_HPP_

#include <map>
#include <string>

namespace bcache {
/// @brief A direct mode cache manifest.
class direct_mode_manifest_t {
public:
  /// @brief Construct an empty/invalid manifest.
  direct_mode_manifest_t();

  /// @brief Construct a valid manifest.
  /// @param hash The preprocessor mode cache entry hash.
  /// @param files_with_hashes Paths to implicit input files (key) and their hashes (value).
  direct_mode_manifest_t(const std::string& hash,
                         const std::map<std::string, std::string>& files_with_hashes);

  /// @returns true if this object represents a valid manifest. E.g. for a cache miss, the
  /// return value is false.
  operator bool() const {
    return m_valid;
  }

  /// @brief Serialize a manifest.
  /// @returns a serialized data as a string object.
  std::string serialize() const;

  /// @brief Deserialize a manifest.
  /// @param data The serialized data.
  /// @returns the deserialized manifest.
  static direct_mode_manifest_t deserialize(const std::string& data);

  /// @returns the preprocessor mode cache entry hash.
  const std::string& hash() const {
    return m_hash;
  }

  /// @returns a mapping from file paths to their hashes.
  const std::map<std::string, std::string>& files_width_hashes() const {
    return m_files_width_hashes;
  }

private:
  std::string m_hash;
  std::map<std::string, std::string> m_files_width_hashes;
  bool m_valid = false;  // true if this is a valid manifest.
};
}  // namespace bcache

#endif  // BUILDCACHE_DIRECT_MODE_MANIFEST_HPP_
