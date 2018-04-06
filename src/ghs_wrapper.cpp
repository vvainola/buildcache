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

#include "ghs_wrapper.hpp"

#include "file_utils.hpp"
#include "hasher.hpp"

#include <stdexcept>

namespace bcache {
ghs_wrapper_t::ghs_wrapper_t(cache_t& cache) : gcc_wrapper_t(cache) {
}

bool ghs_wrapper_t::can_handle_command(const std::string& compiler_exe) {
  // Is this the right compiler?
  const auto cmd = file::get_file_part(compiler_exe);
  return (cmd.find("ccarm") != std::string::npos) || (cmd.find("cxarm") != std::string::npos) ||
         (cmd.find("ccthumb") != std::string::npos) || (cmd.find("cxthumb") != std::string::npos) ||
         (cmd.find("ccintarm") != std::string::npos) || (cmd.find("cxintarm") != std::string::npos);
}

std::string ghs_wrapper_t::get_compiler_id(const string_list_t& args) {
  // Getting a version string from the GHS compiler by passing "--version" is less than trivial. For
  // instance you need to pass valid -bsp and -os_dir arguments, and a dummy source file that does
  // not exist (otherwise it will fail or actually perform the compilation), and then the output is
  // sent to stderr instead of stdout.

  // Instead, we just take a hash of the compiler binary.
  const auto& compiler_exe = args[0];
  hasher_t hasher;
  if (hasher.update_from_file(compiler_exe)) {
    return hasher.final().as_string();
  }
  throw std::runtime_error("Unable to hash the compiler executable file.");
}
}  // namespace bcache
