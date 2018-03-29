#ifndef BUILDCACHE_HASHER_HPP_
#define BUILDCACHE_HASHER_HPP_

#include <cstdint>
#include <string>

extern "C" {
#include <md4/md4.h>
}

namespace bcache {

class hasher_t {
public:
  class hash_t {
  public:
    uint8_t* data() {
      return &m_data[0];
    }

    /// @brief Convert a hash to a hexadecimal string.
    /// @returns A hexadecimal string representation of the given hash.
    std::string as_string() {
      static const char digits[17] = "0123456789abcdef";
      std::string result(SIZE * 2, '0');
      for (size_t i = 0; i < SIZE; ++i) {
        result[i * 2] = digits[m_data[i] >> 4];
        result[i * 2 + 1] = digits[m_data[i] & 0x0fu];
      }
      return result;
    }

  private:
    // The hash size is 128 bits.
    static const size_t SIZE = 16u;

    uint8_t m_data[SIZE];
  };

  hasher_t() {
    MD4_Init(&m_ctx);
  }

  /// @brief Update the hash with more data.
  /// @param text The data to hash.
  void update(const std::string& text) {
    MD4_Update(&m_ctx, text.data(), text.size());
  }

  /// @brief Finalize the hash calculation.
  /// @param[out] result The result of the hash.
  /// @note This method must only be called once.
  // void final(hash_t& result) {
  hash_t final() {
    hash_t result;
    MD4_Final(result.data(), &m_ctx);
    return result;
  }

private:
  MD4_CTX m_ctx;
};

}  // namespace bcache

#endif  // BUILDCACHE_HASHER_HPP_

