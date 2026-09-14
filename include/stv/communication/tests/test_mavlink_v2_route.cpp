/// @file test_mavlink_v2_route.cpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.

#include "etl/unordered_map.h"
#include "stv/communication/mavlink_v2_frame.hpp"
#include "stv/communication/mavlink_v2_route.hpp"
#include "stv/communication/parsed_queue.hpp"
#include "stv/containers/simbuff.hpp"
#include <array>
#include <catch2/catch_test_macros.hpp>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <etl/queue.h>
#include <mutex>
#include <optional>
#include <span>

// NOLINTBEGIN(*-magic-numbers)

// Catch2 SECTION-макросы резко завышают когнитивную сложность теста;
// разбивать единый тестовый сценарий ради метрики нецелесообразно.
// NOLINTBEGIN(readability-function-cognitive-complexity)

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

} // namespace

TEST_CASE(
    "mavlink_v2_route", "[stv][communication]")
{
    using sim_buffer_type = stv::sim_buff<stv::empty_mutex>;
    using queue_type      = etl::queue<sim_buffer_type, 10>;
    using hash_type       = etl::iunordered_map<int, queue_type *>;
    using frame_type      = stv::mavlink_v2_frame<test_crc_extra_provider>;

    using route_setup_type =
        stv::mavlink_v2_route_setup<frame_type, queue_type, hash_type>;
    using router_type = stv::mavlink_v2_route<route_setup_type>;

    // Раскладка сообщения после отрезания парсером заголовка и CRC:
    // [compat_flags, seq, sysid, compid, msgid (3 байта, LE), payload].
    constexpr std::size_t              routing_header_size{7U};
    constexpr std::size_t              compid_offset{3U};

    constexpr std::array<std::byte, 2> payload{
        std::byte{0x48},
        std::byte{0x69},
    }; // "Hi"

    const auto make_message = [](std::uint8_t               compid,
                                 std::span<const std::byte> pload) {
        sim_buffer_type msg{routing_header_size + pload.size()};

        std::byte      *dst = msg.data<std::byte>();
        dst[0]              = std::byte{0x00}; // compat_flags
        dst[1]              = std::byte{0x2A}; // seq
        dst[2]              = std::byte{0x01}; // sysid
        dst[3]              = static_cast<std::byte>(compid);
        dst[4]              = std::byte{0x00}; // msgid (3 байта, LE)
        dst[5]              = std::byte{0x00};
        dst[6]              = std::byte{0x00};

        std::memcpy(dst + routing_header_size, pload.data(), pload.size());

        return msg;
    };

    const auto message_equals = [](const sim_buffer_type     &msg,
                                   std::uint8_t               compid,
                                   std::span<const std::byte> pload) {
        if(msg.size_bytes() != routing_header_size + pload.size())
        {
            return false;
        }

        const std::byte *data = msg.data<std::byte>();
        if(data[compid_offset] != static_cast<std::byte>(compid))
        {
            return false;
        }

        return std::memcmp(data + routing_header_size, pload.data(),
                           pload.size())
               == 0;
    };

    queue_type input_queue;

    SECTION("route by compid")
    {
        queue_type                                queue_one;
        queue_type                                queue_two;

        etl::unordered_map<int, queue_type *, 10> hash_table;
        hash_table.insert({1, &queue_one});
        hash_table.insert({2, &queue_two});

        route_setup_type setup;
        setup.queue_to_read =
            stv::parsed_queue<frame_type, queue_type>{input_queue};
        setup.hash_to_write = &hash_table;

        router_type router{setup};
        REQUIRE(router);

        input_queue.push(make_message(1, payload));

        REQUIRE(router.run() == 1U);
        REQUIRE(input_queue.empty());

        REQUIRE(queue_one.size() == 1U);
        REQUIRE(message_equals(queue_one.front(), 1, payload));
        REQUIRE(queue_two.empty());
    }

    SECTION("unknown compid dropped")
    {
        queue_type                                queue_one;

        etl::unordered_map<int, queue_type *, 10> hash_table;
        hash_table.insert({1, &queue_one});

        route_setup_type setup;
        setup.queue_to_read =
            stv::parsed_queue<frame_type, queue_type>{input_queue};
        setup.hash_to_write = &hash_table;

        router_type router{setup};
        REQUIRE(router);

        input_queue.push(make_message(99, payload));

        REQUIRE(router.run() == 0U);
        REQUIRE(input_queue.empty());
        REQUIRE(queue_one.empty());
    }

    SECTION("full target queue dropped")
    {
        queue_type                                target_queue;

        etl::unordered_map<int, queue_type *, 10> hash_table;
        hash_table.insert({1, &target_queue});

        route_setup_type setup;
        setup.queue_to_read =
            stv::parsed_queue<frame_type, queue_type>{input_queue};
        setup.hash_to_write = &hash_table;

        router_type router{setup};
        REQUIRE(router);

        // Заполняем целевую очередь до отказа.
        for(std::size_t i{0}; i < target_queue.capacity(); ++i)
        {
            target_queue.push(make_message(1, payload));
        }
        REQUIRE(target_queue.full());

        const auto original_front = target_queue.front();
        REQUIRE(message_equals(original_front, 1, payload));

        input_queue.push(make_message(1, payload));

        REQUIRE(router.run() == 0U);
        REQUIRE(input_queue.empty());
        REQUIRE(target_queue.full());
        REQUIRE(target_queue.front().size_bytes()
                == original_front.size_bytes());
    }

    SECTION("multiple messages in a row")
    {
        queue_type                                queue_one;
        queue_type                                queue_two;

        etl::unordered_map<int, queue_type *, 10> hash_table;
        hash_table.insert({1, &queue_one});
        hash_table.insert({2, &queue_two});

        route_setup_type setup;
        setup.queue_to_read =
            stv::parsed_queue<frame_type, queue_type>{input_queue};
        setup.hash_to_write = &hash_table;

        router_type router{setup};
        REQUIRE(router);

        // Второе сообщение для compid 1 имеет другую нагрузку, чтобы
        // проверка содержимого ловила перестановку и порчу сообщений.
        constexpr std::array<std::byte, 3> payload_two{
            std::byte{0x42},
            std::byte{0x79},
            std::byte{0x65},
        }; // "Bye"

        input_queue.push(make_message(1, payload));
        input_queue.push(make_message(2, payload));
        input_queue.push(make_message(99, payload)); // неизвестный compid
        input_queue.push(make_message(1, payload_two));

        REQUIRE(router.run() == 3U);
        REQUIRE(input_queue.empty());

        REQUIRE(queue_one.size() == 2U);
        REQUIRE(message_equals(queue_one.front(), 1, payload));
        queue_one.pop();
        // Содержимое второго сообщения проверяется явно, а не только
        // количество сообщений в очереди.
        REQUIRE(message_equals(queue_one.front(), 1, payload_two));

        REQUIRE(queue_two.size() == 1U);
        REQUIRE(message_equals(queue_two.front(), 2, payload));
    }

    SECTION("unicast with null target queue pointer")
    {
        etl::unordered_map<int, queue_type *, 10> hash_table;
        hash_table.insert({1, nullptr});

        route_setup_type setup;
        setup.queue_to_read =
            stv::parsed_queue<frame_type, queue_type>{input_queue};
        setup.hash_to_write = &hash_table;

        router_type router{setup};
        REQUIRE(router);

        input_queue.push(make_message(1, payload));

        REQUIRE(router.run() == 0U);
        REQUIRE(input_queue.empty());
    }

    SECTION("invalid setup with null queue_to_read")
    {
        queue_type                                queue_one;
        etl::unordered_map<int, queue_type *, 10> hash_table;
        hash_table.insert({1, &queue_one});

        route_setup_type setup;
        // queue_to_read оставлен default-конструированным (nullptr).
        setup.hash_to_write = &hash_table;

        router_type router{setup};
        REQUIRE_FALSE(router);
    }

    SECTION("invalid setup with null hash_to_write")
    {
        route_setup_type setup;
        setup.queue_to_read =
            stv::parsed_queue<frame_type, queue_type>{input_queue};
        // hash_to_write оставлен nullptr.

        router_type router{setup};
        REQUIRE_FALSE(router);
    }

    SECTION("external mutex")
    {
        using ext_setup_type =
            stv::mavlink_v2_route_setup<frame_type, queue_type, hash_type,
                                        std::mutex *>;
        using ext_router_type = stv::mavlink_v2_route<ext_setup_type>;

        std::mutex                                ext_mutex;
        queue_type                                queue_one;

        etl::unordered_map<int, queue_type *, 10> hash_table;
        hash_table.insert({1, &queue_one});

        ext_setup_type setup;
        setup.queue_to_read =
            stv::parsed_queue<frame_type, queue_type>{input_queue};
        setup.hash_to_write = &hash_table;
        setup.mutex         = &ext_mutex;

        ext_router_type router{setup};
        REQUIRE(router);

        input_queue.push(make_message(1, payload));

        REQUIRE(router.run() == 1U);
        REQUIRE(input_queue.empty());
        REQUIRE(queue_one.size() == 1U);
        REQUIRE(message_equals(queue_one.front(), 1, payload));
    }

    SECTION("invalid setup with null external mutex")
    {
        using ext_setup_type =
            stv::mavlink_v2_route_setup<frame_type, queue_type, hash_type,
                                        std::mutex *>;
        using ext_router_type = stv::mavlink_v2_route<ext_setup_type>;

        queue_type                                queue_one;
        etl::unordered_map<int, queue_type *, 10> hash_table;
        hash_table.insert({1, &queue_one});

        ext_setup_type setup;
        setup.queue_to_read =
            stv::parsed_queue<frame_type, queue_type>{input_queue};
        setup.hash_to_write = &hash_table;
        // mutex оставлен nullptr.

        ext_router_type router{setup};
        REQUIRE_FALSE(router);
    }
}

// NOLINTEND(readability-function-cognitive-complexity)

// NOLINTEND(*-magic-numbers)
