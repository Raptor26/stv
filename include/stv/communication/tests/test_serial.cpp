/// @file test_serial.cpp
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
#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <etl/queue.h>
#include <iostream>
#include <queue>

// NOLINTBEGIN(*-magic-numbers, google-build-using-namespace,
// readability-function-cognitive-,
// cppcoreguidelines-avoid-non-const-global-variables)

std::array<std::byte, 64> memory;
static int                alloc_cnt;

template<typename T>
class CustomAllocator
{
  public:
    using value_type = T;

    CustomAllocator() {}

    template<typename U>
    CustomAllocator(
        const CustomAllocator<U> &)
    {
    }

    T *allocate(
        std::size_t n)
    {
        (void)n;
        ++alloc_cnt;
        return memory.data();
    }

    void deallocate(
        T *p, std::size_t n)
    {
        (void)p;
        (void)n;
        --alloc_cnt;
    }

    static auto           get_allocator_cnt() { return alloc_cnt; }

    static constexpr auto GetMemoryPtr() { return memory.data(); }
};

SCENARIO(
    "Serial", "[stv][serial]")
{
    using namespace stv;

    using custom_allocator = CustomAllocator<std::byte>;
    using sim_buffer_type  = stv::sim_buff<stv::empty_mutex, custom_allocator>;
    using queue_type       = etl::queue<sim_buffer_type, 10>;

    STV_NO_PADDING_NO_OPTIMIZE_BEGIN

    struct user_data_t {
        std::uint8_t i{11};
        std::uint8_t j{22};
        std::uint8_t k{33};
        std::uint8_t z{44};
    };

    STV_NO_PADDING_NO_OPTIMIZE_END

    REQUIRE(custom_allocator::get_allocator_cnt() == 0);

    GIVEN("raw data without any decorators")
    {
        auto serial_message_buffer = make_serial_message_buffer<queue_type>(
            stv::empty_serial_decorator{});

        WHEN("Check containers")
        {
            THEN("array")
            {
                {
                    const std::array<std::uint16_t, 3> pload{11, 22, 33};
                    {
                        auto msg = serial_message_buffer.request(pload);
                    }

                    std::memcmp(pload.data(), custom_allocator::GetMemoryPtr(),
                                pload.size());
                }
            }

            THEN("vector")
            {
                {
                    const std::vector<std::uint16_t> pload{22, 33, 44};
                    {
                        auto msg = serial_message_buffer.request(pload);
                    }

                    std::memcmp(pload.data(), custom_allocator::GetMemoryPtr(),
                                pload.size());
                }
            }
        }

        WHEN("Check strings")
        {
            std::fill(memory.begin(), memory.end(), std::byte(0));

            THEN("char *")
            {
                const char *pload{"char string"};
                {
                    auto msg = serial_message_buffer.request(pload);
                }

                REQUIRE(std::strcmp(reinterpret_cast<char *>(
                                        custom_allocator::GetMemoryPtr()),
                                    pload)
                        == 0);
            }

            THEN("std::string")
            {
                const std::string pload{"Hello World!"};
                {
                    auto msg = serial_message_buffer.request(pload);
                }
                REQUIRE(std::strcmp(reinterpret_cast<char *>(
                                        custom_allocator::GetMemoryPtr()),
                                    pload.c_str())
                        == 0);
            }

            THEN("char * in place")
            {
                {
                    auto msg = serial_message_buffer.request("raw string");
                }

                REQUIRE(std::strcmp(reinterpret_cast<char *>(
                                        custom_allocator::GetMemoryPtr()),
                                    "raw string")
                        == 0);
            }
        }
    }

    GIVEN("Serial message buffer with route only")
    {
        auto serial_message_buffer =
            make_serial_message_buffer<queue_type>(stv::head_route{});

        std::fill(memory.begin(), memory.end(), std::byte(0));

        const stv::head_route::head_route_setup_t route_setup{.dst_id  = 111,
                                                              .pack_id = 222};

        const char                               *pload{"char string"};
        const std::string_view                    pload_view{"char string"};

        constexpr std::size_t pload_offset{stv::head_route::header_size()};
        constexpr std::size_t route_offset{0};

        constexpr std::byte  *pload_addr{custom_allocator::GetMemoryPtr()
                                        + pload_offset};

        constexpr std::byte  *route_addr{custom_allocator::GetMemoryPtr()
                                        + route_offset};

        WHEN("Check strings")
        {
            THEN("string view")
            {
                {
                    auto msg =
                        serial_message_buffer.request(pload_view, route_setup);
                }

                REQUIRE(std::strcmp(reinterpret_cast<char *>(pload_addr), pload)
                        == 0);
            }

            THEN("char *")
            {
                {
                    auto msg =
                        serial_message_buffer.request(pload, route_setup);
                }

                REQUIRE(std::strcmp(reinterpret_cast<char *>(pload_addr), pload)
                        == 0);
            }

            const auto *route = reinterpret_cast<
                stv::head_route::head_route_setup_with_pload_t *>(route_addr);
            REQUIRE(route->dst_id == 111);
            REQUIRE(route->pack_id == 222);

            REQUIRE(route->pload_size == std::strlen(pload));
        }

        WHEN("Check containers")
        {
            THEN("array")
            {
                const std::array<std::uint16_t, 3> container_pload{11, 22, 33};
                {
                    auto msg = serial_message_buffer.request(container_pload,
                                                             route_setup);
                }

                std::memcmp(container_pload.data(), pload_addr,
                            container_pload.size());

                const auto *route = reinterpret_cast<
                    stv::head_route::head_route_setup_with_pload_t *>(
                    route_addr);
                REQUIRE(route->pload_size
                        == container_pload.size()
                               * sizeof(decltype(container_pload)::value_type));
            }

            const auto *route = reinterpret_cast<
                stv::head_route::head_route_setup_with_pload_t *>(route_addr);
            REQUIRE(route->dst_id == 111);
            REQUIRE(route->pack_id == 222);
        }
    }

    GIVEN("Serial message buffer with route")
    {
        auto serial_message_buffer = make_serial_message_buffer<queue_type>(
            start_frame_and_crc_16{}, stv::head_route{});

        std::fill(memory.begin(), memory.end(), std::byte(0));

        THEN("Create custom message with route")
        {
            stv::head_route::head_route_setup_t route_setup{.dst_id  = 111,
                                                            .pack_id = 222};

            auto msg = serial_message_buffer.request<user_data_t>(route_setup);
            REQUIRE(msg);

            msg->i = 11;
            msg->j = 22;
            msg->k = 33;
            msg->z = 44;

            // Заголовок и конец сообщения будут заполнены в деструкторе msg.
        }

        THEN("Create custom message with route and copy")
        {
            stv::head_route::head_route_setup_t route_setup{.dst_id  = 111,
                                                            .pack_id = 222};

            //
            auto user_data = user_data_t{};
            auto msg = serial_message_buffer.request(user_data, route_setup);
            REQUIRE(msg);

            // Заголовок и конец сообщения будут заполнены в деструкторе msg.
        }

        THEN("Create custom message with route and copy with rvalue")
        {
            stv::head_route::head_route_setup_t route_setup{.dst_id  = 111,
                                                            .pack_id = 222};

            //
            auto msg =
                serial_message_buffer.request(user_data_t{}, route_setup);
            REQUIRE(msg);

            // Заголовок и конец сообщения будут заполнены в деструкторе msg.
        }

        REQUIRE(custom_allocator::get_allocator_cnt() > 0);

        // Проверка маршрутизации. ---------------------------------------------
        const auto *route =
            // NOLINTNEXTLINE(*-reinterpret-cast)
            reinterpret_cast<stv::head_route::head_route_setup_with_pload_t *>(
                custom_allocator::GetMemoryPtr()
                + start_frame_and_crc_16::header_size());

        REQUIRE(route->dst_id == 111);
        REQUIRE(route->pack_id == 222);
        REQUIRE(route->pload_size == sizeof(user_data_t));

        // Проверка полезной нагрузки.
        // -----------------------------------------
        auto *pload = reinterpret_cast<user_data_t *>(
            custom_allocator::GetMemoryPtr()
            + start_frame_and_crc_16::header_size()
            + stv::head_route::header_size());

        REQUIRE(pload->i == 11);
        REQUIRE(pload->j == 22);
        REQUIRE(pload->k == 33);
        REQUIRE(pload->z == 44);

        // Проверка заголовка. -------------------------------------------------
        auto *head =
            reinterpret_cast<stv::start_frame_and_crc_16::start_frame_t *>(
                custom_allocator::GetMemoryPtr());

        REQUIRE(head->start_frame_first
                == stv::start_frame_and_crc_16::FIRST_BYTE);

        REQUIRE(head->start_frame_second
                == stv::start_frame_and_crc_16::SECOND_BYTE);

        REQUIRE(head->frame_size
                == sizeof(user_data_t) + start_frame_and_crc_16::trailer_size()
                       + stv::head_route::trailer_size()
                       + stv::head_route::header_size());
    }

    REQUIRE(custom_allocator::get_allocator_cnt() == 0);
}

