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

#ifndef BUILDCACHE_COMPILER_WRAPPER_HPP_
#define BUILDCACHE_COMPILER_WRAPPER_HPP_

#include "arg_list.hpp"
#include "cache.hpp"
#include "file_utils.hpp"

#include <string>

namespace bcache {
class compiler_wrapper_t {
public:
  virtual ~compiler_wrapper_t();

  /// @brief Try to wrap a compiler command.
  /// @param args Command and arguments.
  /// @param[out] return_code The command return code (if handled).
  /// @returns true if the command was recognized and handled.
  bool handle_command(const arg_list_t& args, int& return_code);

protected:
  compiler_wrapper_t(cache_t& cache);

  file::tmp_file_t get_temp_file(const std::string& extension) const;

private:
  virtual std::string preprocess_source(const arg_list_t& args) = 0;
  virtual arg_list_t filter_arguments(const arg_list_t& args) = 0;
  virtual std::string get_compiler_id(const arg_list_t& args) = 0;
  virtual std::string get_object_file(const arg_list_t& args) = 0;

  cache_t& m_cache;
};
}  // namespace bcache

#endif  // BUILDCACHE_COMPILER_WRAPPER_HPP_
