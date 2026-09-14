/// @file test_crsf.cpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.

#include "crsf_builder.hpp"
#include "stv/communication/crsf.hpp"
#include "stv/mutex_guard.hpp"
#include "stv/runtime.hpp"
#include <algorithm>
#include <array>
#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <etl/unordered_map.h>
#include <fakeit.hpp>
#include <span>
#include <type_traits>

// NOLINTBEGIN(*-magic-numbers, readability-function-cognitive-complexity)

namespace {

using crsf_lwrb_setup = stv::lwrb_setup<stv::empty_mutex>;
using crsf_lwrb_base  = stv::lwrb_base<crsf_lwrb_setup>;
using crsf_lwrb       = stv::lwrb<crsf_lwrb_base, 128U>;
using crsf_runtime    = stv::runtime_interface<stv::runtime_counter_type>;
using crsf_handler_map_type =
    etl::iunordered_map<std::uint8_t, stv::crsf_handler_type>;
using crsf_handler_map_impl_type =
    etl::unordered_map<std::uint8_t, stv::crsf_handler_type, 8U>;
using crsf_parser_setup_type =
    stv::crsf_parser_setup<crsf_lwrb_base, crsf_runtime, crsf_handler_map_type>;
using crsf_parser_type = stv::crsf_parser<crsf_parser_setup_type>;

/// @brief База обвязки тестов парсера crsf: мок runtime.
///
/// @details Мок фейкуется до создания парсера (базовый класс конструируется
/// первым), потому что конструктор парсера запускает deadline таймер, который
/// сразу вызывает runtime::get().
struct crsf_runtime_mock_base {
    fakeit::Mock<crsf_runtime> runtime_mock;

    crsf_runtime_mock_base()
    {
        using namespace fakeit;
        Fake(Method(runtime_mock, get));
    }
};

/// @brief Типовая обвязка тестов парсера crsf: кольцевой буфер, мок runtime,
/// setup и сам парсер.
struct crsf_parser_fixture: crsf_runtime_mock_base {
    crsf_lwrb                  ringbuff{crsf_lwrb_setup{}};
    crsf_handler_map_impl_type handler_map;
    crsf_parser_setup_type     setup;
    crsf_parser_type           parser;

    crsf_parser_fixture():
        setup{
            .lwrb     = &ringbuff,
            .runtime  = &runtime_mock.get(),
            .handlers = &handler_map,
        },
        parser{setup}
    {
    }

    /// @brief Записывает данные в кольцевой буфер и проверяет, что записано
    /// всё.
    auto write(
        std::span<const std::byte> data)
    { REQUIRE(ringbuff.write(data) == data.size()); }
};

/// @brief Обвязка тестов с доступом к setup до создания парсера (для тестов,
/// изменяющих параметры setup, например max_bytes_per_run).
struct crsf_setup_fixture: crsf_runtime_mock_base {
    crsf_lwrb                  ringbuff{crsf_lwrb_setup{}};
    crsf_handler_map_impl_type handler_map;
    crsf_parser_setup_type     setup;

    crsf_setup_fixture():
        setup{
            .lwrb     = &ringbuff,
            .runtime  = &runtime_mock.get(),
            .handlers = &handler_map,
        }
    {
    }

    /// @brief Записывает данные в кольцевой буфер и проверяет, что записано
    /// всё.
    auto write(
        std::span<const std::byte> data)
    { REQUIRE(ringbuff.write(data) == data.size()); }
};

/// @brief Обвязка тестов failsafe: мок runtime возвращает управляемое тестом
/// время current_time.
struct crsf_failsafe_fixture: crsf_runtime_mock_base {
    /// @brief Текущее время, возвращаемое моком runtime::get().
    stv::runtime_counter_type  current_time{};

    crsf_lwrb                  ringbuff{crsf_lwrb_setup{}};
    crsf_handler_map_impl_type handler_map;
    crsf_parser_setup_type     setup;
    crsf_parser_type           parser;

    crsf_failsafe_fixture():
        setup{
            .lwrb     = &ringbuff,
            .runtime  = &runtime_mock.get(),
            .handlers = &handler_map,
        },
        parser{setup}
    {
        using namespace fakeit;
        // Парсер создан при current_time == 0: failsafe deadline стартовал
        // с нулевой отметки, далее время сдвигается тестом.
        When(Method(runtime_mock, get)).AlwaysDo([this] {
            return current_time;
        });
    }

    /// @brief Записывает данные в кольцевой буфер и проверяет, что записано
    /// всё.
    auto write(
        std::span<const std::byte> data)
    { REQUIRE(ringbuff.write(data) == data.size()); }

    /// @brief Сдвигает возвращаемое моком runtime время вперед.
    auto advance_time(
        const auto &delta)
    {
        current_time +=
            std::chrono::duration_cast<stv::runtime_counter_type>(delta);
    }
};

/// @brief Строит кадр rc channels packed со средними значениями каналов и
/// возвращает фактический размер кадра.
auto build_mid_rc_frame(
    std::span<std::byte> buffer)
{
    crsf_test_channels channels{};
    channels.fill(stv::crsf_channel_value_mid);
    return build_rc_channels_frame(buffer, channels);
}

} // namespace

/// @brief Описывает различные варианты создания
SCENARIO(
    "Create crsf", "[stv][communication][crsf]")
{
    using namespace fakeit;

    // lwrb --------------------------------------------------------------------
    using lwrb_setup = stv::lwrb_setup<stv::empty_mutex>;
    using lwrb_base  = stv::lwrb_base<lwrb_setup>;
    using lwrb       = stv::lwrb<lwrb_base, 128U>;
    lwrb ringbuff{lwrb_setup{}};

    // runtime =---------------------------------------------------------------
    using runtime_type = stv::runtime_interface<stv::runtime_counter_type>;
    Mock<runtime_type> runtime_mock;
    Fake(Method(runtime_mock, get));

    // crsf --------------------------------------------------------------------
    using handler_map_type =
        etl::iunordered_map<std::uint8_t, stv::crsf_handler_type>;
    using handler_map_impl_type =
        etl::unordered_map<std::uint8_t, stv::crsf_handler_type, 8U>;
    using crsf_parser_setup =
        stv::crsf_parser_setup<lwrb_base, runtime_type, handler_map_type>;
    using crsf_parser = stv::crsf_parser<crsf_parser_setup>;

    handler_map_impl_type handler_map{};

    REQUIRE_FALSE(crsf_parser{crsf_parser_setup{}});

    crsf_parser_setup setup{
        .lwrb     = &ringbuff,
        .runtime  = &runtime_mock.get(),
        .handlers = &handler_map,
    };

    GIVEN("Invalid setup")
    {
        WHEN("Invalid runtime interface")
        {
            setup.runtime = nullptr;
            THEN("Create invalid object") { REQUIRE_FALSE(crsf_parser{setup}); }
        }

        WHEN("Invalid lwrb")
        {
            setup.lwrb = nullptr;
            THEN("Create invalid object") { REQUIRE_FALSE(crsf_parser{setup}); }
        }

        WHEN("Invalid handler map")
        {
            setup.handlers = nullptr;
            THEN("Create invalid object") { REQUIRE_FALSE(crsf_parser{setup}); }
        }
    }

    GIVEN("Valid setup")
    {
        crsf_parser crsf{setup};
        REQUIRE(crsf);
    }
}

