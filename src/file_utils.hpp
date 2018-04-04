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

#ifndef BUILDCACHE_FILE_UTILS_HPP_
#define BUILDCACHE_FILE_UTILS_HPP_

#include <string>

namespace bcache {
namespace file {
/// @brief A helper class for handling temporary files.
///
/// When the temp file object is created, a temporary file name is generated. Once the object goes
/// out of scope, it removes the file from disk.
class tmp_file_t {
public:
  /// @brief Construct a temporary file name.
  /// @param dir The base directory in which the temporary file will be located.
  /// @param extension The file name extension (including the leading dot).
  tmp_file_t(const std::string& dir, const std::string& extension);

  /// @brief Remove the temporary file (if any).
  ~tmp_file_t();

  const std::string& path() const {
    return m_path;
  }

private:
  std::string m_path;
};

std::string append_path(const std::string& path, const std::string& append);
std::string append_path(const std::string& path, const char* append);

std::string get_extension(const std::string& path);
std::string get_file_part(const std::string& path);

std::string get_user_home_dir();

bool create_dir(const std::string& path);

void remove(const std::string& path);

bool dir_exists(const std::string& path);

bool file_exists(const std::string& path);

std::string read(const std::string& path);
}  // namespace file
}  // namespace bcache

#endif  // BUILDCACHE_FILE_UTILS_HPP_
