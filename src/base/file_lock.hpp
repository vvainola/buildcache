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

#ifndef BUILDCACHE_FILE_LOCK_HPP_
#define BUILDCACHE_FILE_LOCK_HPP_

#include <string>

namespace bcache {
namespace file {
/// @brief A scoped exclusive global lock class.
///
/// This lock class is intended to be used for granular synchronization of multiple processes that
/// need to access a specific part of a file system, such as a single file or a folder. Locks are
/// expected to be held for a very short time period (typically only for a fraction of a second),
/// during operations such as file renames or writes.
///
/// When the lock is created, a global named system object is created (if necessary) and acquired.
/// Once the lock goes out of scope, the system object is released. If this is the last process that
/// releases the lock, the system object is deleted.
///
/// Two system object types are supported:
///
/// \par Remote locks
/// A remote lock is implemented as a lock file that will safely synchronize access to any file
/// system, even a remote network share.
///
/// \par Local locks
/// A local lock may be implemented as a system synchronization object (e.g. a named mutex or
/// semaphore), that is only guaranteed to be safe for synchronizing file system access on a local
/// system.
///
/// \par
/// The advantage of a local lock is that on some systems it is faster than a remote lock.
///
/// \note
/// Do not mix the use of remote locks and local locks for synchronizing access to a single file
/// system, since the locks may live in different namespaces and thus are unaware of each other.
///
/// \note
/// On some systems, local locks are implemented as remote locks.
class file_lock_t {
public:
  /// @brief Create an empty (unlocked) lock object.
  file_lock_t() {
  }

  /// @brief Acquire a lock for the specified file path.
  /// @param path The full path to the lock file (will be created). The path should be a location on
  /// the file system to which the process requires access synchronization.
  /// @param remote_lock Require the implementation to use a locking mechanism that can synchronize
  /// file system access across several OS instances (e.g. use this for network shares).
  explicit file_lock_t(const std::string& path, const bool remote_lock);

  // Support move semantics.
  file_lock_t(file_lock_t&& other) noexcept;
  file_lock_t& operator=(file_lock_t&& other) noexcept;
  file_lock_t(const file_lock_t& other) = delete;
  file_lock_t& operator=(const file_lock_t& other) = delete;

  /// @brief Release the lock.
  ///
  /// This releases the lock that was held. The sycnhronization object (if any) is deleted if this
  /// is the last process that holds a lock.
  ~file_lock_t();

  /// @returns true if the lock was acquired successfully.
  bool has_lock() const {
    return m_has_lock;
  }

private:
#if defined(_WIN32)
  // WIN32 API HANDLE type is void*.
  using file_handle_t = void*;
  using mutex_handle_t = void*;
#else
  // POSIX file descriptor type is int.
  using file_handle_t = int;
  using mutex_handle_t = void*;  // dummy
#endif

  static file_handle_t invalid_file_handle() {
#if defined(_WIN32)
    return reinterpret_cast<void*>(-1);  // INVALID_HANDLE_VALUE
#else
    return -1;
#endif
  }

  static mutex_handle_t invalid_mutex_handle() {
#if defined(_WIN32)
    return nullptr;
#else
    return nullptr;
#endif
  }

  std::string m_path;
  file_handle_t m_file_handle = invalid_file_handle();
  mutex_handle_t m_mutex_handle = invalid_mutex_handle();
  bool m_has_lock = false;
};

}  // namespace file
}  // namespace bcache

#endif  // BUILDCACHE_FILE_LOCK_HPP_
