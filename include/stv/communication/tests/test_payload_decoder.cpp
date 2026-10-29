/// @file test_payload_decoder.cpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.

#include "stv/communication/payload_decoder.hpp"
#include "stv/communication/tests/demo_messages.hpp"
#include <array>
#include <catch2/catch_test_macros.hpp>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

using stv::demo_command;
using stv::demo_registers;
using stv::demo_status_text;
using stv::demo_telemetry;

/// @brief Концепт: декодер типа предоставляет view() с сигнатурой
///     «span<byte> -> const T*» (удалённый view() не удовлетворяет).
template<typename T>
concept has_view = requires(std::span<std::byte> pload) {
    { stv::payload_decoder<T>::view(pload) } -> std::same_as<const T *>;
};

TEST_CASE(
    "payload_decoder: побайтовое копирование тривиально копируемого типа")
{
    // Срез большего размера: лишние хвостовые байты игнорируются.
    std::array big{
        std::byte{0x78}, std::byte{0x56}, std::byte{0x34},
        std::byte{0x12}, std::byte{0xFE}, std::byte{0xDC},
        std::byte{0xAA}, std::byte{0xBB}, // лишние байты большого кадра
    };
    const auto big_value =
        stv::payload_decoder<demo_telemetry>::decode(std::span{big});
    REQUIRE(big_value.has_value());
    // NOLINTBEGIN(bugprone-unchecked-optional-access) -- наличие значения
    // гарантировано REQUIRE выше; анализатор это не отслеживает.
    CHECK(big_value->counter == 0x12345678U);
    CHECK(big_value->temperature == static_cast<std::int16_t>(0xDCFE));
    // NOLINTEND(bugprone-unchecked-optional-access)

    // Срез меньшего размера: поле temperature заполнено частично
    // (младший байт 0x2A), недостающий старший байт нулевой
    // (принцип обратной совместимости по спецификации CRSF).
    std::array small{
        std::byte{0x01}, std::byte{0x00}, std::byte{0x00},
        std::byte{0x00}, std::byte{0x2A},
    };
    const auto small_value =
        stv::payload_decoder<demo_telemetry>::decode(std::span{small});
    REQUIRE(small_value.has_value());
    // NOLINTBEGIN(bugprone-unchecked-optional-access) -- наличие значения
    // гарантировано REQUIRE выше; анализатор это не отслеживает.
    CHECK(small_value->counter == 1U);
    CHECK(small_value->temperature == 0x2A);
    // NOLINTEND(bugprone-unchecked-optional-access)
}

TEST_CASE(
    "payload_decoder: десериализация битстрим-кодеком сообщения")
{
    // Roundtrip через stv::try_deserialize(): код команды из одного байта.
    std::array raw{std::byte{0x07}, std::byte{0x55}};
    const auto value =
        stv::payload_decoder<demo_command>::decode(std::span{raw});
    REQUIRE(value.has_value());
    // Наличие значения гарантировано REQUIRE выше; анализатор это не
    // отслеживает.
    // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
    CHECK(value->code == 0x07U);

    // Пустой префикс принимается: поле сохраняет default-значение.
    const auto empty =
        stv::payload_decoder<demo_command>::decode(std::span<std::byte>{});
    REQUIRE(empty.has_value());
    CHECK(empty->code == 0U);
}

TEST_CASE(
    "payload_decoder: view() возвращает поля по полной нагрузке без копии")
{
    // Форма API: view() есть только у тривиально копируемого пути.
    static_assert(has_view<demo_registers>);
    // Битстрим-путь: view() удалён, доступ только через decode().
    static_assert(!has_view<demo_command>);
    // Тип с явной специализацией декодера: view() не объявлен.
    static_assert(!has_view<demo_status_text>);

    alignas(demo_registers) std::array<std::byte, sizeof(demo_registers)> raw{
        std::byte{0x78}, std::byte{0x56}, std::byte{0x34}, std::byte{0x12},
        std::byte{0xEF}, std::byte{0xBE}, std::byte{0xAD}, std::byte{0xDE},
        std::byte{0x0D}, std::byte{0xF0}, std::byte{0xED}, std::byte{0xFE},
        std::byte{0xEF}, std::byte{0xCD}, std::byte{0xAB}, std::byte{0x89},
    };

    const auto *registers =
        stv::payload_decoder<demo_registers>::view(std::span{raw});
    REQUIRE(registers != nullptr);
    CHECK(registers->reg0 == 0x12345678U);
    CHECK(registers->reg1 == 0xDEADBEEFU);
    CHECK(registers->reg2 == 0xFEEDF00DU);
    CHECK(registers->reg3 == 0x89ABCDEFU);
}

