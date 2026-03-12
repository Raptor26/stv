/// @file test_deadline_timer.cpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.

#include "stv/deadline_timer.hpp"
#include "stv/runtime.hpp"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <fakeit.hpp>
#include <limits>
#include <ratio>

// NOLINTBEGIN(*-magic-numbers, google-build-using-namespace,
// readability-function-cognitive-,
// cppcoreguidelines-avoid-non-const-global-variables)

TEST_CASE(
    "deadline Ctor", "[stv]")
{
    using namespace fakeit;
    using namespace stv;

    using runtime_setup_type  = stv::runtime_setup<stv::runtime_counter_type>;
    using runtime_type        = stv::runtime<runtime_setup_type>;
    using deadline_setup_type = stv::deadline_timer_setup<runtime_type>;

    SECTION("Default Ctor")
    {
        deadline_setup_type setup;
        stv::deadline_timer deadline{setup};
        REQUIRE_FALSE(deadline);
    }

    SECTION("Valid Ctor")
    {
        Mock<runtime_type>  runtime_mock;
        deadline_setup_type setup{.runtime = &runtime_mock.get()};
        stv::deadline_timer deadline{setup};
        REQUIRE(deadline);
    }
}

TEST_CASE(
    "deadline", "[stv]")
{
    using namespace fakeit;
    using namespace stv;
    using namespace std::chrono_literals;

    using runtime_setup_type  = stv::runtime_setup<stv::runtime_counter_type>;
    using runtime_type        = stv::runtime<runtime_setup_type>;
    using deadline_setup_type = deadline_timer_setup<runtime_type>;

    Mock<runtime_type> runtime_mock;
    Fake(Method(runtime_mock, get));

    SECTION(
        "Check elapsed if not started with is_elapsed_if_not_started = false")
    {
        stv::deadline_timer deadline{
            deadline_setup_type{.runtime = &runtime_mock.get(),
                                .is_elapsed_if_not_started = false}};
        REQUIRE_FALSE(deadline.is_elapsed());
    }

    deadline_setup_type setup{.runtime = &runtime_mock.get()};
    stv::deadline_timer deadline{setup};
    REQUIRE(deadline);

    SECTION("Check elapsed if not started") { REQUIRE(deadline.is_elapsed()); }

    SECTION("Set period")
    {
        SECTION("Check API")
        {
            deadline.set_delay(stv::runtime_counter_type{2s});
            REQUIRE_FALSE(deadline.is_elapsed());

            deadline.set_delay(3s);
            REQUIRE_FALSE(deadline.is_elapsed());
        }

        SECTION("Set zero timeout")
        {
            deadline.set_delay(0s);
            REQUIRE(deadline.is_elapsed());
        }

        SECTION("Set timeout and check is elapsed")
        {
            constexpr runtime_counter_type elapsed_time{2s};
            When(Method(runtime_mock, get)).Return(0s, elapsed_time - 100ms);
            deadline.set_delay(elapsed_time);
            REQUIRE_FALSE(deadline.is_elapsed());

            When(Method(runtime_mock, get)).AlwaysReturn(elapsed_time);
            REQUIRE(deadline.is_elapsed());

            // Повторный вызов всегда возвращает true.
            REQUIRE(deadline.is_elapsed());
        }

        SECTION("Create deadline and start it in Ctor")
        {
            constexpr runtime_counter_type elapsed_time{2s};
            When(Method(runtime_mock, get)).Return(0s, elapsed_time - 100ms);

            stv::deadline_timer deadline_started{deadline_setup_type{
                .runtime = &runtime_mock.get(), .delay = elapsed_time}};
            REQUIRE_FALSE(deadline_started.is_elapsed());

            When(Method(runtime_mock, get)).AlwaysReturn(elapsed_time);
            REQUIRE(deadline_started.is_elapsed());
        }

        SECTION("Set timeout and check is elapsed with overflow")
        {
            constexpr runtime_counter_type      elapsed_time{2s};
            constexpr runtime_counter_type::rep raw_time_offset{10};
            constexpr runtime_counter_type      start_time{
                std::numeric_limits<runtime_counter_type::rep>::max()
                - raw_time_offset};

            When(Method(runtime_mock, get))
                .Return(start_time, start_time + (elapsed_time - 100ms));
            deadline.set_delay(elapsed_time);
            REQUIRE_FALSE(deadline.is_elapsed());

            When(Method(runtime_mock, get))
                .AlwaysReturn(start_time + elapsed_time);
            REQUIRE(deadline.is_elapsed());
        }
    }
}

