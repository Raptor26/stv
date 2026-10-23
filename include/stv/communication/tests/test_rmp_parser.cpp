/// @file test_rmp_parser.cpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.

#include "stv/communication/crc16.hpp"
#include "stv/communication/rmp_parser.hpp"
#include "stv/communication/rmp_sender.hpp"
#include "stv/communication/serial_decorators.hpp"
#include "stv/communication/serial_parser.hpp"
#include "stv/containers/lwrb.hpp"
#include "stv/containers/simbuff.hpp"
#include <array>
#include <catch2/catch_test_macros.hpp>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <etl/queue.h>
#include <span>
#include <vector>

// NOLINTBEGIN(*-magic-numbers)

// Catch2 SECTION-макросы резко завышают когнитивную сложность теста;
// разбивать единый тестовый сценарий ради метрики нецелесообразно.
// NOLINTBEGIN(readability-function-cognitive-complexity)

namespace {

using frame_type = stv::rmp_parser<>;

/// @brief Эталонный кадр из теста GetCrcByReferencePack библиотеки
///     radio_message_parser: CRC-16/CCITT по полезной нагрузке = 0x20B8.
constexpr std::array<std::byte, 20> reference_pack_frame{
    std::byte{0xAA}, std::byte{0x55}, std::byte{0x05}, std::byte{0x00},
    std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{0x00},
    std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{0x00},
    std::byte{0x00}, std::byte{0x00}, std::byte{0x07}, std::byte{0xFF},
    std::byte{0xFA}, std::byte{0x00}, std::byte{0xB8}, std::byte{0x20},
};

/// @brief Эталонный кадр JoyCommand из тестов библиотеки
///     radio_message_parser: CRC-16/CCITT по полезной нагрузке = 0x5990.
constexpr std::array<std::byte, 20> joy_command_frame{
    std::byte{0xAA}, std::byte{0x55}, std::byte{0x05}, std::byte{0x00},
    std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{0x00},
    std::byte{0x00}, std::byte{0x00}, std::byte{0x04}, std::byte{0x00},
    std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{0x00},
    std::byte{0x00}, std::byte{0x23}, std::byte{0x90}, std::byte{0x59},
};

/// @brief Дополняет полезную нагрузку нулями до размера кадра 20 байт.
auto padded_payload(
    const std::vector<std::byte> &payload) -> std::vector<std::byte>
{
    REQUIRE(payload.size() <= 16U);

    std::vector<std::byte> result = payload;
    result.resize(16U, std::byte{0x00});
    return result;
}

/// @brief Собирает валидный кадр RMP вокруг полезной нагрузки.
///
/// @details Кадр RMP имеет фиксированную длину, поэтому полезная
///     нагрузка дополняется нулями до 16 байт.
///
/// @param[in] payload Полезная нагрузка (не более 16 байт для кадра
///     размером 20).
/// @return Кадр со стартовой последовательностью 0xAA 0x55 и CRC-16/CCITT
///     (little-endian) в хвосте.
auto make_frame(
    const std::vector<std::byte> &payload) -> std::vector<std::byte>
{
    const auto             full_payload = padded_payload(payload);

    std::vector<std::byte> frame;
    frame.reserve(full_payload.size() + frame_type::header_size()
                  + frame_type::trailer_size());
    frame.push_back(frame_type::first_byte);
    frame.push_back(frame_type::second_byte);
    frame.insert(frame.end(), full_payload.begin(), full_payload.end());

    const auto crc = stv::crc16_ccitt(full_payload.data(), full_payload.size());
    frame.push_back(static_cast<std::byte>(crc & 0xFFU));
    frame.push_back(static_cast<std::byte>(
        (static_cast<std::uint32_t>(crc) >> 8U) & 0xFFU));

    return frame;
}

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
    "rmp_parser", "[stv][communication]")
{
    SECTION("start bytes and matches")
    {
        STATIC_REQUIRE(frame_type::first_byte == std::byte{0xAA});
        STATIC_REQUIRE(frame_type::second_byte == std::byte{0x55});

        STATIC_REQUIRE(frame_type::matches(std::byte{0xAA}, std::byte{0x55}));
        STATIC_REQUIRE_FALSE(
            frame_type::matches(std::byte{0xAA}, std::byte{0xAA}));
        STATIC_REQUIRE_FALSE(
            frame_type::matches(std::byte{0x55}, std::byte{0x55}));
        STATIC_REQUIRE_FALSE(
            frame_type::matches(std::byte{0x55}, std::byte{0xAA}));
        STATIC_REQUIRE_FALSE(
            frame_type::matches(std::byte{0x00}, std::byte{0xAA}));
    }

    SECTION("header and trailer sizes")
    {
        STATIC_REQUIRE(frame_type::header_size() == 2U);
        STATIC_REQUIRE(frame_type::trailer_size() == 2U);

        // Размер кадра настраивается шаблонным параметром.
        STATIC_REQUIRE(stv::rmp_parser<4>::header_size() == 2U);
        STATIC_REQUIRE(stv::rmp_parser<4>::trailer_size() == 2U);
        STATIC_REQUIRE(stv::rmp_parser<32>::trailer_size() == 2U);
    }

    SECTION("total frame size is fixed")
    {
        // Кадр фиксированной длины: размер не читается из заголовка, а
        // возвращается как шаблонный параметр.
        const std::array<std::byte, 2> header{
            std::byte{0xAA},
            std::byte{0x55},
        };
        const stv::total_message_span header_span{header.data(), header.size()};
        REQUIRE(frame_type::total_frame_size(header_span) == 20U);

        const stv::total_message_span small_header_span{header.data(), 2U};
        REQUIRE(stv::rmp_parser<4>::total_frame_size(small_header_span) == 4U);
        REQUIRE(stv::rmp_parser<32>::total_frame_size(small_header_span)
                == 32U);
    }

    SECTION("crc16_ccitt reference value")
    {
        // Контрольное значение CRC-16/CCITT-FALSE для "123456789" — 0x29B1.
        constexpr std::array<std::byte, 9> check_data{
            std::byte{0x31}, std::byte{0x32}, std::byte{0x33},
            std::byte{0x34}, std::byte{0x35}, std::byte{0x36},
            std::byte{0x37}, std::byte{0x38}, std::byte{0x39},
        };
        STATIC_REQUIRE(stv::crc16_ccitt(check_data.data(), check_data.size())
                       == 0x29B1U);

        // CRC пустой полезной нагрузки равна начальному значению 0xFFFF.
        STATIC_REQUIRE(stv::crc16_ccitt(nullptr, 0U) == 0xFFFFU);
    }

    SECTION("golden vector from GetCrcByReferencePack")
    {
        // CRC эталонного кадра сверяется со значением эталонной C-библиотеки
        // radio_message_parser (тест GetCrcByReferencePack).
        STATIC_REQUIRE(stv::crc16_ccitt(reference_pack_frame.data() + 2U, 16U)
                       == 0x20B8U);

        const stv::total_message_span total{reference_pack_frame.data(),
                                            reference_pack_frame.size()};
        REQUIRE(frame_type::is_crc_valid(total));
    }

    SECTION("golden vector with corrupted tail")
    {
        // Эталонный кадр с испорченным последним байтом CRC
        // (тест IsCrcValidInvalidCrc).
        auto corrupted_frame    = reference_pack_frame;
        corrupted_frame.back() ^= std::byte{0xFF};

        const stv::total_message_span total{corrupted_frame.data(),
                                            corrupted_frame.size()};
        REQUIRE_FALSE(frame_type::is_crc_valid(total));
    }

    SECTION("golden vector JoyCommand")
    {
        // Эталонный кадр JoyCommand: CRC-16/CCITT по полезной нагрузке
        // (последний её байт — 0x23) равна 0x5990.
        STATIC_REQUIRE(stv::crc16_ccitt(joy_command_frame.data() + 2U, 16U)
                       == 0x5990U);

        const stv::total_message_span total{joy_command_frame.data(),
                                            joy_command_frame.size()};
        REQUIRE(frame_type::is_crc_valid(total));

        // Полный размер кадра по peek-нутому заголовку совпадает с
        // реальным размером эталонного кадра.
        const stv::total_message_span header_span{joy_command_frame.data(),
                                                  frame_type::header_size()};
        REQUIRE(frame_type::total_frame_size(header_span)
                == joy_command_frame.size());
    }

    SECTION("frame shorter than fixed size is invalid")
    {
        // Кадр короче TMessageSize не должен приводить к чтению за
        // пределами буфера.
        const std::array<std::byte, 8> short_frame{
            std::byte{0xAA}, std::byte{0x55}, std::byte{0x05}, std::byte{0x00},
            std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{0x00},
        };
        const stv::total_message_span total{short_frame.data(),
                                            short_frame.size()};
        REQUIRE_FALSE(frame_type::is_crc_valid(total));
    }

    SECTION("minimal 4-byte frame with empty payload")
    {
        // Минимальный кадр: только стартовая последовательность и CRC
        // пустой полезной нагрузки (0xFFFF, little-endian).
        const std::array<std::byte, 4> minimal_frame{
            std::byte{0xAA},
            std::byte{0x55},
            std::byte{0xFF},
            std::byte{0xFF},
        };
        const stv::total_message_span total{minimal_frame.data(),
                                            minimal_frame.size()};
        REQUIRE(stv::rmp_parser<4>::is_crc_valid(total));
    }
}