/// @brief Проверяет константы протокола и функцию crsf_crc8.
SCENARIO(
    "Константы протокола и функция crc8", "[stv][communication][crsf]")
{
    static_assert(stv::crsf_frame_length_min == 2);
    static_assert(stv::crsf_frame_length_max == 62);
    static_assert(stv::crsf_frame_size_max == 64);
    static_assert(stv::crsf_type_rc_channels_packed == 0x16);
    static_assert(stv::crsf_type_link_statistics == 0x14);
    static_assert(stv::crsf_rc_channels_payload_size == 22);
    static_assert(stv::crsf_link_statistics_payload_size == 10);
    static_assert(stv::crsf_rc_channels_count == 16);
    static_assert(stv::crsf_channel_value_min == 172);
    static_assert(stv::crsf_channel_value_mid == 992);
    static_assert(stv::crsf_channel_value_max == 1811);

    GIVEN("Пустой диапазон")
    {
        THEN("crc равен нулю") { REQUIRE(stv::crsf_crc8({}) == 0); }
    }

    GIVEN("Строка \"123456789\"")
    {
        static constexpr std::array<char, 9> chars{'1', '2', '3', '4', '5',
                                                   '6', '7', '8', '9',};
        const auto data = std::as_bytes(std::span{chars});

        THEN("crc равен контрольному значению crc-8/dvb-s2")
        { REQUIRE(stv::crsf_crc8(data) == 0xBC); }
    }

    GIVEN("Один нулевой байт")
    {
        static constexpr std::array<std::byte, 1> data{std::byte{0x00}};

        THEN("crc равен нулю") { REQUIRE(stv::crsf_crc8(data) == 0); }
    }

    GIVEN("Произвольный набор данных")
    {
        static constexpr std::array data{
            std::byte{0x16}, std::byte{0xC8}, std::byte{0x03}, std::byte{0xE0},
            std::byte{0xFF}, std::byte{0x00}, std::byte{0x7F},
        };
        const auto                              crc = stv::crsf_crc8(data);

        std::array<std::byte, data.size() + 1U> data_with_crc{};
        std::copy(data.begin(), data.end(), data_with_crc.begin());
        data_with_crc.back() = std::byte{crc};

        THEN("crc от данных с приложенным crc равен нулю")
        { REQUIRE(stv::crsf_crc8(data_with_crc) == 0); }
    }
}

/// @brief Проверяет тестовый helper построения кадров crsf.
SCENARIO(
    "Helper построения кадров crsf", "[stv][communication][crsf]")
{
    GIVEN("Каналы со средним значением")
    {
        crsf_test_channels channels{};
        channels.fill(stv::crsf_channel_value_mid);

        std::array<std::byte, stv::crsf_frame_size_max> buffer{};

        WHEN("Строится кадр rc channels packed")
        {
            const std::size_t frame_size =
                build_rc_channels_frame(buffer, channels);

            THEN("Кадр имеет корректные заголовок, размер и crc")
            {
                // sync + length + type + 22 байта payload + crc.
                REQUIRE(frame_size == 26);
                REQUIRE(buffer[0] == std::byte{0xC8});
                // length = type + 22 байта payload + crc.
                REQUIRE(buffer[1] == std::byte{24});
                REQUIRE(buffer[2]
                        == std::byte{stv::crsf_type_rc_channels_packed});
                REQUIRE(buffer[25]
                        == std::byte{
                            stv::crsf_crc8(std::span{buffer}.subspan(2, 23))});
            }
        }
    }

    GIVEN("Каналы min, max, mid, min и далее средние")
    {
        crsf_test_channels channels{};
        channels.fill(stv::crsf_channel_value_mid);
        channels[0] = stv::crsf_channel_value_min;
        channels[1] = stv::crsf_channel_value_max;
        channels[2] = stv::crsf_channel_value_mid;
        channels[3] = stv::crsf_channel_value_min;

        std::array<std::byte, stv::crsf_frame_size_max> buffer{};

        WHEN("Строится кадр rc channels packed")
        {
            const std::size_t frame_size =
                build_rc_channels_frame(buffer, channels);

            THEN("Каналы упакованы по 11 бит lsb-first")
            {
                REQUIRE(frame_size == 26);
                // Канал 0 (172 = 0b00010101100): младшие 8 бит.
                REQUIRE(buffer[3] == std::byte{0xAC});
                // Старшие 3 бита канала 0 (0b000) + младшие 5 бит канала 1
                // (1811 = 0b11100010011, младшие 5 бит 0b10011).
                REQUIRE(buffer[4] == std::byte{0x98});
                // Биты 5..10 канала 1 (0b111000) + младшие 2 бита канала 2
                // (992 = 0b01111100000, младшие 2 бита 0b00).
                REQUIRE(buffer[5] == std::byte{0x38});
                // Биты 2..9 канала 2 (992 >> 2 = 248).
                REQUIRE(buffer[6] == std::byte{0xF8});
                // Бит 10 канала 2 (0b0) + младшие 7 бит канала 3
                // (172 & 0x7F = 44 = 0b0101100).
                REQUIRE(buffer[7] == std::byte{0x58});
            }
        }
    }

    GIVEN("Поля link statistics")
    {
        const stv::crsf_link_statistics link_statistics{
            .up_rssi_ant1      = 100,
            .up_rssi_ant2      = 95,
            .up_link_quality   = 90,
            .up_snr            = 1,
            .active_antenna    = 0,
            .rf_profile        = 1,
            .up_rf_power       = 2,
            .down_rssi         = 85,
            .down_link_quality = 80,
            .down_snr          = -3,
        };

        std::array<std::byte, stv::crsf_frame_size_max> buffer{};

        WHEN("Строится кадр link statistics")
        {
            const std::size_t frame_size =
                build_link_statistics_frame(buffer, link_statistics);

            THEN("Кадр имеет корректные заголовок, payload и crc")
            {
                // sync + length + type + 10 байт payload + crc.
                REQUIRE(frame_size == 14);
                REQUIRE(buffer[0] == std::byte{0xC8});
                // length = type + 10 байт payload + crc.
                REQUIRE(buffer[1] == std::byte{12});
                REQUIRE(buffer[2] == std::byte{stv::crsf_type_link_statistics});
                REQUIRE(buffer[3] == std::byte{100});
                REQUIRE(buffer[4] == std::byte{95});
                REQUIRE(buffer[5] == std::byte{90});
                REQUIRE(buffer[6] == std::byte{1});
                REQUIRE(buffer[7] == std::byte{0});
                REQUIRE(buffer[8] == std::byte{1});
                REQUIRE(buffer[9] == std::byte{2});
                REQUIRE(buffer[10] == std::byte{85});
                REQUIRE(buffer[11] == std::byte{80});
                REQUIRE(buffer[12]
                        == static_cast<std::byte>(
                            static_cast<std::uint8_t>(std::int8_t{-3})));
                REQUIRE(buffer[13]
                        == std::byte{
                            stv::crsf_crc8(std::span{buffer}.subspan(2, 11))});
            }
        }
    }

    GIVEN("Буфер недостаточного размера, предзаполненный маркером 0xAA")
    {
        constexpr std::byte canary{0xAA};

        WHEN("Строится кадр rc channels packed")
        {
            std::array<std::byte, 25> buffer{};
            buffer.fill(canary);

            const std::size_t frame_size = build_rc_channels_frame(buffer, {});

            THEN("Ничего не записано и возвращён ноль")
            {
                REQUIRE(frame_size == 0);
                REQUIRE(std::ranges::all_of(
                    buffer, [](std::byte byte) { return byte == canary; }));
            }
        }

        WHEN("Строится кадр link statistics")
        {
            std::array<std::byte, 13> buffer{};
            buffer.fill(canary);

            const std::size_t frame_size =
                build_link_statistics_frame(buffer, {});

            THEN("Ничего не записано и возвращён ноль")
            {
                REQUIRE(frame_size == 0);
                REQUIRE(std::ranges::all_of(
                    buffer, [](std::byte byte) { return byte == canary; }));
            }
        }
    }

    GIVEN("Буфер точно по размеру кадра")
    {
        WHEN("Строится кадр rc channels packed")
        {
            std::array<std::byte, 26> buffer{};

            const std::size_t frame_size = build_rc_channels_frame(buffer, {});

            THEN("Кадр построен и возвращён его полный размер")
            {
                REQUIRE(frame_size == 26);
                REQUIRE(buffer[0] == std::byte{0xC8});
            }
        }

        WHEN("Строится кадр link statistics")
        {
            std::array<std::byte, 14> buffer{};

            const std::size_t         frame_size =
                build_link_statistics_frame(buffer, {});

            THEN("Кадр построен и возвращён его полный размер")
            {
                REQUIRE(frame_size == 14);
                REQUIRE(buffer[0] == std::byte{0xC8});
            }
        }
    }

    GIVEN("Буфер с канарейкой 0xAA за границами кадра")
    {
        constexpr std::byte canary{0xAA};

        WHEN("Строится кадр rc channels packed")
        {
            std::array<std::byte, stv::crsf_frame_size_max> buffer{};
            buffer.fill(canary);

            const std::size_t frame_size = build_rc_channels_frame(buffer, {});

            THEN("Байты за границей кадра не изменились")
            {
                REQUIRE(frame_size == 26);
                REQUIRE(std::ranges::all_of(
                    std::span{buffer}.subspan(frame_size),
                    [](std::byte byte) { return byte == canary; }));
            }
        }

        WHEN("Строится кадр link statistics")
        {
            std::array<std::byte, stv::crsf_frame_size_max> buffer{};
            buffer.fill(canary);

            const std::size_t frame_size =
                build_link_statistics_frame(buffer, {});

            THEN("Байты за границей кадра не изменились")
            {
                REQUIRE(frame_size == 14);
                REQUIRE(std::ranges::all_of(
                    std::span{buffer}.subspan(frame_size),
                    [](std::byte byte) { return byte == canary; }));
            }
        }
    }
}

