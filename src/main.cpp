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

#include "cache.hpp"
#include "debug_utils.hpp"
#include "gcc_wrapper.hpp"
#include "ghs_wrapper.hpp"
#include "lua_wrapper.hpp"
#include "msvc_wrapper.hpp"
#include "perf_utils.hpp"
#include "program_wrapper.hpp"
#include "string_list.hpp"
#include "sys_utils.hpp"
#include "unicode_utils.hpp"

#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

namespace {
// The name of the BuildCache executable (excluding the file extension).
const std::string BUILDCACHE_EXE_NAME = "buildcache";

bcache::string_list_t get_lua_paths(const bcache::cache_t& cache) {
  bcache::string_list_t paths;

  // The BUILDCACHE_LUA_PATH env variable can contain a colon-separated list of paths.
  {
    const auto* lua_path_env = std::getenv("BUILDCACHE_LUA_PATH");
    if (lua_path_env != nullptr) {
#ifdef _WIN32
      paths += bcache::string_list_t(std::string(lua_path_env), ";");
#else
      paths += bcache::string_list_t(std::string(lua_path_env), ":");
#endif
    }
  }

  // We also look for Lua files in the cache root dir (e.g. ${BUILDCACHE_DIR}/lua).
  paths += bcache::file::append_path(cache.root_folder(), "lua");

  return paths;
}

bool is_lua_script(const std::string& script_path) {
  return (bcache::lower_case(bcache::file::get_extension(script_path)) == ".lua");
}

std::unique_ptr<bcache::program_wrapper_t> find_suitable_wrapper(bcache::cache_t& cache,
                                                                 const std::string& true_exe_path) {
  std::unique_ptr<bcache::program_wrapper_t> wrapper;

  // Try Lua wrappers first (so you can override internal wrappers).
  // Iterate over the existing Lua paths.
  for (const auto& lua_root_dir : get_lua_paths(cache)) {
    if (bcache::file::dir_exists(lua_root_dir)) {
      // Find all .lua files in the given directory.
      const auto lua_files = bcache::file::walk_directory(lua_root_dir);
      for (const auto& file_info : lua_files) {
        const auto script_path = file_info.path();
        if ((!file_info.is_dir()) && is_lua_script(script_path)) {
          // Check if the given wrapper can handle this command (first match wins).
          if (bcache::lua_wrapper_t::can_handle_command(true_exe_path, script_path)) {
            bcache::debug::log(bcache::debug::DEBUG)
                << "Found matching Lua wrapper for " << true_exe_path << ": " << script_path;
            wrapper.reset(new bcache::lua_wrapper_t(cache, script_path));
            break;
          }
        }
      }
    }
    if (wrapper) {
      break;
    }
  }

  // If no Lua wrappers were found, try built in wrappers.
  if (!wrapper) {
    if (bcache::gcc_wrapper_t::can_handle_command(true_exe_path)) {
      wrapper.reset(new bcache::gcc_wrapper_t(cache));
    } else if (bcache::ghs_wrapper_t::can_handle_command(true_exe_path)) {
      wrapper.reset(new bcache::ghs_wrapper_t(cache));
    } else if (bcache::msvc_wrapper_t::can_handle_command(true_exe_path)) {
      wrapper.reset(new bcache::msvc_wrapper_t(cache));
    }
  }

  return wrapper;
}

[[noreturn]] void clear_cache_and_exit() {
  int return_code = 0;
  try {
    bcache::cache_t cache;
    cache.clear();
  } catch (const std::exception& e) {
    std::cerr << "*** Unexpected error: " << e.what() << "\n";
    return_code = 1;
  } catch (...) {
    std::cerr << "*** Unexpected error.\n";
    return_code = 1;
  }
  std::exit(return_code);
}

[[noreturn]] void show_stats_and_exit() {
  int return_code = 0;
  try {
    bcache::cache_t cache;
    cache.show_stats();
  } catch (const std::exception& e) {
    std::cerr << "*** Unexpected error: " << e.what() << "\n";
    return_code = 1;
  } catch (...) {
    std::cerr << "*** Unexpected error.\n";
    return_code = 1;
  }
  std::exit(return_code);
}

[[noreturn]] void print_version_and_exit() {
  std::cout << "BuildCache version 0.1-dev\n";
  std::exit(0);
}

[[noreturn]] void set_max_size_and_exit(const char* size_arg) {
  int return_code = 0;
  try {
    // TODO(m): Implement me!
    (void)size_arg;
    std::cout << "*** Setting the max size has not yet implemented\n";
  } catch (const std::exception& e) {
    std::cerr << "*** Unexpected error: " << e.what() << "\n";
    return_code = 1;
  } catch (...) {
    std::cerr << "*** Unexpected error.\n";
    return_code = 1;
  }
  std::exit(return_code);
}

[[noreturn]] void wrap_compiler_and_exit(int argc, const char** argv) {
  auto args = bcache::string_list_t(argc, argv);
  bool was_wrapped = false;
  int return_code = 0;

  try {
    if (args.size() < 1) {
      // Should never happen.
      throw std::runtime_error("Missing arguments.");
    }

    // Find the true path to the executable file. This affects things like if we can match the
    // compiler name or not, and what version string we get. We also want to avoid incorrectly
    // identifying other compiler accelerators (e.g. ccache) as actual compilers.
    // TODO(m): This call may throw an excepption, which currently means that we will not even try
    // to run the original command. At the same time this is a protection against endless symlink
    // recursion. Figure something out!
    PERF_START(FIND_EXECUTABLE);
    const auto true_exe_path = bcache::file::find_executable(args[0], BUILDCACHE_EXE_NAME);
    PERF_STOP(FIND_EXECUTABLE);

    // Replace the command with the true exe path. Most of the following operations rely on having
    // a correct executable path. Also, this is important to avoid recursions when we are invoked
    // from a symlink, for instance.
    args[0] = true_exe_path;

    try {
      return_code = 1;

      // Initialize a cache object.
      bcache::cache_t cache;

      // Select a matching compiler wrapper.
      PERF_START(FIND_WRAPPER);
      auto wrapper = find_suitable_wrapper(cache, true_exe_path);
      PERF_STOP(FIND_WRAPPER);

      // Run the wrapper, if any.
      if (wrapper) {
        was_wrapped = wrapper->handle_command(args, return_code);
      } else {
        bcache::debug::log(bcache::debug::INFO) << "No suitable wrapper for " << true_exe_path;
      }
    } catch (const std::exception& e) {
      bcache::debug::log(bcache::debug::ERROR) << "Unexpected error: " << e.what();
      return_code = 1;
    } catch (...) {
      bcache::debug::log(bcache::debug::ERROR) << "Unexpected error.";
      return_code = 1;
    }

    // Fall back to running the command as is.
    if (!was_wrapped) {
      PERF_START(RUN_FOR_FALLBACK);
      auto result = bcache::sys::run_with_prefix(args, false);
      PERF_STOP(RUN_FOR_FALLBACK);
      return_code = result.return_code;
    }
  } catch (const std::exception& e) {
    bcache::debug::log(bcache::debug::FATAL) << "Unexpected error: " << e.what();
    return_code = 1;
  } catch (...) {
    bcache::debug::log(bcache::debug::FATAL) << "Unexpected error.";
    return_code = 1;
  }

  // Report performance timings.
  bcache::perf::report();

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
  // Handle symlink invokation.
  if (bcache::file::get_file_part(std::string(argv[0]), false) != BUILDCACHE_EXE_NAME) {
    bcache::debug::log(bcache::debug::DEBUG) << "Invoked as symlink: " << argv[0];
    wrap_compiler_and_exit(argc, &argv[0]);
  }

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
