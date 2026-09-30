/// @file test_mavlink_v2_sender.cpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.

#include "stv/communication/mavlink_v2_parser.hpp"
#include "stv/communication/mavlink_v2_sender.hpp"
#include "stv/communication/serial_decorators.hpp"
#include "stv/communication/serial_sender.hpp"
#include "stv/containers/simbuff.hpp"
#include <array>
#include <atomic>
#include <catch2/catch_test_macros.hpp>
#include <cstddef>
#include <cstdint>
#include <etl/queue.h>
#include <etl/queue_mpmc_mutex.h>
#include <optional>
#include <string>
#include <thread>

// NOLINTBEGIN(*-magic-numbers)

namespace {

/// @brief Тестовый провайдер CRC_EXTRA с маленькой таблицей на 2 msgid.
struct test_crc_extra_provider {
    static auto crc_extra_for(
        std::uint32_t msgid) -> std::optional<std::uint8_t>
    {
        switch(msgid)
        {
            case 0U:
                return std::uint8_t{50U};  // HEARTBEAT
            case 1U:
                return std::uint8_t{124U}; // SYS_STATUS
            default:
                return std::nullopt;
        }
    }
};

using frame_type    = stv::mavlink_v2_parser<test_crc_extra_provider>;
using frame_tx_type = stv::mavlink_v2_sender<test_crc_extra_provider>;

} // namespace