/// @brief Пустой буфер: разбор не выполняется.
SCENARIO(
    "Ядро парсера: пустой буфер", "[stv][communication][crsf]")
{
    crsf_parser_fixture fixture{};

    REQUIRE(fixture.parser);
    REQUIRE(fixture.parser.processing() == 0);
    REQUIRE(fixture.ringbuff.get_full() == 0);
}

/// @brief Один валидный кадр rc channels packed изымается из буфера.
SCENARIO(
    "Ядро парсера: один валидный кадр 0x16", "[stv][communication][crsf]")
{
    crsf_parser_fixture                             fixture{};

    std::array<std::byte, stv::crsf_frame_size_max> frame_buffer{};
    const auto frame_size = build_mid_rc_frame(frame_buffer);
    fixture.write(std::span{frame_buffer}.subspan(0U, frame_size));

    REQUIRE(fixture.parser.processing() == 1);
    REQUIRE(fixture.ringbuff.get_full() == 0);
}

/// @brief Несколько кадров подряд разбираются за один вызов run().
SCENARIO(
    "Ядро парсера: несколько кадров подряд", "[stv][communication][crsf]")
{
    crsf_parser_fixture                             fixture{};

    std::array<std::byte, stv::crsf_frame_size_max> frame_buffer{};

    const auto rc_frame_size = build_mid_rc_frame(frame_buffer);
    fixture.write(std::span{frame_buffer}.subspan(0U, rc_frame_size));

    const auto link_frame_size =
        build_link_statistics_frame(frame_buffer, stv::crsf_link_statistics{});
    fixture.write(std::span{frame_buffer}.subspan(0U, link_frame_size));

    const auto second_rc_frame_size = build_mid_rc_frame(frame_buffer);
    fixture.write(std::span{frame_buffer}.subspan(0U, second_rc_frame_size));

    REQUIRE(fixture.parser.processing() == 3);
    REQUIRE(fixture.ringbuff.get_full() == 0);
}

/// @brief Неполный кадр не потребляется и разбирается после дозаписи остатка.
SCENARIO(
    "Ядро парсера: неполный кадр", "[stv][communication][crsf]")
{
    crsf_parser_fixture                             fixture{};

    std::array<std::byte, stv::crsf_frame_size_max> frame_buffer{};
    const auto            frame_size = build_mid_rc_frame(frame_buffer);

    constexpr std::size_t first_chunk_size{10U};
    fixture.write(std::span{frame_buffer}.subspan(0U, first_chunk_size));

    REQUIRE(fixture.parser.processing() == 0);
    REQUIRE(fixture.ringbuff.get_full() == first_chunk_size);

    fixture.write(std::span{frame_buffer}.subspan(
        first_chunk_size, frame_size - first_chunk_size));

    REQUIRE(fixture.parser.processing() == 1);
    REQUIRE(fixture.ringbuff.get_full() == 0);
}

/// @brief Мусор с невалидной длиной перед кадром пропускается посимвольно.
SCENARIO(
    "Ядро парсера: мусор перед кадром", "[stv][communication][crsf]")
{
    crsf_parser_fixture fixture{};

    // Каждый из байтов в позиции frame length невалиден (0x01 < 2, 0x3F и
    // 0xFF > 62), парсер должен выполнить посимвольный resync.
    static constexpr std::array garbage{
        std::byte{0x00},
        std::byte{0x01},
        std::byte{0x3F},
        std::byte{0xFF},
    };
    fixture.write(garbage);

    std::array<std::byte, stv::crsf_frame_size_max> frame_buffer{};
    const auto frame_size = build_mid_rc_frame(frame_buffer);
    fixture.write(std::span{frame_buffer}.subspan(0U, frame_size));

    REQUIRE(fixture.parser.processing() == 1);
    REQUIRE(fixture.ringbuff.get_full() == 0);
}

/// @brief Кадр с испорченным crc пропускается посимвольным resync (пропуск
/// одного байта sync), следующий валидный кадр разбирается.
SCENARIO(
    "Ядро парсера: кадр с испорченным crc", "[stv][communication][crsf]")
{
    crsf_parser_fixture                             fixture{};

    std::array<std::byte, stv::crsf_frame_size_max> frame_buffer{};
    const auto frame_size = build_mid_rc_frame(frame_buffer);

    // Портим байт crc: вычисленное значение перестанет совпадать с
    // записанным в кадре.
    frame_buffer.at(frame_size - 1U) ^= std::byte{0xFF};
    fixture.write(std::span{frame_buffer}.subspan(0U, frame_size));

    const auto valid_frame_size = build_mid_rc_frame(frame_buffer);
    fixture.write(std::span{frame_buffer}.subspan(0U, valid_frame_size));

    // Посимвольный resync: на каждой неудачной проверке crc из буфера
    // пропускается ровно один байт. На этом потоке resync доходит до
    // «фантомного» кадра (байт длины 62 внутри payload испорченного кадра),
    // которому не хватает байт в буфере, — разбор приостанавливается в
    // ожидании дозаписи потока, валидных кадров пока не изъято (часть
    // мусора уже пропущена посимвольно). Это отличие от прежнего поведения
    // «отбросить кадр целиком»: цена посимвольного resync — возможное
    // ожидание дозаписи, зато валидные кадры, попавшие в «payload»
    // испорченного кадра, не теряются.
    REQUIRE(fixture.parser.processing() == 0);

    // Дозапись потока (мусор с заведомо невалидной длиной 0xFF): resync
    // завершается, валидный кадр найден посимвольно и разобран.
    std::array<std::byte, stv::crsf_frame_size_max> junk{};
    junk.fill(std::byte{0xFF});
    fixture.write(junk);

    REQUIRE(fixture.parser.processing() == 1);
    // Мусор пропущен посимвольно; в буфере остаётся один байт 0xFF —
    // неполный заголовок, который будет потреблён при дозаписи потока.
    REQUIRE(fixture.ringbuff.get_full() == 1U);
}

/// @brief Невалидная длина 0 в позиции frame length пропускается посимвольным
/// resync, следующий валидный кадр разбирается.
SCENARIO(
    "Ядро парсера: невалидная длина 0 перед кадром",
    "[stv][communication][crsf]")
{
    crsf_parser_fixture fixture{};

    // Длина 0 < crsf_frame_length_min (длины 1, 63 и 255 покрыты сценарием
    // «мусор перед кадром»): парсер должен выполнить посимвольный resync.
    static constexpr std::array garbage{std::byte{0xC8}, std::byte{0x00}};
    fixture.write(garbage);

    std::array<std::byte, stv::crsf_frame_size_max> frame_buffer{};
    const auto frame_size = build_mid_rc_frame(frame_buffer);
    fixture.write(std::span{frame_buffer}.subspan(0U, frame_size));

    REQUIRE(fixture.parser.processing() == 1);
    REQUIRE(fixture.ringbuff.get_full() == 0);
}

