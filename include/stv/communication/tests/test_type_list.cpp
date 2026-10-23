/// @file test_type_list.cpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.

#include "stv/communication/type_list.hpp"
#include <array>
#include <catch2/catch_test_macros.hpp>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <utility>

TEST_CASE(
    "type_list: размер и индексация")
{
    using list = stv::type_list<int, double, char>;

    static_assert(stv::type_list_size_v<list> == 3U);
    static_assert(std::same_as<stv::type_list_element_t<0, list>, int>);
    static_assert(std::same_as<stv::type_list_element_t<1, list>, double>);
    static_assert(std::same_as<stv::type_list_element_t<2, list>, char>);
    REQUIRE(stv::type_list_size_v<list> == 3U);
}

TEST_CASE(
    "type_list: for_each")
{
    using list = stv::type_list<int, double, char>;

    std::size_t visited = 0U;
    stv::type_list_for_each<list>([&visited]<typename> { ++visited; });
    REQUIRE(visited == stv::type_list_size_v<list>);
}

TEST_CASE(
    "enum_uniquely_covered")
{
    enum class demo_id : std::uint16_t {
        a,
        b,
        max,
    };
    static_assert(stv::enum_uniquely_covered(std::array{demo_id::a, demo_id::b},
                                             demo_id::max));
    static_assert(!stv::enum_uniquely_covered(
        std::array{demo_id::a, demo_id::a}, demo_id::max));
    static_assert(
        !stv::enum_uniquely_covered(std::array{demo_id::a}, demo_id::max));
}

TEST_CASE(
    "enum_uniquely_covered: принимает таблицу пар (идентификатор, "
    "обработчик)")
{
    enum class demo_id : std::uint16_t {
        a,
        b,
        max,
    };

    // Пары (идентификатор, обработчик): ключи извлекаются из поля .first,
    // промежуточный массив идентификаторов не строится.
    constexpr auto full_table = std::array{
        std::pair{demo_id::a, 0},
        std::pair{demo_id::b, 1},
    };
    static_assert(stv::enum_uniquely_covered(full_table, demo_id::max));

    constexpr auto partial_table = std::array{
        std::pair{demo_id::a, 0},
    };
    static_assert(!stv::enum_uniquely_covered(partial_table, demo_id::max));

    constexpr auto duplicate_table = std::array{
        std::pair{demo_id::a, 0},
        std::pair{demo_id::a, 1},
    };
    static_assert(!stv::enum_uniquely_covered(duplicate_table, demo_id::max));

    REQUIRE(stv::enum_uniquely_covered(full_table, demo_id::max));
}
