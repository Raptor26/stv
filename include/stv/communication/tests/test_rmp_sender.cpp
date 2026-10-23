/// @file test_rmp_sender.cpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.

#include "stv/communication/rmp_parser.hpp"
#include "stv/communication/rmp_sender.hpp"
#include "stv/communication/serial_decorators.hpp"
#include "stv/communication/serial_parser.hpp"
#include "stv/communication/serial_sender.hpp"
#include "stv/containers/lwrb.hpp"
#include "stv/containers/simbuff.hpp"
#include <array>
#include <catch2/catch_test_macros.hpp>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <etl/queue.h>
#include <string>
#include <vector>

// NOLINTBEGIN(*-magic-numbers)

// Catch2 SECTION-макросы резко завышают когнитивную сложность теста;
// разбивать единый тестовый сценарий ради метрики нецелесообразно.
// NOLINTBEGIN(readability-function-cognitive-complexity)

namespace {

using frame_type    = stv::rmp_parser<>;
using frame_tx_type = stv::rmp_sender<>;

using sim_buffer_type = stv::sim_buff<stv::empty_mutex>;
using queue_type      = etl::queue<sim_buffer_type, 10>;

/// @brief Полезная нагрузка эталонного кадра JoyCommand из тестов
///     библиотеки radio_message_parser.
constexpr std::array<std::byte, 16> joy_payload{
    std::byte{0x05}, std::byte{0x00}, std::byte{0x00}, std::byte{0x00},
    std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{0x00},
    std::byte{0x04}, std::byte{0x00}, std::byte{0x00}, std::byte{0x00},
    std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{0x23},
};

/// @brief Эталонный кадр JoyCommand целиком (со стартовой
///     последовательностью и CRC-16/CCITT = 0x5990, little-endian).
constexpr std::array<std::byte, 20> joy_command_frame{
    std::byte{0xAA}, std::byte{0x55}, std::byte{0x05}, std::byte{0x00},
    std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{0x00},
    std::byte{0x00}, std::byte{0x00}, std::byte{0x04}, std::byte{0x00},
    std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{0x00},
    std::byte{0x00}, std::byte{0x23}, std::byte{0x90}, std::byte{0x59},
};

/// @brief Проверяет, что сообщение в голове очереди парсера совпадает с
///     ожидаемым, и удаляет его из очереди.
template<typename TQueue>
void require_front_equals(
    TQueue &queue, const std::vector<std::byte> &expected)
{
    REQUIRE(!queue.empty());

    const auto msg = std::move(queue.front());
    REQUIRE(msg.size_bytes() == expected.size());
    REQUIRE(std::memcmp(msg.template data<std::byte>(), expected.data(),
                        expected.size())
            == 0);
    queue.pop();
}

} // namespace

