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

#include "gcc_wrapper.hpp"

#include "sys_utils.hpp"

#include <stdexcept>

#ifdef BUILDCACHE_DEBUG_OUTPUT
#include <iostream>
#endif

namespace bcache {
namespace {
bool is_source_file(const std::string& arg) {
  const auto ext = file::get_extension(arg);
  return ((ext == ".cpp") || (ext == ".cc") || (ext == ".cxx") || (ext == ".c"));
}

string_list_t make_preprocessor_cmd(const string_list_t& args,
                                    const std::string& preprocessed_file) {
  string_list_t preprocess_args;

  // Drop arguments that we do not want/need.
  bool drop_next_arg = false;
  for (auto it = args.begin(); it != args.end(); ++it) {
    auto arg = *it;
    auto drop_this_arg = drop_next_arg;
    drop_next_arg = false;
    if (arg == "-c") {
      drop_this_arg = true;
    } else if (arg == "-o") {
      drop_this_arg = true;
      drop_next_arg = true;
    }
    if (!drop_this_arg) {
      preprocess_args += arg;
    }
  }

  // Append the required arguments for producing preprocessed output.
  preprocess_args += std::string("-E");
  preprocess_args += std::string("-P");
  preprocess_args += std::string("-o");
  preprocess_args += preprocessed_file;

  return preprocess_args;
}
}  // namespace

gcc_wrapper_t::gcc_wrapper_t(cache_t& cache) : compiler_wrapper_t(cache) {
}

bool gcc_wrapper_t::can_handle_command(const std::string& compiler_exe) {
  // Is this the right compiler?
  const auto cmd = file::get_file_part(compiler_exe);
  return (cmd.find("gcc") != std::string::npos) || (cmd.find("g++") != std::string::npos) ||
         (cmd.find("clang++") != std::string::npos) || (cmd == "clang");
}

std::string gcc_wrapper_t::preprocess_source(const string_list_t& args) {
  // Are we compiling an object file?
  auto is_object_compilation = false;
  for (auto arg : args) {
    if (arg == std::string("-c")) {
      is_object_compilation = true;
      break;
    }
  }
  if (!is_object_compilation) {
    throw std::runtime_error("Not an object file compilation command.");
  }

  // Run the preprocessor step.
  const auto preprocessed_file = get_temp_file(".i");
  const auto preprocessor_args = make_preprocessor_cmd(args, preprocessed_file.path());
  auto result = sys::run(preprocessor_args);
  if (result.return_code != 0) {
    throw std::runtime_error("Preprocessing command was unsuccessful.");
  }

  // Read and return the preprocessed file.
  const auto preprocessed_source = file::read(preprocessed_file.path());
  if (preprocessed_source.empty()) {
    throw std::runtime_error("Could not read the preprocessed source file.");
  }
  return preprocessed_source;
}

string_list_t gcc_wrapper_t::filter_arguments(const string_list_t& args) {
  string_list_t filtered_args;

  // The first argument is the compiler binary without the path.
  filtered_args += file::get_file_part(args[0]);

  // Note: We always skip the first arg since we have handled it already.
  bool skip_next_arg = true;
  for (auto arg : args) {
    if (!skip_next_arg) {
      // Does this argument specify a file (we don't want to hash those).
      const bool is_arg_plus_file_name =
          ((arg == "-I") || (arg == "-MF") || (arg == "-MT") || (arg == "-MQ") || (arg == "-o"));

      // Generally unwanted argument (things that will not change how we go from preprocessed code
      // to binary object files)?
      const auto first_two_chars = arg.substr(0, 2);
      const bool is_unwanted_arg = ((first_two_chars == "-I") || (first_two_chars == "-D") ||
                                    (first_two_chars == "-M") || is_source_file(arg));

      if (is_arg_plus_file_name) {
        skip_next_arg = true;
      } else if (!is_unwanted_arg) {
        filtered_args += arg;
      }
    } else {
      skip_next_arg = false;
    }
  }

#ifdef BUILDCACHE_DEBUG_OUTPUT
  std::cout << " == Filtered arguments: " << filtered_args.join(" ", true) << "\n";
#endif

  return filtered_args;
}

std::string gcc_wrapper_t::get_compiler_id(const string_list_t& args) {
  // TODO(m): Add things like executable file size too.

  // Get the version string for the compiler.
  string_list_t version_args;
  version_args += args[0];
  version_args += "--version";
  const auto result = sys::run(version_args);
  if (result.return_code != 0) {
    throw std::runtime_error("Unable to get the compiler version information string.");
  }

  return result.stdout;
}

std::string gcc_wrapper_t::get_object_file(const string_list_t& args) {
  for (size_t i = 0u; i < args.size(); ++i) {
    const auto next_idx = i + 1u;
    if ((args[i] == "-o") && (next_idx < args.size())) {
      return args[next_idx];
    }
  }
  throw std::runtime_error("Unable to get the target object file.");
}
}  // namespace bcache
