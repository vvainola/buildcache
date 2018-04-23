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

#include "hasher.hpp"

#include "file_utils.hpp"

namespace bcache {
const std::string hasher_t::hash_t::as_string() const {
  static const char digits[17] = "0123456789abcdef";
  std::string result(SIZE * 2, '0');
  for (size_t i = 0; i < SIZE; ++i) {
    result[i * 2] = digits[m_data[i] >> 4];
    result[i * 2 + 1] = digits[m_data[i] & 0x0fu];
  }
  return result;
}

void hasher_t::update(const std::map<std::string, std::string>& data) {
  // Note: This is guaranteed by the C++ standard to iterate over the elements in ascending key
  // order, so the hash will always be the same for the same map contents.
  for (const auto& item : data) {
    update(item.first);
    update(item.second);
  }
}

void hasher_t::update_from_file(const std::string& path) {
  // TODO(m): Investigate if using buffered input gives better performance (at least it should use
  // less memory, and it should be nicer to the CPU caches).
  const auto file_data = file::read(path);
  update(file_data);
}
}  // namespace bcache
