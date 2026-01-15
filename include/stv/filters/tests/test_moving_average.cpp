/// @file test_moving_average.cpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
/// @author Matvey Simakov <simakov.matvey@mail.ru>
///
/// @copyright (c) 2025 "The Boys"
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

#include "fpm/fixed.hpp"
#include "stv/filters/moving_average.hpp"
#include <array>
#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <iostream>
#include <memory>
#include <mutex>

// NOLINTBEGIN(*-magic-numbers, google-build-using-namespace,
// readability-function-cognitive-,
// cppcoreguidelines-avoid-non-const-global-variables)

TEST_CASE(
    "Usage example (no multithread)", "[stv][moving_average]")
{
    // Объявление псевдонима структуры инициализации.
    using TMovingAverageSetup =
        stv::moving_average_setup<float, stv::empty_mutex>;

    // Объявление псевдонима базового класса, который обеспечивает необходимый
    // функционал/
    using TIMovingAverage = stv::moving_average_base<TMovingAverageSetup>;

    // Инициализация класса фильтра скользящего среднего.
    // NOLINTNEXTLINE(misc-const-correctness)
    stv::SimpleMovingAverage<TIMovingAverage, 20> moving_average{
        TMovingAverageSetup{.window_width = 10U}};
    assert(moving_average);

    // Теперь фильтр готов к использованию.
}

TEST_CASE(
    "Usage example (with multithread by std::recursive_mutex)",
    "[stv][moving_average]")
{
    // Объявление псевдонима структуры инициализации.
    // @note Обратите внимание, обязательно должен использоваться рекурсивный
    // мьютекс.
    using TMovingAverageSetup =
        stv::moving_average_setup<float, std::recursive_mutex>;

    // Объявление псевдонима базового класса, который обеспечивает необходимый
    // функционал/
    using TIMovingAverage = stv::moving_average_base<TMovingAverageSetup>;

    // Инициализация класса фильтра скользящего среднего.
    // NOLINTNEXTLINE(misc-const-correctness)
    stv::SimpleMovingAverage<TIMovingAverage, 20> moving_average{
        TMovingAverageSetup{.window_width = 10U}};
    assert(moving_average);

    // Теперь фильтр готов к использованию.
}

TEST_CASE(
    "Usage example (with custom guard)", "[stv][moving_average]")
{
    struct CustomGuard {
        void lock() {}

        void unlock() {}
    };

    // Объявление псевдонима структуры инициализации.
    using TMovingAverageSetup = stv::moving_average_setup<float, CustomGuard>;

    // Объявление псевдонима базового класса, который обеспечивает необходимый
    // функционал/
    using TIMovingAverage = stv::moving_average_base<TMovingAverageSetup>;

    // Инициализация класса фильтра скользящего среднего.
    // NOLINTNEXTLINE(misc-const-correctness)
    stv::SimpleMovingAverage<TIMovingAverage, 20> moving_average{
        TMovingAverageSetup{.window_width = 10U}};
    assert(moving_average);

    // Теперь фильтр готов к использованию.
}

TEST_CASE(
    "Usage example (with custom guard by pointer and interface)",
    "[stv][moving_average]")
{
    struct ICustomGuard {
        virtual ~ICustomGuard() = default;
        virtual void lock()     = 0;
        virtual void unlock()   = 0;
    };

    struct CustomGuard final: public ICustomGuard {
        ~CustomGuard() override = default;

        void lock() override {}

        void unlock() override {}
    };

    CustomGuard custom_guard;

    // Объявление псевдонима структуры инициализации.
    using TMovingAverageSetup =
        stv::moving_average_setup<float, ICustomGuard, stv::mutex_ext_tag>;

    // Объявление псевдонима базового класса, который обеспечивает необходимый
    // функционал/
    using TIMovingAverage = stv::moving_average_base<TMovingAverageSetup>;

    // Инициализация класса фильтра скользящего среднего.
    // NOLINTNEXTLINE(misc-const-correctness)
    stv::SimpleMovingAverage<TIMovingAverage, 20> moving_average{
        TMovingAverageSetup{.window_width = 10U, .mutex = &custom_guard}};
    assert(moving_average);

    // Теперь фильтр готов к использованию. ------------------------------------

    // -------------------------------------------------------------------------
    // Создадим еще один экземпляр фильтра с другой реализацией защиты.
    // -------------------------------------------------------------------------

    struct CustomGuardV2: public ICustomGuard {
        void lock() override {}

        void unlock() override {}
    };

    CustomGuardV2 custom_guard_v2;
    // NOLINTNEXTLINE(misc-const-correctness)
    stv::SimpleMovingAverage<TIMovingAverage, 20> moving_average_v2{
        TMovingAverageSetup{.window_width = 10U, .mutex = &custom_guard_v2}};
    assert(moving_average_v2);
}

