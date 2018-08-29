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

#include "perf_utils.hpp"

#include "unicode_utils.hpp"

#include <chrono>
#include <iostream>

namespace bcache {
namespace perf {
namespace {
int64_t s_perf_log[NUM_PERF_IDS] = {};

int64_t get_time_in_us() {
  const auto t =
      std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::system_clock::now())
          .time_since_epoch()
          .count();
  return t;
}

bool is_perf_enabled() {
  const auto* perf_env = std::getenv("BUILDCACHE_PERF");
  if (perf_env == nullptr) {
    return false;
  }
  const auto perf = lower_case(std::string(perf_env));
  return (perf != "no") && (perf != "off") && (perf != "false");
}
}  // namespace

int64_t start() {
  return get_time_in_us();
}

void stop(const int64_t start_time, const id_t id) {
  const auto dt = get_time_in_us() - start_time;
  s_perf_log[id] += dt;
}

void report() {
  if (is_perf_enabled()) {
    std::cerr << "Find exectuable:    " << s_perf_log[ID_FIND_EXECUTABLE] << " us\n";
    std::cerr << "Find wrapper:       " << s_perf_log[ID_FIND_WRAPPER] << " us\n";
    std::cerr << "Lua - Init:         " << s_perf_log[ID_LUA_INIT] << " us\n";
    std::cerr << "Lua - Load script:  " << s_perf_log[ID_LUA_LOAD_SCRIPT] << " us\n";
    std::cerr << "Lua - Run:          " << s_perf_log[ID_LUA_RUN] << " us\n";
    std::cerr << "Resolve args:       " << s_perf_log[ID_RESOLVE_ARGS] << " us\n";
    std::cerr << "Preprocess:         " << s_perf_log[ID_PREPROCESS] << " us\n";
    std::cerr << "Filter arguments:   " << s_perf_log[ID_FILTER_ARGS] << " us\n";
    std::cerr << "Get program id:     " << s_perf_log[ID_GET_PRG_ID] << " us\n";
    std::cerr << "Cache lookup:       " << s_perf_log[ID_CACHE_LOOKUP] << " us\n";
    std::cerr << "Get build files:    " << s_perf_log[ID_GET_BUILD_FILES] << " us\n";
    std::cerr << "Run cmd (miss):     " << s_perf_log[ID_RUN_FOR_MISS] << " us\n";
    std::cerr << "Add to cache:       " << s_perf_log[ID_ADD_TO_CACHE] << " us\n";
    std::cerr << "Run cmd (fallback): " << s_perf_log[ID_RUN_FOR_FALLBACK] << " us\n";
  }
}
}  // namespace perf
}  // namespace bcache
