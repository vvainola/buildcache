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

#include <cache/cache.hpp>

#include <base/compressor.hpp>
#include <base/serializer_utils.hpp>

#include <stdexcept>

namespace bcache {
namespace {
// The version of the entry file serialization data format.
const int32_t ENTRY_DATA_FORMAT_VERSION = 2;
}  // namespace

std::string cache_t::serialize_entry(const entry_t& entry) {
  std::string data = serialize::from_int(ENTRY_DATA_FORMAT_VERSION);
  data += serialize::from_int(static_cast<int>(entry.compression_mode));
  data += serialize::from_map(entry.files);
  if (entry.compression_mode == entry_t::comp_mode_t::ALL) {
    data += serialize::from_string(comp::compress(entry.std_out));
    data += serialize::from_string(comp::compress(entry.std_err));
  } else {
    data += serialize::from_string(entry.std_out);
    data += serialize::from_string(entry.std_err);
  }
  data += serialize::from_int(static_cast<int32_t>(entry.return_code));
  return data;
}

cache_t::entry_t cache_t::deserialize_entry(const std::string& data) {
  std::string::size_type pos = 0;

  // Read and check the format version.
  int32_t format_version = serialize::to_int(data, pos);
  if (format_version > ENTRY_DATA_FORMAT_VERSION) {
    throw std::runtime_error("Unsupported serialization format version.");
  }

  // De-serialize the entry.
  entry_t entry;
  if (format_version > 1) {
    entry.compression_mode = static_cast<entry_t::comp_mode_t>(serialize::to_int(data, pos));
  } else {
    entry.compression_mode = entry_t::comp_mode_t::NONE;
  }
  entry.files = serialize::to_map(data, pos);
  entry.std_out = serialize::to_string(data, pos);
  entry.std_err = serialize::to_string(data, pos);
  entry.return_code = static_cast<int>(serialize::to_int(data, pos));

  // Optionally decompress the program output.
  if (entry.compression_mode == entry_t::comp_mode_t::ALL) {
    entry.std_out = comp::decompress(entry.std_out);
    entry.std_err = comp::decompress(entry.std_err);
  }

  return entry;
}

}  // namespace bcache