/// @brief Валидный кадр неизвестного типа пропускается и считается
/// разобранным.
SCENARIO(
    "Ядро парсера: кадр неизвестного типа", "[stv][communication][crsf]")
{
    crsf_parser_fixture fixture{};

    // Кадр типа 0x42 с одним байтом payload, собранный вручную.
    constexpr std::uint8_t                          unknown_type{0x42U};

    std::array<std::byte, stv::crsf_frame_size_max> frame_buffer{};
    frame_buffer[0U] = std::byte{0xC8};
    // frame length = type + 1 байт payload + crc.
    frame_buffer[1U] = std::byte{3U};
    frame_buffer[2U] = std::byte{unknown_type};
    frame_buffer[3U] = std::byte{0xAA};
    frame_buffer[4U] =
        std::byte{stv::crsf_crc8(std::span{frame_buffer}.subspan(2U, 2U))};

    constexpr std::size_t frame_size{5U};
    fixture.write(std::span{frame_buffer}.subspan(0U, frame_size));

    REQUIRE(fixture.parser.processing() == 1);
    REQUIRE(fixture.ringbuff.get_full() == 0);
}

/// @brief Кадр максимального размера (64 байта, frame length = 62) с
/// валидным crc разбирается полностью.
SCENARIO(
    "Ядро парсера: кадр максимального размера 64 байта",
    "[stv][communication][crsf]")
{
    crsf_parser_fixture fixture{};

    // Кадр собран вручную: frame length = crsf_frame_length_max (62),
    // неизвестный тип 0x42, 60 байт payload, crc по type и payload.
    constexpr std::uint8_t            unknown_type{0x42U};
    constexpr std::size_t             payload_size{60U};
    constexpr std::size_t             frame_size{stv::crsf_frame_size_max};

    std::array<std::byte, frame_size> frame_buffer{};
    frame_buffer[0U] = std::byte{0xC8};
    frame_buffer[1U] = std::byte{stv::crsf_frame_length_max};
    frame_buffer[2U] = std::byte{unknown_type};
    for(std::size_t index{3U}; index < frame_size - 1U; ++index)
    {
        frame_buffer.at(index) = static_cast<std::byte>(index);
    }
    frame_buffer[frame_size - 1U] = std::byte{
        stv::crsf_crc8(std::span{frame_buffer}.subspan(2U, payload_size + 1U)),
    };

    fixture.write(frame_buffer);

    REQUIRE(fixture.parser.processing() == 1);
    REQUIRE(fixture.ringbuff.get_full() == 0);
}

/// @brief Пользовательский обработчик вызывается с payload кадра; повторная
/// регистрация типа отклоняется.
SCENARIO(
    "Ядро парсера: register_handler", "[stv][communication][crsf]")
{
    crsf_parser_fixture    fixture{};

    constexpr std::uint8_t custom_type{0x42U};
    constexpr std::byte    payload_first{0xAA};
    constexpr std::byte    payload_second{0x55};

    bool                   is_called{false};
    std::size_t            received_size{0U};
    std::byte              received_first{0x00};
    std::byte              received_second{0x00};

    auto on_custom_frame = [&](std::span<const std::byte> payload) {
        is_called       = true;
        received_size   = payload.size();
        received_first  = payload[0U];
        received_second = payload[1U];
        return true;
    };

    REQUIRE(fixture.parser.register_handler(
        custom_type, stv::crsf_handler_type::create(on_custom_frame)));

    // Повторная регистрация того же типа отклоняется.
    REQUIRE_FALSE(fixture.parser.register_handler(
        custom_type, stv::crsf_handler_type::create(on_custom_frame)));

    // Типы 0x16 и 0x14 уже заняты автоматически зарегистрированными
    // обработчиками парсера.
    REQUIRE_FALSE(fixture.parser.register_handler(
        stv::crsf_type_rc_channels_packed,
        stv::crsf_handler_type::create(on_custom_frame)));
    REQUIRE_FALSE(fixture.parser.register_handler(
        stv::crsf_type_link_statistics,
        stv::crsf_handler_type::create(on_custom_frame)));

    // Кадр типа 0x42 с двумя байтами payload, собранный вручную.
    std::array<std::byte, stv::crsf_frame_size_max> frame_buffer{};
    frame_buffer[0U] = std::byte{0xC8};
    // frame length = type + 2 байта payload + crc.
    frame_buffer[1U] = std::byte{4U};
    frame_buffer[2U] = std::byte{custom_type};
    frame_buffer[3U] = payload_first;
    frame_buffer[4U] = payload_second;
    frame_buffer[5U] =
        std::byte{stv::crsf_crc8(std::span{frame_buffer}.subspan(2U, 3U))};

    constexpr std::size_t frame_size{6U};
    fixture.write(std::span{frame_buffer}.subspan(0U, frame_size));

    REQUIRE(fixture.parser.processing() == 1);
    REQUIRE(fixture.ringbuff.get_full() == 0);

    REQUIRE(is_called);
    REQUIRE(received_size == 2);
    REQUIRE(received_first == payload_first);
    REQUIRE(received_second == payload_second);
}

/// @brief Регистрация обработчика сверх ёмкости таблицы отклоняется.
SCENARIO(
    "Ядро парсера: переполнение таблицы обработчиков",
    "[stv][communication][crsf]")
{
    crsf_parser_fixture fixture{};

    // Таблица ёмкостью 8: два слота заняты автоматически
    // зарегистрированными обработчиками парсера (0x16 и 0x14), остаётся 6
    // слотов для пользовательских обработчиков.
    static_assert(crsf_handler_map_impl_type::MAX_SIZE == 8U);

    auto on_frame = [](std::span<const std::byte>) { return true; };

    // Шесть пользовательских типов заполняют таблицу до предела.
    for(std::uint8_t index{0U}; index < 6U; ++index)
    {
        const auto frame_type = static_cast<std::uint8_t>(0x40U + index);
        REQUIRE(fixture.parser.register_handler(
            frame_type, stv::crsf_handler_type::create(on_frame)));
    }

    // Таблица полна: регистрация седьмого типа отклоняется.
    REQUIRE_FALSE(fixture.parser.register_handler(
        0x50U, stv::crsf_handler_type::create(on_frame)));
}

/// @brief Кадр, разорванный границей кольцевого буфера, разбирается
/// корректно.
SCENARIO(
    "Ядро парсера: кадр на границе кольцевого буфера",
    "[stv][communication][crsf]")
{
    crsf_parser_fixture fixture{};

    // Смещаем указатели чтения/записи почти в конец буфера емкостью 128
    // байт, чтобы следующий кадр лёг на wrap-around.
    std::array<std::byte, 120U> junk{};
    junk.fill(std::byte{0x55});
    fixture.write(junk);
    REQUIRE(fixture.ringbuff.skip(junk.size()) == junk.size());

    std::array<std::byte, stv::crsf_frame_size_max> frame_buffer{};
    const auto frame_size = build_mid_rc_frame(frame_buffer);
    fixture.write(std::span{frame_buffer}.subspan(0U, frame_size));

    REQUIRE(fixture.parser.processing() == 1);
    REQUIRE(fixture.ringbuff.get_full() == 0);
}

/// @brief Невалидный объект (setup с nullptr) не падает и возвращает 0.
SCENARIO(
    "Ядро парсера: невалидный объект", "[stv][communication][crsf]")
{
    const crsf_parser_setup_type empty_setup{};
    crsf_parser_type             parser{empty_setup};

    REQUIRE_FALSE(parser);
    REQUIRE(parser.processing() == 0);
}

