//--------------------------------------------------------------------------------------------------
// Copyright (c) 2019 Alexey Sheplyakov
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

#include <base/debug_utils.hpp>
#include <base/file_utils.hpp>

#include <cache/cache_stats.hpp>
#include <cjson/cJSON.h>

#include <iostream>
#include <memory>

namespace bcache {

namespace {

constexpr char LOCAL_HIT_COUNT[] = "local_hit_count";
constexpr char LOCAL_MISS_COUNT[] = "local_miss_count";
constexpr char REMOTE_HIT_COUNT[] = "remote_hit_count";
constexpr char REMOTE_MISS_COUNT[] = "remote_miss_count";

struct JSON_Deleter {
  void operator()(cJSON* obj) const {
    if (obj != nullptr) {
      cJSON_Delete(obj);
    }
  }
};
}  // namespace

using JSONPtr = std::unique_ptr<cJSON, JSON_Deleter>;

bool cache_stats_t::from_file(const std::string& path) noexcept {
  if (!file::file_exists(path)) {
    return false;
  }
  JSONPtr ptr;
  try {
    const auto str = file::read(path);
    ptr.reset(cJSON_Parse(str.c_str()));
  } catch (...) {
    return false;
  }
  return from_json(ptr.get());
}

bool cache_stats_t::from_json(cJSON const* obj) noexcept {
  if (obj == nullptr) {
    return false;
  }
  auto* node = cJSON_GetObjectItemCaseSensitive(obj, LOCAL_HIT_COUNT);
  if ((node != nullptr) && (cJSON_IsNumber(node) != 0)) {
    m_local_hit_count = node->valueint;
  }
  node = cJSON_GetObjectItemCaseSensitive(obj, LOCAL_MISS_COUNT);
  if ((node != nullptr) && (cJSON_IsNumber(node) != 0)) {
    m_local_miss_count = node->valueint;
  }
  node = cJSON_GetObjectItemCaseSensitive(obj, REMOTE_HIT_COUNT);
  if ((node != nullptr) && (cJSON_IsNumber(node) != 0)) {
    m_remote_hit_count = node->valueint;
  }
  node = cJSON_GetObjectItemCaseSensitive(obj, REMOTE_MISS_COUNT);
  if ((node != nullptr) && (cJSON_IsNumber(node) != 0)) {
    m_remote_miss_count = node->valueint;
  }
  return true;
}

bool cache_stats_t::to_json(cJSON* obj) const noexcept {
  if (obj == nullptr) {
    return false;
  }
  auto* node = cJSON_GetObjectItemCaseSensitive(obj, LOCAL_HIT_COUNT);
  if (node != nullptr) {
    cJSON_SetNumberValue(node, m_local_hit_count);
  } else {
    node = cJSON_AddNumberToObject(obj, LOCAL_HIT_COUNT, m_local_hit_count);
  }
  if (node == nullptr) {
    debug::log(debug::ERROR) << "failed to serialize cache_stats object";
    return false;
  }
  node = cJSON_GetObjectItemCaseSensitive(obj, LOCAL_MISS_COUNT);
  if (node != nullptr) {
    cJSON_SetNumberValue(node, m_local_miss_count);
  } else {
    node = cJSON_AddNumberToObject(obj, LOCAL_MISS_COUNT, m_local_miss_count);
  }
  if (node == nullptr) {
    debug::log(debug::ERROR) << "failed to serialize cache_stats object";
    return false;
  }
  node = cJSON_GetObjectItemCaseSensitive(obj, REMOTE_HIT_COUNT);
  if (node != nullptr) {
    cJSON_SetNumberValue(node, m_remote_hit_count);
  } else {
    node = cJSON_AddNumberToObject(obj, REMOTE_HIT_COUNT, m_remote_hit_count);
  }
  if (node == nullptr) {
    debug::log(debug::ERROR) << "failed to serialize cache_stats object";
    return false;
  }
  node = cJSON_GetObjectItemCaseSensitive(obj, REMOTE_MISS_COUNT);
  if (node != nullptr) {
    cJSON_SetNumberValue(node, m_remote_miss_count);
  } else {
    node = cJSON_AddNumberToObject(obj, REMOTE_MISS_COUNT, m_remote_miss_count);
  }
  if (node == nullptr) {
    debug::log(debug::ERROR) << "failed to serialize cache_stats object";
    return false;
  }

  return true;
}

bool cache_stats_t::to_file(const std::string& path) const noexcept {
  JSONPtr jsonptr{cJSON_CreateObject()};
  if (!jsonptr) {
    return false;
  }
  to_json(jsonptr.get());
  std::unique_ptr<char, decltype(&cJSON_free)> str{nullptr, cJSON_free};
  str.reset(cJSON_Print(jsonptr.get()));
  if (str) {
    try {
      file::create_dir_with_parents(file::get_dir_part(path));
      file::write(std::string(str.get()), path);
      return true;
    } catch (...) {
    }
  }
  return false;
}

void cache_stats_t::dump(std::ostream& os, const std::string& prefix) const {
  os << prefix << "Local hits:        " << m_local_hit_count << std::endl;
  os << prefix << "Local misses:      " << m_local_miss_count << std::endl;
  os << prefix << "Remote hits:       " << m_remote_hit_count << std::endl;
  os << prefix << "Remote misses:     " << m_remote_miss_count << std::endl;
  os << prefix << "Misses:            " << global_miss_count() << std::endl;
  os << prefix << "Local hit ratio:   " << local_hit_ratio() << '%' << std::endl;
  os << prefix << "Remote hit ratio:  " << remote_hit_ratio() << '%' << std::endl;
  os << prefix << "Hit ratio:         " << global_hit_ratio() << '%' << std::endl;
}

}  // namespace bcache
