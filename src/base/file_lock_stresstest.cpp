//--------------------------------------------------------------------------------------------------
// Copyright (c) 2019 Marcus Geelnard
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

#include <base/debug_utils.hpp>
#include <base/file_lock.hpp>
#include <base/file_utils.hpp>

#include <iostream>
#include <string>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#undef ERROR
#undef log
#else
#include <chrono>
#include <thread>
#endif

using namespace bcache;

namespace {
const int NUM_LOOPS = 1000;

void sleep_milliseconds(int t) {
#if defined(_WIN32)
  Sleep(static_cast<DWORD>(t));
#else
  std::this_thread::sleep_for(std::chrono::milliseconds(t));
#endif
}

long read_number_from_file(const std::string& path) {
  // Read the file.
  std::string data("0");
  if (file::file_exists(path)) {
    data = file::read(path);
  }

  // Convert the content to an integer (count).
  long count;
  try {
    count = std::stol(data);
  } catch (std::invalid_argument& e) {
    std::cerr << "*** Error: Unable to parse integer: \"" << data << "\" (" << e.what() << ")"
              << std::endl;
    std::exit(1);
  }

  return count;
}
}  // namespace

int main(int argc, const char** argv) {
  if (argc != 3) {
    std::cout << "Usage: " << argv[0] << " <filename> <local_locks>\n";
    std::cout << "  filename    The name of the file to be updated in a locked fashion\n";
    std::cout << "  local_locks Set this to \"true\" to allow local locks\n";
    exit(0);
  }
  const std::string filename(argv[1]);
  const auto file_lockname = filename + ".lock";
  const bool local_locks = (std::string(argv[2]) == "true");

  long last_count = -1;
  for (int i = 0; i < NUM_LOOPS; ++i) {
    {
      // Acquire a lock, which should guarantee us exclusive access to the data file.
      file::file_lock_t lock(file_lockname, local_locks);
      if (!lock.has_lock()) {
        std::cerr << "*** Error: Unable to acquire lock: " << file_lockname << std::endl;
        exit(1);
      }

      // Read the count from the file (starting at 0 if the file does not exist).
      auto count = read_number_from_file(filename);

      // Update the counter, and write it to the file.
      count++;
      file::write(std::to_string(count), filename);

      last_count = count;
    }

    // At this point, the lock should be released. Sleep a bit to let others aquire the lock.
    sleep_milliseconds(i % 3 + 1);
  }

  std::cout << "After " << NUM_LOOPS << " updates, the file count is: " << last_count << std::endl;
  exit(0);
}
