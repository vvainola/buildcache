//--------------------------------------------------------------------------------------------------
// Copyright (c) 2020 Marcus Geelnard
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

#include <cache/data_store.hpp>

#include <base/debug_utils.hpp>
#include <base/file_utils.hpp>
#include <config/configuration.hpp>

#include <chrono>
#include <cstdint>
#include <stdexcept>

namespace bcache {

namespace {
struct decode_exception_t : public std::runtime_error {
  decode_exception_t(const std::string& what) : std::runtime_error(what) {
  }
};

bool is_time_for_housekeeping() {
  // Get the time since the epoch, in microseconds.
  const auto t =
      std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::system_clock::now())
          .time_since_epoch()
          .count();

  // Scramble the timestamp (to make up for possible problems with timer accuracy).
  const auto rnd = (t ^ (t >> 7)) ^ ((t >> 14) ^ (t >> 20));

  // Only perform housekeeping 0.1% of the times that we are called.
  // Note: We should not need to do this very often, since meta data should be small, and most of
  // the items should be self-purging (whenever an expired item is requested it is removed).
  return (rnd % 1000L) == 0L;
}

std::string encode_file_time(const time::seconds_t t) {
  std::string str(8, 0);
  str[0] = static_cast<char>(static_cast<uint8_t>(t));
  str[1] = static_cast<char>(static_cast<uint8_t>(t >> 8));
  str[2] = static_cast<char>(static_cast<uint8_t>(t >> 16));
  str[3] = static_cast<char>(static_cast<uint8_t>(t >> 24));
  str[4] = static_cast<char>(static_cast<uint8_t>(t >> 32));
  str[5] = static_cast<char>(static_cast<uint8_t>(t >> 40));
  str[6] = static_cast<char>(static_cast<uint8_t>(t >> 48));
  str[7] = static_cast<char>(static_cast<uint8_t>(t >> 56));
  return str;
}

time::seconds_t decode_file_time(const std::string& str) {
  return static_cast<time::seconds_t>(static_cast<uint8_t>(str[0])) |
         (static_cast<time::seconds_t>(static_cast<uint8_t>(str[1])) << 8) |
         (static_cast<time::seconds_t>(static_cast<uint8_t>(str[2])) << 16) |
         (static_cast<time::seconds_t>(static_cast<uint8_t>(str[3])) << 24) |
         (static_cast<time::seconds_t>(static_cast<uint8_t>(str[4])) << 32) |
         (static_cast<time::seconds_t>(static_cast<uint8_t>(str[5])) << 40) |
         (static_cast<time::seconds_t>(static_cast<uint8_t>(str[6])) << 48) |
         (static_cast<time::seconds_t>(static_cast<uint8_t>(str[7])) << 56);
}

char to_hex_4bit(uint8_t c) {
  static const char HEX_LUT[16] = {
      '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
  return HEX_LUT[c & 15];
}

uint8_t from_hex_4bit(char c) {
  if ((c >= '0') && (c <= '9')) {
    return static_cast<uint8_t>(c - '0');
  }
  if ((c >= 'a') && (c <= 'f')) {
    return static_cast<uint8_t>(c - 'a' + 10);
  }

  throw decode_exception_t("Invalid hex character: " + std::string({c}));
}

bool is_literal_key_char(char c) {
  // We allow lower case ASCII alpha-numeric characters plus '_' and '-', as those are valid on all
  // (resonable) file systems without being subject to case insensitivity issues. The period
  // character ('.') is reserved as a prefix for hex encoding.
  return ((c >= '0') && (c <= '9')) || ((c >= 'a') && (c <= 'z')) || (c == '_') || (c == '-');
}

std::string decode_key(const std::string& key_encoded) {
  // Sanitize the key to be used as a file name.
  std::string key;
  for (auto it = key_encoded.cbegin(); it != key_encoded.cend();) {
    const auto c = *it++;
    if (is_literal_key_char(c)) {
      // Literal character: Pass through as is.
      key += c;
    } else if (c == '.') {
      // Decode the hex encoded character.
      if (it == key_encoded.cend()) {
        throw decode_exception_t("Premature end of encoded key");
      }
      const auto c1 = *it++;
      if (it == key_encoded.cend()) {
        throw decode_exception_t("Premature end of encoded key");
      }
      const auto c2 = *it++;
      const auto h1 = from_hex_4bit(c1);
      const auto h2 = from_hex_4bit(c2);
      key += static_cast<char>((h1 << 4) | h2);
    } else {
      throw decode_exception_t("Illegal data store key: " + key_encoded);
    }
  }

  return key;
}

std::string encode_key(const std::string& key) {
  std::string key_encoded;
  for (const auto& c : key) {
    if (is_literal_key_char(c)) {
      // Literal character: Pass through as is.
      key_encoded += c;
    } else {
      // Non-literal character: Convert to hex - it never fails.
      key_encoded += '.';
      key_encoded += to_hex_4bit(static_cast<uint8_t>(c) >> 4);
      key_encoded += to_hex_4bit(static_cast<uint8_t>(c));
    }
  }

  return key_encoded;
}

}  // namespace

data_store_t::data_store_t(const std::string& name) {
  // Define the data store root directory.
  m_root_dir = file::append_path(config::dir(), name);

  // Occassionally perform housekeeping.
  if (is_time_for_housekeeping()) {
    perform_housekeeping();
  }
}

void data_store_t::store_item(const std::string& key,
                              const std::string& value,
                              const time::seconds_t timeout) {
  try {
    // Make sure that we have a local data folder to work in.
    file::create_dir_with_parents(m_root_dir);

    const auto file_path = make_file_path(key);
    const auto raw_data = encode_file_time(time::seconds_since_epoch() + timeout) + value;

    // Save to a temporary file first and once the write operation has succeeded rename it to the
    // target file.
    auto tmp_file = file::tmp_file_t(file::get_dir_part(file_path), ".tmp");
    file::write(raw_data, tmp_file.path());

    // Move the temporary file to its target name.
    file::move(tmp_file.path(), file_path);
  } catch (...) {
    // We just silence errors, since data items are volatile anyway.
  }
}

data_store_t::item_t data_store_t::get_item(const std::string& key) {
  try {
    // Read and decode the data item.
    const auto raw_data = file::read(make_file_path(key));
    if (raw_data.size() < 8) {
      debug::log(debug::WARNING) << "Removing broken data store item \"" << key << "\"";
      remove_item(key);
      return item_t();
    }
    const auto expires = decode_file_time(raw_data);
    if (expires < time::seconds_since_epoch()) {
      debug::log(debug::DEBUG) << "Removing expired data store item \"" << key << "\"";
      remove_item(key);
      return item_t();
    }
    return item_t(raw_data.substr(8));
  } catch (std::runtime_error&) {
    return item_t();
  }
}

void data_store_t::remove_item(const std::string& key) {
  file::remove_file(make_file_path(key), true);
}

void data_store_t::clear() {
  // Remove all data store items.
  if (file::dir_exists(m_root_dir)) {
    const auto files = file::walk_directory(m_root_dir);
    for (const auto& file : files) {
      if (!file.is_dir()) {
        file::remove_file(file.path());
      }
    }
  }
}

std::string data_store_t::make_file_path(const std::string& key) {
  return file::append_path(m_root_dir, encode_key(key));
}

void data_store_t::perform_housekeeping() {
  debug::log(debug::INFO) << "Performing housekeeping for data store \""
                          << file::get_file_part(m_root_dir) << "\"...";

  // Iterate over all data store items with get_item(), which will remove the items that have
  // expired.
  if (file::dir_exists(m_root_dir)) {
    const auto files = file::walk_directory(m_root_dir);
    for (const auto& file : files) {
      if (!file.is_dir()) {
        const auto encoded_key = file::get_file_part(file.path());
        try {
          const auto key = decode_key(encoded_key);
          (void)get_item(key);
        } catch (const decode_exception_t& e) {
          // If we could not decode the key, it is bogus and should be removed
          // from the file system.
          debug::log(debug::WARNING)
              << "Removing bogus data store item \"" << encoded_key << "\" (" << e.what() << ")";
          file::remove_file(file.path(), true);
        }
      }
    }
  }
}

}  // namespace bcache
