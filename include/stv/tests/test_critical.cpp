/// @file test_critical.cpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.

#include "stv/critical.hpp"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <mutex>

// NOLINTBEGIN(*-magic-numbers, google-build-using-namespace,
// readability-function-cognitive-,
// cppcoreguidelines-avoid-non-const-global-variables)

namespace {

std::size_t lock_cnt{0};

void        lock() { ++lock_cnt; }

void        unlock() { --lock_cnt; }

} // namespace

TEST_CASE(
    "critical", "[stv]")
{
    SECTION("Call like free functions")
    {
        SECTION("Like args")
        {
            REQUIRE(lock_cnt == 0U);
            {
                stv::critical<lock, unlock> critical;
                REQUIRE(critical.get_glob_nesting_cnt() == 0U);

                std::scoped_lock lock{critical};
                REQUIRE(lock_cnt == 1U);
                REQUIRE(critical.get_glob_nesting_cnt() == 1U);
            }
            REQUIRE(lock_cnt == 0U);
        }

        SECTION("Like pointers")
        {
            REQUIRE(lock_cnt == 0U);
            {
                stv::critical<&lock, &unlock> critical;

                std::scoped_lock              lock{critical};
                REQUIRE(lock_cnt == 1U);
            }
            REQUIRE(lock_cnt == 0U);
        }
    }

    SECTION("Multiple critical sections")
    {
        stv::critical<lock, unlock> critical_i2c;
        stv::critical<lock, unlock> critical_spi;

        REQUIRE(critical_i2c.get_glob_nesting_cnt()
                == critical_spi.get_glob_nesting_cnt());
        {
            std::scoped_lock lock_i2c{critical_i2c};

            REQUIRE(critical_i2c.get_glob_nesting_cnt() == 1U);
            REQUIRE(critical_i2c.get_glob_nesting_cnt()
                    == critical_spi.get_glob_nesting_cnt());

            std::scoped_lock lock_spi{critical_spi};

            // т.к. critical_i2c и critical_spi имеют один глобальный счетчик
            // вложенности, то ожидается, что вызов  std::scoped_lock
            // lock_spi{critical_spi}; увеличит глобальный общий счетчик.
            REQUIRE(critical_spi.get_glob_nesting_cnt() == 2U);
            REQUIRE(critical_i2c.get_glob_nesting_cnt()
                    == critical_spi.get_glob_nesting_cnt());
        }

        REQUIRE(critical_i2c.get_glob_nesting_cnt()
                == critical_spi.get_glob_nesting_cnt());
        REQUIRE(critical_i2c.get_glob_nesting_cnt() == 0U);
    }
}

// NOLINTEND(*-magic-numbers, google-build-using-namespace,
// readability-function-cognitive-,
// cppcoreguidelines-avoid-non-const-global-variables)
