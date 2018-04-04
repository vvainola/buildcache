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
#include "file_utils.hpp"
#include "hasher.hpp"
#include "sys_utils.hpp"

#include <iostream>
#include <string>

namespace bcache {
class compiler_wrapper {
protected:
  static file::tmp_file_t get_temp_file(const cache_t& cache, const std::string& extension) {
    // Use a suitable folder for temporary files.
    const auto tmp_path = file::append_path(cache.root_folder(), "tmp");
    if (!file::dir_exists(tmp_path)) {
      file::create_dir(tmp_path);
    }

    // Generate a fairly unique file name.
    return file::tmp_file_t(tmp_path, extension);
  }
};

class gcc_wrapper : public compiler_wrapper {
public:
  static bool can_handle_command(const arg_list_t& args) {
    // Is this the right compiler?
    return (args.size() >= 1) && ((args[0].find("gcc") != std::string::npos) ||
                                  (args[0].find("g++") != std::string::npos));
  }

  /// @brief Try to wrap a compiler command.
  /// @param args Command and arguments.
  /// @param[out] return_code The command return code (if handled).
  /// @returns true if the command was recognized and handled.
  static bool handle_command(const arg_list_t& args, cache_t& cache, int& return_code) {
    // Start a hash.
    hasher_t hasher;

    // Hash the preprocessed file contents.
    hasher.update(preprocess_source(args, cache));

    // Hash the (filtered) command line flags.
    hasher.update(filter_arguments(args).join(" "));

    // Hash the compiler version string.
    hasher.update(get_version_info(args));

    // DEBUG
    std::cout << " == HASH: " << hasher.final().as_string() << "\n";

    // Look up the entry in the cache or create a new entry.
    // TODO(m): Implement me!
    (void)return_code;
    return false;
  }

private:
  static arg_list_t make_preprocessor_cmd(const arg_list_t& args,
                                          const std::string& preprocessed_file) {
    arg_list_t preprocess_args;

    // Drop arguments that we do not want/need.
    bool drop_next_arg = false;
    for (auto it = args.begin(); it != args.end(); ++it) {
      auto arg = *it;
      bool drop_this_arg = drop_next_arg;
      drop_next_arg = false;
      if (arg == std::string("-c")) {
        drop_this_arg = true;
      } else if (arg == std::string("-o")) {
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

  static std::string preprocess_source(const arg_list_t& args, cache_t& cache) {
    // Are we compiling an object file?
    auto is_object_compilation = false;
    for (auto arg : args) {
      if (arg == std::string("-c")) {
        is_object_compilation = true;
        break;
      }
    }
    if (!is_object_compilation) {
      return std::string();
    }

    // Run the preprocessor step.
    const auto preprocessed_file = compiler_wrapper::get_temp_file(cache, ".pp");
    const auto preprocessor_args = make_preprocessor_cmd(args, preprocessed_file.path());
    auto result = sys::run(preprocessor_args);
    if (result.return_code != 0) {
      return std::string();
    }

    // Read and return the preprocessed file.
    return file::read(preprocessed_file.path());
  }

  static arg_list_t filter_arguments(const arg_list_t& args) {
    arg_list_t filtered_args;

    // The first argument is the compiler binary without the path.
    filtered_args += file::get_file_part(args[0]);

    // Note: We always skip the first arg since we have handled it already.
    bool skip_next_arg = true;
    for (auto arg : args) {
      if (!skip_next_arg) {
        // Does this argument specify a file (we don't want to hash those).
        const bool is_arg_plus_file_name =
            ((arg == "-I") || (arg == "-MF") || (arg == "-MT") || (arg == "-o"));

        // Is this a source file (we don't want to hash it)?
        const auto ext = file::get_extension(arg);
        const bool is_source_file = ((ext == ".cpp") || (ext == ".c"));

        // Generally unwanted argument (things that will not change how we go from preprocessed code
        // to binary object files)?
        const auto first_two_chars = arg.substr(0, 2);
        const bool is_unwanted_arg =
            ((first_two_chars == "-I") || (first_two_chars == "-D") || is_source_file);

        if (is_arg_plus_file_name) {
          skip_next_arg = true;
        } else if (!is_unwanted_arg) {
          filtered_args += arg;
        }
      } else {
        skip_next_arg = false;
      }
    }

    // DEBUG
    std::cout << " == Filtered arguments: " << filtered_args.join(" ") << "\n";

    return filtered_args;
  }

  static std::string get_version_info(const arg_list_t& args) {
    // Get the version string for the compiler.
    arg_list_t version_args;
    version_args += args[0];
    version_args += "--version";
    const auto result = sys::run(version_args);
    return (result.return_code == 0) ? result.stdout : std::string();
  }

};
}  // namespace bcache

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

[[noreturn]] void wrap_compiler_and_exit(int argc, const char** argv) {
  auto args = bcache::arg_list_t(argc, argv);

  // TODO(m): Follow symlinks to find the true compiler command! It affects things like if we can
  // match the compiler name or not, and what version string we get. We also want to avoid
  // incorrectly identifying other compiler accelerators (e.g. ccache) as actual compilers.

  // Initialize a cache object.
  bcache::cache_t cache;

  // Try different compilers.
  auto handled = false;
  int return_code = 1;
  if (bcache::gcc_wrapper::can_handle_command(args)) {
    handled = bcache::gcc_wrapper::handle_command(args, cache, return_code);
  }

  // Fall back to running the command as is.
  if (!handled) {
    auto result = bcache::sys::run(args, false);
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
