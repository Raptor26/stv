/// @file test_serial_parser.cpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// @copyright (c) 2026 Gagaring
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

#include "etl/unordered_map.h"
#include "stv/communication/serial_decorators.hpp"
#include "stv/communication/serial_parser.hpp"
#include "stv/communication/serial_sender.hpp"
#include "stv/containers/lwrb.hpp"
#include "stv/containers/simbuff.hpp"
#include "stv/mutex_guard.hpp"
#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <cstring>
#include <etl/queue.h>
#include <iterator>
#include <string_view>

// NOLINTBEGIN(*-magic-numbers, google-build-using-namespace,
// readability-function-cognitive-,
// cppcoreguidelines-avoid-non-const-global-variables)

std::array<char, 64> memory_to_serial_parser;
int                  alloc_cnt;

template<typename T>
class custom_allocator
{
    inline static std::size_t alloc_size_{};

  public:
    using value_type = T;

    custom_allocator() {}

    template<typename U>
    custom_allocator(
        const custom_allocator<U> &)
    {
    }

    T *allocate(
        std::size_t n)
    {
        alloc_size_ = n;
        ++alloc_cnt;
        return reinterpret_cast<T *>(memory_to_serial_parser.data());
    }

    void deallocate(
        T *p, std::size_t n)
    {
        (void)p;
        (void)n;
        --alloc_cnt;
    }

    static auto           get_allocator_cnt() { return alloc_cnt; }

    static constexpr auto get_mem_ptr()
    {
        return memory_to_serial_parser.data();
    }

    static auto data() { return memory_to_serial_parser.data(); }

    static auto get_last_alloc_size() { return alloc_size_; }
};