// Catch2 SECTION-макросы резко завышают когнитивную сложность теста;
// разбивать единый тестовый сценарий ради метрики нецелесообразно.
// NOLINTBEGIN(readability-function-cognitive-complexity)
TEST_CASE(
    "mavlink_v2_sender", "[stv][communication]")
{
    using sim_buffer_type = stv::sim_buff<stv::empty_mutex>;
    using queue_type      = etl::queue<sim_buffer_type, 10>;

    struct user_data_t {
        std::uint8_t i{11};
        std::uint8_t j{22};
        std::uint8_t k{33};
        std::uint8_t z{44};
    };

    SECTION("frame layout")
    {
        queue_type                  queue;
        stv::mavlink_v2_seq_counter counter;

        // NOLINTNEXTLINE(misc-const-correctness) request() — не const
        auto serial_message_buffer = stv::make_serial_message_buffer(
            queue,
            frame_tx_type{{.sysid = 5, .compid = 3, .counter = &counter}});

        STATIC_REQUIRE(frame_tx_type::header_size() == 10U);
        STATIC_REQUIRE(frame_tx_type::trailer_size() == 2U);

        {
            auto msg = serial_message_buffer.request(
                user_data_t(), frame_tx_type::route_t{.msgid = 0});
            REQUIRE(msg);
        }

        REQUIRE(queue.size() == 1U);

        const auto &frame = queue.front();
        // Заголовок(10) + payload(4) + CRC(2).
        REQUIRE(frame.size_bytes() == 16U);

        const auto *bytes = frame.data<std::byte>();

        REQUIRE(bytes[0U] == std::byte{0xFD}); // magic
        REQUIRE(bytes[1U] == std::byte{4U});   // len
        REQUIRE(bytes[2U] == std::byte{0x00}); // incompat_flags
        REQUIRE(bytes[3U] == std::byte{0x00}); // compat_flags
        REQUIRE(bytes[4U] == std::byte{0x00}); // seq
        REQUIRE(bytes[5U] == std::byte{0x05}); // sysid
        REQUIRE(bytes[6U] == std::byte{0x03}); // compid

        REQUIRE(bytes[7U] == std::byte{0x00}); // msgid = 0, little-endian
        REQUIRE(bytes[8U] == std::byte{0x00});
        REQUIRE(bytes[9U] == std::byte{0x00});

        REQUIRE(bytes[10U] == std::byte{11U}); // payload
        REQUIRE(bytes[11U] == std::byte{22U});
        REQUIRE(bytes[12U] == std::byte{33U});
        REQUIRE(bytes[13U] == std::byte{44U});

        // Эталонный CRC-16/X.25 по байтам len..payload + CRC_EXTRA(50):
        // 04 00 00 00 05 03 00 00 00 0B 16 21 2C, досчёт 0x32 → 0xE1A2.
        REQUIRE(bytes[14U] == std::byte{0xA2});
        REQUIRE(bytes[15U] == std::byte{0xE1});
    }

    SECTION("seq increments between frames")
    {
        queue_type                  queue;
        stv::mavlink_v2_seq_counter counter;

        auto serial_message_buffer = stv::make_serial_message_buffer(
            queue,
            frame_tx_type{{.sysid = 5, .compid = 3, .counter = &counter}});

        {
            auto msg = serial_message_buffer.request(
                user_data_t(), frame_tx_type::route_t{.msgid = 0});
            REQUIRE(msg);
        }
        {
            auto msg = serial_message_buffer.request(
                user_data_t(), frame_tx_type::route_t{.msgid = 0});
            REQUIRE(msg);
        }

        REQUIRE(queue.size() == 2U);

        // Байты читаются до pop(): деструктор sim_buff освобождает память
        // кадра.
        REQUIRE(queue.front().data<std::byte>()[4U] == std::byte{0x00});
        queue.pop();
        REQUIRE(queue.front().data<std::byte>()[4U] == std::byte{0x01});
    }

    SECTION("seq wraps through 255")
    {
        queue_type                  queue;
        stv::mavlink_v2_seq_counter counter;

        // Счётчик отдаёт значения подряд с нуля.
        for(std::uint16_t i{0}; i < 255U; ++i)
        {
            REQUIRE(counter.next() == static_cast<std::uint8_t>(i));
        }

        auto serial_message_buffer = stv::make_serial_message_buffer(
            queue,
            frame_tx_type{{.sysid = 5, .compid = 3, .counter = &counter}});

        {
            auto msg = serial_message_buffer.request(
                user_data_t(), frame_tx_type::route_t{.msgid = 0});
            REQUIRE(msg);
        }
        {
            auto msg = serial_message_buffer.request(
                user_data_t(), frame_tx_type::route_t{.msgid = 0});
            REQUIRE(msg);
        }

        REQUIRE(queue.size() == 2U);

        // Первый кадр получает seq = 255, второй — 0 (заворот).
        REQUIRE(queue.front().data<std::byte>()[4U] == std::byte{0xFF});
        queue.pop();
        REQUIRE(queue.front().data<std::byte>()[4U] == std::byte{0x00});
    }

    SECTION("apply_setup changes only msgid")
    {
        queue_type                  queue;
        stv::mavlink_v2_seq_counter counter;

        auto serial_message_buffer = stv::make_serial_message_buffer(
            queue,
            frame_tx_type{{.sysid = 5, .compid = 3, .counter = &counter}});

        {
            auto msg = serial_message_buffer.request(
                user_data_t(), frame_tx_type::route_t{.msgid = 0});
            REQUIRE(msg);
        }
        {
            auto msg = serial_message_buffer.request(
                user_data_t(), frame_tx_type::route_t{.msgid = 1});
            REQUIRE(msg);
        }

        REQUIRE(queue.size() == 2U);

        // Байты читаются до pop(): деструктор sim_buff освобождает память
        // кадра.
        const auto *first_frame = queue.front().data<std::byte>();
        REQUIRE(first_frame[7U] == std::byte{0x00}); // msgid = 0, LE
        REQUIRE(first_frame[8U] == std::byte{0x00});
        REQUIRE(first_frame[9U] == std::byte{0x00});
        queue.pop();

        const auto *second_frame = queue.front().data<std::byte>();
        // msgid обновлён из параметра запроса, little-endian.
        REQUIRE(second_frame[7U] == std::byte{0x01});
        REQUIRE(second_frame[8U] == std::byte{0x00});
        REQUIRE(second_frame[9U] == std::byte{0x00});

        // sysid/compid сохранены из setup_t, seq инкрементирован.
        REQUIRE(second_frame[4U] == std::byte{0x01});
        REQUIRE(second_frame[5U] == std::byte{0x05});
        REQUIRE(second_frame[6U] == std::byte{0x03});
    }

    SECTION("tx frame passes mavlink_v2_parser::is_crc_valid")
    {
        queue_type                  queue;
        stv::mavlink_v2_seq_counter counter;

        auto serial_message_buffer = stv::make_serial_message_buffer(
            queue,
            frame_tx_type{{.sysid = 5, .compid = 3, .counter = &counter}});

        {
            auto msg = serial_message_buffer.request(
                user_data_t(), frame_tx_type::route_t{.msgid = 0});
            REQUIRE(msg);
        }

        REQUIRE(queue.size() == 1U);

        const auto                   &frame = queue.front();
        const stv::total_message_span total{
            frame.data<std::byte>(),
            frame.size_bytes(),
        };

        // Сквозной тест TX→parser: кадр, собранный mavlink_v2_sender,
        // проходит проверку CRC парсерным декоратором mavlink_v2_parser.
        REQUIRE(frame_type::is_crc_valid(total));

        // Полный размер кадра, вычисленный по peek-нутому заголовку,
        // совпадает с реальным размером кадра.
        const stv::total_message_span header_span{frame.data<std::byte>(),
                                                  frame_type::header_size()};
        REQUIRE(frame_type::total_frame_size(header_span)
                == frame.size_bytes());
    }

    SECTION("empty payload")
    {
        queue_type                  queue;
        stv::mavlink_v2_seq_counter counter;

        auto serial_message_buffer = stv::make_serial_message_buffer(
            queue,
            frame_tx_type{{.sysid = 5, .compid = 3, .counter = &counter}});

        {
            auto msg = serial_message_buffer.request(
                0U, frame_tx_type::route_t{.msgid = 0});
            REQUIRE(msg);
        }

        REQUIRE(queue.size() == 1U);

        const auto &frame = queue.front();
        // Заголовок(10) + payload(0) + CRC(2).
        REQUIRE(frame.size_bytes() == 12U);

        const auto *bytes = frame.data<std::byte>();

        REQUIRE(bytes[0U] == std::byte{0xFD}); // magic
        REQUIRE(bytes[1U] == std::byte{0x00}); // len
        REQUIRE(bytes[2U] == std::byte{0x00}); // incompat_flags
        REQUIRE(bytes[3U] == std::byte{0x00}); // compat_flags
        REQUIRE(bytes[4U] == std::byte{0x00}); // seq
        REQUIRE(bytes[5U] == std::byte{0x05}); // sysid
        REQUIRE(bytes[6U] == std::byte{0x03}); // compid

        REQUIRE(bytes[7U] == std::byte{0x00}); // msgid = 0, little-endian
        REQUIRE(bytes[8U] == std::byte{0x00});
        REQUIRE(bytes[9U] == std::byte{0x00});

        // Эталонный CRC-16/X.25 по байтам len..msgid + CRC_EXTRA(50):
        // 00 00 00 00 05 03 00 00 00, досчёт 0x32 → 0x875D.
        REQUIRE(bytes[10U] == std::byte{0x5D});
        REQUIRE(bytes[11U] == std::byte{0x87});

        const stv::total_message_span total{
            frame.data<std::byte>(),
            frame.size_bytes(),
        };
        REQUIRE(frame_type::is_crc_valid(total));
    }

    SECTION("max payload boundary")
    {
        queue_type                  queue;
        stv::mavlink_v2_seq_counter counter;

        auto serial_message_buffer = stv::make_serial_message_buffer(
            queue,
            frame_tx_type{{.sysid = 5, .compid = 3, .counter = &counter}});

        const std::string payload(255U, 'A');

        {
            auto msg = serial_message_buffer.request(
                payload, frame_tx_type::route_t{.msgid = 0});
            REQUIRE(msg);
        }

        REQUIRE(queue.size() == 1U);

        const auto &frame = queue.front();
        // Заголовок(10) + payload(255) + CRC(2).
        REQUIRE(frame.size_bytes() == 267U);

        const auto *bytes = frame.data<std::byte>();
        REQUIRE(bytes[0U] == std::byte{0xFD});
        REQUIRE(bytes[1U] == std::byte{0xFF}); // len = 255

        // Эталонный CRC-16/X.25 по байтам len..payload + CRC_EXTRA(50):
        // FF 00 00 00 05 03 00 00 00, 255 байт 0x41, досчёт 0x32 → 0x8081.
        REQUIRE(bytes[265U] == std::byte{0x81});
        REQUIRE(bytes[266U] == std::byte{0x80});

        const stv::total_message_span total{
            frame.data<std::byte>(),
            frame.size_bytes(),
        };
        REQUIRE(frame_type::is_crc_valid(total));

        // Полный размер кадра по peek-нутому заголовку совпадает с
        // реальным размером кадра.
        const stv::total_message_span header_span{frame.data<std::byte>(),
                                                  frame_type::header_size()};
        REQUIRE(frame_type::total_frame_size(header_span)
                == frame.size_bytes());
    }

    SECTION("msgid exceeding 3 bytes is rejected")
    {
        queue_type                  queue;
        stv::mavlink_v2_seq_counter counter;

        auto serial_message_buffer = stv::make_serial_message_buffer(
            queue,
            frame_tx_type{{.sysid = 5, .compid = 3, .counter = &counter}});

        // Поле msgid занимает 3 байта: значение 0x1000000 непредставимо,
        // request() возвращает невалидное сообщение вместо усечения msgid.
        {
            const auto msg = serial_message_buffer.request(
                user_data_t(), frame_tx_type::route_t{.msgid = 0x1000000});
            REQUIRE(!msg);
        }

        // Отклонённый запрос не помещает кадр в очередь и не тратит seq.
        REQUIRE(queue.empty());

        {
            auto msg = serial_message_buffer.request(
                user_data_t(), frame_tx_type::route_t{.msgid = 0});
            REQUIRE(msg);
        }

        REQUIRE(queue.size() == 1U);
        // seq первого реального кадра равен 0.
        REQUIRE(queue.front().data<std::byte>()[4U] == std::byte{0x00});
    }

    SECTION("msgid without known crc extra is rejected")
    {
        queue_type                  queue;
        stv::mavlink_v2_seq_counter counter;

        auto serial_message_buffer = stv::make_serial_message_buffer(
            queue,
            frame_tx_type{{.sysid = 5, .compid = 3, .counter = &counter}});

        // Провайдер знает CRC_EXTRA только для msgid 0 и 1: для msgid 42
        // сформировать корректный CRC невозможно, поэтому request()
        // возвращает невалидное сообщение вместо кадра с битым CRC.
        {
            const auto msg = serial_message_buffer.request(
                user_data_t(), frame_tx_type::route_t{.msgid = 42});
            REQUIRE(!msg);
        }

        // Отклонённый запрос не помещает кадр в очередь и не тратит seq.
        REQUIRE(queue.empty());

        {
            auto msg = serial_message_buffer.request(
                user_data_t(), frame_tx_type::route_t{.msgid = 1});
            REQUIRE(msg);
        }

        REQUIRE(queue.size() == 1U);
        // seq первого реального кадра равен 0.
        REQUIRE(queue.front().data<std::byte>()[4U] == std::byte{0x00});
    }

    SECTION("oversized payload is rejected")
    {
        // NOLINTNEXTLINE(misc-const-correctness)
        queue_type                  queue;
        stv::mavlink_v2_seq_counter counter;

        auto serial_message_buffer = stv::make_serial_message_buffer(
            queue,
            frame_tx_type{{.sysid = 5, .compid = 3, .counter = &counter}});

        STATIC_REQUIRE(frame_tx_type::max_pload_size() == 255U);

        // Поле len однобайтовое: полезная нагрузка 256 байт не помещается
        // в кадр. request() возвращает невалидное сообщение вместо
        // формирования кадра с усечённым полем len.
        const std::string payload(256U, 'A');

        {
            const auto msg = serial_message_buffer.request(
                payload, frame_tx_type::route_t{.msgid = 0});
            REQUIRE(!msg);
        }

        REQUIRE(queue.empty());
    }

    SECTION("concurrent requests produce unique sequential seq")
    {
        constexpr std::size_t thread_count{4};
        constexpr std::size_t requests_per_thread{250};
        constexpr std::size_t total_frames{thread_count * requests_per_thread};

        // etl::queue не потокобезопасна для конкурентных push из
        // нескольких потоков, а etl::queue_spsc_atomic рассчитана на
        // одного производителя. etl::queue_mpmc_mutex защищает push
        // внутренним мьютексом (на хосте - std::mutex), что допускает
        // конкурентные request() из thread_count потоков. Сам счётчик
        // seq мьютексом не защищается: его атомарность - предмет
        // проверки.
        using concurrent_queue_type =
            etl::queue_mpmc_mutex<sim_buffer_type, total_frames>;

        concurrent_queue_type       queue;
        stv::mavlink_v2_seq_counter counter;

        auto serial_message_buffer = stv::make_serial_message_buffer(
            queue,
            frame_tx_type{{.sysid = 5, .compid = 3, .counter = &counter}});

        // Catch2-макросы в рабочих потоках не потокобезопасны: результат
        // каждого request() учитывается общим атомарным счётчиком неудач.
        std::atomic<std::size_t> failed_requests{0};

        const auto worker = [&serial_message_buffer, &failed_requests] {
            for(std::size_t i{0}; i < requests_per_thread; ++i)
            {
                const auto msg = serial_message_buffer.request(
                    user_data_t(), frame_tx_type::route_t{.msgid = 0});
                if(!msg)
                {
                    ++failed_requests;
                }
            }
        };

        std::array<std::thread, thread_count> threads;
        for(auto &thread: threads)
        {
            thread = std::thread{worker};
        }
        for(auto &thread: threads)
        {
            thread.join();
        }

        REQUIRE(failed_requests.load() == 0U);
        REQUIRE(queue.size() == total_frames);

        // seq собираются из реальных кадров очереди после join: гонок при
        // чтении нет. Байты читаются до pop(): деструктор sim_buff
        // освобождает память кадра.
        std::array<std::size_t, 256U> seq_histogram{};
        while(!queue.empty())
        {
            const auto seq =
                static_cast<std::uint8_t>(queue.front().data<std::byte>()[4U]);
            ++seq_histogram.at(seq);
            queue.pop();
        }

        // Порядок seq между потоками не гарантирован, а 8-битный счётчик
        // заворачивается через 255: при 1000 кадрах значения 0..231
        // встречаются по 4 раза, значения 232..255 - по 3 раза.
        constexpr std::size_t full_cycles{total_frames / 256U};
        constexpr std::size_t remainder{total_frames % 256U};
        for(std::size_t i{0}; i < seq_histogram.size(); ++i)
        {
            REQUIRE(seq_histogram.at(i)
                    == (full_cycles + ((i < remainder) ? 1U : 0U)));
        }
    }
}

// NOLINTEND(readability-function-cognitive-complexity)

// NOLINTEND(*-magic-numbers)
