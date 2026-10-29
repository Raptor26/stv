/// @file test_stvlinkv2_kit.cpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.

#include "stv/communication/stvlinkv2_kit.hpp"
#include <array>
#include <catch2/catch_test_macros.hpp>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <utility>

namespace {

/// @brief Перечисление-заглушка идентификаторов сообщений: имитирует
///     перечисление интегрирующего проекта.
enum class test_msg_id : std::uint16_t {
    first  = 0x0123U,
    second = 0x0456U,

    max = 0x0457U, ///< Должен находится в конце перечисляемого типа.
};

/// @brief Структура-заглушка полезной нагрузки с сериализацией
///     little-endian.
struct payload_stub {
    std::uint32_t value{0U};

    /// @brief Возвращает размер сериализованной полезной нагрузки в байтах.
    [[nodiscard]] static constexpr auto serialized_size_bytes() -> std::size_t
    { return sizeof(value); }

    /// @brief Десериализует столько байт, сколько есть в буфере.
    ///
    /// @param[in] begin Итератор на начало входного буфера.
    /// @param[in] end Итератор на конец входного буфера.
    /// @return Количество прочитанных байт.
    template<typename It>
    [[nodiscard]] auto deserialize_from(
        It begin, It end) -> std::size_t
    {
        const auto available =
            static_cast<std::size_t>(std::distance(begin, end));
        const auto to_read =
            (available < sizeof(value)) ? available : sizeof(value);
        std::memcpy(&value, std::to_address(begin), to_read);
        return to_read;
    }
};

bool first_handler(
    stv::stvlink_message_span /*unused*/)
{ return true; }

bool second_handler(
    stv::stvlink_message_span /*unused*/)
{ return true; }

/// @brief Буфер с сериализованным значением expected_value.
struct serialized_buffer {
    static constexpr std::uint32_t expected_value{0xDEADBEEFU};

    std::array<std::byte, payload_stub::serialized_size_bytes()> bytes{};

    serialized_buffer()
    { std::memcpy(bytes.data(), &expected_value, sizeof(expected_value)); }
};

/// @brief Проверяет десериализацию валидного буфера.
void require_valid_buffer_deserializes()
{
    serialized_buffer buffer{};
    const auto        payload =
        stv::try_deserialize<payload_stub>(std::span{buffer.bytes});

    if(!payload.has_value())
    {
        FAIL("try_deserialize must return a value");
    }
    REQUIRE(payload->value == serialized_buffer::expected_value);
}

/// @brief Проверяет десериализацию доступного префикса структуры.
void require_small_buffer_accepted()
{
    serialized_buffer buffer{};
    const auto        payload = stv::try_deserialize<payload_stub>(
        std::span{buffer.bytes}.first(buffer.bytes.size() - 1U));

    REQUIRE(payload.has_value());
}

/// @brief Проверяет десериализацию пустого буфера в default-значение.
void require_zero_sized_buffer_accepted()
{
    const auto payload =
        stv::try_deserialize<payload_stub>(std::span<std::byte>{});

    REQUIRE(payload.has_value());
    REQUIRE(payload->value == 0U);
}

/// @brief Проверяет, что лишние хвостовые байты игнорируются.
void require_trailing_bytes_ignored()
{
    serialized_buffer buffer{};
    std::array<std::byte, payload_stub::serialized_size_bytes() + 3U>
        extended{};
    std::memcpy(extended.data(), buffer.bytes.data(), buffer.bytes.size());

    const auto payload =
        stv::try_deserialize<payload_stub>(std::span{extended});

    if(!payload.has_value())
    {
        FAIL("try_deserialize must return a value");
    }
    REQUIRE(payload->value == serialized_buffer::expected_value);
}

/// @brief Проверяет, что нулевой буфер даёт нулевое значение.
void require_zeroed_buffer_yields_zero()
{
    std::array<std::byte, payload_stub::serialized_size_bytes()> zeroed{};

    const auto payload = stv::try_deserialize<payload_stub>(std::span{zeroed});

    if(!payload.has_value())
    {
        FAIL("try_deserialize must return a value");
    }
    REQUIRE(payload->value == 0U);
}

} // namespace

TEST_CASE(
    "stvlinkv2_kit::make_msg_setup", "[stv][stvlink][stvlinkv2]")
{
    SECTION("Key value equals the underlying identifier")
    {
        constexpr auto setup = stv::make_msg_setup(test_msg_id::first);
        static_assert(setup.msg_id == 0x0123U);
        REQUIRE(setup.msg_id == std::to_underlying(test_msg_id::first));
    }

    SECTION("Different identifiers produce different setups")
    {
        constexpr auto first  = stv::make_msg_setup(test_msg_id::first);
        constexpr auto second = stv::make_msg_setup(test_msg_id::second);

        REQUIRE(first.msg_id != second.msg_id);
    }
}

TEST_CASE(
    "stvlinkv2_kit::try_deserialize", "[stv][stvlink][stvlinkv2]")
{
    SECTION("Valid buffer deserializes")
    { require_valid_buffer_deserializes(); }

    SECTION("Buffer smaller than struct size is accepted")
    { require_small_buffer_accepted(); }

    SECTION("Zero-sized buffer is accepted")
    { require_zero_sized_buffer_accepted(); }

    SECTION("Extra trailing bytes are ignored")
    { require_trailing_bytes_ignored(); }

    SECTION("Zeroed buffer yields zero value")
    { require_zeroed_buffer_yields_zero(); }
}

TEST_CASE(
    "stvlinkv2_kit::make_handler_table", "[stv][stvlink][stvlinkv2]")
{
    auto table = stv::make_handler_table<std::to_underlying(test_msg_id::max)>(
        std::make_pair(test_msg_id::first, first_handler),
        std::make_pair(test_msg_id::second, second_handler));

    SECTION("Table contains all pairs") { REQUIRE(table.size() == 2U); }

    SECTION("Handler is found by identifier")
    {
        const auto first_handler_it =
            table.find(std::to_underlying(test_msg_id::first));
        REQUIRE(first_handler_it != table.end());
        REQUIRE(first_handler_it->second == first_handler);

        const auto second_handler_it =
            table.find(std::to_underlying(test_msg_id::second));
        REQUIRE(second_handler_it != table.end());
        REQUIRE(second_handler_it->second == second_handler);
    }

    SECTION("Unknown identifier is not found")
    {
        constexpr auto missing_id = static_cast<
            decltype(stv::stvlinkv2_parser::message_header_t::msg_id)>(0xFFFFU);
        REQUIRE_FALSE(table.contains(missing_id));
    }
}
