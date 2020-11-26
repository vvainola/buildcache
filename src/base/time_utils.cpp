//--------------------------------------------------------------------------------------------------
// Copyright (c) 2020 Marcus Geelnard
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

#include <base/time_utils.hpp>

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
#include <sys/time.h>
#include <sys/types.h>
#endif

#include <stdexcept>

namespace bcache {
namespace time {

seconds_t seconds_since_epoch() {
#if defined(_WIN32)
  FILETIME file_time;
  GetSystemTimeAsFileTime(&file_time);
  return win32_filetime_to_unix_epoch(file_time.dwLowDateTime, file_time.dwHighDateTime);
#else
  struct timeval now;
  if (::gettimeofday(&now, nullptr) == 0) {
    return static_cast<time::seconds_t>(now.tv_sec);
  }
  throw std::runtime_error("Could not get system time.");

#endif
}

#if defined(_WIN32)
seconds_t win32_filetime_to_unix_epoch(const uint32_t low, const uint32_t high) {
  // Convert the FILETIME struct to a 64-bit integer.
  const auto t64 = static_cast<int64_t>(static_cast<uint64_t>(static_cast<uint32_t>(low)) |
                                        (static_cast<uint64_t>(static_cast<uint32_t>(high)) << 32));

  // The FILETIME represents the number of 100-nanosecond intervals since January 1, 1601 (UTC).
  // I.e. the Windows epoch starts 11644473600 seconds before the UNIX epoch.
  return static_cast<time::seconds_t>((t64 / 10000000L) - 11644473600L);
}
#endif

}  // namespace time
}  // namespace bcache
