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

#ifndef BUILDCACHE_CONFIGURATION_HPP_
#define BUILDCACHE_CONFIGURATION_HPP_

#include <cstdint>
#include <map>
#include <string>

#include "string_list.hpp"

namespace bcache {
namespace config {
/// @brief Initialize the configuration based on environment variables etc.
void init();

/// @brief Update configuration options from the given file.
/// @param file_name The name of the file to load the options from.
void update(const std::string& file_name);

/// @brief Update configuration options from a key-value options map.
/// @param options A key-value map containing the options.
void update(const std::map<std::string, std::string>& options);

/// @brief Save the configuration options object to a file.
void save_to_file(const std::string& file_name);

/// @returns the BuildCache home directory.
const std::string& dir();

/// @returns the Lua search paths.
const string_list_t& lua_paths();

/// @returns the compiler exectution prefix command.
const std::string& prefix();

/// @returns the maximum cache size (in bytes).
const int64_t max_cache_size();

/// @returns the debug level (-1 for no debugging).
const int32_t debug();

/// @returns true if performance profiling output is enabled.
const bool perf();

/// @returns true if BuildCache is disabled.
const bool disable();

}  // namespace config
}  // namespace bcache

#endif  // BUILDCACHE_CONFIGURATION_HPP_