/// @brief Нарушение контракта «парсер — единственный читатель»: буфер
/// опустошён извне, пока автомат ждёт дозаписи кадра. Автомат обязан перейти
/// в wait_head_state и начать чистую ресинхронизацию, а не ждать устаревший
/// frame_size.
SCENARIO(
    "Ядро парсера: буфер опустошён извне в ожидании кадра",
    "[stv][communication][crsf]")
{
    crsf_parser_fixture fixture{};

    // Часть кадра 0x16 (10 байт из 26): автомат запоминает frame_size == 26
    // и ждёт дозаписи остатка кадра.
    std::array<std::byte, stv::crsf_frame_size_max> frame_buffer{};
    const auto            rc_frame_size = build_mid_rc_frame(frame_buffer);
    constexpr std::size_t first_chunk_size{10U};
    fixture.write(std::span{frame_buffer}.subspan(0U, first_chunk_size));
    REQUIRE(fixture.parser.processing() == 0);
    REQUIRE(rc_frame_size == 26U);

    // Контракт нарушен: байты изъяты из буфера извне (буфер «сброшен»).
    REQUIRE(fixture.ringbuff.skip(first_chunk_size) == first_chunk_size);

    // Ближайший run() обнаруживает пустой буфер в ожидании кадра и
    // переходит в wait_head_state (чистая ресинхронизация).
    REQUIRE(fixture.parser.processing() == 0);

    // Записан кадр другого типа и размера (0x14, 14 байт): если бы автомат
    // продолжал ждать устаревшие 26 байт, кадр бы не разобрался.
    const auto link_frame_size =
        build_link_statistics_frame(frame_buffer, stv::crsf_link_statistics{});
    fixture.write(std::span{frame_buffer}.subspan(0U, link_frame_size));

    REQUIRE(fixture.parser.processing() == 1);
    REQUIRE(fixture.ringbuff.get_full() == 0);
}

/// @brief Неблокирующий resync: за один run() пропускается не более
/// max_bytes_per_run байт мусора, остаток добирается следующими вызовами.
SCENARIO(
    "Ядро парсера: лимит max_bytes_per_run", "[stv][communication][crsf]")
{
    crsf_setup_fixture fixture{};
    fixture.setup.max_bytes_per_run = 8U;
    crsf_parser_type parser{fixture.setup};

    // 20 байт мусора с невалидной длиной 0xFF и валидный кадр в хвосте.
    constexpr std::size_t               garbage_size{20U};
    std::array<std::byte, garbage_size> garbage{};
    garbage.fill(std::byte{0xFF});
    fixture.write(garbage);

    std::array<std::byte, stv::crsf_frame_size_max> frame_buffer{};
    const auto frame_size = build_mid_rc_frame(frame_buffer);
    fixture.write(std::span{frame_buffer}.subspan(0U, frame_size));

    // Первый run(): пропущено ровно 8 байт мусора, кадр ещё не добран.
    REQUIRE(parser.processing() == 0);
    REQUIRE(fixture.ringbuff.get_full() == garbage_size + frame_size - 8U);

    // Второй run(): пропущены ещё 8 байт мусора.
    REQUIRE(parser.processing() == 0);
    REQUIRE(fixture.ringbuff.get_full() == garbage_size + frame_size - 16U);

    // Третий run(): остаток мусора (4 байта) и валидный кадр.
    REQUIRE(parser.processing() == 1);
    REQUIRE(fixture.ringbuff.get_full() == 0);
}

/// @brief Лимит по умолчанию (128 байт) позволяет за один run() обработать
/// мусор размером почти в полный буфер.
SCENARIO(
    "Ядро парсера: лимит max_bytes_per_run по умолчанию",
    "[stv][communication][crsf]")
{
    crsf_parser_fixture fixture{};
    REQUIRE(fixture.setup.max_bytes_per_run == 128U);

    // 100 байт мусора с невалидной длиной и валидный кадр в хвосте.
    std::array<std::byte, 100U> garbage{};
    garbage.fill(std::byte{0xFF});
    fixture.write(garbage);

    std::array<std::byte, stv::crsf_frame_size_max> frame_buffer{};
    const auto frame_size = build_mid_rc_frame(frame_buffer);
    fixture.write(std::span{frame_buffer}.subspan(0U, frame_size));

    // Весь мусор (100 < 128) пропущен за один run(), кадр разобран.
    REQUIRE(fixture.parser.processing() == 1);
    REQUIRE(fixture.ringbuff.get_full() == 0);
}

/// @brief Лимит max_bytes_per_run касается только resync-потребления: разбор
/// валидных кадров лимит не расходует и не ограничивается.
SCENARIO(
    "Ядро парсера: валидные кадры не расходуют лимит",
    "[stv][communication][crsf]")
{
    crsf_setup_fixture fixture{};
    fixture.setup.max_bytes_per_run = 4U;
    crsf_parser_type parser{fixture.setup};

    // Мусор ровно на лимит (4 байта с невалидной длиной) и три валидных
    // кадра в хвосте.
    static constexpr std::array garbage{
        std::byte{0xFF},
        std::byte{0xFF},
        std::byte{0xFF},
        std::byte{0xFF},
    };
    fixture.write(garbage);

    std::array<std::byte, stv::crsf_frame_size_max> frame_buffer{};
    const auto frame_size = build_mid_rc_frame(frame_buffer);
    for(std::size_t index{0U}; index < 3U; ++index)
    {
        fixture.write(std::span{frame_buffer}.subspan(0U, frame_size));
    }

    // Единственный run(): лимит (4 байта) полностью израсходован мусором,
    // но разбор валидных кадров лимит не ограничивает — все три кадра
    // разобраны за тот же вызов.
    REQUIRE(parser.processing() == 3);
    REQUIRE(fixture.ringbuff.get_full() == 0);
}

/// @brief Лимит max_bytes_per_run общий для обеих веток resync: кадр с
/// валидным frame length, но несходящимся crc расходует бюджет наравне с
/// байтами невалидной длины, run() неблокирующе возвращает управление.
SCENARIO(
    "Ядро парсера: лимит max_bytes_per_run при ошибке crc",
    "[stv][communication][crsf]")
{
    crsf_setup_fixture fixture{};
    fixture.setup.max_bytes_per_run = 4U;
    crsf_parser_type parser{fixture.setup};

    // Мусорный «кадр»: frame length 0x04 внутри допустимого диапазона 2..62,
    // crc заведомо не сходится — resync идёт по ветке ошибки crc (пропуск
    // sync). Байты type/payload 0x42 и байт crc вне диапазона 2..62, поэтому
    // resync со сдвигом внутри мусора идёт по ветке невалидной длины: за
    // один run() бюджет расходуют обе ветки.
    constexpr std::array junk_body{
        std::byte{0x42},
        std::byte{0x42},
        std::byte{0x42},
    };
    const auto           junk_crc_actual = std::byte{stv::crsf_crc8(junk_body)};
    const auto           junk_crc =
        junk_crc_actual != std::byte{0x00} ? std::byte{0x00} : std::byte{0xFF};
    REQUIRE(junk_crc != junk_crc_actual);

    const std::array junk_frame{
        std::byte{0xC8}, std::byte{0x04}, std::byte{0x42},
        std::byte{0x42}, std::byte{0x42}, junk_crc,
    };

    // 12 байт мусора (два повтора «кадра» с ошибкой crc) и валидный кадр в
    // хвосте.
    constexpr std::size_t               garbage_size{12U};
    std::array<std::byte, garbage_size> garbage{};
    for(std::size_t index{0U}; index < garbage_size; ++index)
    {
        garbage.at(index) = junk_frame.at(index % junk_frame.size());
    }
    fixture.write(garbage);

    std::array<std::byte, stv::crsf_frame_size_max> frame_buffer{};
    const auto frame_size = build_mid_rc_frame(frame_buffer);
    fixture.write(std::span{frame_buffer}.subspan(0U, frame_size));

    // Первый run(): пропущено ровно 4 байта мусора — первый по ветке ошибки
    // crc, остальные по ветке невалидной длины (бюджет общий); валидный кадр
    // ещё не добран.
    REQUIRE(parser.processing() == 0);
    REQUIRE(fixture.ringbuff.get_full() == garbage_size + frame_size - 4U);

    // Второй run(): пропущены ещё 4 байта мусора (ветки снова чередуются).
    REQUIRE(parser.processing() == 0);
    REQUIRE(fixture.ringbuff.get_full() == garbage_size + frame_size - 8U);

    // Третий run(): остаток мусора (4 байта) и валидный кадр.
    REQUIRE(parser.processing() == 1);
    REQUIRE(fixture.ringbuff.get_full() == 0);
}

