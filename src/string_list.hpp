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

#ifndef BUILDCACHE_STRING_LIST_HPP_
#define BUILDCACHE_STRING_LIST_HPP_

#include <string>
#include <vector>

namespace bcache {
class string_list_t {
public:
  typedef typename std::vector<std::string>::iterator iterator;
  typedef typename std::vector<std::string>::const_iterator const_iterator;

  string_list_t() {
  }

  string_list_t(const int argc, const char** argv) {
    for (int i = 0; i < argc; ++i) {
      m_args.emplace_back(std::string(argv[i]));
    }
  }

  string_list_t(const std::string& str, const std::string& delimiter) {
    std::string::size_type current_str_start = 0u;
    while (current_str_start < str.size()) {
      const auto pos = str.find(delimiter, current_str_start);
      if (pos == std::string::npos) {
        m_args.emplace_back(str.substr(current_str_start));
        current_str_start = str.size();
      } else {
        m_args.emplace_back(str.substr(current_str_start, pos - current_str_start));
        current_str_start = pos + 1;
      }
    }
  }

  std::string join(const std::string& separator) const {
    std::string result;
    for (auto arg : m_args) {
      const auto escaped_arg = escape_arg(arg);
      if (result.empty()) {
        result = result + escaped_arg;
      } else {
        result = result + separator + escaped_arg;
      }
    }
    return result;
  }

  std::string& operator[](const size_t idx) {
    return m_args[idx];
  }

  const std::string& operator[](const size_t idx) const {
    return m_args[idx];
  }

  string_list_t& operator+=(const std::string& str) {
    m_args.emplace_back(str);
    return *this;
  }

  size_t size() const {
    return m_args.size();
  }

  iterator begin() {
    return m_args.begin();
  }

  const_iterator begin() const {
    return m_args.begin();
  }

  const_iterator cbegin() const {
    return m_args.cbegin();
  }

  iterator end() {
    return m_args.end();
  }

  const_iterator end() const {
    return m_args.end();
  }

  const_iterator cend() const {
    return m_args.cend();
  }

private:
  static std::string escape_arg(const std::string& arg) {
    std::string escaped_arg;

    // Replace all occurances of " with \".
    auto has_space = false;
    for (auto c : arg) {
      if (c == '"') {
        escaped_arg += "\\\"";
      } else {
        if (c == ' ') {
          has_space = true;
        }
        escaped_arg += c;
      }
    }

    // Do we need to surround with quotes?
    if (has_space) {
      escaped_arg = std::string("\"") + escaped_arg + std::string("\"");
    }

    return escaped_arg;
  }

  std::vector<std::string> m_args;
};
}  // namespace bcache

#endif  // BUILDCACHE_STRING_LIST_HPP_