TEST_CASE(
    "deadline overflow", "[stv]")
{
    using namespace fakeit;
    using namespace stv;
    using namespace std::chrono_literals;

    SECTION("std::uint32_t, std::micro")
    {
        using counter_type = std::chrono::duration<std::uint32_t, std::micro>;
        using runtime_setup_type  = stv::runtime_setup<counter_type>;
        using runtime_type        = stv::runtime<runtime_setup_type>;
        using deadline_setup_type = deadline_timer_setup<runtime_type>;

        Mock<runtime_type> runtime_mock;
        Fake(Method(runtime_mock, get));

        stv::deadline_timer deadline{
            deadline_setup_type{.runtime = &runtime_mock.get(),
                                .is_elapsed_if_not_started = false}};

        REQUIRE(deadline.set_delay(1h));
        REQUIRE(deadline.set_delay(std::chrono::hours{1}));

        REQUIRE_FALSE(deadline.set_delay(2h));
        REQUIRE_FALSE(deadline.set_delay(std::chrono::hours{2}));

        REQUIRE_FALSE(deadline.set_delay(std::chrono::days{1}));
        REQUIRE_FALSE(deadline.set_delay(std::chrono::days{28}));

        // Compile time.
        static_assert(deadline.can_set_delay(1h), "Overflow detected");
    }

    SECTION("std::uint32_t, std::milli")
    {
        using counter_type = std::chrono::duration<std::uint32_t, std::milli>;
        using runtime_setup_type  = stv::runtime_setup<counter_type>;
        using runtime_type        = stv::runtime<runtime_setup_type>;
        using deadline_setup_type = deadline_timer_setup<runtime_type>;

        Mock<runtime_type> runtime_mock;
        Fake(Method(runtime_mock, get));

        stv::deadline_timer deadline{
            deadline_setup_type{.runtime = &runtime_mock.get(),
                                .is_elapsed_if_not_started = false}};

        REQUIRE(deadline.set_delay(1h));
        REQUIRE(deadline.set_delay(std::chrono::hours{1}));

        REQUIRE(deadline.set_delay(2h));
        REQUIRE(deadline.set_delay(std::chrono::hours{2}));

        REQUIRE(deadline.set_delay(std::chrono::days{1}));
        REQUIRE(deadline.set_delay(std::chrono::days{28}));

        // Compile time.
        static_assert(deadline.can_set_delay(2h), "Overflow detected");
        static_assert(deadline.can_set_delay(std::chrono::hours{2}),
                      "Overflow detected");
    }

    SECTION("std::uint64_t, sstd::micro")
    {
        using counter_type = std::chrono::duration<std::uint64_t, std::micro>;
        using runtime_setup_type  = stv::runtime_setup<counter_type>;
        using runtime_type        = stv::runtime<runtime_setup_type>;
        using deadline_setup_type = deadline_timer_setup<runtime_type>;

        Mock<runtime_type> runtime_mock;
        Fake(Method(runtime_mock, get));

        stv::deadline_timer deadline{
            deadline_setup_type{.runtime = &runtime_mock.get(),
                                .is_elapsed_if_not_started = false}};

        REQUIRE(deadline.set_delay(1h));
        REQUIRE(deadline.set_delay(std::chrono::hours{1}));

        REQUIRE(deadline.set_delay(2h));
        REQUIRE(deadline.set_delay(std::chrono::hours{2}));

        REQUIRE(deadline.set_delay(std::chrono::days{1}));
        REQUIRE(deadline.set_delay(std::chrono::days{28}));
    }
}

// NOLINTEND(*-magic-numbers, google-build-using-namespace,
// readability-function-cognitive-,
// cppcoreguidelines-avoid-non-const-global-variables)
