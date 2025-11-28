/// @file bench_serial.cpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// @copyright (c) 2025 Gagaring
///
/// MIT License:
///
/// Permission is hereby granted, free of charge, to any person obtaining a copy
/// of this software and associated documentation files (the 'Software'), to
/// deal in the Software without restriction, including without limitation the
/// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
/// sell copies of the Software, and to permit persons to whom the Software is
/// furnished to do so, subject to the following conditions:
///
/// The above copyright notice and this permission notice shall be included in
/// all copies or substantial portions of the Software.
///
/// THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
/// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
/// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
/// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
/// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
/// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
/// IN THE SOFTWARE.

#include "stv/communication/serial.hpp"
#include "stv/containers/simbuff.hpp"
#include "stv/mutex_guard.hpp"
#include <benchmark/benchmark.h>
#include <queue>

namespace bm = benchmark;

static void request(
    bm::State &state)
{
    using namespace stv;

    using SimBuffType = stv::sim_buff<stv::EmptyMutex>;
    using queue_type  = std::queue<SimBuffType>;

    struct UserData {
        std::uint8_t i{11};
        std::uint8_t j{22};
        std::uint8_t k{33};
        std::uint8_t z{44};
    };

    auto serial_message_buffer = make_serial_message_buffer<queue_type>(
        stv::start_frame_and_crc_16{}, stv::head_route{});

    for(auto unused: state) {
        auto msg = serial_message_buffer.request<UserData>();

        msg->i = 1;
        msg->j = 2;
        msg->z = 4;
    }
}

BENCHMARK(request);

BENCHMARK_MAIN();
