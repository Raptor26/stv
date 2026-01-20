/// @file test_runtime.cpp
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

#include "stv/mutex_guard.hpp"
#include "stv/runtime.hpp"
#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <mutex>

// NOLINTBEGIN(*-magic-numbers, google-build-using-namespace,
// readability-function-cognitive-,
// cppcoreguidelines-avoid-non-const-global-variables)

TEMPLATE_PRODUCT_TEST_CASE(
    "runtime", "[stv][containers]", (stv::runtime_setup),
    ((stv::runtime_counter_type, stv::empty_mutex),
     (stv::runtime_counter_type, stv::empty_mutex *),
     (stv::runtime_counter_type, std::recursive_mutex),
     (stv::runtime_counter_type, std::recursive_mutex *)))
{
    using namespace stv;

    using setup_type = TestType;

    std::recursive_mutex std_mutex{};
    (void)std_mutex;

    stv::empty_mutex empty_mutex;
    (void)empty_mutex;

    SECTION("Ctor")
    {
        SECTION("Default Setup")
        {
            const setup_type setup;
            stv::runtime     runtime{setup};
            REQUIRE_FALSE(runtime);
        }

        SECTION("Valid Setup")
        {
            using namespace std::chrono_literals;
            constexpr auto expected_elapsed_time =
                stv::runtime_counter_type{1s};
            setup_type setup{.increment_period = expected_elapsed_time};

            if constexpr(std::is_same_v<typename setup_type::mutex_type,
                                        std::recursive_mutex *>)
            {
                setup.mutex = &std_mutex;
            }

            if constexpr(std::is_same_v<typename setup_type::mutex_type,
                                        stv::empty_mutex *>)
            {
                setup.mutex = &empty_mutex;
            }

            stv::runtime runtime{setup};
            REQUIRE(runtime);

            SECTION("Increment")
            {
                runtime.inc();
                const auto current_time = runtime.get();
                REQUIRE(current_time == expected_elapsed_time);
            }

            SECTION("Update period, then increment")
            {
                constexpr auto expected_elapsed_time_again =
                    stv::runtime_counter_type{2s};
                SECTION("Set as stv::runtime_type")
                {
                    runtime.set_period(expected_elapsed_time_again);
                }

                SECTION("Set as std::chrono literal")
                {
                    runtime.set_period(2000ms);
                }

                runtime.inc();
                REQUIRE(runtime.get() == expected_elapsed_time_again);
            }
        }
    }
}

// NOLINTEND(*-magic-numbers, google-build-using-namespace,
// readability-function-cognitive-,
// cppcoreguidelines-avoid-non-const-global-variables)
