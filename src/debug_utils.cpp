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

#include "debug_utils.hpp"

#include <atomic>
#include <cstdlib>
#include <iostream>
#include <string>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#undef ERROR
#undef log
#include <vector>
#else
#include <sys/types.h>
#include <unistd.h>
#endif

namespace bcache {
namespace debug {
namespace {
std::atomic_int s_log_level(-1);

std::string get_level_string(const log_level_t level) {
  switch (level) {
    case DEBUG:
      return "DEBUG";
    case INFO:
      return "INFO";
    case ERROR:
      return "ERROR";
    case FATAL:
      return "FATAL";
    default:
      return "?";
  }
}

log_level_t get_log_level() {
  int log_level = s_log_level;

  // The first time get_log_level() is called, s_log_level is undefined (negative).
  if (log_level < 0) {
    // Get the log level from the environment variable BUILDCACHE_DEBUG.
    const auto* log_level_env = std::getenv("BUILDCACHE_DEBUG");
    if (log_level_env != nullptr) {
      try {
        log_level = std::stoi(std::string(log_level_env));
      } catch (...) {
      }
      if ((log_level < static_cast<int>(DEBUG)) || (log_level > static_cast<int>(FATAL))) {
        log_level = -1;
      }
    }

    // If we did not get a valid log level, fall back to NONE (higher than the highest level).
    if (log_level < 0) {
      log_level = static_cast<int>(NONE);
    }
    s_log_level = log_level;
  }

  return static_cast<log_level_t>(log_level);
}

int get_process_id() {
#ifdef _WIN32
  return static_cast<int>(GetCurrentProcessId());
#else
  return static_cast<int>(getpid());
#endif
}

std::string pad_string(const std::string& str, const size_t width) {
  return (str.size() < width) ? (str + std::string(width - str.size(), ' ')) : str;
}
}  // namespace

log::log(const log_level_t level) : m_level(level) {
}

log::~log() {
  if (m_level >= get_log_level()) {
    const auto level_str = std::string("(") + get_level_string(m_level) + ")";
    std::cout << "buildcache[" << get_process_id() << "] " << pad_string(level_str, 7) << " "
              << m_stream.str() << "\n"
              << std::flush;
  }
}
}  // namespace debug
}  // namespace bcache
