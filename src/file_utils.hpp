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

///@{
/// @brief Append two paths.
/// @param path The base path.
/// @param append The path to be appended (e.g. a file name).
/// @returns the concatenated paths, using the system path separator.
std::string append_path(const std::string& path, const std::string& append);
std::string append_path(const std::string& path, const char* append);
///@}

/// @brief Get the file extension of a path.
/// @param path The path to a file.
/// @returns The file extension of the file (including the leading period), or an empty string if
/// the file does not have an extension.
std::string get_extension(const std::string& path);

/// @brief Get the file name part of a path.
/// @param path The path to a file.
/// @returns The part of the path after the final path separator (including any extension). If the
/// path does not contain a separator, the entire path is returned.
std::string get_file_part(const std::string& path);

/// @brief Get the user home directory.
/// @returns the full path to the user home directory.
std::string get_user_home_dir();

/// @brief Resolve a path.
///
/// Relative paths are converted into absolute paths, and symbolic links are resolved.
/// @param path The path to resolve.
/// @returns an absolute path to a regular file, or an empty string if the path could not be
/// resolved.
std::string resolve_path(const std::string& path);

/// @brief Find the true path to an executable file.
/// @param path The file to find.
/// @returns an absolute path to the true executable file (symlinks resolved and all), or an empty
/// string if the file could not be found.
std::string find_executable(const std::string& path);

/// @brief Create a directory.
/// @param path The path to the directory.
/// @returns true if the directory was successfully created.
bool create_dir(const std::string& path);

/// @brief Remove an existing file.
/// @param path The path to the file.
void remove_file(const std::string& path);

/// @brief Check if a directory exists.
/// @param path The path to the directory.
/// @returns true if the directory exists.
bool dir_exists(const std::string& path);

/// @brief Check if a file exists.
/// @param path The path to the file.
/// @returns true if the file exists.
bool file_exists(const std::string& path);

/// @brief Make a hard link or a full copy of a file.
/// @param from_path The source file.
/// @param to_path The destination file.
/// @returns true if the operation was successful.
bool link_or_copy(const std::string& from_path, const std::string& to_path);

/// @brief Read a file into a string.
/// @param path The path to the file.
/// @returns the contents of the file as a string. If the file could not be read, an empty string is
/// returned.
std::string read(const std::string& path);
}  // namespace file
}  // namespace bcache

#endif  // BUILDCACHE_FILE_UTILS_HPP_
