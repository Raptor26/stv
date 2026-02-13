/// @file test_latch.cpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// @copyright (c) 2026 Gagaring
///
/// MIT License:
///
/// Permission is hereby granted, free of charge, to any person obtaining a copy
/// of this software and associated documentation files (the 'Software'), to
/// deal in the Software without restriction, including without limitation the
/// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
/// sell copies of the Software, and to permit persons to whom the Software is
/// furnished to do so, subject to the following conditions:
///
/// The above copyright notice and this permission notice shall be included in
/// all copies or substantial portions of the Software.
///
/// THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
/// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
/// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
/// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
/// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
/// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
/// IN THE SOFTWARE.

#include "stv/latch.hpp"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

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
