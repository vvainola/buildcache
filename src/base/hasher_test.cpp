//--------------------------------------------------------------------------------------------------
// Copyright (c) 2021 Marcus Geelnard
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

#include <base/hasher.hpp>

#include <doctest/doctest.h>

#include <string>

// Workaround for macOS build errors.
// See: https://github.com/onqtam/doctest/issues/126
#include <iostream>

using namespace bcache;

TEST_CASE("hasher_t() produces expected results") {
  SUBCASE("Case 1 - string") {
    const std::string data("Hello world!");
    hasher_t hasher;
    hasher.update(data);
    const auto result = hasher.final().as_string();
    CHECK_EQ(result, "f77fd43775f580e50d008636dd245e8b");
  }

  SUBCASE("Case 2 - raw data") {
    const unsigned char data[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17};
    hasher_t hasher;
    hasher.update(data, sizeof(data) / sizeof(data[0]));
    const auto result = hasher.final().as_string();
    CHECK_EQ(result, "232cafb06bc9a3e38d6d57b56b99ce87");
  }
}

TEST_CASE("hasher_t() can be copied") {
  SUBCASE("Copy constructor copies state") {
    // Create a hasher_t object and start hashing.
    hasher_t hasher1;
    hasher1.update("This is a test string that we want to hash...");

    // Make a copy of the first hasher_t object.
    hasher_t hasher2(hasher1);

    // The two hashers should produce identical results.
    const auto result1 = hasher1.final().as_string();
    const auto result2 = hasher2.final().as_string();
    CHECK_EQ(result1, result2);
    CHECK_EQ(result1, "327d15b124ea5ca1068f3dacc4a89bfa");
  }

  SUBCASE("Copy constructor creates a unique copy") {
    // Create a hasher_t object and start hashing.
    hasher_t hasher1;
    hasher1.update("Bla bla bla bla - 1 2 43 45 6 76 87!?");

    // Make a copy of the first hasher_t object.
    hasher_t hasher2(hasher1);

    // Hash more data with the first hasher_t object.
    hasher1.update("here comes more data");

    // The two hashers should produce different results.
    const auto result1 = hasher1.final().as_string();
    const auto result2 = hasher2.final().as_string();
    CHECK_EQ(result1, "1bebe04c3c403b5657b0203c63ecc288");
    CHECK_EQ(result2, "485f60a2f429a735f284a0d6539549aa");
  }

  SUBCASE("Assignment operator copies state") {
    // Create a hasher_t object and start hashing.
    hasher_t hasher1;
    hasher1.update("This is another string that goes to the hash...");

    // Make a copy of the first hasher_t object.
    hasher_t hasher2;
    hasher2 = hasher1;

    // The two hashers should produce identical results.
    const auto result1 = hasher1.final().as_string();
    const auto result2 = hasher2.final().as_string();
    CHECK_EQ(result1, result2);
    CHECK_EQ(result1, "f37b1731a876c5ba4adca41c3e38ee7c");
  }

  SUBCASE("Assignment operator creates a unique copy") {
    // Create a hasher_t object and start hashing.
    hasher_t hasher1;
    hasher1.update("Suspendisse id rutrum libero. Proin sollicitudin mollis neque eget malesuada.");

    // Make a copy of the first hasher_t object.
    hasher_t hasher2;
    hasher2 = hasher1;

    // Hash more data with the first hasher_t object.
    hasher1.update("Suspendisse potenti. Praesent dignissim pulvinar blandit.");

    // The two hashers should produce different results.
    const auto result1 = hasher1.final().as_string();
    const auto result2 = hasher2.final().as_string();
    CHECK_EQ(result1, "2a37863a467602317bf176453d4d6b4a");
    CHECK_EQ(result2, "b58be4dce016a838d28afd10f0fb7ee5");
  }
}
