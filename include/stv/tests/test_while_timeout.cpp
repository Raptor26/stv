/// @file test_while_timeout.cpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.

#include "stv/runtime.hpp"
#include "stv/while_timeout.hpp"
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <fakeit.hpp>

// NOLINTBEGIN(*-magic-numbers, google-build-using-namespace,
// readability-function-cognitive-,
// cppcoreguidelines-avoid-non-const-global-variables)

TEST_CASE(
    "while_timeout", "[stv]")
{
    using namespace fakeit;
    using namespace stv;
    using namespace std::chrono_literals;

    using runtime_setup_type = stv::runtime_setup<stv::runtime_counter_type>;
    using runtime_type       = stv::runtime<runtime_setup_type>;

    SECTION("Условие выполнено сразу")
    {
        Mock<runtime_type> runtime_mock;
        Fake(Method(runtime_mock, get));

        auto polls{0};

        // Тип while_timeout выводится автоматически (CTAD).
        stv::while_timeout wait{runtime_mock.get(), 200ms, [&polls]() {
                                    ++polls;
                                    return true;
                                }};

        // Условие выполнилось до истечения таймаута.
        REQUIRE_FALSE(wait.is_elapsed());
        // Условие опрошено ровно один раз.
        REQUIRE(polls == 1);
    }

    SECTION("Условие выполняется после нескольких опросов")
    {
        Mock<runtime_type> runtime_mock;
        // Время не продвигается: таймаут не наступает никогда.
        Fake(Method(runtime_mock, get));

        auto               polls{0};

        stv::while_timeout wait{runtime_mock.get(), 200ms, [&polls]() {
                                    ++polls;
                                    return polls >= 3;
                                }};

        // Условие выполнилось до истечения таймаута.
        REQUIRE_FALSE(wait.is_elapsed());
        // Условие опрашивалось до выполнения.
        REQUIRE(polls == 3);
    }

    SECTION("Условие не выполняется, ожидание прервано по таймауту")
    {
        Mock<runtime_type> runtime_mock;
        // Первый вызов get() - запуск deadline (0 мкс), далее время
        // продвигается: 100 мс, 200 мс.
        When(Method(runtime_mock, get))
            .Return(stv::runtime_counter_type{0us},
                    stv::runtime_counter_type{100ms},
                    stv::runtime_counter_type{200ms});

        auto               polls{0};

        stv::while_timeout wait{runtime_mock.get(), 200ms, [&polls]() {
                                    ++polls;
                                    return false;
                                }};

        // Ожидание прервано по таймауту.
        REQUIRE(wait.is_elapsed());
        // Условие опрашивалось до истечения таймаута.
        REQUIRE(polls > 1);
    }
}

// NOLINTEND(*-magic-numbers, google-build-using-namespace,
// readability-function-cognitive-,
// cppcoreguidelines-avoid-non-const-global-variables)
