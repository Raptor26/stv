/// @file test_latch.cpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.

#include <catch2/catch_test_macros.hpp>

#include "stv/latch.hpp"

// NOLINTBEGIN(*-magic-numbers, google-build-using-namespace,
// readability-function-cognitive-,
// cppcoreguidelines-avoid-non-const-global-variables)

TEST_CASE(
    "latch class tests", "[latch]")
{
    SECTION("Initial state is false")
    {
        stv::latch latch;
        REQUIRE_FALSE(latch.read());
    }
    SECTION("Update with false doesn't change state")
    {
        stv::latch latch;
        latch.update(false);
        REQUIRE_FALSE(latch.read());
    }
    SECTION("Update with true sets state to true")
    {
        stv::latch latch;
        latch.update(true);
        REQUIRE(latch.read());
    }
    SECTION("Read resets state to false")
    {
        stv::latch latch;
        latch.update(true);
        REQUIRE(latch.read());
        REQUIRE_FALSE(latch.read());
    }
    SECTION("Multiple updates with true keep state true")
    {
        stv::latch latch;
        latch.update(true);
        latch.update(true);
        REQUIRE(latch.read());
    }
    SECTION("Update after read works correctly")
    {
        stv::latch latch;
        latch.update(true);
        REQUIRE(latch.read());
        latch.update(true);
        REQUIRE(latch.read());
    }

    SECTION("Read after state set to false")
    {
        stv::latch latch;
        latch.update(true);
        latch.update(false);
        SECTION("Read like operator bool")
        {
            REQUIRE(latch);
            REQUIRE_FALSE(latch);
        }
        SECTION("Read directly")
        {
            REQUIRE(latch.read());
            REQUIRE_FALSE(latch.read());
        }
    }
}

// NOLINTEND(*-magic-numbers, google-build-using-namespace,
// readability-function-cognitive-,
// cppcoreguidelines-avoid-non-const-global-variables)
