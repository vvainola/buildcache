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

#include "lua_wrapper.hpp"

#include "debug_utils.hpp"
#include "perf_utils.hpp"

extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

#include <stdexcept>

namespace bcache {
namespace {
int panic_handler(lua_State* state) {
  debug::log(debug::FATAL) << "Unprotected error in call to Lua API (" << lua_tostring(state, -1)
                           << ")";
  return 0;  // Return to Lua to abort.
}

int l_require_std(lua_State* state) {
  // Get the module name from the stack.
  luaL_checkstring(state, 1);
  size_t arg_len;
  const auto* arg = lua_tolstring(state, 1, &arg_len);
  const auto module_name = std::string(arg, arg_len);

  // Load the selected standard library module.
  if (module_name == "package") {
    luaL_requiref(state, module_name.c_str(), luaopen_package, 1);
  } else if (module_name == "coroutine") {
    luaL_requiref(state, module_name.c_str(), luaopen_coroutine, 1);
  } else if (module_name == "table") {
    luaL_requiref(state, module_name.c_str(), luaopen_table, 1);
  } else if (module_name == "io") {
    luaL_requiref(state, module_name.c_str(), luaopen_io, 1);
  } else if (module_name == "os") {
    luaL_requiref(state, module_name.c_str(), luaopen_os, 1);
  } else if (module_name == "string") {
    luaL_requiref(state, module_name.c_str(), luaopen_string, 1);
  } else if (module_name == "math") {
    luaL_requiref(state, module_name.c_str(), luaopen_math, 1);
  } else if (module_name == "utf8") {
    luaL_requiref(state, module_name.c_str(), luaopen_utf8, 1);
  } else if (module_name == "debug") {
    luaL_requiref(state, module_name.c_str(), luaopen_debug, 1);
  } else if (module_name == "*") {
    luaL_openlibs(state);
    return 0;
  } else {
    return luaL_error(state, "Invalid standard library: \"%s\".", module_name.c_str());
  }

  return 1;  // Number of results.
}

void load_default_libs(lua_State* state) {
  // To minimize startup overhead when loading Lua scripts, we only pre-load a minimal environment.
  // Standard libraries can be loaded by Lua scripts on an op-in basis using the require_std()
  // function.

  // Load the basic library.
  luaL_requiref(state, "_G", luaopen_base, 1);
  lua_pop(state, 1);

  // We provide a custom function, require_std(), that can be used for loading standard libraries.
  lua_pushcfunction(state, l_require_std);
  lua_setglobal(state, "require_std");
}
}  // namespace

lua_wrapper_t::runner_t::runner_t(const std::string& script_path) : m_script_path(script_path) {
  // Init Lua.
  PERF_START(LUA_INIT);
  m_state = luaL_newstate();
  (void)lua_atpanic(m_state, panic_handler);
  load_default_libs(m_state);
  PERF_STOP(LUA_INIT);

  // Load the program file.
  {
    PERF_START(LUA_LOAD_SCRIPT);
    const auto success = (luaL_loadfile(m_state, script_path.c_str()) == 0);
    PERF_STOP(LUA_LOAD_SCRIPT);
    if (!success) {
      bail("Couldn't load file.");
    }
  }

  // Prime run of the script (define functions etc).
  {
    PERF_START(LUA_RUN);
    const auto success = (lua_pcall(m_state, 0, 0, 0) == 0);
    PERF_STOP(LUA_RUN);
    if (!success) {
      bail("Couldn't run script.");
    }
  }
}

lua_wrapper_t::runner_t::~runner_t() {
  if (m_state != nullptr) {
    lua_close(m_state);
  }
}

bool lua_wrapper_t::runner_t::call(const std::string& func) {
  lua_getglobal(m_state, func.c_str());
  if (!lua_isfunction(m_state, -1)) {
    debug::log(debug::ERROR) << "Missing Lua function: " << func;
    return false;
  }
  PERF_START(LUA_RUN);
  const auto success = (lua_pcall(m_state, 0, 1, 0) == 0);
  PERF_STOP(LUA_RUN);
  if (!success) {
    bail("Lua error");
  }
  return true;
}

bool lua_wrapper_t::runner_t::call(const std::string& func, const std::string& arg) {
  lua_getglobal(m_state, func.c_str());
  if (!lua_isfunction(m_state, -1)) {
    debug::log(debug::ERROR) << "Missing Lua function: " << func;
    return false;
  }
  (void)lua_pushlstring(m_state, arg.c_str(), arg.size());
  PERF_START(LUA_RUN);
  const auto success = (lua_pcall(m_state, 1, 1, 0) == 0);
  PERF_STOP(LUA_RUN);
  if (!success) {
    bail("Lua error");
  }
  return true;
}

bool lua_wrapper_t::runner_t::call(const std::string& func, const string_list_t& args) {
  lua_getglobal(m_state, func.c_str());
  if (!lua_isfunction(m_state, -1)) {
    debug::log(debug::DEBUG) << "Missing Lua function: " << func;
    return false;
  }
  lua_newtable(m_state);
  for (size_t i = 0; i < args.size(); ++i) {
    (void)lua_pushlstring(m_state, args[i].c_str(), args[i].size());
    lua_rawseti(m_state, -2, static_cast<lua_Integer>(i));
  }
  PERF_START(LUA_RUN);
  const auto success = (lua_pcall(m_state, 1, 1, 0) == 0);
  PERF_STOP(LUA_RUN);
  if (!success) {
    bail("Lua error");
  }
  return true;
}

bool lua_wrapper_t::runner_t::pop_bool() {
  if (lua_isboolean(m_state, -1) == 0) {
    throw std::runtime_error("Expected a boolean value on the stack.");
  }
  return (lua_toboolean(m_state, -1) != 0);
}

std::string lua_wrapper_t::runner_t::pop_string(bool keep_value_on_the_stack) {
  if (lua_isstring(m_state, -1) == 0) {
    throw std::runtime_error("Expected a string value on the stack.");
  }
  size_t str_size;
  const auto* str = lua_tolstring(m_state, -1, &str_size);
  if (!keep_value_on_the_stack) {
    lua_pop(m_state, 1);
  }
  return std::string(str, str_size);
}

string_list_t lua_wrapper_t::runner_t::pop_string_list(bool keep_value_on_the_stack) {
  if (lua_istable(m_state, -1) == 0) {
    throw std::runtime_error("Expected a table on the stack.");
  }
  string_list_t result;
  const auto len = lua_rawlen(m_state, -1);
  for (size_t i = 0; i < len; ++i) {
    lua_rawgeti(m_state, -1, static_cast<int>(i) + 1);
    result += pop_string();
  }
  if (!keep_value_on_the_stack) {
    lua_pop(m_state, 1);
  }
  return result;
}

std::map<std::string, std::string> lua_wrapper_t::runner_t::pop_map(bool keep_value_on_the_stack) {
  if (lua_istable(m_state, -1) == 0) {
    throw std::runtime_error("Expected a table on the stack.");
  }
  std::map<std::string, std::string> result;
  lua_pushnil(m_state);  // Make sure lua_next starts at beginning.
  while (lua_next(m_state, -2) != 0) {
    const auto value = pop_string();
    const auto key = pop_string(true);
    result[key] = value;
  }
  if (!keep_value_on_the_stack) {
    lua_pop(m_state, 1);
  }
  return result;
}

[[noreturn]] void lua_wrapper_t::runner_t::bail(const std::string& message) {
  debug::log(debug::ERROR) << lua_tostring(m_state, -1);
  lua_close(m_state);
  m_state = nullptr;
  throw std::runtime_error(message);
}

lua_wrapper_t::lua_wrapper_t(const string_list_t& args,
                             cache_t& cache,
                             const std::string& lua_script_path)
    : program_wrapper_t(args, cache), m_runner(lua_script_path) {
}

bool lua_wrapper_t::can_handle_command(const std::string& program_exe,
                                       const std::string& lua_script_path) {
  runner_t runner(lua_script_path);
  auto result = false;
  try {
    if (runner.call("can_handle_command", program_exe)) {
      result = runner.pop_bool();
    }
  } catch (...) {
    // If anything went wrong when running this function, we can not be trusted to handle this
    // command.
    result = false;
  }
  return result;
}

std::string lua_wrapper_t::preprocess_source() {
  if (m_runner.call("preprocess_source", m_args)) {
    return m_runner.pop_string();
  } else {
    return program_wrapper_t::preprocess_source();
  }
}

string_list_t lua_wrapper_t::get_relevant_arguments() {
  if (m_runner.call("get_relevant_arguments", m_args)) {
    return m_runner.pop_string_list();
  } else {
    return program_wrapper_t::get_relevant_arguments();
  }
}

std::map<std::string, std::string> lua_wrapper_t::get_relevant_env_vars() {
  if (m_runner.call("get_relevant_env_vars")) {
    return m_runner.pop_map();
  } else {
    return program_wrapper_t::get_relevant_env_vars();
  }
}

std::string lua_wrapper_t::get_program_id() {
  if (m_runner.call("get_program_id", m_args)) {
    return m_runner.pop_string();
  } else {
    return program_wrapper_t::get_program_id();
  }
}

std::map<std::string, std::string> lua_wrapper_t::get_build_files() {
  if (m_runner.call("get_build_files", m_args)) {
    return m_runner.pop_map();
  } else {
    return program_wrapper_t::get_build_files();
  }
}
}  // namespace bcache
