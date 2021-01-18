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

#ifndef BUILDCACHE_SERIALIZER_UTILS_HPP_
#define BUILDCACHE_SERIALIZER_UTILS_HPP_

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace bcache {
namespace serialize {
std::string from_bool(const bool x);
std::string from_int(const int32_t x);
std::string from_string(const std::string& x);
std::string from_vector(const std::vector<std::string>& x);
std::string from_map(const std::map<std::string, std::string>& x);

bool to_bool(const std::string& data, std::string::size_type& pos);
int32_t to_int(const std::string& data, std::string::size_type& pos);
std::string to_string(const std::string& data, std::string::size_type& pos);
std::vector<std::string> to_vector(const std::string& data, std::string::size_type& pos);
std::map<std::string, std::string> to_map(const std::string& data, std::string::size_type& pos);
}  // namespace serialize
}  // namespace bcache

#endif  // BUILDCACHE_SERIALIZER_UTILS_HPP_
