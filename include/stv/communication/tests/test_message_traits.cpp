/// @file test_message_traits.cpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.

#include "stv/communication/message_traits.hpp"
#include "stv/communication/tests/demo_messages.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstddef>
#include <cstdint>

using stv::demo_command;
using stv::demo_msg_id;
using stv::demo_telemetry;

static_assert(stv::has_message_traits<demo_telemetry>);
static_assert(stv::has_message_traits<demo_command>);
static_assert(!stv::has_message_traits<int>);
static_assert(!stv::has_period<demo_telemetry>); // у demo-типов period нет

TEST_CASE(
    "message_traits: концепты на демо-протоколе")
{
    static_assert(stv::message_traits<demo_telemetry>::msg_id
                  == demo_msg_id::telemetry);
    static_assert(stv::message_traits<demo_command>::msg_id
                  == demo_msg_id::command);
    static_assert(stv::message_traits<demo_telemetry>::min_pload_size_bytes
                  == sizeof(std::uint32_t) + sizeof(std::int16_t));
    static_assert(stv::message_traits<demo_command>::min_pload_size_bytes
                  == demo_command::serialized_size_bytes());

    REQUIRE(stv::message_traits<demo_telemetry>::msg_id
            == demo_msg_id::telemetry);
    REQUIRE(stv::message_traits<demo_command>::min_pload_size_bytes == 1U);
}
