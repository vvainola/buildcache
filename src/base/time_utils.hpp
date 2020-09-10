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

#ifndef BUILDCACHE_TIME_UTILS_HPP_
#define BUILDCACHE_TIME_UTILS_HPP_

#include <cstdint>

namespace bcache {
namespace time {
/// @brief Time in seconds since the Unix epoch.
///
/// Integer number of seconds since the Unix epoch, i.e. 00:00:00 UTC on 1 January 1970. The value
/// is represented with 64 bits (so we're good wrt the 2038 problem).
using seconds_t = int64_t;

/// @brief Get the current time in seconds.
///
/// Time values returned by this function are compatible with file system time values.
/// @returns the number of seconds since the Unix epoch.
seconds_t seconds_since_epoch();

#if defined(_WIN32)
/// @brief Convert a Win32 FILETIME to the Unix epoch.
/// @param low The 32 lower bits of the FILETIME (dwLowDateTime).
/// @param high The 32 upper bits of the FILETIME (dwHighDateTime).
/// @returns the time in seconds since the Unix epoch.
seconds_t win32_filetime_to_unix_epoch(const uint32_t low, const uint32_t high);
#endif

}  // namespace time
}  // namespace bcache

#endif  // BUILDCACHE_TIME_UTILS_HPP_
