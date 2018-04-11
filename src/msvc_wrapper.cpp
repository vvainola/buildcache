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

#include "msvc_wrapper.hpp"

#include "debug_utils.hpp"
#include "sys_utils.hpp"

#include <stdexcept>

namespace bcache {
namespace {
bool is_source_file(const std::string& arg) {
  const auto ext = file::get_extension(arg);
  return ((ext == ".cpp") || (ext == ".cc") || (ext == ".cxx") || (ext == ".c"));
}

bool arg_starts_with(const std::string& str, const std::string& sub_str) {
  const auto size = sub_str.size();
  const auto is_flag = (size >= 1) && ((str[0] == '/') || (str[0] == '-'));
  return is_flag && ((str.size() >= (size + 1)) && (str.substr(1, size) == sub_str));
}

bool arg_equals(const std::string& str, const std::string& sub_str) {
  const auto size = sub_str.size();
  const auto is_flag = (size >= 1) && ((str[0] == '/') || (str[0] == '-'));
  return is_flag && ((str.size() >= (size + 1)) && (str.substr(1) == sub_str));
}

string_list_t make_preprocessor_cmd(const string_list_t& args) {
  string_list_t preprocess_args;

  // Drop arguments that we do not want/need.
  bool drop_next_arg = false;
  for (auto it = args.begin(); it != args.end(); ++it) {
    auto arg = *it;
    auto drop_this_arg = drop_next_arg;
    drop_next_arg = false;
    if (arg_equals(arg, "c") || arg_starts_with(arg, "Fo") || arg_equals(arg, "C") ||
        arg_equals(arg, "E")) {
      drop_this_arg = true;
    }
    if (!drop_this_arg) {
      preprocess_args += arg;
    }
  }

  // Append the required arguments for producing preprocessed output.
  preprocess_args += std::string("/EP");

  return preprocess_args;
}
}  // namespace

msvc_wrapper_t::msvc_wrapper_t(cache_t& cache) : compiler_wrapper_t(cache) {
}

bool msvc_wrapper_t::can_handle_command(const std::string& compiler_exe) {
  // Is this the right compiler?
  const auto cmd = file::get_file_part(compiler_exe, false);
  return (cmd == "cl");
}

std::string msvc_wrapper_t::preprocess_source(const string_list_t& args) {
  // Check if this is a compilation command that we support.
  auto is_object_compilation = false;
  auto has_object_output = false;
  for (const auto& arg : args) {
    if (arg_equals(arg, "c")) {
      is_object_compilation = true;
    } else if (arg_starts_with(arg, "Fo") && (file::get_extension(arg) == ".obj")) {
      has_object_output = true;
    } else if (arg_equals(arg, "Zi") || arg_equals(arg, "ZI")) {
      throw std::runtime_error("PDB generation is not supported.");
    } else if (arg.substr(0, 1) == "@") {
      throw std::runtime_error("Response files are currently not supported.");
    }
  }
  if ((!is_object_compilation) || (!has_object_output)) {
    throw std::runtime_error("Unsupported complation command.");
  }

  // Run the preprocessor step.
  const auto preprocessor_args = make_preprocessor_cmd(args);
  auto result = sys::run(preprocessor_args);
  if (result.return_code != 0) {
    throw std::runtime_error("Preprocessing command was unsuccessful.");
  }

  // Return the preprocessed file (from stdout).
  return result.std_out;
}

string_list_t msvc_wrapper_t::filter_arguments(const string_list_t& args) {
  string_list_t filtered_args;

  // The first argument is the compiler binary without the path.
  filtered_args += file::get_file_part(args[0]);

  // Note: We always skip the first arg since we have handled it already.
  bool skip_next_arg = true;
  for (const auto& arg : args) {
    if (!skip_next_arg) {
      // Generally unwanted argument (things that will not change how we go from preprocessed code
      // to binary object files)?
      const auto first_two_chars = arg.substr(0, 2);
      const bool is_unwanted_arg = ((arg_equals(first_two_chars, "F") && !arg_equals(arg, "F")) ||
                                    arg_equals(first_two_chars, "I") ||
                                    arg_equals(first_two_chars, "D") || is_source_file(arg));

      if (!is_unwanted_arg) {
        filtered_args += arg;
      }
    } else {
      skip_next_arg = false;
    }
  }

  debug::log(debug::DEBUG) << "Filtered arguments: " << filtered_args.join(" ", true);

  return filtered_args;
}

std::string msvc_wrapper_t::get_compiler_id(const string_list_t& args) {
  // TODO(m): Add things like executable file size too.

  // Get the version string for the compiler.
  string_list_t version_args;
  version_args += args[0];
  version_args += "2>&1";  // The version information is given on stderr.
  const auto result = sys::run(version_args);
  if (result.std_out.empty()) {
    throw std::runtime_error("Unable to get the compiler version information string.");
  }

  return result.std_out;
}

std::string msvc_wrapper_t::get_object_file(const string_list_t& args) {
  for (const auto& arg : args) {
    if (arg_starts_with(arg, "Fo") && (file::get_extension(arg) == ".obj")) {
      return arg.substr(3);
    }
  }
  throw std::runtime_error("Unable to get the target object file.");
}
}  // namespace bcache
