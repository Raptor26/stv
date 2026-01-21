/// @file test_deadline_timer.cpp
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

#include "stv/deadline_timer.hpp"
#include "stv/runtime.hpp"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <fakeit.hpp>
#include <limits>

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
    using deadline_setup_type = deadline_timer_setup<runtime_type>;

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

// NOLINTEND(*-magic-numbers, google-build-using-namespace,
// readability-function-cognitive-,
// cppcoreguidelines-avoid-non-const-global-variables)