/// @brief «Зависший» буфер из чистого мусора (валидный кадр не соберётся
/// никогда): каждый run() неблокирующе потребляет не более max_bytes_per_run
/// байт и возвращает управление; неполный заголовок в хвосте ожидает
/// дозаписи, не крутясь впустую.
SCENARIO(
    "Ядро парсера: «зависший» буфер из чистого мусора",
    "[stv][communication][crsf]")
{
    crsf_setup_fixture fixture{};
    fixture.setup.max_bytes_per_run = 8U;
    crsf_parser_type parser{fixture.setup};

    // 100 байт мусора с невалидной длиной 0xFF.
    constexpr std::size_t               garbage_size{100U};
    std::array<std::byte, garbage_size> garbage{};
    garbage.fill(std::byte{0xFF});
    fixture.write(garbage);

    // Каждый run() потребляет ровно 8 байт и возвращает управление.
    for(std::size_t index{0U}; index < 12U; ++index)
    {
        REQUIRE(parser.processing() == 0);
        REQUIRE(fixture.ringbuff.get_full()
                == garbage_size - (8U * (index + 1U)));
    }

    // Остаток (4 байта): за один run() пропущены 3 байта, последний байт —
    // неполный заголовок, автомат ждёт дозаписи (не виснет).
    REQUIRE(parser.processing() == 0);
    REQUIRE(fixture.ringbuff.get_full() == 1U);

    // Повторный run() завершается сразу: неполный заголовок не потребляется,
    // разбор продолжится после дозаписи потока.
    REQUIRE(parser.processing() == 0);
    REQUIRE(fixture.ringbuff.get_full() == 1U);
}

/// @brief До первого принятого кадра 0x16 буфер каналов заполнен нулями.
SCENARIO(
    "Обработчик 0x16: каналы до первого кадра — нули",
    "[stv][communication][crsf]")
{
    // Публичный буфер каналов значение-совместим с типом helper'а.
    static_assert(std::is_same_v<stv::crsf_rc_channels, crsf_test_channels>);

    const crsf_parser_fixture fixture{};

    REQUIRE(
        std::ranges::all_of(fixture.parser.channels(),
                            [](std::uint16_t value) { return value == 0; }));
}

/// @brief Кадр 0x16 распаковывается в буфер каналов с точными значениями.
SCENARIO(
    "Обработчик 0x16: распаковка различных значений каналов",
    "[stv][communication][crsf]")
{
    crsf_parser_fixture fixture{};

    // min, mid, max и промежуточные значения, включая границы 11 бит.
    const crsf_test_channels channels{
        172, 992,  1811, 500, 700,  1000, 1200, 1500,
        250, 1750, 300,  0,   2047, 888,  111,  1911,
    };

    std::array<std::byte, stv::crsf_frame_size_max> frame_buffer{};
    const auto frame_size = build_rc_channels_frame(frame_buffer, channels);
    fixture.write(std::span{frame_buffer}.subspan(0U, frame_size));

    REQUIRE(fixture.parser.processing() == 1);
    REQUIRE(fixture.parser.channels() == channels);
}

/// @brief Крайние значения каналов распаковываются точно (битовые границы
/// упаковки).
SCENARIO(
    "Обработчик 0x16: крайние значения каналов", "[stv][communication][crsf]")
{
    GIVEN("Все каналы равны crsf_channel_value_min")
    {
        crsf_parser_fixture fixture{};

        crsf_test_channels  channels{};
        channels.fill(stv::crsf_channel_value_min);

        std::array<std::byte, stv::crsf_frame_size_max> frame_buffer{};
        const auto frame_size = build_rc_channels_frame(frame_buffer, channels);
        fixture.write(std::span{frame_buffer}.subspan(0U, frame_size));

        REQUIRE(fixture.parser.processing() == 1);
        REQUIRE(fixture.parser.channels() == channels);
    }

    GIVEN("Все каналы равны crsf_channel_value_max")
    {
        crsf_parser_fixture fixture{};

        crsf_test_channels  channels{};
        channels.fill(stv::crsf_channel_value_max);

        std::array<std::byte, stv::crsf_frame_size_max> frame_buffer{};
        const auto frame_size = build_rc_channels_frame(frame_buffer, channels);
        fixture.write(std::span{frame_buffer}.subspan(0U, frame_size));

        REQUIRE(fixture.parser.processing() == 1);
        REQUIRE(fixture.parser.channels() == channels);
    }
}

/// @brief Кадр 0x16 с расширенным payload: каналы разобраны, лишние байты
/// проигнорированы.
SCENARIO(
    "Обработчик 0x16: расширенный payload", "[stv][communication][crsf]")
{
    crsf_parser_fixture fixture{};

    crsf_test_channels  channels{};
    channels.fill(stv::crsf_channel_value_mid);
    channels[0U]  = stv::crsf_channel_value_min;
    channels[15U] = stv::crsf_channel_value_max;

    // Кадр собран вручную: упакованные каналы из кадра helper'а + 3 лишних
    // байта payload; длина кадра и crc пересчитаны на новый payload.
    std::array<std::byte, stv::crsf_frame_size_max> base_frame{};
    const auto base_frame_size = build_rc_channels_frame(base_frame, channels);
    REQUIRE(base_frame_size == 26);

    constexpr std::size_t extra_size{3U};
    constexpr std::size_t payload_size{
        stv::crsf_rc_channels_payload_size + extra_size,
    };
    constexpr std::size_t payload_index{3U};

    std::array<std::byte, stv::crsf_frame_size_max> frame_buffer{};
    frame_buffer[0U] = std::byte{0xC8};
    // frame length = type + payload + crc.
    frame_buffer[1U] = static_cast<std::byte>(payload_size + 2U);
    frame_buffer[2U] = std::byte{stv::crsf_type_rc_channels_packed};
    std::copy_n(base_frame.begin() + payload_index,
                stv::crsf_rc_channels_payload_size,
                frame_buffer.begin() + payload_index);
    frame_buffer[payload_index + stv::crsf_rc_channels_payload_size + 0U] =
        std::byte{0xDE};
    frame_buffer[payload_index + stv::crsf_rc_channels_payload_size + 1U] =
        std::byte{0xAD};
    frame_buffer[payload_index + stv::crsf_rc_channels_payload_size + 2U] =
        std::byte{0xBE};
    frame_buffer[payload_index + payload_size] = std::byte{
        stv::crsf_crc8(std::span{frame_buffer}.subspan(2U, payload_size + 1U)),
    };

    constexpr std::size_t frame_size{payload_size + 4U};
    fixture.write(std::span{frame_buffer}.subspan(0U, frame_size));

    REQUIRE(fixture.parser.processing() == 1);
    REQUIRE(fixture.ringbuff.get_full() == 0);
    REQUIRE(fixture.parser.channels() == channels);
}

