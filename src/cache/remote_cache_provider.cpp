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

#include <base/debug_utils.hpp>
#include <base/string_list.hpp>

namespace bcache {

remote_cache_provider_t::remote_cache_provider_t() {
}

remote_cache_provider_t::~remote_cache_provider_t() {
}

bool remote_cache_provider_t::parse_host_description(const std::string& host_description,
                                                     std::string& host,
                                                     int& port,
                                                     std::string& path) {
  const auto colon_pos = host_description.find(':');
  const auto slash_pos = host_description.find('/');

  // The slash, if any, must come after the colon, if any.
  if (slash_pos != std::string::npos && colon_pos != std::string::npos && slash_pos < colon_pos) {
    debug::log(debug::ERROR) << "Invalid remote address: \"" << host_description << "\"";
    return false;
  }

  // Extract the host name / IP address.
  auto host_end = colon_pos;
  if (host_end == std::string::npos) {
    host_end = slash_pos;
  }
  if (host_end == std::string::npos) {
    host_end = host_description.size();
  }
  host = host_description.substr(0, host_end);
  if (host.empty()) {
    debug::log(debug::ERROR) << "Invalid remote address: \"" << host_description << "\"";
    return false;
  }

  // Extract the port.
  if (colon_pos != std::string::npos) {
    const auto port_end = (slash_pos != std::string::npos) ? slash_pos : host_description.size();
    const auto port_str = host_description.substr(colon_pos + 1, port_end - (colon_pos + 1));
    try {
      port = std::stoi(port_str);
    } catch (std::exception& e) {
      debug::log(debug::ERROR) << "Invalid remote address port: \"" << port_str << "\" ("
                               << e.what() << ")";
      return false;
    }
  } else {
    port = -1;
  }

  // Extract the path.
  if (slash_pos != std::string::npos) {
    path = host_description.substr(slash_pos + 1);
  } else {
    path = "";
  }

  return true;
}

int remote_cache_provider_t::connection_timeout_ms() {
  // We set this relatively low, since a high timeout value would defeat the purpose of BuildCache.
  // TODO(m): Make this configurable.
  return 100;
}

int remote_cache_provider_t::transfer_timeout_ms() {
  // Since we need to support large cache entries (hundreds of MiB), and we don't know the network
  // connection speed, set this high.
  // TODO(m): Make this configurable.
  return 10000;
}

}  // namespace bcache
