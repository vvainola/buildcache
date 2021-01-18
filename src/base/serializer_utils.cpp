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

#include <base/serializer_utils.hpp>

#include <stdexcept>

namespace bcache {
namespace serialize {

std::string from_bool(const bool x) {
  auto data = std::string(1, 0);
  data[0] = static_cast<char>(x ? 1 : 0);
  return data;
}

std::string from_int(const int32_t x) {
  auto data = std::string(4, 0);
  data[0] = static_cast<char>(x);
  data[1] = static_cast<char>(x >> 8);
  data[2] = static_cast<char>(x >> 16);
  data[3] = static_cast<char>(x >> 24);
  return data;
}

std::string from_string(const std::string& x) {
  return from_int(static_cast<int32_t>(x.size())) + x;
}

std::string from_vector(const std::vector<std::string>& x) {
  auto result = from_int(static_cast<int32_t>(x.size()));
  for (const auto& element : x) {
    result += from_string(element);
  }
  return result;
}

std::string from_map(const std::map<std::string, std::string>& x) {
  auto result = from_int(static_cast<int32_t>(x.size()));
  for (const auto& element : x) {
    result += (from_string(element.first) + from_string(element.second));
  }
  return result;
}

bool to_bool(const std::string& data, std::string::size_type& pos) {
  if ((pos + 1U) > data.size()) {
    throw std::runtime_error("Premature end of serialized data stream.");
  }
  pos += 1;
  return static_cast<uint8_t>(data[pos - 1]) != 0;
}

int32_t to_int(const std::string& data, std::string::size_type& pos) {
  if ((pos + 4U) > data.size()) {
    throw std::runtime_error("Premature end of serialized data stream.");
  }
  pos += 4;
  return static_cast<int32_t>((static_cast<uint32_t>(static_cast<uint8_t>(data[pos - 4]))) |
                              (static_cast<uint32_t>(static_cast<uint8_t>(data[pos - 3])) << 8) |
                              (static_cast<uint32_t>(static_cast<uint8_t>(data[pos - 2])) << 16) |
                              (static_cast<uint32_t>(static_cast<uint8_t>(data[pos - 1])) << 24));
}

std::string to_string(const std::string& data, std::string::size_type& pos) {
  const auto size = static_cast<std::string::size_type>(to_int(data, pos));
  if ((pos + size) > data.size()) {
    throw std::runtime_error("Premature end of serialized data stream.");
  }
  pos += size;
  return std::string(&data[pos - size], size);
}

std::vector<std::string> to_vector(const std::string& data, std::string::size_type& pos) {
  const auto size = to_int(data, pos);
  std::vector<std::string> result;
  result.reserve(size);
  for (int32_t i = 0; i < size; ++i) {
    result.emplace_back(to_string(data, pos));
  }
  return result;
}

std::map<std::string, std::string> to_map(const std::string& data, std::string::size_type& pos) {
  const auto size = to_int(data, pos);
  std::map<std::string, std::string> result;
  for (int32_t i = 0; i < size; ++i) {
    const auto key = to_string(data, pos);
    const auto value = to_string(data, pos);
    result[key] = value;
  }
  return result;
}

}  // namespace serialize
}  // namespace bcache
