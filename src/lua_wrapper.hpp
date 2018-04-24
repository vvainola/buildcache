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

#ifndef BUILDCACHE_LUA_WRAPPER_HPP_
#define BUILDCACHE_LUA_WRAPPER_HPP_

#include "program_wrapper.hpp"

extern "C" {
typedef struct lua_State lua_State;
}

namespace bcache {
class lua_wrapper_t : public program_wrapper_t {
public:
  lua_wrapper_t(const string_list_t& args, cache_t& cache, const std::string& lua_script_path);

  static bool can_handle_command(const std::string& program_exe,
                                 const std::string& lua_script_path);

private:
  // A helper class for managing the Lua state.
  class runner_t {
  public:
    runner_t(const std::string& script_path);
    ~runner_t();

    bool call(const std::string& func);
    bool call(const std::string& func, const std::string& arg);
    bool call(const std::string& func, const string_list_t& arg);

    bool pop_bool();
    std::string pop_string(bool keep_value_on_the_stack = false);
    string_list_t pop_string_list(bool keep_value_on_the_stack = false);
    std::map<std::string, std::string> pop_map(bool keep_value_on_the_stack = false);

  private:
    void init_lua_state();
    void setup_lua_libs_and_globals();
    [[noreturn]] void bail(const std::string& message);

    lua_State* m_state = nullptr;
    std::string m_script_path;
  };

  std::string preprocess_source() override;
  string_list_t get_relevant_arguments() override;
  std::map<std::string, std::string> get_relevant_env_vars() override;
  std::string get_program_id() override;
  std::map<std::string, std::string> get_build_files() override;

  runner_t m_runner;
};
}  // namespace bcache
#endif  // BUILDCACHE_LUA_WRAPPER_HPP_
