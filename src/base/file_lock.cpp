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

#include <base/debug_utils.hpp>
#include <base/file_lock.hpp>
#include <base/file_utils.hpp>
#include <base/hasher.hpp>
#include <base/time_utils.hpp>
#include <base/unicode_utils.hpp>

#include <cstdint>
#include <cstring>

#include <algorithm>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#undef ERROR
#undef log
#else
#include <chrono>
#include <csignal>
#include <fcntl.h>
#include <unistd.h>

#include <thread>
#endif

namespace bcache {
namespace file {
namespace {
#if defined(_WIN32)
std::wstring construct_mutex_name(const std::string& path) {
  const std::wstring NAME_PREFIX = L"Global\\buildcache_";  // Place in the Global namespace.

  // Use the full path, without backslashes, as the name.
  auto name = utf8_to_ucs2(path);
  std::replace(name.begin(), name.end(), L'\\', L'_');
  if ((NAME_PREFIX.size() + name.size()) >= MAX_PATH) {
    // If the name does not fit within MAX_PATH, use a hash instead.
    hasher_t hasher;
    hasher.update(ucs2_to_utf8(name));
    name = utf8_to_ucs2(hasher.final().as_string());
  }

  return NAME_PREFIX + name;
}
#endif
}  // namespace

file_lock_t::file_lock_t(const std::string& path, bool remote_lock, blocking_t blocking)
    : m_path(path) {
#if defined(_WIN32)
  // Time values are in milliseconds.
  const DWORD MAX_WAIT_TIME = 10000;  // We'll fail if the lock can't be acquired in 10s.

  // For Windows, we can use local, named mutexes instead of file locks, if we're allowed to.
  if (!remote_lock) {
    // Construct a unique mutex name that identifies the given path.
    const auto name = construct_mutex_name(path);

    // Open the named mutex (create it if it does not already exist).
    m_mutex_handle = CreateMutexW(nullptr, FALSE, name.c_str());
    if (m_mutex_handle != invalid_mutex_handle()) {
      // Select timeout, depending on the blocking mode.
      const DWORD timeout = (blocking == blocking_t::YES ? MAX_WAIT_TIME : 0);

      // Acquire the mutex.
      // WAIT_OBJECT_0 and WAIT_ABANDONED both indicate the lock has been acquired. WAIT_ABANDONED
      // additionally indicates that the previous thread owning the lock terminated without properly
      // releasing it.
      const auto status = WaitForSingleObject(m_mutex_handle, timeout);
      if (!(status == WAIT_OBJECT_0 || status == WAIT_ABANDONED)) {
        CloseHandle(m_mutex_handle);
        m_mutex_handle = invalid_file_handle();
      }
    }
  } else {
    // Time values are in milliseconds.
    const DWORD MIN_SLEEP_TIME = 0;  // We start with 0, which is essentially just a yield.
    const DWORD MAX_SLEEP_TIME = 50;

    DWORD total_wait_time = 0;
    DWORD sleep_time = MIN_SLEEP_TIME;

    while (total_wait_time < MAX_WAIT_TIME) {
      // Try to create the lock file in exclusive mode.
      m_file_handle = CreateFileW(utf8_to_ucs2(path).c_str(),
                                  GENERIC_READ | GENERIC_WRITE,
                                  0,  // No sharing => exclusive access.
                                  nullptr,
                                  CREATE_ALWAYS,
                                  FILE_ATTRIBUTE_NORMAL | FILE_FLAG_DELETE_ON_CLOSE,
                                  nullptr);
      if (m_file_handle != invalid_file_handle()) {
        // We got the lock!
        break;
      }

      // We were unable to create the file. We allow the following errors by waiting and trying
      // again later: ERROR_SHARING_VIOLATION (the lock is already held by another process) and
      // ERROR_ACCESS_DENIED (this can can in fact be due to a pending delete, see
      // https://stackoverflow.com/q/6680491/5778708). Any other errors are treated as
      // unrecoverable and result in an unlocked file_lock_t object.
      const auto last_error = GetLastError();
      if (last_error != ERROR_SHARING_VIOLATION && last_error != ERROR_ACCESS_DENIED) {
        debug::log(debug::ERROR) << "Failed to open the lock file " << path
                                 << " (error code: " << last_error << ")";
        break;
      }

      // In non-blocking mode: Wait no more.
      if (blocking == blocking_t::NO) {
        break;
      }

      // Wait for a small period of time to give other processes a chance to release the lock.
      // Note: Handle both short and long blocks by increasing the sleep time for every new try.
      Sleep(sleep_time);
      total_wait_time += sleep_time;
      sleep_time = std::min(sleep_time * 2 + 1, MAX_SLEEP_TIME);
    }
  }
#else
  // For POSIX, there's no need to use local locks (e.g. named semaphores), since the performance
  // benefit is negligable, and there are many inherent problems with that solution.
  (void)remote_lock;

  // Open the lock file (create if necessary).
  m_file_handle = ::open(path.c_str(), O_WRONLY | O_CREAT, 0666);
  if (m_file_handle != invalid_file_handle()) {
    // Select fcntl command based on the blocking mode (i.e. wait or don't wait for the lock to be
    // available).
    const int cmd = (blocking == blocking_t::YES ? F_SETLKW : F_SETLK);

    // Use POSIX record locks to lock the entire file in exclusive mode.
    struct flock fl = {};
    fl.l_type = F_WRLCK;     // Exclusive mode.
    fl.l_whence = SEEK_SET;  // Lock entire file (l_start = 0, l_len = 0).
    if (::fcntl(m_file_handle, cmd, &fl) == -1) {
      if (blocking == blocking_t::YES) {
        debug::log(debug::ERROR) << "Failed to acquire a lock on the lock file " << m_path;
      }
      ::close(m_file_handle);
      m_file_handle = invalid_file_handle();
    }
  }
#endif

  // Did we get the lock?
  m_has_lock =
      (m_file_handle != invalid_file_handle()) || (m_mutex_handle != invalid_mutex_handle());
  if (m_has_lock) {
    debug::log(debug::DEBUG) << "Locked " << path;
  }
}

file_lock_t::file_lock_t(file_lock_t&& other) noexcept
    : m_path(std::move(other.m_path)),
      m_file_handle(other.m_file_handle),
      m_mutex_handle(other.m_mutex_handle),
      m_has_lock(other.m_has_lock) {
  other.m_file_handle = invalid_file_handle();
  other.m_mutex_handle = invalid_mutex_handle();
  other.m_has_lock = false;
}

file_lock_t& file_lock_t::operator=(file_lock_t&& other) noexcept {
  m_path = std::move(other.m_path);
  m_file_handle = other.m_file_handle;
  m_mutex_handle = other.m_mutex_handle;
  m_has_lock = other.m_has_lock;

  other.m_file_handle = invalid_file_handle();
  other.m_mutex_handle = invalid_mutex_handle();
  other.m_has_lock = false;

  return *this;
}

file_lock_t::~file_lock_t() {
#if defined(_WIN32)
  if (m_mutex_handle != invalid_mutex_handle()) {
    if (m_has_lock) {
      ReleaseMutex(m_mutex_handle);
    }
    CloseHandle(m_mutex_handle);
  }
  if (m_file_handle != invalid_file_handle()) {
    // Close the file handle. This also deletes the file due to FILE_FLAG_DELETE_ON_CLOSE.
    CloseHandle(m_file_handle);
  }
#else
  if (m_file_handle != invalid_file_handle()) {
    // Close (and thus unlock) the lock file. Note that the lock file will stay around on the file
    // system (this is intentional, and any cleanup needs to be handled elsewhere).
    ::close(m_file_handle);
  }
#endif
}
}  // namespace file
}  // namespace bcache
