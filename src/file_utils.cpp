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

#include "debug_utils.hpp"
#include "string_list.hpp"
#include "unicode_utils.hpp"

#include <atomic>
#include <cstdio>
#include <cstdint>
#include <sstream>
#include <vector>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shlobj.h>
#include <userenv.h>
#undef ERROR
#undef log
#else
#include <cstdlib>
#include <dirent.h>
#include <limits.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>

namespace bcache {
namespace file {
namespace {
// Directory separator for paths.
#ifdef _WIN32
const char PATH_SEPARATOR_CHR = '\\';
#else
const char PATH_SEPARATOR_CHR = '/';
#endif
const auto PATH_SEPARATOR = std::string(1, PATH_SEPARATOR_CHR);

// Delimiter character for the PATH environment variable.
#ifdef _WIN32
const char PATH_DELIMITER_CHR = ';';
#else
const char PATH_DELIMITER_CHR = ':';
#endif
const auto PATH_DELIMITER = std::string(1, PATH_DELIMITER_CHR);

// This is a static variable that holds a strictly incrementing number used for generating unique
// temporary file names.
std::atomic_uint_fast32_t s_tmp_name_number;

int get_process_id() {
#ifdef _WIN32
  return static_cast<int>(GetCurrentProcessId());
#else
  return static_cast<int>(getpid());
#endif
}

std::string::size_type get_last_path_separator_pos(const std::string& path) {
#if defined(_WIN32)
  const auto pos1 = path.rfind("/");
  const auto pos2 = path.rfind("\\");
  std::string::size_type pos;
  if (pos1 == std::string::npos) {
    pos = pos2;
  } else if (pos2 == std::string::npos) {
    pos = pos1;
  } else {
    pos = std::max(pos1, pos2);
  }
#else
  const auto pos = path.rfind(PATH_SEPARATOR);
#endif
  return pos;
}

#ifdef _WIN32
int64_t two_dwords_to_int64(const DWORD low, const DWORD high) {
  return static_cast<int64_t>(static_cast<uint64_t>(static_cast<uint32_t>(low)) |
                              (static_cast<uint64_t>(static_cast<uint32_t>(high)) << 32));
}

file_info_t::time_t win32_filetime_as_unix_epoch(const FILETIME& file_time) {
  // The FILETIME represents the number of 100-nanosecond intervals since January 1, 1601 (UTC).
  // I.e. the Windows epoch starts 11644473600 seconds before the UNIX epoch.
  const auto t64 = two_dwords_to_int64(file_time.dwLowDateTime, file_time.dwHighDateTime);
  return static_cast<file_info_t::time_t>((t64 / 10000000L) - 11644473600L);
}
#endif

}  // namespace

tmp_file_t::tmp_file_t(const std::string& dir, const std::string& extension) {
  // Get unique identifiers for this file.
  const auto pid = get_process_id();
  const auto number = ++s_tmp_name_number;

  // Generate a file name from the unique identifiers.
  std::ostringstream ss;
  ss << "bcache" << pid << "_" << number;
  std::string file_name = ss.str();

  // Concatenate base dir, file name and extension into the full path.
  m_path = append_path(dir, file_name + extension);
}

tmp_file_t::~tmp_file_t() {
  if (file_exists(m_path)) {
    try {
      remove_file(m_path);
    } catch (const std::exception& e) {
      debug::log(debug::ERROR) << e.what();
    }
  }
}

file_info_t::file_info_t(const std::string& path,
                         const file_info_t::time_t modify_time,
                         const file_info_t::time_t access_time,
                         const int64_t size,
                         const bool is_dir)
    : m_path(path),
      m_modify_time(modify_time),
      m_access_time(access_time),
      m_size(size),
      m_is_dir(is_dir) {
}

std::string append_path(const std::string& path, const std::string& append) {
  return path + PATH_SEPARATOR + append;
}

std::string append_path(const std::string& path, const char* append) {
  return append_path(path, std::string(append));
}

std::string get_extension(const std::string& path) {
  const auto pos = path.rfind(".");
  return (pos != std::string::npos) ? path.substr(pos) : std::string();
}

std::string get_file_part(const std::string& path, const bool include_ext) {
  const auto pos = get_last_path_separator_pos(path);
  const auto file_name = (pos != std::string::npos) ? path.substr(pos + 1) : path;
  const auto ext_pos = file_name.rfind(".");
  return (include_ext || (ext_pos == std::string::npos) || (ext_pos == 0))
             ? file_name
             : file_name.substr(0, ext_pos);
}

std::string get_dir_part(const std::string& path) {
  const auto pos = get_last_path_separator_pos(path);
  return (pos != std::string::npos) ? path.substr(0, pos) : path;
}

std::string get_user_home_dir() {
#if defined(_WIN32)
#if 0
  // TODO(m): We should use SHGetKnownFolderPath() for Vista and later, but this fails to build in
  // older MinGW so we skip it a.t.m.
  std::string local_app_data;
  PWSTR path = nullptr;
  try {
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
  std::string user_home;
  HANDLE token = 0;
  if (SUCCEEDED(OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token))) {
    WCHAR buf[MAX_PATH] = {0};
    DWORD buf_size = MAX_PATH;
    if (SUCCEEDED(GetUserProfileDirectoryW(token, buf, &buf_size))) {
      user_home = ucs2_to_utf8(std::wstring(buf, buf_size));
    }
    CloseHandle(token);
  }
  return user_home;
#endif
#else
  const auto* home_env = getenv("HOME");
  return (home_env != nullptr) ? std::string(home_env) : std::string();
#endif
}

bool is_absolute_path(const std::string& path) {
#ifdef _WIN32
  const bool is_abs_drive = (path.size() >= 3) && (path[1] == ':') && (path[2] == '\\');
  const bool is_abs_net = (path.size() >= 2) && (path[0] == '\\') && (path[1] == '\\');
  return is_abs_drive || is_abs_net;
#else
  return (path.size() >= 1) && (path[0] == PATH_SEPARATOR_CHR);
#endif
}

std::string resolve_path(const std::string& path) {
#if defined(_WIN32)
  // TODO(m): Implement me!
  return path;
#else
  auto* char_ptr = realpath(path.c_str(), nullptr);
  if (char_ptr != nullptr) {
    const auto result = std::string(char_ptr);
    std::free(char_ptr);
    return result;
  }
  return std::string();
#endif
}

std::string find_executable(const std::string& path, const std::string& exclude) {
  auto file_to_find = path;

  // Handle absolute paths.
  if (is_absolute_path(file_to_find)) {
    // Return the full path unless it points to the excluded executable.
    const auto true_path = resolve_path(file_to_find);
    if (true_path.empty()) {
      throw std::runtime_error("Could not resolve absolute path for the executable file.");
    }
    if (get_file_part(true_path, false) != exclude) {
      debug::log(debug::DEBUG) << "Found exe: " << true_path << " (looked for " << path << ")";
      return true_path;
    }

    // ...otherwise search for the named file (which should be a symlink) in the PATH.
    file_to_find = get_file_part(file_to_find);
  }

  // Get the PATH environment variable.
  const auto* path_env = getenv("PATH");
  if (!path_env) {
    return std::string();
  }
  const auto search_path = string_list_t(std::string(path_env), PATH_DELIMITER);

  // Iterate the path from start to end and see if we can find the executable file.
  for (const auto& base_path : search_path) {
    const auto true_path = resolve_path(append_path(base_path, file_to_find));
    if ((!true_path.empty()) && file_exists(true_path)) {
      // Check that this is not the excluded file name.
      const auto true_name = get_file_part(true_path, false);
      if (true_name != exclude) {
        debug::log(debug::DEBUG) << "Found exe: " << true_path << " (looked for " << file_to_find
                                 << ")";
        return true_path;
      }
    }
  }

  throw std::runtime_error("Could not find the executable file.");
}

bool create_dir(const std::string& path) {
#ifdef _WIN32
  return (_wmkdir(utf8_to_ucs2(path).c_str()) == 0);
#else
  return (mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == 0);
#endif
}

void remove_file(const std::string& path) {
#ifdef _WIN32
  const auto success = (_wremove(utf8_to_ucs2(path).c_str()) == 0);
#else
  const auto success = (std::remove(path.c_str()) == 0);
#endif
  if (!success) {
    throw std::runtime_error("Unable to remove file.");
  }
}

bool dir_exists(const std::string& path) {
#ifdef _WIN32
  struct __stat64 buffer;
  const auto success = (_wstat64(utf8_to_ucs2(path).c_str(), &buffer) == 0);
  return success && S_ISDIR(buffer.st_mode);
#else
  struct stat buffer;
  const auto success = (stat(path.c_str(), &buffer) == 0);
  return success && S_ISDIR(buffer.st_mode);
#endif
}

bool file_exists(const std::string& path) {
#ifdef _WIN32
  struct __stat64 buffer;
  const auto success = (_wstat64(utf8_to_ucs2(path).c_str(), &buffer) == 0);
  return success && S_ISREG(buffer.st_mode);
#else
  struct stat buffer;
  const auto success = (stat(path.c_str(), &buffer) == 0);
  return success && S_ISREG(buffer.st_mode);
#endif
}

void copy(const std::string& from_path, const std::string& to_path) {
  // Copy to a temporary file first and once the copy has succeeded rename it to the target file.
  // This should prevent half-finished copies if the process is terminated prematurely (e.g.
  // CTRL+C).
  const auto base_path = get_dir_part(to_path);
  auto tmp_file = tmp_file_t(base_path, ".tmp");

#ifdef _WIN32
  // TODO(m): We could handle paths longer than MAX_PATH, e.g. by prepending strings with "\\?\"?
  bool success =
      (CopyFileW(utf8_to_ucs2(from_path).c_str(), utf8_to_ucs2(tmp_file.path()).c_str(), FALSE) !=
       0);
#else
  // For non-Windows systems we use a classic buffered read-write loop.
  bool success = false;
  auto* from_file = std::fopen(from_path.c_str(), "rb");
  if (from_file != nullptr) {
    auto* to_file = std::fopen(tmp_file.path().c_str(), "wb");
    if (to_file != nullptr) {
      // We use a buffer size that typically fits in an L1 cache.
      static const int BUFFER_SIZE = 8192;
      std::vector<std::uint8_t> buf(BUFFER_SIZE);
      success = true;
      while (!std::feof(from_file)) {
        const auto bytes_read = std::fread(buf.data(), 1, buf.size(), from_file);
        if (bytes_read == 0u) {
          break;
        }
        const auto bytes_written = std::fwrite(buf.data(), 1, buf.size(), to_file);
        if (bytes_written != bytes_read) {
          success = false;
          break;
        }
      }
      std::fclose(to_file);
    }
    std::fclose(from_file);
  }
#endif

  // Rename the temporary file to its target name.
  if (success) {
    // First remove the old file, if any (otherwise the rename will fail).
    if (file_exists(to_path)) {
      remove_file(to_path);
    }

#ifdef _WIN32
    success = (_wrename(utf8_to_ucs2(tmp_file.path()).c_str(), utf8_to_ucs2(to_path).c_str()) == 0);
#else
    success = (std::rename(tmp_file.path().c_str(), to_path.c_str()) == 0);
#endif
    if (!success) {
      debug::log(debug::ERROR) << "Copying failed due to a failing rename operation.";
    }
  }

  if (!success) {
    // Note: At this point the temporary file (if any) will be deleted.
    throw std::runtime_error("Unable to copy file.");
  }
}

void link_or_copy(const std::string& from_path, const std::string& to_path) {
  // First remove the old file, if any (otherwise the hard link will fail).
  if (file_exists(to_path)) {
    remove_file(to_path);
  }

  // First try to make a hard link. However this may fail if the files are on different volumes for
  // instance.
  bool success;
#ifdef _WIN32
  success = (CreateHardLinkW(
                 utf8_to_ucs2(to_path).c_str(), utf8_to_ucs2(from_path).c_str(), nullptr) != 0);
#else
  success = (link(from_path.c_str(), to_path.c_str()) == 0);
#endif

  // If the hard link failed, make a full copy instead.
  if (!success) {
    debug::log(debug::DEBUG) << "Hard link failed - copying instead.";
    copy(from_path, to_path);
  }
}

std::string read(const std::string& path) {
  FILE* f;

// Open the file.
#ifdef _WIN32
  const auto err = _wfopen_s(&f, utf8_to_ucs2(path).c_str(), L"rb");
  if (err != 0) {
    throw std::runtime_error("Unable to open the file.");
  }
#else
  f = std::fopen(path.c_str(), "rb");
  if (f == nullptr) {
    throw std::runtime_error("Unable to open the file.");
  }
#endif

  // Get file size.
  std::fseek(f, 0, SEEK_END);
  const auto file_size = static_cast<size_t>(std::ftell(f));
  std::fseek(f, 0, SEEK_SET);

  // Read the data into a string.
  std::string str;
  str.resize(static_cast<std::string::size_type>(file_size));
  auto bytes_left = file_size;
  while ((bytes_left != 0u) && !std::feof(f)) {
    auto* ptr = &str[file_size - bytes_left];
    const auto bytes_read = std::fread(ptr, 1, bytes_left, f);
    bytes_left -= bytes_read;
  }

  // Close the file.
  std::fclose(f);

  if (bytes_left != 0u) {
    throw std::runtime_error("Unable to read the file.");
  }

  return str;
}

std::vector<file_info_t> walk_directory(const std::string& path) {
  std::vector<file_info_t> files;

#ifdef _WIN32
  const auto search_str = utf8_to_ucs2(append_path(path, "*"));
  WIN32_FIND_DATAW find_data;
  auto find_handle = FindFirstFileW(search_str.c_str(), &find_data);
  if (find_handle == INVALID_HANDLE_VALUE) {
    throw std::runtime_error("Unable to walk the directory.");
  }
  do {
    const auto name = ucs2_to_utf8(std::wstring(&find_data.cFileName[0]));
    if ((name != ".") && (name != "..")) {
      const auto file_path = append_path(path, name);

      const auto modify_time = win32_filetime_as_unix_epoch(find_data.ftLastWriteTime);
      const auto access_time = win32_filetime_as_unix_epoch(find_data.ftLastAccessTime);

      int64_t size = 0;
      bool is_dir = false;
      if ((find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
        auto subdir_files = walk_directory(file_path);
        for (const auto& entry : subdir_files) {
          files.emplace_back(entry);
        }
        is_dir = true;
      } else {
        size = two_dwords_to_int64(find_data.nFileSizeLow, find_data.nFileSizeHigh);
      }
      files.emplace_back(file_info_t(file_path, modify_time, access_time, size, is_dir));
    }
  } while (FindNextFileW(find_handle, &find_data) != 0);

  FindClose(find_handle);

  const auto find_error = GetLastError();
  if (find_error != ERROR_NO_MORE_FILES) {
    throw std::runtime_error("Failed to walk the directory.");
  }

#else
  auto* dir = opendir(path.c_str());
  if (dir == nullptr) {
    throw std::runtime_error("Unable to walk the directory.");
  }

  auto* entity = readdir(dir);
  while (entity != nullptr) {
    const auto name = std::string(entity->d_name);
    if ((name != ".") && (name != "..")) {
      const auto file_path = append_path(path, name);
      struct stat file_stat;
      if (stat(file_path.c_str(), &file_stat) == 0) {
#ifdef __APPLE__
        const auto modify_time = static_cast<file_info_t::time_t>(file_stat.st_mtimespec.tv_sec);
        const auto access_time = static_cast<file_info_t::time_t>(file_stat.st_atimespec.tv_sec);
#else
        const auto modify_time = static_cast<file_info_t::time_t>(file_stat.st_mtim.tv_sec);
        const auto access_time = static_cast<file_info_t::time_t>(file_stat.st_atim.tv_sec);
#endif
        int64_t size = 0;
        bool is_dir = false;
        if (entity->d_type == DT_DIR) {
          auto subdir_files = walk_directory(file_path);
          for (const auto& entry : subdir_files) {
            files.emplace_back(entry);
          }
          is_dir = true;
        } else if (entity->d_type == DT_REG) {
          size = static_cast<int64_t>(file_stat.st_size);
        }
        files.emplace_back(file_info_t(file_path, modify_time, access_time, size, is_dir));
      }
    }
    entity = readdir(dir);
  }

  closedir(dir);
#endif

  return files;
}
}  // namespace file
}  // namespace bcache
