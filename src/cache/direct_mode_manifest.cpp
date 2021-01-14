//--------------------------------------------------------------------------------------------------
// Copyright (c) 2021 Marcus Geelnard
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

#include <cache/direct_mode_manifest.hpp>

#include <base/serializer_utils.hpp>

#include <stdexcept>

namespace bcache {
namespace {
// The version of the manifest file serialization data format.
const int32_t MANIFEST_DATA_FORMAT_VERSION = 1;
}  // namespace

direct_mode_manifest_t::direct_mode_manifest_t() {
}

direct_mode_manifest_t::direct_mode_manifest_t(
    const std::string& hash,
    const std::map<std::string, std::string>& files_with_hashes)
    : m_hash(hash), m_files_width_hashes(files_with_hashes), m_valid(true) {
}

std::string direct_mode_manifest_t::serialize() const {
  std::string data = serialize::from_int(MANIFEST_DATA_FORMAT_VERSION);
  data += serialize::from_string(m_hash);
  data += serialize::from_map(m_files_width_hashes);
  return data;
}

direct_mode_manifest_t direct_mode_manifest_t::deserialize(const std::string& data) {
  std::string::size_type pos = 0;

  // Read and check the format version.
  int32_t format_version = serialize::to_int(data, pos);
  if (format_version > MANIFEST_DATA_FORMAT_VERSION) {
    throw std::runtime_error("Unsupported serialization format version.");
  }

  // De-serialize the manifest.
  const auto hash = serialize::to_string(data, pos);
  const auto files_with_hashes = serialize::to_map(data, pos);

  return direct_mode_manifest_t(hash, files_with_hashes);
}

}  // namespace bcache