/// @brief Кадр известного типа с payload короче номинального (валидный crc)
/// изымается из буфера, но разбор не выполняется: буферы каналов и
/// статистики не изменяются.
SCENARIO(
    "Ядро парсера: короткий payload с валидным crc",
    "[stv][communication][crsf]")
{
    GIVEN("Кадр 0x16 с 21 байтом payload (на 1 меньше номинала)")
    {
        crsf_parser_fixture fixture{};

        // Предзаполняем буфер каналов валидным кадром, чтобы отличить
        // «каналы не изменились» от «каналы остались нулями».
        std::array<std::byte, stv::crsf_frame_size_max> frame_buffer{};
        const auto valid_frame_size = build_mid_rc_frame(frame_buffer);
        fixture.write(std::span{frame_buffer}.subspan(0U, valid_frame_size));
        REQUIRE(fixture.parser.processing() == 1);

        // Кадр собран вручную: тип 0x16, 21 байт payload, валидный crc.
        constexpr std::size_t payload_size{21U};
        frame_buffer[0U] = std::byte{0xC8};
        // frame length = type + payload + crc.
        frame_buffer[1U] = static_cast<std::byte>(payload_size + 2U);
        frame_buffer[2U] = std::byte{stv::crsf_type_rc_channels_packed};
        for(std::size_t index{3U}; index < payload_size + 3U; ++index)
        {
            frame_buffer.at(index) = std::byte{0x5A};
        }
        frame_buffer[payload_size + 3U] = std::byte{
            stv::crsf_crc8(
                std::span{frame_buffer}.subspan(2U, payload_size + 1U)),
        };

        constexpr std::size_t frame_size{payload_size + 4U};
        fixture.write(std::span{frame_buffer}.subspan(0U, frame_size));

        // crc валиден, кадр изымается из буфера и считается разобранным,
        // но распаковка не выполняется: буфер каналов не изменился.
        REQUIRE(fixture.parser.processing() == 1);
        REQUIRE(fixture.ringbuff.get_full() == 0);
        REQUIRE(std::ranges::all_of(
            fixture.parser.channels(), [](std::uint16_t value) {
                return value == stv::crsf_channel_value_mid;
            }));
    }

    GIVEN("Кадр 0x14 с 9 байтами payload (на 1 меньше номинала)")
    {
        crsf_parser_fixture fixture{};

        // Предзаполняем статистику валидным кадром.
        const stv::crsf_link_statistics expected{
            .up_rssi_ant1      = 85,
            .up_rssi_ant2      = 80,
            .up_link_quality   = 75,
            .up_snr            = -12,
            .active_antenna    = 0,
            .rf_profile        = 1,
            .up_rf_power       = 25,
            .down_rssi         = 70,
            .down_link_quality = 65,
            .down_snr          = -8,
        };

        std::array<std::byte, stv::crsf_frame_size_max> frame_buffer{};
        const auto                                      valid_frame_size =
            build_link_statistics_frame(frame_buffer, expected);
        fixture.write(std::span{frame_buffer}.subspan(0U, valid_frame_size));
        REQUIRE(fixture.parser.processing() == 1);

        // Кадр собран вручную: тип 0x14, 9 байт payload, валидный crc.
        constexpr std::size_t payload_size{9U};
        frame_buffer[0U] = std::byte{0xC8};
        // frame length = type + payload + crc.
        frame_buffer[1U] = static_cast<std::byte>(payload_size + 2U);
        frame_buffer[2U] = std::byte{stv::crsf_type_link_statistics};
        for(std::size_t index{3U}; index < payload_size + 3U; ++index)
        {
            frame_buffer.at(index) = std::byte{0xA5};
        }
        frame_buffer[payload_size + 3U] = std::byte{
            stv::crsf_crc8(
                std::span{frame_buffer}.subspan(2U, payload_size + 1U)),
        };

        constexpr std::size_t frame_size{payload_size + 4U};
        fixture.write(std::span{frame_buffer}.subspan(0U, frame_size));

        // crc валиден, кадр изымается из буфера и считается разобранным,
        // но разбор не выполняется: статистика не изменилась.
        REQUIRE(fixture.parser.processing() == 1);
        REQUIRE(fixture.ringbuff.get_full() == 0);

        const auto &actual = fixture.parser.link_statistics();
        REQUIRE(actual.up_rssi_ant1 == expected.up_rssi_ant1);
        REQUIRE(actual.up_rssi_ant2 == expected.up_rssi_ant2);
        REQUIRE(actual.up_link_quality == expected.up_link_quality);
        REQUIRE(actual.up_snr == expected.up_snr);
        REQUIRE(actual.active_antenna == expected.active_antenna);
        REQUIRE(actual.rf_profile == expected.rf_profile);
        REQUIRE(actual.up_rf_power == expected.up_rf_power);
        REQUIRE(actual.down_rssi == expected.down_rssi);
        REQUIRE(actual.down_link_quality == expected.down_link_quality);
        REQUIRE(actual.down_snr == expected.down_snr);
    }
}

/// @brief Повторный кадр 0x16 перезаписывает буфер новыми значениями.
SCENARIO(
    "Обработчик 0x16: повторный кадр перезаписывает буфер",
    "[stv][communication][crsf]")
{
    crsf_parser_fixture fixture{};

    crsf_test_channels  first{};
    first.fill(stv::crsf_channel_value_min);

    crsf_test_channels second{};
    second.fill(stv::crsf_channel_value_max);

    std::array<std::byte, stv::crsf_frame_size_max> frame_buffer{};

    auto frame_size = build_rc_channels_frame(frame_buffer, first);
    fixture.write(std::span{frame_buffer}.subspan(0U, frame_size));

    REQUIRE(fixture.parser.processing() == 1);
    REQUIRE(fixture.parser.channels() == first);

    frame_size = build_rc_channels_frame(frame_buffer, second);
    fixture.write(std::span{frame_buffer}.subspan(0U, frame_size));

    REQUIRE(fixture.parser.processing() == 1);
    REQUIRE(fixture.parser.channels() == second);
}

/// @brief До первого принятого кадра 0x14 статистика заполнена нулями.
SCENARIO(
    "Обработчик 0x14: статистика до первого кадра — нули",
    "[stv][communication][crsf]")
{
    const crsf_parser_fixture fixture{};

    const auto &link_statistics = fixture.parser.link_statistics();
    REQUIRE(link_statistics.up_rssi_ant1 == 0);
    REQUIRE(link_statistics.up_rssi_ant2 == 0);
    REQUIRE(link_statistics.up_link_quality == 0);
    REQUIRE(link_statistics.up_snr == 0);
    REQUIRE(link_statistics.active_antenna == 0);
    REQUIRE(link_statistics.rf_profile == 0);
    REQUIRE(link_statistics.up_rf_power == 0);
    REQUIRE(link_statistics.down_rssi == 0);
    REQUIRE(link_statistics.down_link_quality == 0);
    REQUIRE(link_statistics.down_snr == 0);
}

/// @brief Кадр 0x14 разбирается в структуру статистики; отрицательные snr
/// интерпретируются как знаковые.
SCENARIO(
    "Обработчик 0x14: разбор характерных значений",
    "[stv][communication][crsf]")
{
    crsf_parser_fixture             fixture{};

    const stv::crsf_link_statistics expected{
        .up_rssi_ant1      = 85,
        .up_rssi_ant2      = 80,
        .up_link_quality   = 99,
        .up_snr            = -12,
        .active_antenna    = 1,
        .rf_profile        = 2,
        .up_rf_power       = 25,
        .down_rssi         = 70,
        .down_link_quality = 65,
        .down_snr          = -8,
    };

    std::array<std::byte, stv::crsf_frame_size_max> frame_buffer{};
    const auto frame_size = build_link_statistics_frame(frame_buffer, expected);
    fixture.write(std::span{frame_buffer}.subspan(0U, frame_size));

    REQUIRE(fixture.parser.processing() == 1);

    const auto &actual = fixture.parser.link_statistics();
    REQUIRE(actual.up_rssi_ant1 == expected.up_rssi_ant1);
    REQUIRE(actual.up_rssi_ant2 == expected.up_rssi_ant2);
    REQUIRE(actual.up_link_quality == expected.up_link_quality);
    REQUIRE(actual.up_snr == -12);
    REQUIRE(actual.active_antenna == expected.active_antenna);
    REQUIRE(actual.rf_profile == expected.rf_profile);
    REQUIRE(actual.up_rf_power == expected.up_rf_power);
    REQUIRE(actual.down_rssi == expected.down_rssi);
    REQUIRE(actual.down_link_quality == expected.down_link_quality);
    REQUIRE(actual.down_snr == expected.down_snr);
}