TEST_CASE(
    "Usage example (no multithread) with heap allocate memory",
    "[stv][moving_average]")
{
    // Объявление псевдонима структуры инициализации.
    using TMovingAverageSetup =
        stv::moving_average_setup<float, stv::empty_mutex>;

    // Объявление псевдонима базового класса, который обеспечивает необходимый
    // функционал/
    using TIMovingAverage = stv::moving_average_base<TMovingAverageSetup>;

    // Инициализация класса фильтра скользящего среднего на куче через
    // std::unique_ptr.
    auto moving_average =
        std::make_unique<stv::SimpleMovingAverage<TIMovingAverage, 20>>(
            TMovingAverageSetup{.window_width = 10U});
    assert(moving_average);        ///< true если память на куче выделена
    assert(moving_average.get()
               ->operator bool()); ///< true если объект успешно создан

    // Теперь фильтр готов к использованию.
}

TEST_CASE(
    "setup, [stv][moving_average]")
{
    using namespace stv;
    using namespace boost;

    using TValue = float;
    using TSetup = moving_average_setup<TValue, stv::empty_mutex>;

    SECTION("Default")
    {
        const TSetup init;

        REQUIRE(init.window_width == TSetup::default_window_width);
        REQUIRE_FALSE(init.is_valid(0));
        REQUIRE(init.is_valid(TSetup::default_window_width));
        REQUIRE(init.is_valid(10));
    }

    SECTION("Zero window width")
    {
        const TSetup init{.window_width = 0};

        REQUIRE(init.window_width == 0);

        REQUIRE_FALSE(init.is_valid(0));
        REQUIRE_FALSE(init.is_valid(1));
        REQUIRE_FALSE(init.is_valid(10));
    }

    SECTION("Normal window width")
    {
        constexpr decltype(std::declval<TSetup>().window_width) window_width{
            10};
        const TSetup init{.window_width = window_width};
        REQUIRE(init.window_width == window_width);

        REQUIRE_FALSE(init.is_valid(0));
        REQUIRE_FALSE(init.is_valid(1));
        REQUIRE(init.is_valid(window_width));
        REQUIRE(init.is_valid(
            window_width
            + static_cast<std::decay_t<decltype(window_width)>>(1)));
    }

    const TSetup init{.window_width = 12};

    REQUIRE_FALSE(init.is_valid(10));
}

