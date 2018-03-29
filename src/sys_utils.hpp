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

#include "arg_list.hpp"

#include <string>

namespace bcache {
namespace sys {
std::string ucs2_to_utf8(const std::wstring& str16);
std::wstring utf8_to_ucs2(const std::string& str8);

/// @brief Run results from an external command.
struct run_result_t {
  int return_code;    ///< The program return code (zero for success).
  std::string stdout; ///< The contents of stdout.
  std::string stderr; ///< The contents of stderr.
};

/// @brief Run the given command.
/// @param args The command and its arguments (the first item is the command).
/// @returns The result from the command.
run_result_t run(const arg_list_t& args);
}  // namespace sys
}  // namespace bcache

#endif  // BUILDCACHE_SYS_UTILS_HPP_
