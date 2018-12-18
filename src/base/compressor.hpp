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

#ifndef BUILDCACHE_COMPRESSOR_HPP_
#define BUILDCACHE_COMPRESSOR_HPP_

#include <string>

namespace bcache {
namespace comp {
/// @brief Compress a string in memory.
/// @param str The string to compress.
/// @returns the compressed string.
std::string compress(const std::string& str);

/// @brief Decompress a string in memory.
/// @param str The compressed string to decompress.
/// @returns the original (decompressed) string.
std::string decompress(const std::string& str);

/// @brief Compress a file.
/// @param from_path The source file (uncompressed).
/// @param to_path The destination file (compressed).
/// @throws runtime_error if the operation could not be completed.
void compress_file(const std::string& from_path, const std::string& to_path);

/// @brief Decompress a file.
/// @param from_path The source file (compressed).
/// @param to_path The destination file (uncompressed).
/// @throws runtime_error if the operation could not be completed.
void decompress_file(const std::string& from_path, const std::string& to_path);
}  // namespace comp
}  // namespace bcache

#endif  // BUILDCACHE_COMPRESSOR_HPP_
