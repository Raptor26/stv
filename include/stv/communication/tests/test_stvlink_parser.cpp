/// @file test_stvlink_parser.cpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.

#include "stv/communication/parsed_queue.hpp"
#include "stv/communication/serial_decorators.hpp"
#include "stv/communication/serial_sender.hpp"
#include "stv/communication/stvlink_parser.hpp"
#include "stv/communication/stvlink_sender.hpp"
#include "stv/containers/simbuff.hpp"
#include <array>
#include <catch2/catch_test_macros.hpp>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <etl/queue.h>
#include <string_view>
#include <type_traits>
#include <vector>

// NOLINTBEGIN(*-magic-numbers)

TEST_CASE(
    "stvlink_parser", "[stv][communication]")
{
    using sim_buffer_type = stv::sim_buff<stv::empty_mutex>;
    using queue_type      = etl::queue<sim_buffer_type, 10>;
    queue_type tx_queue;

    SECTION("start bytes")
    {
        REQUIRE(stv::stvlink_parser::first_byte == std::byte{0xAA});
        REQUIRE(stv::stvlink_parser::second_byte == std::byte{0xAA});
    }

    SECTION("header and trailer sizes")
    {
        REQUIRE(stv::stvlink_parser::header_size() == 4U);
        REQUIRE(stv::stvlink_parser::trailer_size() == 2U);
    }

    SECTION("frame size extraction")
    {
        constexpr std::string_view payload{"Hello world"};

        auto                       serial_message_buffer =
            stv::make_serial_message_buffer(tx_queue, stv::stvlink_sender{});

        {
            const auto msg = serial_message_buffer.request(payload);
            (void)msg;
        }

        REQUIRE(!tx_queue.empty());
        const auto                   &frame = tx_queue.front();

        const stv::total_message_span header_span{
            frame.data<std::byte>(), stv::stvlink_parser::header_size()};

        REQUIRE(stv::stvlink_parser::total_frame_size(header_span)
                == frame.size_bytes());
    }

    SECTION("crc validation")
    {
        constexpr std::string_view payload{"Hello world"};

        auto                       serial_message_buffer =
            stv::make_serial_message_buffer(tx_queue, stv::stvlink_sender{});

        {
            const auto msg = serial_message_buffer.request(payload);
            (void)msg;
        }

        REQUIRE(!tx_queue.empty());
        const auto                   &frame = tx_queue.front();

        const stv::total_message_span total{frame.data<std::byte>(),
                                            frame.size_bytes()};

        REQUIRE(stv::stvlink_parser::is_crc_valid(total));

        std::vector<std::byte> corrupted_frame{frame.data<std::byte>(),
                                               frame.data<std::byte>()
                                                   + frame.size_bytes()};

        REQUIRE(corrupted_frame.size() > 4U);
        corrupted_frame[4U] = ~corrupted_frame[4U];

        const stv::total_message_span corrupted_total{corrupted_frame.data(),
                                                      corrupted_frame.size()};

        REQUIRE_FALSE(stv::stvlink_parser::is_crc_valid(corrupted_total));
    }

    SECTION("parsed_queue tag")
    {
        queue_type                                   queue;
        const stv::parsed_queue<stv::stvlink_parser> stv_queue_tag{queue};

        REQUIRE(stv_queue_tag.queue() == &queue);

        using pq_stv_type   = decltype(stv_queue_tag);
        using pq_other_type = stv::parsed_queue<int>;

        REQUIRE_FALSE((std::is_same_v<pq_stv_type, pq_other_type>));
    }
}