// NOLINTNEXTLINE(readability-avoid-unconditional-preprocessor-if)
#if 0
TEST_CASE(
    "RND", "[stv][serial]")

{
    using namespace stv;

    using sim_buffer_type =
        stv::sim_buff<stv::empty_mutex, CustomAllocator<std::byte>>;
    using queue_type = etl::queue<sim_buffer_type, 10>;

    struct user_data_t {
        std::uint8_t i{11};
        std::uint8_t j{22};
        std::uint8_t k{33};
        std::uint8_t z{44};
    };

    class start_frame_and_crc_16_test
    {
        using start_frame_type = std::uint16_t;
        using frame_size_type  = std::uint16_t;
        using crc_type         = std::uint16_t;

      public:
        STV_NO_PADDING_NO_OPTIMIZE_BEGIN

        struct start_frame_and_crc_16_setup_t {
            crc_type poly{0xABCD};
        };

        STV_NO_PADDING_NO_OPTIMIZE_END

        start_frame_and_crc_16_test(
            const start_frame_and_crc_16_setup_t &setup =
                start_frame_and_crc_16_setup_t{0xABCD}):
            setup_{setup}
        {
        }

        static constexpr size_t header_size()
        {
            return sizeof(start_frame_type) + sizeof(frame_size_type);
        }

        static constexpr size_t trailer_size() { return sizeof(crc_type); }

        /// @brief
        /// @param buffer
        /// @param payload_size
        void setup_header(
            std::byte *const dst, const std::span<const std::byte> &total,
            const std::span<const std::byte> &pload) const
        {
            (void)total;
            (void)pload;
            dst[0] = static_cast<std::byte>(0xAA); // Start byte 1
            dst[1] = static_cast<std::byte>(0xAA); // Start byte 2
            auto *total_size_ptr{reinterpret_cast<frame_size_type *>(&dst[2])};
            *total_size_ptr = static_cast<frame_size_type>(total.size_bytes()
                                                           - header_size());
        }

        void setup_trailer(
            std::byte *dst, std::span<const std::byte> total) const
        {
            (void)total;
            crc_type *crc = reinterpret_cast<crc_type *>(&dst[0]);
            *crc          = setup_.poly;
        }

      private:
        start_frame_and_crc_16_setup_t setup_;
    };

    SECTION("Ctor")
    {
        auto serial_message_buffer = make_serial_message_buffer<queue_type>(
            start_frame_and_crc_16{}, stv::head_route{});

        {
            stv::head_route::head_route_setup_t route_setup{.dst_id  = 111,
                                                            .pack_id = 222};
            auto msg = serial_message_buffer.request<user_data_t>(route_setup);
            REQUIRE(msg);

            msg->i = 11;
            msg->j = 22;
            msg->k = 33;
            msg->z = 44;
        }
        {
            auto msg = serial_message_buffer.request<user_data_t>(
                start_frame_and_crc_16_test::start_frame_and_crc_16_setup_t{});
            REQUIRE(msg);
            auto size = sizeof(msg);
            (void)msg;
            (void)size;
            auto buffer_size = sizeof(sim_buffer_type);
            (void)buffer_size;

            msg->i = 1;
            msg->j = 2;
            msg->k = 3;
        }

        {
            auto msg = serial_message_buffer.request(
                user_data_t{},
                start_frame_and_crc_16_test::start_frame_and_crc_16_setup_t{
                    0x1234});
            REQUIRE(msg);
            auto size = sizeof(msg);
            (void)msg;
            (void)size;
            auto buffer_size = sizeof(sim_buffer_type);
            (void)buffer_size;
        }

        {
            auto msg  = serial_message_buffer.request<user_data_t>();
            auto size = sizeof(msg);
            (void)msg;
            (void)size;
            auto buffer_size = sizeof(sim_buffer_type);
            (void)buffer_size;

            msg->i = 1;
            msg->j = 2;
            msg->z = 4;
        }

        {
            std::string pload{"Hello World!"};
            auto        msg = serial_message_buffer.request(pload);
        }

        {
            const char *pload{"char string"};
            auto        msg = serial_message_buffer.request(pload);
        }

        {
            auto msg = serial_message_buffer.request("raw string");
        }

        {
            std::array<std::uint16_t, 3> pload{11, 22, 33};
            auto                         msg =
                serial_message_buffer.request(pload.cbegin(), pload.cend());
        }

        {
            std::array<float, 3> pload{11.1, 22.2, 33.3};
            auto                 msg =
                serial_message_buffer.request(pload.cbegin(), pload.cend());
        }

        {
            std::vector<float> pload{11.1, 22.2, 33.3};
            auto               msg = serial_message_buffer.request(pload);
        }

        // Dtor serial_message_buffer освободит всю память.
    }
}
#endif

// NOLINTEND(*-magic-numbers, google-build-using-namespace,
// readability-function-cognitive-complexity,
// cppcoreguidelines-avoid-non-const-global-variables)
