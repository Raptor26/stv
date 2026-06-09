/// @file bench_serial.cpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.

#include "stv/communication/serial_sender.hpp"
#include "stv/containers/simbuff.hpp"
#include "stv/mutex_guard.hpp"
#include <benchmark/benchmark.h>
#include <queue>

namespace bm = benchmark;

static void request(
    bm::State &state)
{
    using namespace stv;

    using sim_buff_type = stv::sim_buff<stv::empty_mutex>;

    struct UserData {
        std::uint8_t i{11};
        std::uint8_t j{22};
        std::uint8_t k{33};
        std::uint8_t z{44};
    };

    auto serial_message_buffer = make_serial_message_buffer<sim_buff_type, 10>(
        stv::start_frame_and_crc_16{}, stv::head_route{});

    for(auto unused: state)
    {
        auto msg = serial_message_buffer.request<UserData>();

        msg->i = 1;
        msg->j = 2;
        msg->z = 4;
    }
}

BENCHMARK(request);

BENCHMARK_MAIN();