TEST_CASE(
    "Serial Parser", "[stv][communication]")
{
    // using custom_allocator = custom_allocator<std::byte>;
    // using sim_buffer_with_custom_allocator_type =
    //     stv::sim_buff<stv::empty_mutex, custom_allocator>;
    using sim_buffer_type = stv::sim_buff<stv::empty_mutex>;
    using queue_base_type = etl::iqueue<sim_buffer_type>;
    using queue_type      = etl::queue<sim_buffer_type, 10>;
    queue_type parsed_msg_queue;

    using lwrb_setup_type = stv::lwrb_setup<stv::empty_mutex>;
    using lwrb_base_type  = stv::lwrb_base<lwrb_setup_type>;
    constexpr std::size_t                       lwrb_buffer_size{128};
    stv::lwrb<lwrb_base_type, lwrb_buffer_size> lwrb{lwrb_setup_type{}};

    SECTION("Serial Parser")
    {
        /// @brief Объект используется для создания сообщений требуемой для
        /// проверки парсера структуры.
        auto serial_message_buffer = make_serial_message_buffer<queue_type>(
            stv::start_frame_and_crc_16{});

        using serial_parser_setup_type =
            stv::serial_parser_setup<lwrb_base_type, queue_type>;

        SECTION("Invalid Setup")
        {
            const serial_parser_setup_type setup;
            auto                           parser =
                stv::make_serial_parser<serial_parser_setup_type>(setup);
            REQUIRE_FALSE(parser);
        }

        SECTION("Valid Setup")
        {
            serial_parser_setup_type setup;
            setup.lwrb  = &lwrb;
            setup.queue = &parsed_msg_queue;

            auto parser =
                stv::make_serial_parser<serial_parser_setup_type>(setup);
            REQUIRE(parser);

            SECTION("Parse per message")
            {
                constexpr std::string_view test_message{"Hello world"};

                SECTION("Invalid message only")
                {
                    lwrb.write(test_message);
                    REQUIRE_FALSE(parser.compute());
                }

                SECTION("Without offset")
                {
                    {
                        auto msg = serial_message_buffer.request(test_message);

                        // В деструкторе msg будет записан вызван декоратор,
                        // который вычислит CRC.
                    }

                    auto queue_instance =
                        serial_message_buffer.queue_instance();
                    auto msg = queue_instance.front();
                    lwrb.write(msg.begin(), msg.end());
                    REQUIRE(parser.compute());
                    queue_instance.pop();
                }

                SECTION("With offset")
                {
                    // Вначале запишем просто тексТ, без начала кадра и
                    // контрольной суммы.
                    lwrb.write(test_message);

                    {
                        // А это уже запись обернутого с помощью декоратора
                        // сообщения.
                        auto msg = serial_message_buffer.request(test_message);

                        // В деструкторе msg будет записан вызван декоратор,
                        // который вычислит CRC.
                    }

                    auto queue_instance =
                        serial_message_buffer.queue_instance();
                    auto msg = queue_instance.front();
                    lwrb.write(msg.begin(), msg.end());
                    REQUIRE(parser.compute());
                    queue_instance.pop();
                }

                if(!parsed_msg_queue.empty())
                {
                    auto msg = parsed_msg_queue.front();
                    REQUIRE(memcmp(test_message.data(), msg.data(),
                                   test_message.size())
                            == 0);

                    REQUIRE(msg.begin() == msg.data<std::byte>());
                    REQUIRE(msg.end() == msg.data<std::byte>() + msg.size());

                    parsed_msg_queue.pop();
                }
            }

            SECTION("Parse chain of a messages")
            {
                constexpr std::string_view test_message_one{"Hello"};
                constexpr std::string_view test_message_two{"World"};

                {
                    auto msg = serial_message_buffer.request(test_message_one);

                    // В деструкторе msg будет записан вызван декоратор, который
                    // вычислит CRC.
                }

                {
                    auto msg = serial_message_buffer.request(test_message_two);

                    // В деструкторе msg будет записан вызван декоратор, который
                    // вычислит CRC.
                }

                auto queue_instance = serial_message_buffer.queue_instance();

                {
                    decltype(auto) msg = queue_instance.front();
                    lwrb.write(msg.begin(), msg.end());
                    queue_instance.pop();
                }

                {
                    decltype(auto) msg = queue_instance.front();
                    lwrb.write(msg.begin(), msg.end());
                    queue_instance.pop();
                }

                REQUIRE(parser.compute());

                {
                    auto msg = parsed_msg_queue.front();
                    REQUIRE(memcmp(test_message_one.data(), msg.data(),
                                   test_message_one.size())
                            == 0);
                    parsed_msg_queue.pop();
                }

                {
                    auto msg = std::move(parsed_msg_queue.front());
                    REQUIRE(memcmp(test_message_two.data(), msg.data(),
                                   test_message_two.size())
                            == 0);
                    parsed_msg_queue.pop();
                }
            }
        }
    }

    SECTION("Route")
    {
        using hash_type = etl::iunordered_map<int, queue_base_type *>;
        using serial_route_setup_type =
            stv::serial_route_setup<queue_base_type, hash_type>;
        using serial_route_type =
            stv::serial_parser_route<serial_route_setup_type>;

        auto serial_message_buffer =
            make_serial_message_buffer<queue_type>(stv::head_route{});

        etl::unordered_map<int, queue_base_type *, 10U> hash_table;
        constexpr int                                   parsed_msg_queue_id{10};
        hash_table.insert({parsed_msg_queue_id, &parsed_msg_queue});

        serial_route_setup_type setup;
        setup.queue_to_read = &serial_message_buffer.queue_instance();
        setup.hash_to_write = &hash_table;
        serial_route_type route{setup};
        REQUIRE(route);

        SECTION("Route message")
        {
            constexpr std::string_view test_message{"Hello world"};
            SECTION("If valid key ID")
            {
                stv::head_route::head_route_setup_t route_setup{
                    .dst_id  = parsed_msg_queue_id,
                    .pack_id = 0,
                };

                {
                    auto msg = serial_message_buffer.request(test_message,
                                                             route_setup);
                }

                REQUIRE(route.compute());
                decltype(auto) queue_to_check =
                    hash_table.at(parsed_msg_queue_id);
                REQUIRE_FALSE(queue_to_check->empty());
                auto msg = queue_to_check->front();
                msg.trim_head(stv::head_route::header_size());
                REQUIRE(
                    memcmp(test_message.data(), msg.data(), test_message.size())
                    == 0);
                queue_to_check->pop();
            }
        }
    }
}

// NOLINTEND(*-magic-numbers, google-build-using-namespace,
// readability-function-cognitive-complexity,
// cppcoreguidelines-avoid-non-const-global-variables)
