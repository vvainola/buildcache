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

#include <base/string_list.hpp>

#include <string>

namespace bcache {
namespace sys {
/// @brief Run results from an external command.
struct run_result_t {
  std::string std_out;  ///< The contents of stdout.
  std::string std_err;  ///< The contents of stderr.
  int return_code = 1;  ///< The program return code (zero for success).
};

/// @brief Run the given command.
/// @param args The command and its arguments (the first item is the command).
/// @param quiet Supress output to stdout/stderr during execution.
/// @returns The result from the command.
/// @throws runtime_error if the command could not be run.
run_result_t run(const string_list_t& args, const bool quiet = true);

/// @brief Run the given command with an optional prefix.
///
/// Unlike the run() function, this function will optinally use a prefix (wrapper) as specified by
/// the environment variable BUILDCACHE_PREFIX.
///
/// @param args The command and its arguments (the first item is the command).
/// @param quiet Supress output to stdout/stderr during execution.
/// @returns The result from the command.
/// @throws runtime_error if the command could not be run.
run_result_t run_with_prefix(const string_list_t& args, const bool quiet = true);

/// @brief Open a file in the user default editor.
/// @param path Path to the file to edit.
void open_in_default_editor(const std::string& path);

/// @brief Print a string to stdout.
///
/// Unlike using standard C++ methods, such as writing to std::cout, this function sends the
/// string to stdout without any text mode translations (such as LF -> CRLF). This is useful for
/// correctly reproducing program output 1:1 (especially important on Windows).
///
/// @param str The string to be printed.
/// @throws runtime_error if the string could not be printed.
void print_raw_stdout(const std::string& str);

/// @brief Print a string to stderr.
///
/// Unlike using standard C++ methods, such as writing to std::cerr, this function sends the
/// string to stderr without any text mode translations (such as LF -> CRLF). This is useful for
/// correctly reproducing program output 1:1 (especially important on Windows).
///
/// @param str The string to be printed.
/// @throws runtime_error if the string could not be printed.
void print_raw_stderr(const std::string& str);

/// @brief Get the temporary folder for this BuildCache instance.
///
/// The temporary folder is located somewhere under $BUILDCACHE_DIR, and as such is suitable for
/// files that later need to be moved or linked inside the local cache structure (for instance).
/// The folder can also be considered a separate namespace that is dedicated to BuildCache, which
/// means that it is very unlikely that there will be any file name conflicts with other programs.
std::string get_local_temp_folder();
}  // namespace sys
}  // namespace bcache

#endif  // BUILDCACHE_SYS_UTILS_HPP_
