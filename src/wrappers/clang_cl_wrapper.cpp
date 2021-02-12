//--------------------------------------------------------------------------------------------------
// Copyright (c) 2021 Marcus Geelnard
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

#include <wrappers/clang_cl_wrapper.hpp>

#include <base/file_utils.hpp>
#include <base/unicode_utils.hpp>

#include <stdexcept>

namespace bcache {
namespace {
// Tick this to a new number if the format has changed in a non-backwards-compatible way.
const std::string HASH_VERSION = "1";
}  // namespace

clang_cl_wrapper_t::clang_cl_wrapper_t(const file::exe_path_t& exe_path, const string_list_t& args)
    : msvc_wrapper_t(exe_path, args) {
}

bool clang_cl_wrapper_t::can_handle_command() {
  // clang-cl may be installed as a symbolic link to clang, so check the virtual path.
  if (lower_case(file::get_file_part(m_exe_path.virtual_path(), false)) == "clang-cl") {
    return true;
  }

  // Also check the real executable path.
  if (lower_case(file::get_file_part(m_exe_path.real_path(), false)) == "clang-cl") {
    return true;
  }

  return false;
}

std::string clang_cl_wrapper_t::get_program_id() {
  // Get the version string for the compiler.
  // Note: Unlike cl.exe, clang-cl requires --version, and returns version information on std_out.
  string_list_t version_args;
  version_args += m_exe_path.virtual_path();
  version_args += "--version";

  const auto result = sys::run(version_args, true);
  if (result.std_out.empty()) {
    throw std::runtime_error("Unable to get the compiler version information string.");
  }

  return HASH_VERSION + result.std_out;
}

}  // namespace bcache
