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

#include "arg_list.hpp"
#include "cache.hpp"
#include "sys_utils.hpp"

#include <iostream>
#include <string>

namespace {
[[noreturn]] void clear_cache_and_exit() {
  bcache::cache_t cache;
  cache.clear();
  std::exit(0);
}

[[noreturn]] void show_stats_and_exit() {
  bcache::cache_t cache;
  cache.show_stats();
  std::exit(0);
}

[[noreturn]] void print_version_and_exit() {
  std::cout << "BuildCache version 0.0-dev\n";
  std::exit(0);
}

[[noreturn]] void set_max_size_and_exit(const char* size_arg) {
  // TODO(m): Implement me!
  (void)size_arg;
  std::cout << "*** Setting the max size has not yet implemented\n";
  std::exit(0);
}

class gcc_wrapper {
public:
  /// @brief Try to wrap a compiler command.
  /// @param args Command and arguments.
  /// @param[out] return_code The command return code (if handled).
  /// @returns true if the command was recognized and handled.
  static bool handle_command(const bcache::arg_list_t& args, int& return_code) {
    // Is this the right compiler?
    if ((args[0].find("gcc") == std::string::npos) && (args[0].find("g++") == std::string::npos)) {
      return false;
    }

    // Are we compiling an object file?
    auto is_object_compilation = false;
    for (auto arg : args) {
      if (arg == std::string("-c")) {
        is_object_compilation = true;
        break;
      }
    }
    if (!is_object_compilation) {
      return false;
    }

    // Run the preprocessor step.
    // TODO(m): Implement me!
    (void)return_code;

    return false;
  }
};

[[noreturn]] void wrap_compiler_and_exit(int argc, const char** argv) {
  auto args = bcache::arg_list_t(argc, argv);

  // Try different compilers.
  auto handled = false;
  int return_code = 1;
  if (!handled) {
    handled = gcc_wrapper::handle_command(args, return_code);
  }

  // Fall back to running the command as is.
  if (!handled) {
    auto result = bcache::sys::run(args);
    return_code = result.return_code;
  }

  std::exit(return_code);
}

bool compare_arg(const std::string& arg,
                 const std::string& short_form,
                 const std::string& long_form = "") {
  return (arg == short_form) || (arg == long_form);
}

void print_help(const char* program_name) {
  std::cout << "Usage:\n";
  std::cout << "    " << program_name << " [options]\n";
  std::cout << "    " << program_name << " compiler [compiler-options]\n";
  std::cout << "\n";
  std::cout << "Options:\n";
  std::cout << "    -C, --clear           clear the cache completely (except configuration)\n";
  std::cout << "    -M, --max-size SIZE   set maximum size of cache to SIZE (use 0 for no\n";
  std::cout << "                          limit); available suffixes: k, M, G, T (decimal) and\n";
  std::cout << "                          Ki, Mi, Gi, Ti (binary); default suffix: G\n";
  std::cout << "    -s, --show-stats      show statistics summary\n";
  std::cout << "\n";
  std::cout << "    -h, --help            print this help text\n";
  std::cout << "    -V, --version         print version and copyright information\n";
}
}  // namespace

int main(int argc, const char** argv) {
  if (argc < 2) {
    print_help(argv[0]);
    std::exit(1);
  }

  // Check if we are running any gcache commands.
  const std::string arg_str(argv[1]);
  if (compare_arg(arg_str, "-C", "--clear")) {
    clear_cache_and_exit();
  } else if (compare_arg(arg_str, "-s", "--show-stats")) {
    show_stats_and_exit();
  } else if (compare_arg(arg_str, "-V", "--version")) {
    print_version_and_exit();
  } else if (compare_arg(arg_str, "-M", "--max-size")) {
    if (argc < 3) {
      std::cerr << argv[0] << ": option requires an argument -- " << arg_str << "\n";
      print_help(argv[0]);
      std::exit(1);
    }
    set_max_size_and_exit(argv[2]);
  } else if (compare_arg(arg_str, "-h", "--help")) {
    print_help(argv[0]);
    std::exit(0);
  } else if (arg_str[0] == '-') {
    std::cerr << argv[0] << ": invalid option -- " << arg_str << "\n";
    print_help(argv[0]);
    std::exit(1);
  }

  // We got this far, so we're running as a compiler wrapper.
  wrap_compiler_and_exit(argc - 1, &argv[1]);
}
