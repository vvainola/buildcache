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

#include <wrappers/qcc_wrapper.hpp>

#include <base/env_utils.hpp>
#include <base/string_list.hpp>
#include <base/unicode_utils.hpp>

#include <regex>

namespace bcache {
namespace {
// Tick this to a new number if the format has changed in a non-backwards-compatible way.
const std::string HASH_VERSION = "1";
}  // namespace

qcc_wrapper_t::qcc_wrapper_t(const file::exe_path_t& exe_path, const string_list_t& args)
    : gcc_wrapper_t(exe_path, args) {
}

bool qcc_wrapper_t::can_handle_command() {
  // Is this the right compiler?
  const auto cmd = lower_case(file::get_file_part(m_exe_path.real_path(), false));
  return (cmd == "qcc") || (cmd == "q++");
}

string_list_t qcc_wrapper_t::get_capabilities() {
  // Unlike the gcc wrapper, we don't support direct mode. It seems that while qcc accepts the -H
  // flag, no results are returned on stderr (perhaps the qcc wrapper consumes the stderr output?).
  return string_list_t{};
}

std::map<std::string, expected_file_t> qcc_wrapper_t::get_build_files() {
  // Check for arguments that we do not support.
  for (const auto& arg : m_resolved_args) {
    if (arg == "-set-default") {
      throw std::runtime_error("We can't reproduce -set-default from a cached entry.");
    }
  }

  // Use the gcc method to get the actual build files.
  return gcc_wrapper_t::get_build_files();
}

std::string qcc_wrapper_t::get_program_id() {
  // Get some version information for the compiler. Unfortunately qcc does not have a "--version"
  // option or similar, so what we do here is to list the available targets and their toolchain
  // (gcc) versions.
  string_list_t version_args;
  version_args += m_args[0];
  version_args += "-V";
  const auto result = sys::run(version_args);
  if (result.return_code != 0) {
    throw std::runtime_error("Unable to get the compiler version information string.");
  }

  // Filter the output.
  const string_list_t lines(result.std_err, "\n");
  string_list_t filtered_lines;
  const std::regex exclude_re("cc: targets available in");
  for (const auto& line : lines) {
    if (!std::regex_search(line, exclude_re)) {
      filtered_lines += line;
    }
  }

  // Prepend the hash format version.
  auto id = HASH_VERSION + filtered_lines.join("\n");
  return id;
}

std::map<std::string, std::string> qcc_wrapper_t::get_relevant_env_vars() {
  // qcc uses gcc under the hood, so start with the env vars that are relevant to gcc.
  auto env_vars = gcc_wrapper_t::get_relevant_env_vars();

  // Append the env vars that affect qcc behaviour.
  static const std::string QCC_ENV_VARS[] = {"QNX_HOST", "QNX_TARGET", "QCC_CONF_PATH"};
  for (const auto& key : QCC_ENV_VARS) {
    env_var_t env(key);
    if (env) {
      env_vars[key] = env.as_string();
    }
  }

  return env_vars;
}
}  // namespace bcache
