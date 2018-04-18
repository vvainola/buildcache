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

extern "C" {
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}

#include <stdexcept>

namespace bcache {
namespace {
int panic_handler(lua_State* state) {
  debug::log(debug::FATAL) << "Unprotected error in call to Lua API (" << lua_tostring(state, -1)
                           << ")";
  return 0;  // Return to Lua to abort.
}
}  // namespace

lua_wrapper_t::runner_t::runner_t(const std::string& program_path) : m_program_path(program_path) {
  // Init Lua.
  m_state = luaL_newstate();
  (void)lua_atpanic(m_state, panic_handler);
  luaL_openlibs(m_state);

  // Load the program file.
  if (luaL_loadfile(m_state, program_path.c_str()) != 0) {
    bail("Couldn't load file.");
  }

  // Prime run of the script (define functions etc).
  if (lua_pcall(m_state, 0, 0, 0) != 0) {
    bail("Couldn't run script.");
  }
}

lua_wrapper_t::runner_t::~runner_t() {
  if (m_state != nullptr) {
    lua_close(m_state);
  }
}

void lua_wrapper_t::runner_t::call(const std::string& func, const std::string& arg) {
  lua_getglobal(m_state, func.c_str());
  if (!lua_isfunction(m_state, -1)) {
    debug::log(debug::ERROR) << "Missing Lua function: " << func;
    throw std::runtime_error("Missing Lua function.");
  }
  (void)lua_pushlstring(m_state, arg.c_str(), arg.size());
  if (lua_pcall(m_state, 1, 1, 0) != 0) {
    bail("Lua error");
  }
}

void lua_wrapper_t::runner_t::call(const std::string& func, const string_list_t& args) {
  lua_getglobal(m_state, func.c_str());
  if (!lua_isfunction(m_state, -1)) {
    debug::log(debug::ERROR) << "Missing Lua function: " << func;
    throw std::runtime_error("Missing Lua function.");
  }
  lua_newtable(m_state);
  for (size_t i = 0; i < args.size(); ++i) {
    (void)lua_pushlstring(m_state, args[i].c_str(), args[i].size());
    lua_rawseti(m_state, -2, static_cast<lua_Integer>(i));
  }
  if (lua_pcall(m_state, 1, 1, 0) != 0) {
    bail("Lua error");
  }
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
  debug::log(debug::ERROR) << message << " [" << m_program_path
                           << "]: " << lua_tostring(m_state, -1);
  lua_close(m_state);
  m_state = nullptr;
  throw std::runtime_error(message);
}

lua_State* lua_wrapper_t::runner_t::state() {
  return m_state;
}

const std::string& lua_wrapper_t::runner_t::program_path() const {
  return m_program_path;
}

lua_wrapper_t::lua_wrapper_t(cache_t& cache, const std::string& lua_program_path)
    : compiler_wrapper_t(cache), m_runner(lua_program_path) {
}

bool lua_wrapper_t::can_handle_command(const std::string& compiler_exe,
                                       const std::string& lua_program_path) {
  runner_t runner(lua_program_path);
  auto result = false;
  try {
    runner.call("can_handle_command", compiler_exe);
    result = runner.pop_bool();
  } catch (...) {
    // If anything went wrong when running this function, we can not be trusted to handle this
    // command.
    result = false;
  }
  return result;
}

std::string lua_wrapper_t::preprocess_source(const string_list_t& args) {
  m_runner.call("preprocess_source", args);
  return m_runner.pop_string();
}

string_list_t lua_wrapper_t::filter_arguments(const string_list_t& args) {
  m_runner.call("filter_arguments", args);
  return m_runner.pop_string_list();
}

std::string lua_wrapper_t::get_compiler_id(const string_list_t& args) {
  m_runner.call("get_compiler_id", args);
  return m_runner.pop_string();
}

std::map<std::string, std::string> lua_wrapper_t::get_build_files(const string_list_t& args) {
  m_runner.call("get_build_files", args);
  return m_runner.pop_map();
}
}  // namespace bcache