TEST_CASE(
    "rmp_parser through serial_parser", "[stv][communication]")
{
    using sim_buffer_type = stv::sim_buff<stv::empty_mutex>;
    using queue_type      = etl::queue<sim_buffer_type, 10>;
    using lwrb_setup_type = stv::lwrb_setup<stv::empty_mutex>;
    using lwrb_base_type  = stv::lwrb_base<lwrb_setup_type>;

    constexpr std::size_t                       lwrb_buffer_size{256};
    stv::lwrb<lwrb_base_type, lwrb_buffer_size> lwrb{lwrb_setup_type{}};
    queue_type                                  parsed_queue;

    using parser_setup_type =
        stv::serial_parser_setup<lwrb_base_type, queue_type, stv::empty_mutex,
                                 frame_type>;

    parser_setup_type setup;
    setup.lwrb = &lwrb;
    setup.set_queue<frame_type>(parsed_queue);

    auto parser = stv::make_serial_parser<parser_setup_type, frame_type>(setup);
    REQUIRE(parser);

    const std::vector<std::byte> joy_payload{
        std::byte{0x05}, std::byte{0x00}, std::byte{0x00}, std::byte{0x00},
        std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{0x00},
        std::byte{0x04}, std::byte{0x00}, std::byte{0x00}, std::byte{0x00},
        std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{0x23},
    };

    SECTION("whole frame")
    {
        lwrb.write(joy_command_frame);

        REQUIRE(parser.run());
        REQUIRE(parsed_queue.size() == 1U);
        require_front_equals(parsed_queue, joy_payload);
    }

    SECTION("frame arriving byte by byte")
    {
        // Побайтная запись с периодическим вызовом run(): кадр выдаётся
        // только после прихода последнего байта.
        for(const std::byte byte: joy_command_frame)
        {
            lwrb.write(&byte, 1U);
            if(!parser.run())
            {
                REQUIRE(parsed_queue.empty());
            }
        }

        REQUIRE(parsed_queue.size() == 1U);
        require_front_equals(parsed_queue, joy_payload);
    }

    SECTION("frame split across runs")
    {
        lwrb.write(joy_command_frame.data(), 7U);
        REQUIRE_FALSE(parser.run());
        REQUIRE(parsed_queue.empty());

        lwrb.write(joy_command_frame.data() + 7U,
                   joy_command_frame.size() - 7U);
        REQUIRE(parser.run());
        REQUIRE(parsed_queue.size() == 1U);

        require_front_equals(parsed_queue, joy_payload);
    }

    SECTION("garbage before frame")
    {
        // Мусор не содержит стартовой последовательности 0xAA 0x55, в том
        // числе на границе с кадром.
        constexpr std::array<std::byte, 5> garbage{
            std::byte{0x00}, std::byte{0x11}, std::byte{0x22},
            std::byte{0x33}, std::byte{0x44},
        };

        lwrb.write(garbage);
        lwrb.write(joy_command_frame);

        REQUIRE(parser.run());
        REQUIRE(parsed_queue.size() == 1U);
        require_front_equals(parsed_queue, joy_payload);
    }

    SECTION("multiple frames in one run")
    {
        const auto frame_two = make_frame({
            std::byte{0xDE},
            std::byte{0xAD},
            std::byte{0xBE},
            std::byte{0xEF},
        });

        lwrb.write(joy_command_frame);
        lwrb.write(frame_two);

        // За один вызов run() успевают разобраться оба кадра.
        REQUIRE(parser.run());
        REQUIRE(parsed_queue.size() == 2U);

        require_front_equals(parsed_queue, joy_payload);
        require_front_equals(parsed_queue, padded_payload({
                                               std::byte{0xDE},
                                               std::byte{0xAD},
                                               std::byte{0xBE},
                                               std::byte{0xEF},
                                           }));
    }

    SECTION("false start: 0xAA without 0x55")
    {
        constexpr std::array<std::byte, 2> false_start{
            std::byte{0xAA},
            std::byte{0x00},
        };

        lwrb.write(false_start);
        lwrb.write(joy_command_frame);

        REQUIRE(parser.run());
        REQUIRE(parsed_queue.size() == 1U);
        require_front_equals(parsed_queue, joy_payload);
    }

    SECTION("false start: double 0xAA")
    {
        // 0xAA 0xAA: первая пара не совпадает со стартовой
        // последовательностью и пропускается побайтно, вторая пара
        // (второй 0xAA и первый байт кадра) образует настоящий старт.
        constexpr std::array<std::byte, 2> false_start{
            std::byte{0xAA},
            std::byte{0xAA},
        };

        lwrb.write(false_start);
        lwrb.write(joy_command_frame);

        REQUIRE(parser.run());
        REQUIRE(parsed_queue.size() == 1U);
        require_front_equals(parsed_queue, joy_payload);
    }

    SECTION("corrupted crc frame dropped")
    {
        auto corrupted_frame    = joy_command_frame;
        corrupted_frame.back() ^= std::byte{0xFF};

        lwrb.write(corrupted_frame);

        // Кадр с битым CRC не выдаётся; остатки потока пропускаются
        // побайтно без ложных совпадений.
        REQUIRE_FALSE(parser.run());
        REQUIRE(parsed_queue.empty());
    }

    SECTION("valid frame after frame with corrupted crc")
    {
        // Поток: невалидный кадр (0xAA 0x55 и мусор с битой CRC), за ним —
        // валидный кадр. После ошибки CRC парсер пропускает один байт и
        // продолжает поиск, находя валидный кадр.
        std::vector<std::byte> stream;
        stream.reserve(40U);
        stream.push_back(std::byte{0xAA});
        stream.push_back(std::byte{0x55});
        for(std::size_t i{2U}; i < 20U; ++i)
        {
            // Мусор не содержит 0xAA, чтобы не образовывать ложные старты.
            stream.push_back(static_cast<std::byte>(i + 100U));
        }
        stream.insert(stream.end(), joy_command_frame.begin(),
                      joy_command_frame.end());

        lwrb.write(stream);

        REQUIRE(parser.run());
        REQUIRE(parsed_queue.size() == 1U);
        require_front_equals(parsed_queue, joy_payload);
    }

    SECTION("incomplete frame waits for the rest")
    {
        // Заголовок и часть полезной нагрузки (без 0xAA в остатке):
        // кадр не выдаётся, пока не дописаны оставшиеся байты.
        lwrb.write(joy_command_frame.data(), 10U);
        REQUIRE_FALSE(parser.run());
        REQUIRE(parsed_queue.empty());

        lwrb.write(joy_command_frame.data() + 10U,
                   joy_command_frame.size() - 10U);
        REQUIRE(parser.run());
        REQUIRE(parsed_queue.size() == 1U);

        require_front_equals(parsed_queue, joy_payload);
    }

    SECTION("incomplete frame discarded by timeout")
    {
        // Недописанный кадр не выдаётся до таймаута: после
        // max_wait_message_ready_polls опросов без новых байт стартовый
        // кадр отбрасывается и парсер возвращается к побайтному поиску.
        lwrb.write(joy_command_frame.data(), 10U);

        // Первый вызов находит стартовый кадр и переходит в ожидание
        // остатка кадра, фиксируя число байт в буфере.
        REQUIRE_FALSE(parser.run());
        REQUIRE(parsed_queue.empty());

        // Таймаут: после max_wait_message_ready_polls опросов без новых
        // байт недописанный кадр отбрасывается.
        for(std::size_t i{0};
            i < decltype(parser)::max_wait_message_ready_polls; ++i)
        {
            REQUIRE_FALSE(parser.run());
        }
        REQUIRE(parsed_queue.empty());

        // Следующий валидный кадр после отброшенного недописанного
        // принимается корректно.
        lwrb.write(joy_command_frame);
        REQUIRE(parser.run());
        REQUIRE(parsed_queue.size() == 1U);

        require_front_equals(parsed_queue, joy_payload);
    }

    SECTION("empty buffer")
    {
        REQUIRE_FALSE(parser.run());
        REQUIRE(parsed_queue.empty());
    }

    SECTION("payload starting with 0xAA 0x55")
    {
        // Полезная нагрузка, начинающаяся со стартовой последовательности:
        // парсер читает кадр фиксированной длины и не должен спутать
        // байты нагрузки с началом следующего кадра.
        const auto frame      = make_frame({
            std::byte{0xAA},
            std::byte{0x55},
            std::byte{0x01},
            std::byte{0x02},
        });
        const auto next_frame = make_frame({std::byte{0x03}});

        lwrb.write(frame);
        lwrb.write(next_frame);

        REQUIRE(parser.run());
        REQUIRE(parsed_queue.size() == 2U);

        require_front_equals(parsed_queue, padded_payload({
                                               std::byte{0xAA},
                                               std::byte{0x55},
                                               std::byte{0x01},
                                               std::byte{0x02},
                                           }));
        require_front_equals(parsed_queue, padded_payload({std::byte{0x03}}));
    }
}