TEST_CASE(
    "rmp_sender", "[stv][communication]")
{
    SECTION("frame layout matches golden vector")
    {
        queue_type queue;
        auto       serial_message_buffer =
            stv::make_serial_message_buffer(queue, frame_tx_type{});

        STATIC_REQUIRE(frame_tx_type::header_size() == 2U);
        STATIC_REQUIRE(frame_tx_type::trailer_size() == 2U);
        STATIC_REQUIRE(frame_tx_type::max_pload_size() == 16U);

        {
            auto msg = serial_message_buffer.request(joy_payload);
            REQUIRE(msg);
        }

        REQUIRE(queue.size() == 1U);

        const auto &frame = queue.front();
        // Заголовок(2) + payload(16) + CRC(2).
        REQUIRE(frame.size_bytes() == 20U);

        const auto *bytes = frame.data<std::byte>();

        REQUIRE(bytes[0U] == std::byte{0xAA}); // стартовая последовательность
        REQUIRE(bytes[1U] == std::byte{0x55});

        // Полезная нагрузка расположена между заголовком и CRC.
        REQUIRE(std::memcmp(bytes + 2U, joy_payload.data(), joy_payload.size())
                == 0);

        // CRC-16/CCITT по полезной нагрузке = 0x5990, little-endian.
        REQUIRE(bytes[18U] == std::byte{0x90});
        REQUIRE(bytes[19U] == std::byte{0x59});

        // Кадр побайтово совпадает с эталонным кадром JoyCommand из
        // библиотеки radio_message_parser.
        REQUIRE(std::memcmp(bytes, joy_command_frame.data(),
                            joy_command_frame.size())
                == 0);
    }

    SECTION("short payload is padded with zeros to fixed frame size")
    {
        queue_type queue;
        auto       serial_message_buffer =
            stv::make_serial_message_buffer(queue, frame_tx_type{});

        constexpr std::array<std::byte, 3> short_payload{
            std::byte{0xDE},
            std::byte{0xAD},
            std::byte{0xBE},
        };

        {
            auto msg = serial_message_buffer.request(short_payload);
            REQUIRE(msg);
        }

        REQUIRE(queue.size() == 1U);

        const auto &frame = queue.front();
        // Кадр на проводе всегда имеет фиксированный размер: заголовок(2) +
        // payload(16) + CRC(2), независимо от длины запрошенной нагрузки.
        REQUIRE(frame.size_bytes() == 20U);

        const auto *bytes = frame.data<std::byte>();

        REQUIRE(bytes[0U] == std::byte{0xAA});
        REQUIRE(bytes[1U] == std::byte{0x55});

        // Запрошенные данные расположены в начале полезной нагрузки.
        REQUIRE(bytes[2U] == std::byte{0xDE});
        REQUIRE(bytes[3U] == std::byte{0xAD});
        REQUIRE(bytes[4U] == std::byte{0xBE});

        // Недостающее до полного размера кадра дополнено нулями.
        for(std::size_t i{5U}; i < 18U; ++i)
        {
            REQUIRE(bytes[i] == std::byte{0x00});
        }

        // CRC-16/CCITT считается по ВСЕМ 16 байтам полезной нагрузки,
        // включая нулевое дополнение: {DE AD BE + 13 нулей} -> 0x2B20,
        // little-endian.
        REQUIRE(bytes[18U] == std::byte{0x20});
        REQUIRE(bytes[19U] == std::byte{0x2B});

        const stv::total_message_span total{
            frame.data<std::byte>(),
            frame.size_bytes(),
        };
        REQUIRE(frame_type::is_crc_valid(total));
    }

    SECTION("empty payload is padded to full frame size")
    {
        // Пустая полезная нагрузка допустима: кадр по-прежнему имеет
        // фиксированный размер, вся полезная нагрузка -- нулевое
        // дополнение.
        queue_type queue;
        auto       serial_message_buffer =
            stv::make_serial_message_buffer(queue, frame_tx_type{});

        {
            auto msg = serial_message_buffer.request(0U);
            REQUIRE(msg);
        }

        REQUIRE(queue.size() == 1U);

        const auto &frame = queue.front();
        // Заголовок(2) + payload(16) + CRC(2).
        REQUIRE(frame.size_bytes() == 20U);

        const auto *bytes = frame.data<std::byte>();

        REQUIRE(bytes[0U] == std::byte{0xAA});
        REQUIRE(bytes[1U] == std::byte{0x55});

        for(std::size_t i{2U}; i < 18U; ++i)
        {
            REQUIRE(bytes[i] == std::byte{0x00});
        }

        // CRC-16/CCITT по 16 нулевым байтам = 0x6A0A, little-endian.
        REQUIRE(bytes[18U] == std::byte{0x0A});
        REQUIRE(bytes[19U] == std::byte{0x6A});

        const stv::total_message_span total{
            frame.data<std::byte>(),
            frame.size_bytes(),
        };
        REQUIRE(frame_type::is_crc_valid(total));
    }

    SECTION("max payload boundary")
    {
        queue_type queue;
        auto       serial_message_buffer =
            stv::make_serial_message_buffer(queue, frame_tx_type{});

        // Полезная нагрузка максимального размера принимается.
        const std::string max_payload(16U, 'A');
        {
            auto msg = serial_message_buffer.request(max_payload);
            REQUIRE(msg);
        }
        REQUIRE(queue.size() == 1U);

        // Полезная нагрузка на один байт больше максимума отклоняется:
        // request() возвращает невалидное сообщение вместо формирования
        // кадра с усечённой нагрузкой.
        const std::string oversized_payload(17U, 'A');
        {
            const auto msg = serial_message_buffer.request(oversized_payload);
            REQUIRE(!msg);
        }
        REQUIRE(queue.size() == 1U);
    }

    SECTION("tx frame passes rmp_parser::is_crc_valid")
    {
        queue_type queue;
        auto       serial_message_buffer =
            stv::make_serial_message_buffer(queue, frame_tx_type{});

        {
            auto msg = serial_message_buffer.request(joy_payload);
            REQUIRE(msg);
        }

        REQUIRE(queue.size() == 1U);

        const auto                   &frame = queue.front();
        const stv::total_message_span total{
            frame.data<std::byte>(),
            frame.size_bytes(),
        };

        // Сквозной тест TX→parser: кадр, собранный rmp_sender, проходит
        // проверку CRC парсерным декоратором rmp_parser.
        REQUIRE(frame_type::is_crc_valid(total));

        // Полный размер кадра, вычисленный по peek-нутому заголовку,
        // совпадает с реальным размером кадра.
        const stv::total_message_span header_span{frame.data<std::byte>(),
                                                  frame_type::header_size()};
        REQUIRE(frame_type::total_frame_size(header_span)
                == frame.size_bytes());
    }

    SECTION("custom frame size")
    {
        queue_type queue;
        auto       serial_message_buffer =
            stv::make_serial_message_buffer(queue, stv::rmp_sender<8>{});

        STATIC_REQUIRE(stv::rmp_sender<8>::max_pload_size() == 4U);

        constexpr std::array<std::byte, 4> payload{
            std::byte{0xDE},
            std::byte{0xAD},
            std::byte{0xBE},
            std::byte{0xEF},
        };

        {
            auto msg = serial_message_buffer.request(payload);
            REQUIRE(msg);
        }

        REQUIRE(queue.size() == 1U);

        const auto &frame = queue.front();
        // Заголовок(2) + payload(4) + CRC(2).
        REQUIRE(frame.size_bytes() == 8U);

        const stv::total_message_span total{
            frame.data<std::byte>(),
            frame.size_bytes(),
        };
        REQUIRE(stv::rmp_parser<8>::is_crc_valid(total));

        // CRC-16/CCITT по {0xDE, 0xAD, 0xBE, 0xEF} = 0x4097, little-endian.
        const auto *bytes = frame.data<std::byte>();
        REQUIRE(bytes[6U] == std::byte{0x97});
        REQUIRE(bytes[7U] == std::byte{0x40});

        // Короткая нагрузка дополняется нулями до фиксированного размера
        // кадра в 8 байт.
        constexpr std::array<std::byte, 2> short_payload{
            std::byte{0x01},
            std::byte{0x02},
        };

        {
            auto msg = serial_message_buffer.request(short_payload);
            REQUIRE(msg);
        }

        REQUIRE(queue.size() == 2U);
        queue.pop();

        const auto &padded_frame = queue.front();
        REQUIRE(padded_frame.size_bytes() == 8U);

        const auto *padded_bytes = padded_frame.data<std::byte>();
        REQUIRE(padded_bytes[2U] == std::byte{0x01});
        REQUIRE(padded_bytes[3U] == std::byte{0x02});
        REQUIRE(padded_bytes[4U] == std::byte{0x00});
        REQUIRE(padded_bytes[5U] == std::byte{0x00});

        // CRC-16/CCITT по {01 02 00 00} = 0x9C14, little-endian.
        REQUIRE(padded_bytes[6U] == std::byte{0x14});
        REQUIRE(padded_bytes[7U] == std::byte{0x9C});
    }

    SECTION("tx to parser loopback")
    {
        queue_type tx_queue;
        auto       serial_message_buffer =
            stv::make_serial_message_buffer(tx_queue, frame_tx_type{});

        {
            auto msg = serial_message_buffer.request(joy_payload);
            REQUIRE(msg);
        }

        REQUIRE(tx_queue.size() == 1U);

        // Кадр, собранный отправителем, побайтово передаётся в парсер.
        const auto                  &frame = tx_queue.front();
        const std::vector<std::byte> wire_bytes{frame.data<std::byte>(),
                                                frame.data<std::byte>()
                                                    + frame.size_bytes()};

        using lwrb_setup_type = stv::lwrb_setup<stv::empty_mutex>;
        using lwrb_base_type  = stv::lwrb_base<lwrb_setup_type>;

        constexpr std::size_t                       lwrb_buffer_size{64};
        stv::lwrb<lwrb_base_type, lwrb_buffer_size> lwrb{lwrb_setup_type{}};
        queue_type                                  parsed_queue;

        using parser_setup_type =
            stv::serial_parser_setup<lwrb_base_type, queue_type,
                                     stv::empty_mutex, frame_type>;

        parser_setup_type parser_setup;
        parser_setup.lwrb = &lwrb;
        parser_setup.set_queue<frame_type>(parsed_queue);

        auto parser = stv::make_serial_parser<parser_setup_type, frame_type>(
            parser_setup);
        REQUIRE(parser);

        lwrb.write(wire_bytes);

        REQUIRE(parser.run());
        REQUIRE(parsed_queue.size() == 1U);

        // Парсер отрезает заголовок и хвост: получатель читает исходную
        // полезную нагрузку.
        require_front_equals(parsed_queue,
                             {joy_payload.begin(), joy_payload.end()});
    }
}

// NOLINTEND(readability-function-cognitive-complexity)

// NOLINTEND(*-magic-numbers)