/// Сторож обратной совместимости бинарного протокола stvlink.
///
/// Эталонные последовательности байт (golden vectors) зафиксированы по
/// формату кадра, действовавшему до рефакторинга декораторов
/// (start_frame_and_crc_16 -> stvlink_sender/stvlink_parser,
/// head_route -> stvlink_route_tx), и независимо проверены эталонной
/// реализацией CRC-16/MODBUS (init 0xFFFF, полином 0xA001, контрольное
/// значение для "123456789" == 0x4B37). После объединения передающих
/// декораторов (stvlink_route_tx поглощён stvlink_sender) раскладка байт
/// на проводе не изменилась: секция «заголовок сообщения» собирается
/// объединённым декоратором и побайтово совпадает с эталоном, зафиксированным
/// до рефакторинга. Секции «только полезная нагрузка» и «пустая полезная
/// нагрузка» собираются тем же объединённым декоратором с параметрами по
/// умолчанию (dst_id = 0, msg_id = 0): заголовок сообщения теперь
/// присутствует в каждом кадре, поэтому их эталоны пересчитаны для формата
/// «старотовый кадр | заголовок сообщения | полезная нагрузка | CRC».
///
/// Падение этого теста означает, что изменился БИНАРНЫЙ формат кадра
/// stvlink: устройства со старой прошивкой и обновлённый конфигуратор
/// (и наоборот) перестанут понимать друг друга. Тест не чинить
/// «под новый формат» — сначала убедиться, что изменение протокола
/// осознанно и согласовано со всеми сторонами обмена.
TEST_CASE(
    "stvlink_parser golden vectors",
    "[stv][communication][golden][backward-compatibility]")
{
    using sim_buffer_type = stv::sim_buff<stv::empty_mutex>;
    using queue_type      = etl::queue<sim_buffer_type, 10>;
    queue_type tx_queue;

    SECTION("payload only: 0x01..0x05, default message header")
    {
        // 0xAA 0xAA | frame_size=11 (заголовок сообщения + payload + CRC) |
        // dst=0 | msg=0 | pload_size=5 | payload | CRC-16
        constexpr std::array golden{
            std::byte{0xAA}, std::byte{0xAA}, std::byte{0x0B}, std::byte{0x00},
            std::byte{0x00}, std::byte{0x00}, std::byte{0x05}, std::byte{0x00},
            std::byte{0x01}, std::byte{0x02}, std::byte{0x03}, std::byte{0x04},
            std::byte{0x05}, std::byte{0x7E}, std::byte{0x06},
        };

        auto serial_message_buffer =
            stv::make_serial_message_buffer(tx_queue, stv::stvlink_sender{});

        {
            constexpr std::array payload{
                std::byte{0x01}, std::byte{0x02}, std::byte{0x03},
                std::byte{0x04}, std::byte{0x05},
            };
            const auto msg = serial_message_buffer.request(payload);
            (void)msg;
        }

        REQUIRE(!tx_queue.empty());
        const auto &frame = tx_queue.front();

        REQUIRE(frame.size_bytes() == golden.size());
        REQUIRE(
            std::memcmp(frame.data<std::byte>(), golden.data(), golden.size())
            == 0);
    }

    SECTION("empty payload")
    {
        // 0xAA 0xAA | frame_size=6 (заголовок сообщения + CRC) |
        // dst=0 | msg=0 | pload_size=0 | CRC-16
        constexpr std::array golden{
            std::byte{0xAA}, std::byte{0xAA}, std::byte{0x06}, std::byte{0x00},
            std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{0x00},
            std::byte{0xC0}, std::byte{0x60},
        };

        auto serial_message_buffer =
            stv::make_serial_message_buffer(tx_queue, stv::stvlink_sender{});

        {
            constexpr std::array<std::byte, 0> payload{};
            const auto msg = serial_message_buffer.request(payload);
            (void)msg;
        }

        REQUIRE(!tx_queue.empty());
        const auto &frame = tx_queue.front();

        REQUIRE(frame.size_bytes() == golden.size());
        REQUIRE(
            std::memcmp(frame.data<std::byte>(), golden.data(), golden.size())
            == 0);
    }

    SECTION("message header: dst_id=1, msg_id=7, payload 5 bytes")
    {
        // 0xAA 0xAA | frame_size=11 (заголовок сообщения + payload + CRC) |
        // dst=1 | msg=7 | pload_size=5 | payload | CRC-16
        constexpr std::array golden{
            std::byte{0xAA}, std::byte{0xAA}, std::byte{0x0B}, std::byte{0x00},
            std::byte{0x01}, std::byte{0x07}, std::byte{0x05}, std::byte{0x00},
            std::byte{0xDE}, std::byte{0xAD}, std::byte{0xBE}, std::byte{0xEF},
            std::byte{0x42}, std::byte{0x18}, std::byte{0x51},
        };

        auto serial_message_buffer =
            stv::make_serial_message_buffer(tx_queue, stv::stvlink_sender{});

        {
            constexpr std::array payload{
                std::byte{0xDE}, std::byte{0xAD}, std::byte{0xBE},
                std::byte{0xEF}, std::byte{0x42},
            };
            const auto msg = serial_message_buffer.request(
                payload,
                stv::stvlink_sender::setup_t{.dst_id = 1, .msg_id = 7});
            (void)msg;
        }

        REQUIRE(!tx_queue.empty());
        const auto &frame = tx_queue.front();

        REQUIRE(frame.size_bytes() == golden.size());
        REQUIRE(
            std::memcmp(frame.data<std::byte>(), golden.data(), golden.size())
            == 0);
    }

    SECTION("parse side accepts golden bytes")
    {
        // Кадр, сформированный старой прошивкой/конфигуратором, обязан
        // распознаваться и проходить проверку CRC.
        constexpr std::array golden{
            std::byte{0xAA}, std::byte{0xAA}, std::byte{0x0B}, std::byte{0x00},
            std::byte{0x01}, std::byte{0x07}, std::byte{0x05}, std::byte{0x00},
            std::byte{0xDE}, std::byte{0xAD}, std::byte{0xBE}, std::byte{0xEF},
            std::byte{0x42}, std::byte{0x18}, std::byte{0x51},
        };

        REQUIRE(stv::stvlink_parser::matches(golden[0], golden[1]));

        const stv::total_message_span header_span{
            golden.data(), stv::stvlink_parser::header_size()};
        REQUIRE(stv::stvlink_parser::total_frame_size(header_span)
                == golden.size());

        const stv::total_message_span total{golden.data(), golden.size()};
        REQUIRE(stv::stvlink_parser::is_crc_valid(total));

        std::array corrupted{golden};
        corrupted[8] = ~corrupted[8];
        const stv::total_message_span corrupted_total{corrupted.data(),
                                                      corrupted.size()};
        REQUIRE_FALSE(stv::stvlink_parser::is_crc_valid(corrupted_total));
    }
}

// NOLINTEND(*-magic-numbers)
