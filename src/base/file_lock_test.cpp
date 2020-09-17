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

#include <base/file_lock.hpp>

#include <base/file_utils.hpp>

#include <doctest/doctest.h>

using namespace bcache;

TEST_CASE("file_lock_t constructors are behaving as expected") {
  SUBCASE("Default constructor holds no lock") {
    file::file_lock_t lock;
    CHECK_EQ(lock.has_lock(), false);
  }
}

TEST_CASE("Remote locks") {
  SUBCASE("Acquiring a lock creates and removes a file") {
    // Get a temporary file name.
    file::tmp_file_t tmp_file(file::get_temp_dir(), ".lock");
    REQUIRE_GT(tmp_file.path().size(), 0);

    // The file should not yet exist.
    CHECK_EQ(file::file_exists(tmp_file.path()), false);

    // Aquire a lock in a scope.
    {
      file::file_lock_t lock(tmp_file.path(), true);

      // We should now have the lock, and the file should exist.
      CHECK_EQ(lock.has_lock(), true);
      CHECK_EQ(file::file_exists(tmp_file.path()), true);
    }

    // The lock file should no longer exist after the lock has gone out of scope.
    CHECK_EQ(file::file_exists(tmp_file.path()), false);
  }

  SUBCASE("Transfering lock ownership works as expected") {
    // Get a temporary file name.
    file::tmp_file_t tmp_file(file::get_temp_dir(), ".lock");
    REQUIRE_GT(tmp_file.path().size(), 0);

    // The file should not yet exist.
    CHECK_EQ(file::file_exists(tmp_file.path()), false);

    {
      file::file_lock_t lock;

      // Aquire a lock in a scope.
      {
        file::file_lock_t child_lock(tmp_file.path(), true);

        // We should now have the lock, and the file should exist.
        CHECK_EQ(child_lock.has_lock(), true);
        CHECK_EQ(file::file_exists(tmp_file.path()), true);

        // Move the lock object to the parent scope.
        lock = std::move(child_lock);

        // TThe ownership of the lock should now have moved, and the file should still exist.
        CHECK_EQ(child_lock.has_lock(), false);
        CHECK_EQ(lock.has_lock(), true);
        CHECK_EQ(file::file_exists(tmp_file.path()), true);
      }

      // The lock in the parent scope should now have the lock, and the file should still exist.
      CHECK_EQ(lock.has_lock(), true);
      CHECK_EQ(file::file_exists(tmp_file.path()), true);
    }

    // The lock file should no longer exist after the lock has gone out of scope.
    CHECK_EQ(file::file_exists(tmp_file.path()), false);
  }
}

TEST_CASE("Local locks") {
  SUBCASE("Acquiring a lock creates and removes a file") {
    // Get a temporary file name.
    file::tmp_file_t tmp_file(file::get_temp_dir(), ".lock");
    REQUIRE_GT(tmp_file.path().size(), 0);

    // Repeatedly aquire and release a lock in a loop scope.
    for (int i = 0; i < 10; ++i) {
      file::file_lock_t lock(tmp_file.path(), false);

      // We should now have the lock.
      CHECK_EQ(lock.has_lock(), true);
    }
  }

  SUBCASE("Transfering lock ownership works as expected") {
    // Get a temporary file name.
    file::tmp_file_t tmp_file(file::get_temp_dir(), ".lock");
    REQUIRE_GT(tmp_file.path().size(), 0);

    {
      file::file_lock_t lock;

      // Aquire a lock in a scope.
      {
        file::file_lock_t child_lock(tmp_file.path(), false);

        // We should now have the lock.
        CHECK_EQ(child_lock.has_lock(), true);

        // Move the lock object to the parent scope.
        lock = std::move(child_lock);

        // The ownership of the lock should now have moved.
        CHECK_EQ(child_lock.has_lock(), false);
        CHECK_EQ(lock.has_lock(), true);
      }

      // The lock in the parent scope should now have the lock.
      CHECK_EQ(lock.has_lock(), true);
    }
  }
}
