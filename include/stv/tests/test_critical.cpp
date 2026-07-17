/// @file test_critical.cpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.

#include "stv/critical.hpp"
#include "stv/mutex_guard.hpp"
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

namespace critical_with_isr_test {

struct fixture {
    inline static std::size_t lock_cnt{0};
    inline static std::size_t unlock_cnt{0};
    inline static std::size_t lock_isr_cnt{0};
    inline static std::size_t unlock_isr_cnt{0};

    static void               lock() { ++lock_cnt; }

    static void               unlock() { ++unlock_cnt; }

    static void               lock_isr() { ++lock_isr_cnt; }

    static void               unlock_isr() { ++unlock_isr_cnt; }

    static void               reset()
    {
        lock_cnt       = 0U;
        unlock_cnt     = 0U;
        lock_isr_cnt   = 0U;
        unlock_isr_cnt = 0U;
    }
};

using critical = stv::critical_with_isr<fixture::lock, fixture::unlock,
                                        fixture::lock_isr, fixture::unlock_isr>;

} // namespace critical_with_isr_test

TEST_CASE(
    "critical_with_isr", "[stv]")
{
    using critical_with_isr_test::critical;
    using critical_with_isr_test::fixture;

    SECTION("std::lock_guard uses non-ISR handlers by default")
    {
        fixture::reset();

        critical critical_section;

        REQUIRE(critical_section.get_glob_nesting_cnt() == 0U);

        {
            std::lock_guard lock{critical_section};
            REQUIRE(fixture::lock_cnt == 1U);
            REQUIRE(fixture::unlock_cnt == 0U);
            REQUIRE(fixture::lock_isr_cnt == 0U);
            REQUIRE(fixture::unlock_isr_cnt == 0U);
            REQUIRE(critical_section.get_glob_nesting_cnt() == 1U);
        }

        REQUIRE(fixture::lock_cnt == 1U);
        REQUIRE(fixture::unlock_cnt == 1U);
        REQUIRE(fixture::lock_isr_cnt == 0U);
        REQUIRE(fixture::unlock_isr_cnt == 0U);
        REQUIRE(critical_section.get_glob_nesting_cnt() == 0U);
    }

    SECTION("stv::lock_guard with is_isr=false uses non-ISR handlers")
    {
        fixture::reset();

        critical critical_section;

        REQUIRE(critical_section.get_glob_nesting_cnt() == 0U);

        {
            stv::lock_guard lock{critical_section, false};
            REQUIRE(fixture::lock_cnt == 1U);
            REQUIRE(fixture::unlock_cnt == 0U);
            REQUIRE(fixture::lock_isr_cnt == 0U);
            REQUIRE(fixture::unlock_isr_cnt == 0U);
            REQUIRE(critical_section.get_glob_nesting_cnt() == 1U);
        }

        REQUIRE(fixture::lock_cnt == 1U);
        REQUIRE(fixture::unlock_cnt == 1U);
        REQUIRE(fixture::lock_isr_cnt == 0U);
        REQUIRE(fixture::unlock_isr_cnt == 0U);
        REQUIRE(critical_section.get_glob_nesting_cnt() == 0U);
    }

    SECTION("stv::lock_guard with is_isr=true uses ISR handlers")
    {
        fixture::reset();

        critical critical_section;

        REQUIRE(critical_section.get_glob_nesting_cnt() == 0U);

        {
            stv::lock_guard lock{critical_section, true};
            REQUIRE(fixture::lock_cnt == 0U);
            REQUIRE(fixture::unlock_cnt == 0U);
            REQUIRE(fixture::lock_isr_cnt == 1U);
            REQUIRE(fixture::unlock_isr_cnt == 0U);
            REQUIRE(critical_section.get_glob_nesting_cnt() == 1U);
        }

        REQUIRE(fixture::lock_cnt == 0U);
        REQUIRE(fixture::unlock_cnt == 0U);
        REQUIRE(fixture::lock_isr_cnt == 1U);
        REQUIRE(fixture::unlock_isr_cnt == 1U);
        REQUIRE(critical_section.get_glob_nesting_cnt() == 0U);
    }

    SECTION("Nested std::lock_guard calls handlers only on outer enter/exit")
    {
        fixture::reset();

        critical critical_outer;
        critical critical_inner;

        {
            std::lock_guard lock_outer{critical_outer};
            REQUIRE(fixture::lock_cnt == 1U);
            REQUIRE(fixture::unlock_cnt == 0U);
            REQUIRE(fixture::lock_isr_cnt == 0U);
            REQUIRE(fixture::unlock_isr_cnt == 0U);
            REQUIRE(critical_outer.get_glob_nesting_cnt() == 1U);

            {
                std::lock_guard lock_inner{critical_inner};
                REQUIRE(fixture::lock_cnt == 1U);
                REQUIRE(fixture::unlock_cnt == 0U);
                REQUIRE(fixture::lock_isr_cnt == 0U);
                REQUIRE(fixture::unlock_isr_cnt == 0U);
                REQUIRE(critical_inner.get_glob_nesting_cnt() == 2U);
            }

            REQUIRE(fixture::lock_cnt == 1U);
            REQUIRE(fixture::unlock_cnt == 0U);
            REQUIRE(fixture::lock_isr_cnt == 0U);
            REQUIRE(fixture::unlock_isr_cnt == 0U);
            REQUIRE(critical_outer.get_glob_nesting_cnt() == 1U);
        }

        REQUIRE(fixture::lock_cnt == 1U);
        REQUIRE(fixture::unlock_cnt == 1U);
        REQUIRE(fixture::lock_isr_cnt == 0U);
        REQUIRE(fixture::unlock_isr_cnt == 0U);
        REQUIRE(critical_outer.get_glob_nesting_cnt() == 0U);
    }

    SECTION("Nested stv::lock_guard with mixed ISR contexts")
    {
        fixture::reset();

        critical critical_outer;
        critical critical_inner;

        {
            stv::lock_guard lock_outer{critical_outer, true};
            REQUIRE(fixture::lock_cnt == 0U);
            REQUIRE(fixture::unlock_cnt == 0U);
            REQUIRE(fixture::lock_isr_cnt == 1U);
            REQUIRE(fixture::unlock_isr_cnt == 0U);
            REQUIRE(critical_outer.get_glob_nesting_cnt() == 1U);

            {
                stv::lock_guard lock_inner{critical_inner, false};
                REQUIRE(fixture::lock_cnt == 0U);
                REQUIRE(fixture::unlock_cnt == 0U);
                REQUIRE(fixture::lock_isr_cnt == 1U);
                REQUIRE(fixture::unlock_isr_cnt == 0U);
                REQUIRE(critical_inner.get_glob_nesting_cnt() == 2U);
            }

            REQUIRE(fixture::lock_cnt == 0U);
            REQUIRE(fixture::unlock_cnt == 0U);
            REQUIRE(fixture::lock_isr_cnt == 1U);
            REQUIRE(fixture::unlock_isr_cnt == 0U);
            REQUIRE(critical_outer.get_glob_nesting_cnt() == 1U);
        }

        REQUIRE(fixture::lock_cnt == 0U);
        REQUIRE(fixture::unlock_cnt == 0U);
        REQUIRE(fixture::lock_isr_cnt == 1U);
        REQUIRE(fixture::unlock_isr_cnt == 1U);
        REQUIRE(critical_outer.get_glob_nesting_cnt() == 0U);
    }
}

// NOLINTEND(*-magic-numbers, google-build-using-namespace,
// readability-function-cognitive-,
// cppcoreguidelines-avoid-non-const-global-variables)
