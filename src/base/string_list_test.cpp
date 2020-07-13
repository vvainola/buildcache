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

#include <base/string_list.hpp>

#include <doctest/doctest.h>

#include <vector>

// Workaround for macOS build errors.
// See: https://github.com/onqtam/doctest/issues/126
#include <iostream>

using namespace bcache;

TEST_CASE("string_list_t constructors are behaving as expected") {
  SUBCASE("Default") {
    string_list_t list;
    CHECK_EQ(list.size(), 0);
  }

  SUBCASE("Initializer list") {
    string_list_t list{"Hello", "world"};
    CHECK_EQ(list.size(), 2);
    CHECK_EQ(list[0], "Hello");
    CHECK_EQ(list[1], "world");
  }

  SUBCASE("argc + argv") {
    const char* argv[] = {"Hello", "world"};
    const size_t argc = 2;
    string_list_t list(argc, argv);
    CHECK_EQ(list.size(), 2);
    CHECK_EQ(list[0], "Hello");
    CHECK_EQ(list[1], "world");
  }

  SUBCASE("string + delimiter") {
    const std::string str("Hello world");
    const std::string delimiter(" ");
    string_list_t list(str, delimiter);
    CHECK_EQ(list.size(), 2);
    CHECK_EQ(list[0], "Hello");
    CHECK_EQ(list[1], "world");
  }
}

TEST_CASE("Command line argument parsing works correctly") {
  SUBCASE("Regular command line") {
    const std::string cmd("hello  world");
    string_list_t list = string_list_t::split_args(cmd);
    CHECK_EQ(list.size(), 2);
    CHECK_EQ(list[0], "hello");
    CHECK_EQ(list[1], "world");
  }

  SUBCASE("String argument with spaces and escaped slashes and quotes") {
    const std::string cmd("hello \"beautiful \\\\ \\\"  world\"");
    string_list_t list = string_list_t::split_args(cmd);
    CHECK_EQ(list.size(), 2);
    CHECK_EQ(list[0], "hello");
#ifdef _WIN32
    CHECK_EQ(list[1], "beautiful \\\\ \"  world");
#else
    CHECK_EQ(list[1], "beautiful \\ \"  world");
#endif
  }
}

TEST_CASE("Joining elements works correctly") {
  SUBCASE("Un-escaped result") {
    string_list_t list{"Hello", "\"beautiful world\""};
    const auto str = list.join(" ; ", false);
    CHECK_EQ(str, "Hello ; \"beautiful world\"");
  }

  SUBCASE("Escaped result") {
    string_list_t list{"Hello", "\"beautiful world\""};
    const auto str = list.join(" ; ", true);
    CHECK_EQ(str, "Hello ; \"\\\"beautiful world\\\"\"");
  }
}

TEST_CASE("Appending strings works correctly") {
  string_list_t list;
  CHECK_EQ(list.size(), 0);
  list += "Hello";
  list += "world";
  CHECK_EQ(list.size(), 2);
  CHECK_EQ(list[0], "Hello");
  CHECK_EQ(list[1], "world");
}

TEST_CASE("Appending string lists works correctly") {
  string_list_t list{"Hello", "world"};
  CHECK_EQ(list.size(), 2);
  list += string_list_t{"of", "testing"};
  CHECK_EQ(list.size(), 4);
  CHECK_EQ(list[0], "Hello");
  CHECK_EQ(list[1], "world");
  CHECK_EQ(list[2], "of");
  CHECK_EQ(list[3], "testing");
}

TEST_CASE("Iterators work correctly") {
  string_list_t list{"Hello", "world"};
  std::vector<std::string> items;
  for (const auto& item : list) {
    items.push_back(item);
  }
  CHECK_EQ(items.size(), 2);
  CHECK_EQ(items[0], "Hello");
  CHECK_EQ(items[1], "world");
}
