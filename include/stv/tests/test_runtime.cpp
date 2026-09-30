/// @file test_runtime.cpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.

#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <mutex>
#include <type_traits>

#include "stv/mutex_guard.hpp"
#include "stv/runtime.hpp"

// NOLINTBEGIN(*-magic-numbers, google-build-using-namespace)
// NOLINTBEGIN(readability-function-cognitive-complexity)
// NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables)

TEMPLATE_PRODUCT_TEST_CASE(
    "runtime", "[stv][containers]", (stv::runtime_setup),
    ((stv::runtime_counter_type, stv::empty_mutex),
     (stv::runtime_counter_type, stv::empty_mutex *),
     (stv::runtime_counter_type, std::recursive_mutex),
     (stv::runtime_counter_type, std::recursive_mutex *)))
{
    using namespace stv;

    using setup_type = TestType;

    // адрес берётся как T* в if constexpr
    // NOLINTNEXTLINE(misc-const-correctness)
    std::recursive_mutex std_mutex{};
    (void)std_mutex;

    // адрес берётся как T* в if constexpr
    // NOLINTNEXTLINE(misc-const-correctness)
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
            // поле задаётся в if constexpr
            // NOLINTNEXTLINE(misc-const-correctness)
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
                { runtime.set_period(expected_elapsed_time_again); }

                SECTION("Set as std::chrono literal")
                { runtime.set_period(2000ms); }

                runtime.inc();
                REQUIRE(runtime.get() == expected_elapsed_time_again);
            }
        }
    }
}

// NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables)
// NOLINTEND(readability-function-cognitive-complexity)
// NOLINTEND(*-magic-numbers, google-build-using-namespace)
