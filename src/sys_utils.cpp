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

#include "debug_utils.hpp"
#include "unicode_utils.hpp"

#include <cstdio>
#include <cstdlib>
#include <iostream>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <vector>
#endif

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

#if defined(_WIN32)
std::string read_from_win_file(HANDLE file_handle,
                               std::ostream& stream,
                               const bool output_to_stream) {
  std::string result;
  const size_t BUFSIZE = 4096u;
  std::vector<char> buf(BUFSIZE);
  while (true) {
    DWORD bytes_read;
    const auto success = ReadFile(file_handle, &buf[0], buf.size(), &bytes_read, nullptr);
    if (!success || (bytes_read == 0)) {
      break;
    }
    if (output_to_stream) {
      stream.write(buf.data(), static_cast<size_t>(bytes_read));
    }
    result.append(buf.data(), static_cast<size_t>(bytes_read));
  }
  return result;
}

#endif
}  // namespace

run_result_t run(const string_list_t& args, const bool quiet) {
  // Initialize the run result.
  auto result = make_run_result();

  auto successfully_launched_program = false;
  const auto cmd = args.join(" ", true);
  debug::log(debug::DEBUG) << "Invoking: " << cmd;

#if defined(_WIN32)
// TODO(m): We want to use proper CreateProcess() to get both stdout and stderr etc. Right now the
// code is broken (it occasionally hangs), so we currently use _wpopen() instead.
#if 0
  HANDLE std_out_read_handle = nullptr;
  HANDLE std_out_write_handle = nullptr;
  HANDLE std_err_read_handle = nullptr;
  HANDLE std_err_write_handle = nullptr;

  try {
    SECURITY_ATTRIBUTES security_attr;
    security_attr.nLength = sizeof(SECURITY_ATTRIBUTES);
    security_attr.bInheritHandle = TRUE;
    security_attr.lpSecurityDescriptor = nullptr;

    // Create a pipe for the child process's stdout.
    if (!CreatePipe(&std_out_read_handle, &std_out_write_handle, &security_attr, 0)) {
      throw std::runtime_error("Unable to create stdout pipe.");
    }
    if (!SetHandleInformation(std_out_read_handle, HANDLE_FLAG_INHERIT, 0)) {
      throw std::runtime_error("Unable to create stdout pipe.");
    }

    // Create a pipe for the child process's stderr.
    if (!CreatePipe(&std_err_read_handle, &std_err_write_handle, &security_attr, 0)) {
      throw std::runtime_error("Unable to create stderr pipe.");
    }
    if (!SetHandleInformation(std_err_read_handle, HANDLE_FLAG_INHERIT, 0)) {
      throw std::runtime_error("Unable to create stderr pipe.");
    }

    // Set up process information.
    STARTUPINFOW startup_info = {0};
    startup_info.cb = sizeof(STARTUPINFOW);
    startup_info.hStdOutput = std_out_write_handle;
    startup_info.hStdError = std_err_write_handle;
    startup_info.hStdInput = nullptr;
    startup_info.dwFlags |= STARTF_USESTDHANDLES;

    // Note: The cmdw string may be modified by CreateProcessW (!) so it must not be const.
    auto cmdw = utf8_to_ucs2(cmd);

    // Start the child process.
    PROCESS_INFORMATION process_info = {0};
    if (!CreateProcessW(nullptr,
                        &cmdw[0],
                        nullptr,
                        nullptr,
                        TRUE,
                        0,
                        nullptr,
                        nullptr,
                        &startup_info,
                        &process_info)) {
      throw std::runtime_error("Unable to create child process.");
    }

    // Close the write ends of the pipes (to avoid wating forever).
    CloseHandle(std_out_write_handle);
    std_out_write_handle = nullptr;
    CloseHandle(std_err_write_handle);
    std_err_write_handle = nullptr;

    // Read stdout/stderr.
    result.std_out = read_from_win_file(std_out_read_handle, std::cout, !quiet);
    result.std_err = read_from_win_file(std_err_read_handle, std::cerr, !quiet);

    // Wait for the process to finish.
    WaitForSingleObject(process_info.hProcess, INFINITE);

    // Get process return code.
    DWORD exit_code = 1;
    if (!GetExitCodeProcess(process_info.hProcess, &exit_code)) {
      exit_code = 1;
    }
    result.return_code = static_cast<int>(exit_code);

    CloseHandle(process_info.hProcess);
    CloseHandle(process_info.hThread);

    successfully_launched_program = true;
  } catch (std::exception& e) {
    successfully_launched_program = false;
  }

  if (std_out_read_handle != nullptr) {
    CloseHandle(std_out_read_handle);
  }
  if (std_out_write_handle != nullptr) {
    CloseHandle(std_out_write_handle);
  }
  if (std_err_read_handle != nullptr) {
    CloseHandle(std_err_read_handle);
  }
  if (std_err_write_handle != nullptr) {
    CloseHandle(std_err_write_handle);
  }
#else
  auto* fp = _wpopen(utf8_to_ucs2(cmd).c_str(), L"r");
  if (fp != nullptr) {
    // Collect stdout until the pipe is closed.
    char buf[1000];
    while (fgets(buf, sizeof(buf), fp)) {
      if (!quiet) {
        std::cout << buf;
      }
      result.std_out += std::string(buf);
    }
    result.return_code = _pclose(fp);
    successfully_launched_program = true;
  }
#endif
#else
  // TODO(m): We should use proper fork/exec to get both stdout and stderr etc.
  auto* fp = popen(cmd.c_str(), "r");
  if (fp != nullptr) {
    // Collect stdout until the pipe is closed.
    char buf[1000];
    while (fgets(buf, sizeof(buf), fp)) {
      if (!quiet) {
        std::cout << buf;
      }
      result.std_out += std::string(buf);
    }
    result.return_code = pclose(fp);
    successfully_launched_program = true;
  }
#endif

  if (!successfully_launched_program) {
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
