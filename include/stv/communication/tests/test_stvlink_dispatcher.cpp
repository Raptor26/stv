/// @file test_stvlink_dispatcher.cpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.

#include "stv/communication/serial_sender.hpp"
#include "stv/communication/stvlink_dispatcher.hpp"
#include "stv/communication/stvlink_parser.hpp"
#include "stv/communication/stvlink_sender.hpp"
#include "stv/containers/simbuff.hpp"
#include <array>
#include <catch2/catch_test_macros.hpp>
#include <cstddef>
#include <etl/queue.h>
#include <etl/unordered_map.h>
#include <utility>

namespace {

/// @brief Структура-заглушка полезной нагрузки.
/// @details Диспетчер payload-независим, поэтому вместо реальной
/// payload-структуры интегрирующего проекта (заголовки karavan в тестах
/// stv недоступны — линкуется только stv::stv) используется тривиальная
/// структура подходящего размера.
struct telemetry_stub {
    std::array<std::byte, 40U> raw{};
};

bool telemetry_processing(
    stv::stvlink_message_span pload)
{
    auto success{false};

    if(pload.size_bytes() == sizeof(telemetry_stub))
    {
        success = true;
    }

    return success;
}

constexpr std::pair<int, stv::stvlink_message_handler_fnc_type>
    telemetry_processing_pair{0, telemetry_processing};

} // namespace

TEST_CASE(
    "stvlink_dispatcher", "[stv][stvlink]")
{
    const auto processing_hash_table =
        etl::make_unordered_map<int, stv::stvlink_message_handler_fnc_type>(
            telemetry_processing_pair);

    using sim_buff_type = stv::sim_buff<stv::empty_mutex>;
    using hash_table_type =
        etl::iunordered_map<int, stv::stvlink_message_handler_fnc_type>;
    using queue_type = etl::queue<sim_buff_type, 10>;
    queue_type serial_sender_queue{};

    auto       serial_sender =
        make_serial_message_buffer(serial_sender_queue, stv::stvlink_sender{});

    using setup_type  = stv::stvlink_setup<hash_table_type>;
    using module_type = stv::stvlink_module<queue_type, setup_type>;

    queue_type module_queue;

    SECTION("Create invalid object with default setup")
    { REQUIRE_FALSE(module_type{module_queue, setup_type{}}); }

    module_type module{module_queue,
                       setup_type{.hash_table = &processing_hash_table}};
    REQUIRE(module);

    SECTION("Processing messages")
    {
        auto expected_queue_processing_status{false};
        SECTION("Send telemetry")
        {
            constexpr auto msg_id{std::get<0>(telemetry_processing_pair)};

            auto           msg = serial_sender.request<telemetry_stub>(
                stv::stvlink_sender::setup_t{
                    .dst_id = 111, ///< в данном тесте не имеет значения,
                                   ///< т.к. приёмник по dst_id не
                                   ///< фильтрует.
                    .msg_id = static_cast<
                        decltype(stv::stvlink_sender::setup_t::msg_id)>(msg_id),
                });
            msg->raw[0] = std::byte{10};

            expected_queue_processing_status = true;
            // В деструкторе msg выполняется сериализация сообщения.
        }

        SECTION("Send invalid msg id")
        {
            const auto msg = serial_sender.request<telemetry_stub>(
                stv::stvlink_sender::setup_t{
                    .dst_id = 111, ///< в данном тесте не имеет значения,
                                   ///< т.к. приёмник по dst_id не
                                   ///< фильтрует.
                    .msg_id = static_cast<
                        decltype(stv::stvlink_sender::setup_t::msg_id)>(10),
                });

            expected_queue_processing_status = false;
            // В деструкторе msg выполняется сериализация сообщения.
        }

        REQUIRE(!serial_sender_queue.empty());

        /// Перемещение сериализованного сообщения из очереди передатчика в
        /// очередь обработчика. Парсер на приёмной стороне отрезает
        /// стартовый кадр и CRC, а заголовок сообщения оставляет в начале
        /// сообщения -- эмулируем это здесь.
        decltype(auto) msg = serial_sender_queue.front();
        msg.trim_head(stv::stvlink_parser::header_size());
        msg.trim_tail(stv::stvlink_parser::trailer_size());
        module_queue.push(std::move(msg));
        serial_sender_queue.pop();

        // После вызова process_one() размер очереди должен уменьшиться
        // т.к. обработанное сообщение удаляется из очереди.
        const auto queue_size = module_queue.size();
        REQUIRE(module.process_one() == expected_queue_processing_status);
        REQUIRE(module_queue.size() == queue_size - 1);
    }
}