TEST_CASE(
    "minimal rmp_parser<4> frame through serial_parser", "[stv][communication]")
{
    using sim_buffer_type    = stv::sim_buff<stv::empty_mutex>;
    using queue_type         = etl::queue<sim_buffer_type, 10>;
    using lwrb_setup_type    = stv::lwrb_setup<stv::empty_mutex>;
    using lwrb_base_type     = stv::lwrb_base<lwrb_setup_type>;
    using minimal_frame_type = stv::rmp_parser<4>;

    constexpr std::size_t                       lwrb_buffer_size{64};
    stv::lwrb<lwrb_base_type, lwrb_buffer_size> lwrb{lwrb_setup_type{}};
    queue_type                                  parsed_queue;

    using parser_setup_type =
        stv::serial_parser_setup<lwrb_base_type, queue_type, stv::empty_mutex,
                                 minimal_frame_type>;

    parser_setup_type setup;
    setup.lwrb = &lwrb;
    setup.set_queue<minimal_frame_type>(parsed_queue);

    auto parser =
        stv::make_serial_parser<parser_setup_type, minimal_frame_type>(setup);
    REQUIRE(parser);

    // Минимальный кадр: стартовая последовательность и CRC пустой
    // полезной нагрузки.
    constexpr std::array<std::byte, 4> minimal_frame{
        std::byte{0xAA},
        std::byte{0x55},
        std::byte{0xFF},
        std::byte{0xFF},
    };

    lwrb.write(minimal_frame);

    REQUIRE(parser.run());
    REQUIRE(parsed_queue.size() == 1U);

    // После обрезки заголовка и хвоста сообщение пустое.
    const auto msg = std::move(parsed_queue.front());
    REQUIRE(msg.size_bytes() == 0U);
    parsed_queue.pop();
}

// NOLINTEND(readability-function-cognitive-complexity)

// NOLINTEND(*-magic-numbers)
