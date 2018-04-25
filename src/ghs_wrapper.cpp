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
#include "unicode_utils.hpp"

#include <stdexcept>

namespace bcache {
ghs_wrapper_t::ghs_wrapper_t(const string_list_t& args, cache_t& cache)
    : gcc_wrapper_t(args, cache) {
}

bool ghs_wrapper_t::can_handle_command() {
  // Is this the right compiler?
  const auto cmd = lower_case(file::get_file_part(m_args[0], false));
  return (cmd.find("ccarm") != std::string::npos) || (cmd.find("cxarm") != std::string::npos) ||
         (cmd.find("ccthumb") != std::string::npos) || (cmd.find("cxthumb") != std::string::npos) ||
         (cmd.find("ccintarm") != std::string::npos) || (cmd.find("cxintarm") != std::string::npos);
}

std::map<std::string, std::string> ghs_wrapper_t::get_relevant_env_vars() {
  // TODO(m): What environment variables can affect the build result?
  std::map<std::string, std::string> env_vars;
  return env_vars;
}

std::string ghs_wrapper_t::get_program_id() {
  // Getting a version string from the GHS compiler by passing "--version" is less than trivial. For
  // instance you need to pass valid -bsp and -os_dir arguments, and a dummy source file that does
  // not exist (otherwise it will fail or actually perform the compilation), and then the output is
  // sent to stderr instead of stdout.

  // Instead, we fall back to the default program ID logic.
  return program_wrapper_t::get_program_id();
}
}  // namespace bcache
