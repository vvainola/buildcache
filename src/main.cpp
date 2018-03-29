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

#include <iostream>
#include <string>

namespace bcache {
void clear_cache() {
  // TODO(m): Implement me!
  std::cout << "*** Not yet implemented ***\n";
}

void show_stats() {
  // TODO(m): Implement me!
  std::cout << "*** Not yet implemented ***\n";
}

void print_version() {
  // TODO(m): Implement me!
  std::cout << "*** Not yet implemented ***\n";
}

void set_max_size(const char* size_arg) {
  // TODO(m): Implement me!
  (void)size_arg;
  std::cout << "*** Not yet implemented ***\n";
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

[[noreturn]] void wrap_compiler(int argc, const char** argv) {
  // TODO(m): Implement me!
  std::cout << "*** Not yet implemented ***\n";
  auto args = make_arg_list(argc, argv);
  std::exit(1);
}
}  // namespace bcache

namespace {
bool compare_arg(
    const std::string& arg, const std::string& short_form, const std::string& long_form = "") {
  return (arg == short_form) || (arg == long_form);
}
}  // namespace

int main(int argc, const char** argv) {
  if (argc < 2) {
    bcache::print_help(argv[0]);
    std::exit(1);
  }

  // Check if we are running any gcache commands.
  const std::string arg_str(argv[1]);
  if (compare_arg(arg_str, "-C", "--clear")) {
    bcache::clear_cache();
    std::exit(0);
  } else if (compare_arg(arg_str, "-s", "--show-stats")) {
    bcache::show_stats();
    std::exit(0);
  } else if (compare_arg(arg_str, "-V", "--version")) {
    bcache::print_version();
    std::exit(0);
  } else if (compare_arg(arg_str, "-M", "--max-size")) {
    if (argc < 3) {
      std::cerr << argv[0] << ": option requires an argument -- " << arg_str << "\n";
      bcache::print_help(argv[0]);
      std::exit(1);
    }
    bcache::set_max_size(argv[2]);
    std::exit(0);
  } else if (compare_arg(arg_str, "-h", "--help")) {
    bcache::print_help(argv[0]);
    std::exit(0);
  } else if (arg_str[0] == '-') {
    std::cerr << argv[0] << ": invalid option -- " << arg_str << "\n";
    bcache::print_help(argv[0]);
    std::exit(1);
  }

  // We got this far, so we're running as a compiler wrapper.
  bcache::wrap_compiler(argc - 1, &argv[1]);
}
