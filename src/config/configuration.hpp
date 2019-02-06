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

#include <base/string_list.hpp>

#include <cstdint>
#include <map>
#include <string>

namespace bcache {
namespace config {
/// @brief Initialize the configuration based on environment variables etc.
void init();

/// @returns the BuildCache home directory.
const std::string& dir();

/// @returns the BuildCache configuration file.
const std::string& config_file();

/// @returns the Lua search paths.
const string_list_t& lua_paths();

/// @returns the compiler exectution prefix command.
const std::string& prefix();

/// @returns the remote cache service address.
const std::string& remote();

/// @returns the maximum cache size (in bytes).
int64_t max_cache_size();

/// @returns the debug level (-1 for no debugging).
int32_t debug();

/// @returns true if BuildCache should use hard links when possible.
bool hard_links();

/// @returns true if BuildCache should compress data in the cache.
bool compress();

/// @returns true if performance profiling output is enabled.
bool perf();

/// @returns true if BuildCache is disabled.
bool disable();

}  // namespace config
}  // namespace bcache

#endif  // BUILDCACHE_CONFIGURATION_HPP_
