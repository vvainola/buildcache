//--------------------------------------------------------------------------------------------------
// Copyright (c) 2019 Marcus Geelnard
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

#ifndef BUILDCACHE_LOCK_FILE_HPP_
#define BUILDCACHE_LOCK_FILE_HPP_

#include <string>

namespace bcache {
namespace file {
/// @brief A scoped exclusive lock file class.
///
/// When the locked file object is created, a lock file is created (if necessary) and acquired. Once
/// the object goes out of scope, it releases the lock and deletes the file if this is the last
/// process that releases the lock.
class lock_file_t {
public:
  /// @brief Create an empty (unlocked) file lock object.
  lock_file_t() {
  }

  /// @brief Acquire a lock using the specified file path.
  /// @param path The full path to the lock file (will be created).
  explicit lock_file_t(const std::string& path);

  // Support move semantics.
  lock_file_t(lock_file_t&& other) noexcept;
  lock_file_t& operator=(lock_file_t&& other) noexcept;
  lock_file_t(const lock_file_t& other) = delete;
  lock_file_t& operator=(const lock_file_t& other) = delete;

  /// @brief Release the lock.
  ///
  /// This releases the lock that was held on the file (if any), and deletes the lock file (if this
  /// is the last process that holds a lock on the file).
  ~lock_file_t();

  /// @returns true if the lock was acquired successfully.
  bool has_lock() const {
    return m_has_lock;
  }

private:
#if defined(_WIN32)
  // WIN32 API file handle type: HANDLE (i.e. void*).
  using handle_t = void*;
#else
  // POSIX file descriptor type: int.
  using handle_t = int;
#endif

  static handle_t invalid_handle();

  std::string m_path;
  handle_t m_handle = invalid_handle();
  bool m_has_lock = false;
};

}  // namespace file
}  // namespace bcache

#endif  // BUILDCACHE_LOCK_FILE_HPP_
