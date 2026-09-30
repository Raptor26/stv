/// @file test_crc16.cpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.

#include "stv/communication/crc16.hpp"
#include <array>
#include <catch2/catch_test_macros.hpp>
#include <cstddef>
#include <cstdint>

// NOLINTBEGIN(*-magic-numbers, readability-function-cognitive-complexity)
// Когнитивная сложность подавлена: TEST_CASE разворачивается Catch2 в
// одну функцию, секции внутри неё не считаются отдельными функциями.

TEST_CASE(
    "crc16_x25", "[stv][communication]")
{
    SECTION("check value")
    {
        // Эталонный вектор CRC-16/MCRF4XX: строка "123456789" -> 0x6F91.
        const std::array<std::byte, 9> data{
            std::byte{'1'}, std::byte{'2'}, std::byte{'3'},
            std::byte{'4'}, std::byte{'5'}, std::byte{'6'},
            std::byte{'7'}, std::byte{'8'}, std::byte{'9'},
        };

        REQUIRE(stv::crc16_x25(data.data(), data.size()) == 0x6F91U);
    }

    SECTION("empty data")
    {
        // Пустой блок: результат равен начальному значению 0xFFFF
        // (финальный XOR отсутствует).
        REQUIRE(stv::crc16_x25(nullptr, 0) == 0xFFFFU);
    }

    SECTION("single byte")
    {
        // CRC-16/X.25 от одного байта 0x00 -> 0x0F87.
        const std::array<std::byte, 1> data{std::byte{0x00}};

        REQUIRE(stv::crc16_x25(data.data(), data.size()) == 0x0F87U);
    }

    SECTION("accumulate equals whole buffer")
    {
        // Побайтовый досчёт через crc16_x25_accumulate совпадает с расчётом
        // crc16_x25 по всему буферу (модель X25Accumulate из MAVLink).
        const std::array<std::byte, 9> data{
            std::byte{'1'}, std::byte{'2'}, std::byte{'3'},
            std::byte{'4'}, std::byte{'5'}, std::byte{'6'},
            std::byte{'7'}, std::byte{'8'}, std::byte{'9'},
        };

        std::uint16_t crc = 0xFFFFU;
        for(const std::byte value: data)
        {
            crc = stv::crc16_x25_accumulate(crc, value);
        }

        REQUIRE(crc == stv::crc16_x25(data.data(), data.size()));
        REQUIRE(crc == 0x6F91U);
    }

    SECTION("accumulate extra byte")
    {
        // Досчёт дополнительного байта (CRC_EXTRA): CRC по конкатенации
        // данных и байта равен accumulate от CRC данных.
        const std::array<std::byte, 9> data{
            std::byte{'1'}, std::byte{'2'}, std::byte{'3'},
            std::byte{'4'}, std::byte{'5'}, std::byte{'6'},
            std::byte{'7'}, std::byte{'8'}, std::byte{'9'},
        };
        const std::byte                 extra{0x42};

        const std::array<std::byte, 10> extended{
            std::byte{'1'}, std::byte{'2'}, std::byte{'3'}, std::byte{'4'},
            std::byte{'5'}, std::byte{'6'}, std::byte{'7'}, std::byte{'8'},
            std::byte{'9'}, extra,
        };

        const std::uint16_t accumulated = stv::crc16_x25_accumulate(
            stv::crc16_x25(data.data(), data.size()), extra);

        REQUIRE(accumulated
                == stv::crc16_x25(extended.data(), extended.size()));
    }
}

// NOLINTEND(*-magic-numbers, readability-function-cognitive-complexity)