TEST_CASE(
    "payload_decoder: view() возвращает nullptr при короткой нагрузке")
{
    alignas(demo_registers) std::array<std::byte, sizeof(demo_registers) - 1U>
        raw{};

    // view() требует полный размер sizeof(T): принцип CRSF-нулевого хвоста
    // decode() для него недоступен.
    CHECK(stv::payload_decoder<demo_registers>::view(std::span{raw})
          == nullptr);
    CHECK(stv::payload_decoder<demo_registers>::view(std::span<std::byte>{})
          == nullptr);
}

TEST_CASE(
    "payload_decoder: view() возвращает nullptr при невыравненном буфере "
    "при включённой проверке выравнивания")
{
    static_assert(alignof(demo_registers) > 1U);

    // Буфер с offset 1 в выровненном массиве: размер полный, адрес
    // невыравнен для demo_registers (на Cortex-M23 невыравненный доступ —
    // UsageFault). Строгая политика payload_alignment::checked возвращает
    // nullptr — это режим для ядер без аппаратной поддержки невыровненного
    // доступа.
    alignas(demo_registers) std::array<std::byte, sizeof(demo_registers) + 1U>
               storage{};
    const auto pload = std::span{storage}.subspan(1U, sizeof(demo_registers));

    CHECK(stv::payload_decoder<demo_registers>::view<
              stv::payload_alignment::checked>(pload)
          == nullptr);
}

TEST_CASE(
    "payload_decoder: view() по умолчанию разрешает невыравненный буфер")
{
    static_assert(alignof(demo_registers) > 1U);

    // Буфер с offset 1 в выровненном массиве: адрес невыравнен для
    // demo_registers. Дефолтная политика relaxed проверку выравнивания не
    // выполняет: доступ возвращает указатель, поля читаются корректно
    // (на PC невыравненный доступ легален).
    alignas(demo_registers) std::array<std::byte, sizeof(demo_registers) + 1U>
                storage{
                    std::byte{0x00}, // невыравненный offset
                    std::byte{0x78}, std::byte{0x56}, std::byte{0x34}, std::byte{0x12},
                    std::byte{0xEF}, std::byte{0xBE}, std::byte{0xAD}, std::byte{0xDE},
                    std::byte{0x0D}, std::byte{0xF0}, std::byte{0xED}, std::byte{0xFE},
                    std::byte{0xEF}, std::byte{0xCD}, std::byte{0xAB}, std::byte{0x89},
                };
    const auto  pload = std::span{storage}.subspan(1U, sizeof(demo_registers));

    const auto *registers = stv::payload_decoder<demo_registers>::view(pload);
    REQUIRE(registers != nullptr);
    CHECK(registers->reg0 == 0x12345678U);
    CHECK(registers->reg1 == 0xDEADBEEFU);
    CHECK(registers->reg2 == 0xFEEDF00DU);
    CHECK(registers->reg3 == 0x89ABCDEFU);
}

TEST_CASE(
    "payload_decoder: строка статуса декодируется явной специализацией")
{
    std::array<char, 10> text = {"status ok"};
    const auto           pload =
        std::as_writable_bytes(std::span{text.data(), text.size() - 1U});
    const auto value = stv::payload_decoder<demo_status_text>::decode(pload);
    REQUIRE(value.has_value());
    // Наличие значения гарантировано REQUIRE выше; анализатор это не
    // отслеживает.
    // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
    CHECK(value->text == std::string_view{text.data(), text.size() - 1U});
}