TEMPLATE_PRODUCT_TEST_CASE_SIG(
    "Test copy operators and move operator", "[stv]",
    ((typename TData, typename TMutex), TData, TMutex),
    (stv::moving_average_setup),
    ((double, std::recursive_mutex, stv::mutex_ext_tag),
     (float, stv::empty_mutex, stv::mutex_ext_tag),
     (float, stv::empty_mutex, stv::mutex_int_tag)))
{
    using namespace stv;

    using TSetup = TestType;
    using TData  = TSetup::value_type;
    using TBase  = moving_average_base<TSetup>;

    SECTION(
        "Object of the class that publicly inherits SimpleMovingAverage and "
        "uses protected <buffer> from base class")
    {
        std::recursive_mutex std_mutex{};
        (void)std_mutex;

        stv::empty_mutex empty_mutex;
        (void)empty_mutex;

        TSetup attr;
        if constexpr(std::is_same_v<typename TSetup::mutex_tag,
                                    stv::mutex_ext_tag>
                     && std::is_same_v<typename TSetup::mutex_type,
                                       std::recursive_mutex>)
        {
            attr.mutex = &std_mutex;
        }

        if constexpr(std::is_same_v<typename TSetup::mutex_tag,
                                    stv::mutex_ext_tag>
                     && std::is_same_v<typename TSetup::mutex_type,
                                       stv::empty_mutex>)
        {
            attr.mutex = &empty_mutex;
        }

        constexpr int                            window_width = 5;
        SimpleMovingAverage<TBase, window_width> src_average{attr};
        REQUIRE(src_average);
        constexpr std::array<TData, window_width> samples{1, 1, 1, 1, 1};

        // Buffer is filled with some values for further checking.
        for(const auto &sample: samples)
        {
            const auto filtered = src_average.filt(sample);
            (void)filtered;
        }

        WHEN("User copies source object to destination one")
        {
            SimpleMovingAverage<TBase, window_width> dst_average{attr};
            REQUIRE(dst_average);
            dst_average = src_average;
            REQUIRE(dst_average);

            THEN("<buffer> from <dst_average> should point to new memory "
                 "address")
            {
                REQUIRE(&dst_average.buffer != &src_average.buffer);
            }

            AND_THEN("<buffer> from <dst_average> should have all values from "
                     "<src_average> object")
            {
                REQUIRE(dst_average.buffer == src_average.buffer);
            }
        }

        WHEN("User copy assign source object to destination one")
        {
            SimpleMovingAverage<TBase, window_width> dst_average{src_average};

            THEN("<buffer> from <dst_average> should point to new memory "
                 "address")
            {
                REQUIRE(&dst_average.buffer != &src_average.buffer);
            }

            AND_THEN("<buffer> from <dst_average> should have all values from "
                     "<src_average> object")
            {
                REQUIRE(dst_average.buffer == src_average.buffer);
            }
        }

        WHEN("User move source object to destination one")
        {
            auto *src_average_buffer_address = &src_average.buffer;
            auto  src_buffer_span            = src_average.buffer;

            SimpleMovingAverage<TBase, window_width> dst_average{
                std::move(src_average)};

            THEN("<buffer> from <dst_average> should point to new memory "
                 "address")
            {
                REQUIRE(&dst_average.buffer != src_average_buffer_address);
            }

            AND_THEN("<buffer> from <dst_average> should have all values from "
                     "<src_average> object")
            {
                REQUIRE(dst_average.buffer == src_buffer_span);
            }
        }

        WHEN("User move assign source object to destination one")
        {
            // Create another object because <src_average> was moved in previous
            // section.
            SimpleMovingAverage<TBase, window_width> src{attr};

            // Buffer is filled with some values for further checking.
            for(const auto &sample: samples)
            {
                const auto filtered = src.filt(sample);
                (void)filtered;
            }

            auto *src_average_buffer_address = &src.buffer;
            auto  src_buffer_span            = src.buffer;

            SimpleMovingAverage<TBase, window_width> dst_average{attr};

            dst_average = std::move(src);

            THEN("<buffer> from <dst_average> should point to new memory "
                 "address")
            {
                REQUIRE(&dst_average.buffer != src_average_buffer_address);
            }

            AND_THEN("<buffer> from <dst_average> should have all values from "
                     "<src> object")
            {
                REQUIRE(dst_average.buffer == src_buffer_span);
            }
        }
    }
}