/// @brief Повторный кадр 0x14 перезаписывает статистику новыми значениями.
SCENARIO(
    "Обработчик 0x14: повторный кадр перезаписывает статистику",
    "[stv][communication][crsf]")
{
    crsf_parser_fixture             fixture{};

    const stv::crsf_link_statistics first{
        .up_rssi_ant1      = 85,
        .up_rssi_ant2      = 80,
        .up_link_quality   = 99,
        .up_snr            = -12,
        .active_antenna    = 0,
        .rf_profile        = 1,
        .up_rf_power       = 25,
        .down_rssi         = 70,
        .down_link_quality = 65,
        .down_snr          = -8,
    };

    const stv::crsf_link_statistics second{
        .up_rssi_ant1      = 40,
        .up_rssi_ant2      = 35,
        .up_link_quality   = 55,
        .up_snr            = 7,
        .active_antenna    = 1,
        .rf_profile        = 2,
        .up_rf_power       = 10,
        .down_rssi         = 30,
        .down_link_quality = 25,
        .down_snr          = 3,
    };

    std::array<std::byte, stv::crsf_frame_size_max> frame_buffer{};

    auto frame_size = build_link_statistics_frame(frame_buffer, first);
    fixture.write(std::span{frame_buffer}.subspan(0U, frame_size));

    REQUIRE(fixture.parser.processing() == 1);
    REQUIRE(fixture.parser.link_statistics().up_rssi_ant1
            == first.up_rssi_ant1);
    REQUIRE(fixture.parser.link_statistics().up_link_quality
            == first.up_link_quality);
    REQUIRE(fixture.parser.link_statistics().up_snr == first.up_snr);

    frame_size = build_link_statistics_frame(frame_buffer, second);
    fixture.write(std::span{frame_buffer}.subspan(0U, frame_size));

    REQUIRE(fixture.parser.processing() == 1);

    const auto &actual = fixture.parser.link_statistics();
    REQUIRE(actual.up_rssi_ant1 == second.up_rssi_ant1);
    REQUIRE(actual.up_rssi_ant2 == second.up_rssi_ant2);
    REQUIRE(actual.up_link_quality == second.up_link_quality);
    REQUIRE(actual.up_snr == second.up_snr);
    REQUIRE(actual.active_antenna == second.active_antenna);
    REQUIRE(actual.rf_profile == second.rf_profile);
    REQUIRE(actual.up_rf_power == second.up_rf_power);
    REQUIRE(actual.down_rssi == second.down_rssi);
    REQUIRE(actual.down_link_quality == second.down_link_quality);
    REQUIRE(actual.down_snr == second.down_snr);
}

/// @brief Failsafe: свежий парсер без кадров переходит в failsafe после
/// истечения failsafe_deadline.
SCENARIO(
    "Failsafe: истечение deadline без кадров", "[stv][communication][crsf]")
{
    crsf_failsafe_fixture fixture{};

    REQUIRE_FALSE(fixture.parser.is_failsafe());

    // До истечения deadline failsafe не наступает.
    fixture.advance_time(std::chrono::milliseconds{999});
    REQUIRE(fixture.parser.processing() == 0);
    REQUIRE_FALSE(fixture.parser.is_failsafe());

    // После истечения deadline ближайший run() переводит парсер в failsafe.
    fixture.advance_time(std::chrono::milliseconds{2});
    REQUIRE(fixture.parser.processing() == 0);
    REQUIRE(fixture.parser.is_failsafe());
}

/// @brief Failsafe: валидный кадр 0x16 сбрасывает failsafe и перезапускает
/// deadline.
SCENARIO(
    "Failsafe: кадр 0x16 перезапускает deadline", "[stv][communication][crsf]")
{
    crsf_failsafe_fixture fixture{};

    // Доводим парсер до failsafe.
    fixture.advance_time(std::chrono::seconds{2});
    REQUIRE(fixture.parser.processing() == 0);
    REQUIRE(fixture.parser.is_failsafe());

    // Принятый кадр 0x16 снимает failsafe.
    std::array<std::byte, stv::crsf_frame_size_max> frame_buffer{};
    const auto frame_size = build_mid_rc_frame(frame_buffer);
    fixture.write(std::span{frame_buffer}.subspan(0U, frame_size));

    REQUIRE(fixture.parser.processing() == 1);
    REQUIRE_FALSE(fixture.parser.is_failsafe());

    // Deadline перезапущен: run() в пределах timeout не приводит к failsafe.
    fixture.advance_time(std::chrono::milliseconds{999});
    REQUIRE(fixture.parser.processing() == 0);
    REQUIRE_FALSE(fixture.parser.is_failsafe());
}

/// @brief Failsafe: истечение deadline после кадра 0x16 сбрасывает каналы в
/// нулевые значения (управление сброшено).
SCENARIO(
    "Failsafe: истечение deadline обнуляет каналы",
    "[stv][communication][crsf]")
{
    crsf_failsafe_fixture                           fixture{};

    std::array<std::byte, stv::crsf_frame_size_max> frame_buffer{};
    const auto frame_size = build_mid_rc_frame(frame_buffer);
    fixture.write(std::span{frame_buffer}.subspan(0U, frame_size));

    REQUIRE(fixture.parser.processing() == 1);
    REQUIRE_FALSE(fixture.parser.is_failsafe());
    REQUIRE(
        std::ranges::all_of(fixture.parser.channels(), [](std::uint16_t value) {
            return value == stv::crsf_channel_value_mid;
        }));

    // Время сдвинуто за timeout: failsafe, каналы сброшены в нули.
    fixture.advance_time(std::chrono::seconds{2});
    REQUIRE(fixture.parser.processing() == 0);
    REQUIRE(fixture.parser.is_failsafe());
    REQUIRE(
        std::ranges::all_of(fixture.parser.channels(),
                            [](std::uint16_t value) { return value == 0; }));
}

/// @brief Failsafe: первый валидный кадр 0x16 после failsafe восстанавливает
/// прием: failsafe снят, каналы соответствуют новому кадру.
SCENARIO(
    "Failsafe: восстановление приема кадром 0x16", "[stv][communication][crsf]")
{
    crsf_failsafe_fixture fixture{};

    // Доводим парсер до failsafe.
    fixture.advance_time(std::chrono::seconds{2});
    REQUIRE(fixture.parser.processing() == 0);
    REQUIRE(fixture.parser.is_failsafe());

    // Новый валидный кадр 0x16: failsafe снят, каналы обновлены.
    crsf_test_channels channels{};
    channels.fill(stv::crsf_channel_value_max);

    std::array<std::byte, stv::crsf_frame_size_max> frame_buffer{};
    const auto frame_size = build_rc_channels_frame(frame_buffer, channels);
    fixture.write(std::span{frame_buffer}.subspan(0U, frame_size));

    REQUIRE(fixture.parser.processing() == 1);
    REQUIRE_FALSE(fixture.parser.is_failsafe());
    REQUIRE(fixture.parser.channels() == channels);
}

/// @brief Failsafe: кадр 0x14 (link statistics, не управляющий) не
/// перезапускает failsafe deadline.
SCENARIO(
    "Failsafe: кадр 0x14 не перезапускает deadline",
    "[stv][communication][crsf]")
{
    crsf_failsafe_fixture fixture{};

    // Половина timeout прошла: кадр 0x14 разобран, failsafe еще не наступил.
    fixture.advance_time(std::chrono::milliseconds{500});

    std::array<std::byte, stv::crsf_frame_size_max> frame_buffer{};
    const auto                                      frame_size =
        build_link_statistics_frame(frame_buffer, stv::crsf_link_statistics{});
    fixture.write(std::span{frame_buffer}.subspan(0U, frame_size));

    REQUIRE(fixture.parser.processing() == 1);
    REQUIRE_FALSE(fixture.parser.is_failsafe());

    // С момента старта deadline прошло больше timeout. Если бы кадр 0x14
    // перезапускал deadline, failsafe бы не наступил: с момента приема кадра
    // прошло лишь 600 мс.
    fixture.advance_time(std::chrono::milliseconds{600});
    REQUIRE(fixture.parser.processing() == 0);
    REQUIRE(fixture.parser.is_failsafe());
}

// NOLINTEND(*-magic-numbers, readability-function-cognitive-complexity)
