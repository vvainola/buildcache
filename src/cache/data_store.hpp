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

#ifndef BUILDCACHE_DATA_STORE_HPP_
#define BUILDCACHE_DATA_STORE_HPP_

#include <base/time_utils.hpp>

#include <string>

namespace bcache {
/// @brief Provide access to a local key/value store.
///
/// The key/value store provides a convenient way to store data that is to be shared between
/// multiple BuildCache instances on a local machine.
///
/// The values are not stored indefinitely. For instance, the store may be cleared between system
/// sessions, and values that are stored in the data store may have an expiry time after which
/// they are considered invalid.
class data_store_t {
public:
  /// @brief A data item.
  class item_t {
  public:
    /// @brief Construct a valid data item.
    /// @param value The data string.
    item_t(const std::string& value) : m_value(value), m_is_valid(true) {
    }

    /// @brief Construct an invalid (empty) data item.
    item_t() {
    }

    /// @returns the data string.
    const std::string& value() const {
      return m_value;
    }

    /// @brief Check if the item is valid.
    /// @returns true if the item is valid, else false.
    bool is_valid() const {
      return m_is_valid;
    }

  private:
    const std::string m_value;
    const bool m_is_valid = false;
  };

  /// @brief Construct a data store.
  /// @param name Name of the data store.
  data_store_t(const std::string& name);

  /// @brief Add or overwrite a data item.
  /// @param key The ID of the item to store.
  /// @param value The data to store.
  /// @param timeout The life time of the item.
  void store_item(const std::string& key, const std::string& value, const time::seconds_t timeout);

  /// @brief Get a data item.
  /// @param key The key of the item to retrieve.
  /// @returns a valid item if one could be found, or an invalid (empty) item if the requested item
  /// could not be found, or if the item expiry time has past.
  item_t get_item(const std::string& key);

  /// @brief Remove a data item.
  /// @param key The ID of the item to retrieve.
  void remove_item(const std::string& key);

  /// @brief Clear the data store.
  void clear();

private:
  std::string make_file_path(const std::string& key);
  void perform_housekeeping();

  std::string m_root_dir;
};
}  // namespace bcache

#endif  // BUILDCACHE_DATA_STORE_HPP_
