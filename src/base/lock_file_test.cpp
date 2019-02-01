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

#include <base/lock_file.hpp>

#include <base/file_utils.hpp>

#include <doctest/doctest.h>

using namespace bcache;

TEST_CASE("lock_file_t constructors are behaving as expected") {
  SUBCASE("Default constructor holds no lock") {
    file::lock_file_t lock;
    CHECK_EQ(lock.has_lock(), false);
  }
}

TEST_CASE("Acquiring a lock creates and removes a file") {
  // Get a temporary file name.
  file::tmp_file_t tmp_file(file::get_temp_dir(), ".lock");
  REQUIRE_GT(tmp_file.path().size(), 0);

  // The file should not yet exist.
  CHECK_EQ(file::file_exists(tmp_file.path()), false);

  // Aquire a lock in a scope.
  {
    file::lock_file_t lock(tmp_file.path());

    // We should now have the lock, and the file should exist.
    CHECK_EQ(lock.has_lock(), true);
    CHECK_EQ(file::file_exists(tmp_file.path()), true);
  }

  // The lock file should no longer exist after the lock has gone out of scope.
  CHECK_EQ(file::file_exists(tmp_file.path()), false);
}

TEST_CASE("Transfering lock ownership works as expected") {
  // Get a temporary file name.
  file::tmp_file_t tmp_file(file::get_temp_dir(), ".lock");
  REQUIRE_GT(tmp_file.path().size(), 0);

  // The file should not yet exist.
  CHECK_EQ(file::file_exists(tmp_file.path()), false);

  {
    file::lock_file_t lock;

    // Aquire a lock in a scope.
    {
      file::lock_file_t child_lock(tmp_file.path());

      // We should now have the lock, and the file should exist.
      CHECK_EQ(child_lock.has_lock(), true);
      CHECK_EQ(file::file_exists(tmp_file.path()), true);

      // Move the lock object to the parent scope.
      lock = std::move(child_lock);

      // The lock in the child scope should no longer have the lock, but the file should still
      // exist.
      CHECK_EQ(child_lock.has_lock(), false);
      CHECK_EQ(file::file_exists(tmp_file.path()), true);
    }

    // The lock in the parent scope should now have the lock, and the file should still exist.
    // We should now have the lock, and the file should exist.
    CHECK_EQ(lock.has_lock(), true);
    CHECK_EQ(file::file_exists(tmp_file.path()), true);
  }

  // The lock file should no longer exist after the lock has gone out of scope.
  CHECK_EQ(file::file_exists(tmp_file.path()), false);
}
