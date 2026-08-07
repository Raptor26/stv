/// @file test_stvlink_frame.cpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.

#include "stv/communication/parsed_queue.hpp"
#include "stv/communication/serial_decorators.hpp"
#include "stv/communication/serial_sender.hpp"
#include "stv/communication/stvlink_frame.hpp"
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
    "stvlink_frame", "[stv][communication]")
{
    using sim_buffer_type = stv::sim_buff<stv::empty_mutex>;
    using queue_type      = etl::queue<sim_buffer_type, 10>;
    queue_type tx_queue;

    SECTION("start bytes")
    {
        REQUIRE(stv::stvlink_frame::first_byte == std::byte{0xAA});
        REQUIRE(stv::stvlink_frame::second_byte == std::byte{0xAA});
    }

    SECTION("header and trailer sizes")
    {
        REQUIRE(stv::stvlink_frame::header_size() == 4U);
        REQUIRE(stv::stvlink_frame::trailer_size() == 2U);
    }

    SECTION("frame size extraction")
    {
        constexpr std::string_view payload{"Hello world"};

        auto                       serial_message_buffer =
            stv::make_serial_message_buffer(tx_queue, stv::stvlink_frame_tx{});

        {
            auto msg = serial_message_buffer.request(payload);
            (void)msg;
        }

        REQUIRE(!tx_queue.empty());
        const auto                   &frame = tx_queue.front();

        const stv::total_message_span header_span{
            frame.data<std::byte>(), stv::stvlink_frame::header_size()};

        REQUIRE(stv::stvlink_frame::total_frame_size(header_span)
                == frame.size_bytes());
    }

    SECTION("crc validation")
    {
        constexpr std::string_view payload{"Hello world"};

        auto                       serial_message_buffer =
            stv::make_serial_message_buffer(tx_queue, stv::stvlink_frame_tx{});

        {
            auto msg = serial_message_buffer.request(payload);
            (void)msg;
        }

        REQUIRE(!tx_queue.empty());
        const auto                   &frame = tx_queue.front();

        const stv::total_message_span total{frame.data<std::byte>(),
                                            frame.size_bytes()};

        REQUIRE(stv::stvlink_frame::is_crc_valid(total));

        std::vector<std::byte> corrupted_frame{frame.data<std::byte>(),
                                               frame.data<std::byte>()
                                                   + frame.size_bytes()};

        REQUIRE(corrupted_frame.size() > 4U);
        corrupted_frame[4U] = ~corrupted_frame[4U];

        const stv::total_message_span corrupted_total{corrupted_frame.data(),
                                                      corrupted_frame.size()};

        REQUIRE_FALSE(stv::stvlink_frame::is_crc_valid(corrupted_total));
    }

    SECTION("parsed_queue tag")
    {
        queue_type                                  queue;
        const stv::parsed_queue<stv::stvlink_frame> stv_queue_tag{queue};

        REQUIRE(stv_queue_tag.queue() == &queue);

        using pq_stv_type   = decltype(stv_queue_tag);
        using pq_other_type = stv::parsed_queue<int>;

        REQUIRE_FALSE((std::is_same_v<pq_stv_type, pq_other_type>));
    }
}

// NOLINTEND(*-magic-numbers)