TEMPLATE_PRODUCT_TEST_CASE_SIG(
    "Moving Average", "[stv]",
    ((typename TData, typename TMutex), TData, TMutex),
    (stv::moving_average_setup),
    ((double, std::recursive_mutex), (float, stv::empty_mutex),
     (int, std::recursive_mutex), (fpm::fixed_16_16, stv::empty_mutex)))
{
    using namespace stv;
    using namespace boost;
    using TData  = TestType::value_type;
    using TSetup = TestType;
    using TBase  = moving_average_base<TSetup>;
    constexpr decltype(std::declval<TSetup>().window_width) window_width{15};
    TSetup                                                  attr;

    SECTION("Create invalid object")
    {
        attr.window_width = window_width;

        SimpleMovingAverage<TBase, window_width - 1> average(attr);
        REQUIRE_FALSE(average);
    }

    SECTION("SetupParams equality")
    {
        SECTION("Parameters are different")
        {
            TSetup params_first;
            params_first.window_width = 2;

            TSetup params_second;
            params_second.window_width = params_first.window_width + 1;

            REQUIRE_FALSE(params_first == params_second);
            REQUIRE(params_first != params_second);
        }

        SECTION("Parameters are equal")
        {
            TSetup params_first;
            params_first.window_width = 2;

            TSetup params_second;
            params_second.window_width = params_first.window_width;

            REQUIRE(params_first == params_second);
            REQUIRE_FALSE(params_first != params_second);
        }
    }

    SECTION("Apply user default setup parameters in constructor")
    {
        TSetup attr_with_user_default;

        TSetup user_default_params;
        user_default_params.window_width = 5;

        attr_with_user_default.window_width = user_default_params.window_width;

        const SimpleMovingAverage<TBase> average(attr_with_user_default);

        REQUIRE(average.get_default_setup() == user_default_params);
    }

    SECTION("filt without setup")
    {
        SimpleMovingAverage<TBase> average(attr);

        const std::array<TData, 8> samples{
            static_cast<TData>(1), static_cast<TData>(2), static_cast<TData>(3),
            static_cast<TData>(4), static_cast<TData>(5), static_cast<TData>(6),
            static_cast<TData>(7), static_cast<TData>(8)};

        constexpr double eps{0.01};
        for(const auto &new_sample: samples)
        {
            // Without setup filt() must return <new_sample>.
            REQUIRE_THAT(static_cast<double>(average.filt(new_sample)),
                         Catch::Matchers::WithinRel(
                             static_cast<double>(new_sample), eps));
        }
    }

    SECTION("filt with setup")
    {
        SimpleMovingAverage<TBase, window_width> average(attr);
        //
        REQUIRE(average.setup(TSetup{.window_width = 3}));

        const std::array<TData, 8> samples{
            static_cast<TData>(1), static_cast<TData>(2), static_cast<TData>(3),
            static_cast<TData>(4), static_cast<TData>(5), static_cast<TData>(6),
            static_cast<TData>(7), static_cast<TData>(8)};

        const std::array<TData, 8> expected{
            static_cast<TData>(1), static_cast<TData>(2), static_cast<TData>(2),
            static_cast<TData>(3), static_cast<TData>(4), static_cast<TData>(5),
            static_cast<TData>(6), static_cast<TData>(7)};

        CHECK(samples.size() == expected.size());

        constexpr double eps{0.01};
        SECTION("With cycle")
        {
            for(std::size_t i = 0; i < samples.size(); ++i)
            {
                const auto filtered =
                    static_cast<double>(average.filt(samples.at(i)));
                REQUIRE_THAT(filtered,
                             Catch::Matchers::WithinRel(
                                 static_cast<double>(expected.at(i)), eps));
            }
        }

        SECTION("With arg pack")
        {
            const auto filtered = static_cast<double>(average.filt(
                samples.at(0), samples.at(1), samples.at(2), samples.at(3),
                samples.at(4), samples.at(5), samples.at(6), samples.at(7)));

            REQUIRE_THAT(filtered,
                         Catch::Matchers::WithinRel(
                             static_cast<double>(expected.at(7)), eps));
        }

        SECTION("With iterators")
        {
            const auto filtered = static_cast<double>(
                average.filt(samples.cbegin(), samples.cend()));

            REQUIRE_THAT(filtered,
                         Catch::Matchers::WithinRel(
                             static_cast<double>(expected.at(7)), eps));
        }
    }

    SECTION("Change Window Width to Smaller One")
    {
        constexpr std::size_t                    window_width_init  = 5;
        constexpr std::size_t                    window_width_lower = 3;

        SimpleMovingAverage<TBase, window_width> average(attr);

        TSetup                                   setup_params;
        setup_params.window_width = window_width_init;
        REQUIRE(average.setup(setup_params));

        const std::array<TData, 10> samples{
            static_cast<TData>(10),  static_cast<TData>(200),
            static_cast<TData>(30),  static_cast<TData>(100),
            static_cast<TData>(-20), static_cast<TData>(500),
            static_cast<TData>(400), static_cast<TData>(-20),
            static_cast<TData>(300), static_cast<TData>(20)};

        // Index of sample values from which lower window width begins.
        constexpr std::size_t       width_lower_first_idx = 6;

        const std::array<TData, 10> expected{
            static_cast<TData>(10),  static_cast<TData>(200),
            static_cast<TData>(30),  static_cast<TData>(100),
            static_cast<TData>(64),  static_cast<TData>(162),
            static_cast<TData>(293), static_cast<TData>(293),
            static_cast<TData>(227), static_cast<TData>(100)};

        CHECK(samples.size() == expected.size());

        constexpr double eps{0.01};

        for(std::size_t i = 0; i < expected.size(); ++i)
        {
            if(i == width_lower_first_idx)
            {
                setup_params.window_width = window_width_lower;
                REQUIRE(average.setup(setup_params));
            }

            REQUIRE_THAT(static_cast<double>(average.filt(samples.at(i))),
                         Catch::Matchers::WithinRel(
                             static_cast<double>(expected.at(i)), eps));
        }
    }

    SECTION("Change Window Width to Greater One")
    {
        constexpr std::size_t                    window_width_init    = 3;
        constexpr std::size_t                    window_width_greater = 5;

        SimpleMovingAverage<TBase, window_width> average(attr);

        TSetup                                   setup_params;
        setup_params.window_width = window_width_init;
        REQUIRE(average.setup(setup_params));

        const std::array<TData, 10> samples{
            static_cast<TData>(10),  static_cast<TData>(200),
            static_cast<TData>(30),  static_cast<TData>(100),
            static_cast<TData>(-20), static_cast<TData>(500),
            static_cast<TData>(400), static_cast<TData>(-20),
            static_cast<TData>(300), static_cast<TData>(20)};

        // Index of sample values from which greater window width begins.
        constexpr std::size_t       width_greater_first_idx = 6;

        const std::array<TData, 10> expected{
            static_cast<TData>(10),  static_cast<TData>(200),
            static_cast<TData>(80),  static_cast<TData>(110),
            static_cast<TData>(37),  static_cast<TData>(193),
            static_cast<TData>(400), static_cast<TData>(192),
            static_cast<TData>(232), static_cast<TData>(240)};

        CHECK(samples.size() == expected.size());

        constexpr double eps{1};

        for(std::size_t i = 0; i < expected.size(); ++i)
        {
            if(i == width_greater_first_idx)
            {
                setup_params.window_width = window_width_greater;
                REQUIRE(average.setup(setup_params));
            }

            REQUIRE_THAT(static_cast<double>(average.filt(samples.at(i))),
                         Catch::Matchers::WithinAbs(
                             static_cast<double>(expected.at(i)), eps));
        }
    }

    SECTION("reset To Default")
    {
        SimpleMovingAverage<TBase, window_width> average(attr);

        TSetup                                   setup_params;
        setup_params.window_width = window_width;
        REQUIRE(average.setup(setup_params));

        const std::array<TData, 8> samples{
            static_cast<TData>(1), static_cast<TData>(2), static_cast<TData>(3),
            static_cast<TData>(4), static_cast<TData>(5), static_cast<TData>(6),
            static_cast<TData>(7), static_cast<TData>(8)};

        constexpr double eps{0.01};

        for(const auto &new_sample: samples)
        {
            const auto filtered = average.filt(new_sample);
            (void)filtered;
        }

        // After reset filt() must return input <samples>.
        average.reset();

        for(const auto &new_sample: samples)
        {
            REQUIRE_THAT(static_cast<double>(average.filt(new_sample)),
                         Catch::Matchers::WithinRel(
                             static_cast<double>(new_sample), eps));
        }
    }
}

// NOLINTEND(*-magic-numbers, google-build-using-namespace,
// readability-function-cognitive-complexity,
// cppcoreguidelines-avoid-non-const-global-variables)
