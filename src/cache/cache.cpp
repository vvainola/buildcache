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

#include <cache/cache.hpp>

#include <base/debug_utils.hpp>
#include <base/file_utils.hpp>
#include <sys/perf_utils.hpp>

#include <iostream>

namespace bcache {
bool cache_t::lookup(const hasher_t::hash_t hash,
                     const std::map<std::string, std::string>& file_paths,
                     const bool allow_hard_links,
                     int& return_code) {
  // First try the local cache.
  if (lookup_in_local_cache(hash, file_paths, allow_hard_links, return_code)) {
    return true;
  }

  // Then try the remote cache.
  if (lookup_in_remote_cache(hash, file_paths, return_code)) {
    return true;
  }

  return false;
}

void cache_t::add(const hasher_t::hash_t hash,
                  const cache_entry_t& entry,
                  const std::map<std::string, std::string>& file_paths,
                  const bool allow_hard_links) {
  PERF_START(ADD_TO_CACHE);

  // Add the entry to the local cache.
  m_local_cache.add(hash, entry, file_paths, allow_hard_links);

  // Add the entry to the remote cache.
  if (m_remote_cache.is_connected()) {
    // Note: We always compress entries for the remote cache.
    const cache_entry_t remote_entry(entry.file_ids(),
                                     cache_entry_t::comp_mode_t::ALL,
                                     entry.std_out(),
                                     entry.std_err(),
                                     entry.return_code());
    m_remote_cache.add(hash, entry, file_paths);
  }

  PERF_STOP(ADD_TO_CACHE);
}

bool cache_t::lookup_in_local_cache(const hasher_t::hash_t hash,
                                    const std::map<std::string, std::string>& file_paths,
                                    const bool allow_hard_links,
                                    int& return_code) {
  PERF_START(CACHE_LOOKUP);
  // Note: The lookup will give us a lock file that is locked until we go out of scope.
  auto lookup_result = m_local_cache.lookup(hash);
  const auto& cached_entry = lookup_result.first;
  PERF_STOP(CACHE_LOOKUP);

  if (!cached_entry) {
    return false;
  }

  // Copy all files from the cache to their respective target paths.
  // Note: If there is a mismatch in the expected (target) files and the actual (cached)
  // files, this will throw an exception (i.e. fall back to full program execution).
  PERF_START(RETRIEVE_CACHED_FILES);
  for (const auto& file_id : cached_entry.file_ids()) {
    const auto& target_path = file_paths.at(file_id);
    debug::log(debug::INFO) << "Cache hit (" << hash.as_string() << "): " << file_id << " => "
                            << target_path;
    const auto is_compressed = (cached_entry.compression_mode() == cache_entry_t::comp_mode_t::ALL);
    m_local_cache.get_file(hash, file_id, target_path, is_compressed, allow_hard_links);
  }
  PERF_STOP(RETRIEVE_CACHED_FILES);

  // Return/print the cached program results.
  std::cout << cached_entry.std_out();
  std::cerr << cached_entry.std_err();
  return_code = cached_entry.return_code();

  return true;
}

bool cache_t::lookup_in_remote_cache(const hasher_t::hash_t hash,
                                     const std::map<std::string, std::string>& file_paths,
                                     int& return_code) {
  // Start by trying to connect to the remote cache.
  if (!m_remote_cache.connect()) {
    return false;
  }

  PERF_START(CACHE_LOOKUP);
  const auto cached_entry = m_remote_cache.lookup(hash);
  PERF_STOP(CACHE_LOOKUP);

  if (!cached_entry) {
    return false;
  }

  // Copy all files from the cache to their respective target paths.
  // Note: If there is a mismatch in the expected (target) files and the actual (cached)
  // files, this will throw an exception (i.e. fall back to full program execution).
  PERF_START(RETRIEVE_CACHED_FILES);
  for (const auto& file_id : cached_entry.file_ids()) {
    const auto& target_path = file_paths.at(file_id);
    debug::log(debug::INFO) << "Remote cache hit (" << hash.as_string() << "): " << file_id
                            << " => " << target_path;
    const auto is_compressed = (cached_entry.compression_mode() == cache_entry_t::comp_mode_t::ALL);
    m_remote_cache.get_file(hash, file_id, target_path, is_compressed);
  }
  PERF_STOP(RETRIEVE_CACHED_FILES);

  // Return/print the cached program results.
  std::cout << cached_entry.std_out();
  std::cerr << cached_entry.std_err();
  return_code = cached_entry.return_code();

  return true;
}
}  // namespace bcache
