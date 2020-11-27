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

#include <base/hmac.hpp>

#ifdef HAS_OPENSSL
#include <openssl/hmac.h>
#elif defined(__APPLE__) && defined(__MACH__)
#include <CommonCrypto/CommonHMAC.h>
#define HAS_APPLE_CRYPTO
#elif defined(_WIN32)
#undef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#undef NOMINMAX
#define NOMINMAX
// Include order matters.
// clang-format off
#include <windows.h>
#include <wincrypt.h>
#include <cstring>
// clang-format on
#define HAS_WIN32_CRYPTO
#endif

#include <stdexcept>
#include <vector>

namespace bcache {

std::string sha1_hmac(const std::string& key, const std::string& data) {
  static const int DIGEST_SIZE = 20;  // SHA1
  std::vector<unsigned char> digest(DIGEST_SIZE);
#if defined(HAS_OPENSSL)
  (void)HMAC(EVP_sha1(),
             key.data(),
             static_cast<int>(key.size()),
             reinterpret_cast<const unsigned char*>(data.data()),
             data.size(),
             digest.data(),
             nullptr);
#elif defined(HAS_APPLE_CRYPTO)
  CCHmac(kCCHmacAlgSHA1, key.data(), key.size(), data.data(), data.size(), digest.data());
#elif defined(HAS_WIN32_CRYPTO)
  // Windows implementation inspired by:
  // https://docs.microsoft.com/en-us/windows/desktop/seccrypto/example-c-program--creating-an-hmac
  HCRYPTPROV crypt_prov = 0;
  HCRYPTKEY crypt_key = 0;
  HCRYPTHASH crypt_hash = 0;

  // Acquire a handle to the default RSA cryptographic service provider.
  if (!CryptAcquireContext(&crypt_prov, nullptr, nullptr, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
    throw std::runtime_error("Unable to acquire a crtypto context");
  }

  try {
    {
      // Import the key as a plain text key blob. This is dirty.
      // See: https://stackoverflow.com/a/32365048
      using plain_text_key_blob_t = struct {
        BLOBHEADER hdr;
        DWORD key_length;
      };

      std::vector<BYTE> key_blob(sizeof(plain_text_key_blob_t) + key.size());
      plain_text_key_blob_t* kb = reinterpret_cast<plain_text_key_blob_t*>(key_blob.data());
      std::memset(kb, 0, sizeof(plain_text_key_blob_t));
      kb->hdr.aiKeyAlg = CALG_RC2;
      kb->hdr.bType = PLAINTEXTKEYBLOB;
      kb->hdr.bVersion = CUR_BLOB_VERSION;
      kb->hdr.reserved = 0;
      kb->key_length = static_cast<DWORD>(key.size());
      std::memcpy(&key_blob[sizeof(plain_text_key_blob_t)], key.data(), key.size());
      if (CryptImportKey(crypt_prov,
                         key_blob.data(),
                         static_cast<DWORD>(key_blob.size()),
                         0,
                         CRYPT_IPSEC_HMAC_KEY,
                         &crypt_key) == 0) {
        throw std::runtime_error("Unable to import the key");
      }
    }

    if (CryptCreateHash(crypt_prov, CALG_HMAC, crypt_key, 0, &crypt_hash) == 0) {
      throw std::runtime_error("Unable to create the hash object");
    }

    HMAC_INFO hmac_info;
    ZeroMemory(&hmac_info, sizeof(hmac_info));
    hmac_info.HashAlgid = CALG_SHA1;
    if (CryptSetHashParam(crypt_hash, HP_HMAC_INFO, reinterpret_cast<const BYTE*>(&hmac_info), 0) ==
        0) {
      throw std::runtime_error("Unable to set the hash parameters");
    }

    if (CryptHashData(crypt_hash,
                      reinterpret_cast<const BYTE*>(data.data()),
                      static_cast<DWORD>(data.size()),
                      0) == 0) {
      throw std::runtime_error("Unable to hash the data");
    }

    DWORD hash_len = 0;
    if (CryptGetHashParam(crypt_hash, HP_HASHVAL, 0, &hash_len, 0) == 0) {
      throw std::runtime_error("Unable to retrieve the hashed data");
    }
    digest.resize(hash_len);
    if (CryptGetHashParam(crypt_hash, HP_HASHVAL, digest.data(), &hash_len, 0) == 0) {
      throw std::runtime_error("Unable to retrieve the hashed data");
    }

    // Release resources.
    CryptDestroyHash(crypt_hash);
    CryptDestroyKey(crypt_key);
    CryptReleaseContext(crypt_prov, 0);
  } catch (...) {
    // Release resources...
    if (crypt_hash != 0U) {
      CryptDestroyHash(crypt_hash);
    }
    if (crypt_key != 0U) {
      CryptDestroyKey(crypt_key);
    }
    CryptReleaseContext(crypt_prov, 0);

    // ...and re-throw the exception.
    throw;
  }
#endif
  std::string result;
  result.reserve(digest.size());
  for (const auto& e : digest) {
    result += static_cast<char>(e);
  }
  return result;
}

}  // namespace bcache
