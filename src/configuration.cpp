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

#include "configuration.hpp"

#include "file_utils.hpp"

#include <algorithm>
#include <stdexcept>

namespace bcache {
namespace {
// Various constants.
const std::string ROOT_FOLDER_NAME = ".buildcache";
const std::string CONFIGURATION_FILE_NAME = "buildcache.conf";
const int64_t DEFAULT_MAX_CACHE_SIZE = 5368709120L;  // 5 GB

// Configuration options.
std::string s_dir;
string_list_t s_lua_paths;
std::string s_prefix;
int64_t s_max_cache_size = DEFAULT_MAX_CACHE_SIZE;
int32_t s_debug = -1;
bool s_perf = false;
bool s_disable = false;

std::string to_lower(const std::string& str) {
  std::string str_lower(str.size(), ' ');
  std::transform(str.begin(), str.end(), str_lower.begin(), ::tolower);
  return str_lower;
}

/// @brief A helper class for reading and parsing environment variables.
class env_var_t {
public:
  env_var_t(const std::string& name) : m_defined(false) {
    const auto* env_var = std::getenv(name.c_str());
    if (env_var != nullptr) {
      m_value = std::string(env_var);
      m_defined = true;
    }
  }

  operator bool() const {
    return m_defined;
  }

  const std::string& as_string() const {
    return m_value;
  }

  const int64_t as_int64() const {
    return std::stoll(m_value);
  }

  const bool as_bool() const {
    const auto value_lower = to_lower(m_value);
    return m_defined && (!m_value.empty()) && (value_lower != "false") && (value_lower != "no") &&
           (value_lower != "off") && (value_lower != "0");
  }

private:
  std::string m_value;
  bool m_defined;
};

std::string get_dir() {
  // Is the environment variable BUILDCACHE_DIR defined?
  {
    const env_var_t dir_env("BUILDCACHE_DIR");
    if (dir_env) {
      return dir_env.as_string();
    }
  }

  // Use the user home directory if possible.
  {
    auto home = file::get_user_home_dir();
    if (!home.empty()) {
      // TODO(m): Should use ".cache/buildcache".
      return file::append_path(home, ROOT_FOLDER_NAME);
    }
  }

  // We failed.
  throw std::runtime_error("Unable to determine a home directory for BuildCache.");
}

string_list_t get_lua_paths(const std::string& dir) {
  string_list_t paths;

  // The BUILDCACHE_LUA_PATH env variable can contain a colon-separated list of paths.
  {
    const env_var_t lua_path_env("BUILDCACHE_LUA_PATH");
    if (lua_path_env) {
#ifdef _WIN32
      paths += string_list_t(lua_path_env.as_string(), ";");
#else
      paths += string_list_t(lua_path_env.as_string(), ":");
#endif
    }
  }

  // We also look for Lua files in the cache root dir (e.g. ${BUILDCACHE_DIR}/lua).
  paths += file::append_path(dir, "lua");

  return paths;
}

}  // namespace

namespace config {
void init() {
  // Guard: Only initialize once.
  static bool s_initialized = false;
  if (s_initialized) {
    return;
  }
  s_initialized = true;

  try {
    // Get the BuildCache home directory.
    s_dir = get_dir();

    // Get the Lua paths.
    s_lua_paths = get_lua_paths(s_dir);

    // Get the command prefix from the environment.
    {
      const env_var_t prefix_env("BUILDCACHE_PREFIX");
      if (prefix_env) {
        s_prefix = prefix_env.as_string();
      }
    }

    // Get the max cache size from the environment.
    {
      const env_var_t max_cache_size_env("BUILDCACHE_MAX_CACHE_SIZE");
      if (max_cache_size_env) {
        try {
          s_max_cache_size = max_cache_size_env.as_int64();
        } catch (...) {
          // Ignore...
        }
      }
    }

    // Get the debug level from the environment.
    {
      const env_var_t debug_env("BUILDCACHE_DEBUG");
      if (debug_env) {
        try {
          s_debug = static_cast<int32_t>(debug_env.as_int64());
        } catch (...) {
          // Ignore...
        }
      }
    }

    // Get the perf flag from the environment.
    {
      const env_var_t perf_env("BUILDCACHE_PERF");
      if (perf_env) {
        s_perf = perf_env.as_bool();
      }
    }

    // Get the disabled flag from the environment.
    {
      const env_var_t disable_env("BUILDCACHE_DISABLE");
      if (disable_env) {
        s_disable = disable_env.as_bool();
      }
    }
  } catch (...) {
    // If we could not initialize the configuration, we can't proceed. We need to disable the cache.
    s_disable = true;
  }
}

void update(const std::string& file_name) {
  // TODO(m): Implement me!
}

void update(const std::map<std::string, std::string>& options) {
  // TODO(m): Implement me!
}

void save_to_file(const std::string& file_name) {
  // TODO(m): Implement me!
}

const std::string& dir() {
  return s_dir;
}

const string_list_t& lua_paths() {
  return s_lua_paths;
}

const std::string& prefix() {
  return s_prefix;
}

const int64_t max_cache_size() {
  return s_max_cache_size;
}

const int32_t debug() {
  return s_debug;
}

const bool perf() {
  return s_perf;
}

const bool disable() {
  return s_disable;
}
}  // namespace config
}  // namespace bcache
