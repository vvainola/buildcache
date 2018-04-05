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

#include "sys_utils.hpp"

#include <codecvt>
#include <cstdio>
#include <cstdlib>
#include <locale>
#include <iostream>

namespace bcache {
namespace sys {
namespace {
run_result_t make_run_result() {
  run_result_t result;
  result.return_code = 1;
  result.std_out = std::string();
  result.std_err = std::string();
  return result;
}
}  // namespace

std::string ucs2_to_utf8(const std::wstring& str16) {
  std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
  try {
    return conv.to_bytes(str16);
  } catch (...) {
    return std::string();
  }
}

std::wstring utf8_to_ucs2(const std::string& str8) {
  std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
  try {
    return conv.from_bytes(str8);
  } catch (...) {
    return std::wstring();
  }
}

run_result_t run(const string_list_t& args, const bool quiet) {
  // Initialize the run result.
  auto result = make_run_result();

  std::string cmd = args.join(" ", true);

  FILE* fp;
#if defined(_WIN32)
  fp = _wpopen(utf8_to_ucs2(cmd).c_str(), "r");
#else
  fp = popen(cmd.c_str(), "r");
#endif
  if (fp != nullptr) {
    // Collect stdout until the pipe is closed.
    char buf[1000];
    while (fgets(buf, sizeof(buf), fp)) {
      if (!quiet) {
        std::cout << buf;
      }
      result.std_out += std::string(buf);
    }

#if defined(_WIN32)
    result.return_code = _pclose(fp);
#else
    result.return_code = pclose(fp);
#endif
  } else {
    std::cerr << "*** Unable to run command:\n    " << cmd << "\n";
  }

  return result;
}

run_result_t run_with_prefix(const string_list_t& args, const bool quiet) {
  // Prepend the argument list with a prefix, if any.
  string_list_t prefixed_args;
  const auto* prefix_env = std::getenv("BUILDCACHE_PREFIX");
  if (prefix_env != nullptr) {
    prefixed_args += std::string(prefix_env);
  }
  prefixed_args += args;

  // Run the command.
  return run(prefixed_args, quiet);
}
}  // namespace sys
}  // namespace bcache
