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
lua_wrapper_t::runner_t::runner_t(const std::string& program_path) : m_program_path(program_path) {
  // Init Lua.
  m_state = luaL_newstate();
  luaL_openlibs(m_state);
  const auto status = luaL_loadfile(m_state, program_path.c_str());
  if (status) {
    // If something went wrong, error message is at the top of the stack.
    debug::log(debug::ERROR) << "Couldn't load file: " << lua_tostring(m_state, -1);

    lua_close(m_state);
    m_state = nullptr;

    throw std::runtime_error("Failed to load Lua program.");
  }
}

lua_wrapper_t::runner_t::~runner_t() {
  if (m_state != nullptr) {
    lua_close(m_state);
  }
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

  // TODO(m): Implement me!
  (void)compiler_exe;
  return false;
}

std::string lua_wrapper_t::preprocess_source(const string_list_t& args) {
  // TODO(m): Implement me!
  (void)args;
  throw std::runtime_error("Not yet implemented.");
}

string_list_t lua_wrapper_t::filter_arguments(const string_list_t& args) {
  // TODO(m): Implement me!
  (void)args;
  throw std::runtime_error("Not yet implemented.");
}

std::string lua_wrapper_t::get_compiler_id(const string_list_t& args) {
  // TODO(m): Implement me!
  (void)args;
  throw std::runtime_error("Not yet implemented.");
}

std::map<std::string, std::string> lua_wrapper_t::get_build_files(const string_list_t& args) {
  // TODO(m): Implement me!
  (void)args;
  throw std::runtime_error("Not yet implemented.");
}
}  // namespace bcache
