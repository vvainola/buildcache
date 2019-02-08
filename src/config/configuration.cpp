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

#include <config/configuration.hpp>

#include <base/file_utils.hpp>

#include <cjson/cJSON.h>

#include <algorithm>
#include <sstream>
#include <stdexcept>

namespace bcache {
namespace {
// Various constants.
const std::string ROOT_FOLDER_NAME = ".buildcache";
const std::string CONFIGURATION_FILE_NAME = "config.json";
const int64_t DEFAULT_MAX_CACHE_SIZE = 5368709120L;  // 5 GB

// Configuration options.
std::string s_dir;
std::string s_config_file;
string_list_t s_lua_paths;
std::string s_prefix;
std::string s_remote;
int64_t s_max_cache_size = DEFAULT_MAX_CACHE_SIZE;
int32_t s_debug = -1;
bool s_hard_links = true;
bool s_compress = false;
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

  int64_t as_int64() const {
    return std::stoll(m_value);
  }

  bool as_bool() const {
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

std::string get_config_file(const std::string& dir) {
  return file::append_path(dir, CONFIGURATION_FILE_NAME);
}

void load_from_file(const std::string& file_name) {
  // Load the configuration file.
  if (!file::file_exists(file_name)) {
    // Nothing to do.
    return;
  }
  const auto data = file::read(file_name);

  // Parse the JSON data.
  auto* root = cJSON_Parse(data.data());
  if (root == nullptr) {
    std::ostringstream ss;
    ss << "Configuration file JSON parse error before:\n";
    const auto* json_error = cJSON_GetErrorPtr();
    if (json_error != nullptr) {
      ss << json_error;
    } else {
      ss << "(N/A)";
    }
    throw std::runtime_error(ss.str());
  }

  // Get "lua_paths".
  {
    const auto* node = cJSON_GetObjectItemCaseSensitive(root, "lua_paths");
    cJSON* child_node;
    cJSON_ArrayForEach(child_node, node) {
      const auto str = std::string(child_node->valuestring);
      s_lua_paths += str;
    }
  }

  // Get "prefix".
  {
    const auto* node = cJSON_GetObjectItemCaseSensitive(root, "prefix");
    if (cJSON_IsString(node) && node->valuestring != nullptr) {
      s_prefix = std::string(node->valuestring);
    }
  }

  // Get "remote".
  {
    const auto* node = cJSON_GetObjectItemCaseSensitive(root, "remote");
    if (cJSON_IsString(node) && node->valuestring != nullptr) {
      s_remote = std::string(node->valuestring);
    }
  }

  // Get "max_cache_size".
  {
    const auto* node = cJSON_GetObjectItemCaseSensitive(root, "max_cache_size");
    if (cJSON_IsNumber(node)) {
      s_max_cache_size = static_cast<int64_t>(node->valuedouble);
    }
  }

  // Get "debug".
  {
    const auto* node = cJSON_GetObjectItemCaseSensitive(root, "debug");
    if (cJSON_IsNumber(node)) {
      s_debug = static_cast<int32_t>(node->valueint);
    }
  }

  // Get "hard_links".
  {
    const auto* node = cJSON_GetObjectItemCaseSensitive(root, "hard_links");
    if (cJSON_IsBool(node)) {
      s_hard_links = cJSON_IsTrue(node);
    }
  }

  // Get "compress".
  {
    const auto* node = cJSON_GetObjectItemCaseSensitive(root, "compress");
    if (cJSON_IsBool(node)) {
      s_compress = cJSON_IsTrue(node);
    }
  }

  // Get "perf".
  {
    const auto* node = cJSON_GetObjectItemCaseSensitive(root, "perf");
    if (cJSON_IsBool(node)) {
      s_perf = cJSON_IsTrue(node);
    }
  }

  // Get "disable".
  {
    const auto* node = cJSON_GetObjectItemCaseSensitive(root, "disable");
    if (cJSON_IsBool(node)) {
      s_disable = cJSON_IsTrue(node);
    }
  }

  cJSON_Delete(root);
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

    // Get the Lua paths from the environment.
    // Note: We need do this before loading the configuration file, in order for the environment to
    // have priority over the JSON file.
    {
      const env_var_t lua_path_env("BUILDCACHE_LUA_PATH");
      if (lua_path_env) {
#ifdef _WIN32
        s_lua_paths += string_list_t(lua_path_env.as_string(), ";");
#else
        s_lua_paths += string_list_t(lua_path_env.as_string(), ":");
#endif
      }
    }

    // Load any paramaters from the user configuration file.
    // Note: We do this before reading the configuration from the environment, so that the
    // environment overrides the configuration file.
    s_config_file = get_config_file(s_dir);
    load_from_file(s_config_file);

    // We also look for Lua files in the cache root dir (e.g. ${BUILDCACHE_DIR}/lua).
    // Note: We need do this after loading the configuration file, to give the default Lua path the
    // lowest priority.
    s_lua_paths += file::append_path(s_dir, "lua");

    // Get the command prefix from the environment.
    {
      const env_var_t prefix_env("BUILDCACHE_PREFIX");
      if (prefix_env) {
        s_prefix = prefix_env.as_string();
      }
    }

    // Get the remote cache address from the environment.
    {
      const env_var_t remote_env("BUILDCACHE_REMOTE");
      if (remote_env) {
        s_remote = remote_env.as_string();
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

    // Get the hard_links flag from the environment.
    {
      const env_var_t hard_links_env("BUILDCACHE_HARD_LINKS");
      if (hard_links_env) {
        s_hard_links = hard_links_env.as_bool();
      }
    }

    // Get the compress flag from the environment.
    {
      const env_var_t compress_env("BUILDCACHE_COMPRESS");
      if (compress_env) {
        s_compress = compress_env.as_bool();
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
    throw;
  }
}

const std::string& dir() {
  return s_dir;
}

const std::string& config_file() {
  return s_config_file;
}

const string_list_t& lua_paths() {
  return s_lua_paths;
}

const std::string& prefix() {
  return s_prefix;
}

const std::string& remote() {
  return s_remote;
}

int64_t max_cache_size() {
  return s_max_cache_size;
}

int32_t debug() {
  return s_debug;
}

bool hard_links() {
  return s_hard_links;
}

bool compress() {
  return s_compress;
}

bool perf() {
  return s_perf;
}

bool disable() {
  return s_disable;
}
}  // namespace config
}  // namespace bcache
