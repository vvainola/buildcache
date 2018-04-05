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

#ifndef BUILDCACHE_SYS_UTILS_HPP_
#define BUILDCACHE_SYS_UTILS_HPP_

#include "string_list.hpp"

#include <string>

namespace bcache {
namespace sys {
/// @brief Run results from an external command.
struct run_result_t {
  int return_code;     ///< The program return code (zero for success).
  std::string stdout;  ///< The contents of stdout.
  std::string stderr;  ///< The contents of stderr.
};

/// @brief Convert a UCS-2 string to a UTF-8 string.
/// @param str16 The UCS-2 encoded wide character string.
/// @returns a UTF-8 encoded string.
std::string ucs2_to_utf8(const std::wstring& str16);

/// @brief Convert a UTF-8 string to a UCS-2 string.
/// @param str8 The UTF-8 encoded string.
/// @returns a UCS-2 encoded wide charater string.
std::wstring utf8_to_ucs2(const std::string& str8);

/// @brief Run the given command.
/// @param args The command and its arguments (the first item is the command).
/// @param quiet Supress output to stdout/stderr during execution.
/// @returns The result from the command.
run_result_t run(const string_list_t& args, const bool quiet = true);

/// @brief Run the given command with an optional prefix.
///
/// Unlike the run() function, this function will optinally use a prefix (wrapper) as specified by
/// the environment variable BUILDCACHE_PREFIX.
///
/// @param args The command and its arguments (the first item is the command).
/// @param quiet Supress output to stdout/stderr during execution.
/// @returns The result from the command.
run_result_t run_with_prefix(const string_list_t& args, const bool quiet = true);
}  // namespace sys
}  // namespace bcache

#endif  // BUILDCACHE_SYS_UTILS_HPP_
