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
#include <fcntl.h>
#include <signal.h>
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

#if !defined(_WIN32)
// The max file lock age before it is considered *definitely* stale, in seconds.
// Note: We set this high to accomodate for time zone differences, clock errors, etc.
const time::seconds_t MAX_FILE_LOCK_AGE = 24 * 3600;

bool file_is_too_old(const std::string& path) {
  try {
    const auto age = time::seconds_since_epoch() - get_file_info(path).modify_time();
    return age > MAX_FILE_LOCK_AGE;
  } catch (const std::exception& e) {
    // Note: This is not necessarily an error, since the lock file may no longer exist.
    debug::log(debug::DEBUG) << "Unable to determine file age for " << path << ": " << e.what();
    return false;
  }
}
#endif  // !_WIN32
}  // namespace

file_lock_t::file_lock_t(const std::string& path, const bool remote_lock) : m_path(path) {
#if defined(_WIN32)
  // Time values are in milliseconds.
  const DWORD MAX_WAIT_TIME = 10000;  // We'll fail if the lock can't be acquired in 10s.

  // For Windows, we can use local, named mutexes instead of file locks, if we're allowed to.
  if (!remote_lock) {
    // Construct a unique mutex name that identifies the given path.
    const auto name = construct_mutex_name(path);

    // Open the named mutex (create it if it does not already exist).
    m_mutex_handle = CreateMutexW(nullptr, FALSE, name.c_str());
    if (m_mutex_handle == invalid_mutex_handle()) {
      return;
    }

    // Acquire the mutex.
    // WAIT_OBJECT_0 and WAIT_ABANDONED both indicate the lock has been acquired. WAIT_ABANDONED
    // additionally indicates that the previous thread owning the lock terminated without properly
    // releasing it.
    const auto status = WaitForSingleObject(m_mutex_handle, MAX_WAIT_TIME);
    if (!(status == WAIT_OBJECT_0 || status == WAIT_ABANDONED)) {
      return;
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

  // Time values are in microseconds.
  const int64_t MAX_WAIT_TIME = 10000000;  // We'll fail if the lock can't be acquired in 10s.
  const int64_t TIME_BETWEEN_LOCK_BREAKS = 100000;  // We try to break the lock every 100ms.
  const int64_t MIN_SLEEP_TIME = 10;
  const int64_t MAX_SLEEP_TIME = 50000;

  int64_t total_wait_time = 0;
  int64_t sleep_time = MIN_SLEEP_TIME;
  int64_t time_until_lock_break = TIME_BETWEEN_LOCK_BREAKS;

  while (total_wait_time < MAX_WAIT_TIME) {
    // Try to create the lock file in exclusive mode.
    m_file_handle = ::open(path.c_str(), O_WRONLY | O_CREAT | O_EXCL, 0666);
    if (m_file_handle != invalid_file_handle()) {
      // We got the lock! Write our PID to the file.
      const auto pid_str = std::to_string(getpid());
      const auto bytes_written = ::write(m_file_handle, pid_str.data(), pid_str.size());
      if (bytes_written != static_cast<ssize_t>(pid_str.size())) {
        ::unlink(path.c_str());
        ::close(m_file_handle);
        m_file_handle = invalid_file_handle();
        debug::log(debug::ERROR) << "Failed to write our PID to the lock file " << path;
      }
      break;
    }
    if (errno != EEXIST) {
      debug::log(debug::ERROR) << "Failed to open the lock file (\"" << std::strerror(errno)
                               << "\")" << path;
      break;
    }

    // The file was already taken (errno == EEXIST), so try again soon.

    // Time to check if we can break the lock if it's stale.
    if (time_until_lock_break < 0) {
      auto fd = ::open(path.c_str(), O_RDONLY);
      if (fd < 0) {
        if (errno != ENOENT) {
          debug::log(debug::ERROR) << "Unable to open possibly stale lock for reading: " << path;
          break;
        }

        // File was removed since we last tried to open it. Try again...

      } else {
        // Read the PID from the file.
        std::string owner_pid_str;
        {
          char buf[100];
          size_t bytes_left = sizeof(buf) - 1;
          ssize_t bytes_read = 0;
          bool finished_reading = false;
          while (!finished_reading) {
            const auto bytes = ::read(fd, &buf[bytes_read], bytes_left);
            if (bytes == 0) {
              finished_reading = true;
            } else if (bytes < 0) {
              debug::log(debug::INFO) << "Unable to read PID from possibly stale lock " << path;
              break;
            } else {
              bytes_left -= static_cast<size_t>(bytes);
              bytes_read += static_cast<size_t>(bytes);
            }
          }
          close(fd);
          if (finished_reading) {
            owner_pid_str = std::string(buf, static_cast<std::string::size_type>(bytes_read));
          }
        }

        // Convert the file string to a pid_t.
        pid_t owner_pid = -1;
        try {
          owner_pid = static_cast<pid_t>(std::stol(owner_pid_str));
        } catch (...) {
          debug::log(debug::INFO) << "Invalid PID for possibly stale lock " << path << ": "
                                  << owner_pid_str;
        }

        // If we have the PID of the owner process, we can check if it's still alive. We also check
        // if the file is too old to reasonably be held at the moment (this should also work across
        // different nodes sharing locks on a network drive, provided that their clocks are not
        // wildly out of sync).
        const auto owner_process_is_dead =
            (owner_pid >= 0 && ::kill(owner_pid, 0) == -1) || file_is_too_old(path);
        if (owner_process_is_dead) {
          // Ok, the process is dead. Let's remove the lock file and hope for better luck during
          // the next iteration.
          const auto file_removed = (::unlink(m_path.c_str()) != -1);
          if (file_removed) {
            debug::log(debug::INFO) << "Removed stale lock " << path << " for PID " << owner_pid;
          } else {
            debug::log(debug::INFO)
                << "Unable to remove stale lock " << path << " for PID " << owner_pid;
          }

          // Restart the timer until we try to break the lock again.
          time_until_lock_break = TIME_BETWEEN_LOCK_BREAKS;
          sleep_time = MIN_SLEEP_TIME;
        }
      }
    }

    // Wait for a small period of time to give other processes a chance to release the lock.
    // Note: Handle both short and long blocks by increasing the sleep time for every new try.
    std::this_thread::sleep_for(std::chrono::microseconds(sleep_time));
    total_wait_time += sleep_time;
    time_until_lock_break -= sleep_time;
    sleep_time = std::min(sleep_time * 2, MAX_SLEEP_TIME);
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
    // Delete and close the lock file.
    ::unlink(m_path.c_str());
    ::close(m_file_handle);
  }
#endif
}
}  // namespace file
}  // namespace bcache
