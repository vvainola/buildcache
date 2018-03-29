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

#include "file_utils.hpp"

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <shlobj.h>
#include <codecvt>
#endif

#include <sys/types.h>
#include <sys/stat.h>

namespace bcache {
namespace file {
namespace {
#if defined(_WIN32)
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
    return conv.from_bytes(str16);
  } catch (...) {
    return std::wstring();
  }
}
#endif  // _WIN32
}  // namespace

std::string append_path(const std::string& path, const std::string& append) {
#if defined(_WIN32)
  return path + std::string("\\") + append;
#else
  return path + std::string("/") + append;
#endif
}

std::string append_path(const std::string& path, const char* append) {
  return append_path(path, std::string(append));
}

std::string get_user_home_dir() {
#if defined(_WIN32)
  std::string local_app_data;
  PWSTR path = nullptr;
  try {
    // Using SHGetKnownFolderPath() for Vista and later.
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, KF_FLAG_CREATE, NULL, &path))) {
      local_app_data = ucs2_to_utf8(std::wstring(path));
    }
  } finally {
    if (path != nullptr) {
      CoTaskMemFree(path);
    }
  }
  return local_app_data;
#else
  const auto* home_env = getenv("HOME");
  return (home_env != nullptr) ? std::string(home_env) : std::string();
#endif
}

bool create_dir(const std::string& path) {
#ifdef _WIN32
  return (_wmkdir(utf8_to_ucs2(path).c_str()) == 0);
#else
  return (mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == 0);
#endif
}

void remove(const std::string& path) {
  // TODO(m): Implement me!
  (void)path;
}

bool dir_exists(const std::string& path) {
// TODO(m): Check that it's acutally a dir!
#ifdef _WIN32
  struct __stat64 buffer;
  return (_wstat64(utf8_to_ucs2(path).c_str(), &buffer) == 0);
#else
  struct stat buffer;
  return (stat(path.c_str(), &buffer) == 0);
#endif
}

bool file_exists(const std::string& path) {
// TODO(m): Check that it's acutally a file!
#ifdef _WIN32
  struct __stat64 buffer;
  return (_wstat64(utf8_to_ucs2(path).c_str(), &buffer) == 0);
#else
  struct stat buffer;
  return (stat(path.c_str(), &buffer) == 0);
#endif
}
}  // namespace file
}  // namespace bcache
