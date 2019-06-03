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

#include <cache/remote_cache_provider.hpp>

#include <doctest/doctest.h>

// Workaround for macOS build errors.
// See: https://github.com/onqtam/doctest/issues/126
#include <iostream>

namespace {
// A test implementation of the remote cache provider. We use this class to get public access to the
// protected method, parse_host_description().
class test_cache_provider_t : public bcache::remote_cache_provider_t {
public:
  struct parsed_host_description_t {
    std::string host;
    std::string path;
    int port;
    bool success;
  };

  static parsed_host_description_t parse_host_description(const std::string& host_description) {
    parsed_host_description_t result;
    result.success = remote_cache_provider_t::parse_host_description(
        host_description, result.host, result.port, result.path);
    return result;
  }
};
}  // namespace

TEST_CASE("parse_host_description() parses correct host descriptions") {
  SUBCASE("Host name only") {
    const auto result = test_cache_provider_t::parse_host_description("myhost.org");
    CHECK_EQ(result.success, true);
    CHECK_EQ(result.host, "myhost.org");
    CHECK_EQ(result.port, -1);
    CHECK_EQ(result.path, "");
  }

  SUBCASE("Host name and port") {
    const auto result = test_cache_provider_t::parse_host_description("myhost.org:8080");
    CHECK_EQ(result.success, true);
    CHECK_EQ(result.host, "myhost.org");
    CHECK_EQ(result.port, 8080);
    CHECK_EQ(result.path, "");
  }

  SUBCASE("Host name and path") {
    const auto result = test_cache_provider_t::parse_host_description("myhost.org/home");
    CHECK_EQ(result.success, true);
    CHECK_EQ(result.host, "myhost.org");
    CHECK_EQ(result.port, -1);
    CHECK_EQ(result.path, "home");
  }

  SUBCASE("Host name and port and path") {
    const auto result = test_cache_provider_t::parse_host_description("myhost.org:8080/home");
    CHECK_EQ(result.success, true);
    CHECK_EQ(result.host, "myhost.org");
    CHECK_EQ(result.port, 8080);
    CHECK_EQ(result.path, "home");
  }
}

TEST_CASE("parse_host_description() fails for incorrect host descriptions") {
  SUBCASE("Bad: Empty host name (empty string)") {
    const auto result = test_cache_provider_t::parse_host_description("");
    CHECK_EQ(result.success, false);
  }

  SUBCASE("Bad: Empty host name, but with port") {
    const auto result = test_cache_provider_t::parse_host_description(":20");
    CHECK_EQ(result.success, false);
  }

  SUBCASE("Bad: Port comes after path") {
    const auto result = test_cache_provider_t::parse_host_description("myhost.org/home:20");
    CHECK_EQ(result.success, false);
  }

  SUBCASE("Bad: Port is an invalid number") {
    const auto result = test_cache_provider_t::parse_host_description("myhost.org:xyz");
    CHECK_EQ(result.success, false);
  }

  SUBCASE("Bad: Port is an empty string") {
    const auto result = test_cache_provider_t::parse_host_description("myhost.org:");
    CHECK_EQ(result.success, false);
  }
}
