/// @file test_serial_sender.cpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.

#include "stv/communication/serial_sender.hpp"
#include "stv/containers/simbuff.hpp"
#include "stv/mutex_guard.hpp"
#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <cstddef>
#include <cstring>
#include <etl/queue.h>
#include <iostream>
#include <memory>
#include <queue>
#include <span>
#include <type_traits>
#include <variant>

// NOLINTBEGIN(*-magic-numbers, google-build-using-namespace,
// readability-function-cognitive-,
// cppcoreguidelines-avoid-non-const-global-variables)

static int alloc_cnt;

template<typename T>
class custom_allocator
{
  public:
    using value_type = T;

    custom_allocator() = default;

    template<typename U>
    custom_allocator(
        const custom_allocator<U> &)
    {
    }

    T *allocate(
        std::size_t n)
    {
        ++alloc_cnt;
        T *p = std::allocator<T>{}.allocate(n);
        std::memset(p, 0, n * sizeof(T));
        last_ptr_ = reinterpret_cast<std::byte *>(p);
        return p;
    }

    void deallocate(
        T *p, std::size_t n)
    {
        std::allocator<T>{}.deallocate(p, n);
        --alloc_cnt;
        last_ptr_ = nullptr;
    }

    static auto get_allocator_cnt() { return alloc_cnt; }

    static auto get_mem_ptr() { return last_ptr_; }

  private:
    inline static std::byte *last_ptr_{nullptr};
};

SCENARIO(
    "Serial", "[stv][serial]")
{
    using namespace stv;

    using custom_allocator = custom_allocator<std::byte>;
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
        queue_type queue{};
        auto       serial_message_buffer =
            make_serial_message_buffer(queue, stv::empty_serial_decorator{});

        WHEN("Request empty span")
        {
            char symb{'h'};
            serial_message_buffer.request(std::span(&symb, 0));

            THEN("Nothing placed to queue")
            {
                decltype(auto) queue_instance =
                    serial_message_buffer.queue_instance();
                REQUIRE(queue_instance.size() == 0);
            }
        }
        WHEN("Check containers")
        {
            THEN("array")
            {
                {
                    const std::array<std::uint16_t, 3> pload{11, 22, 33};
                    {
                        auto msg = serial_message_buffer.request(pload);
                    }

                    std::memcmp(pload.data(), custom_allocator::get_mem_ptr(),
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

                    std::memcmp(pload.data(), custom_allocator::get_mem_ptr(),
                                pload.size());
                }
            }
        }

        WHEN("Check strings")
        {
            THEN("char *")
            {
                const char *pload{"char string"};
                {
                    auto msg = serial_message_buffer.request(pload);
                }

                REQUIRE(std::memcmp(reinterpret_cast<char *>(
                                        custom_allocator::get_mem_ptr()),
                                    pload, std::strlen(pload))
                        == 0);
            }

            THEN("std::string")
            {
                const std::string pload{"Hello World!"};
                {
                    auto msg = serial_message_buffer.request(pload);
                }
                REQUIRE(std::memcmp(reinterpret_cast<char *>(
                                        custom_allocator::get_mem_ptr()),
                                    pload.c_str(), pload.size())
                        == 0);
            }

            THEN("char * in place")
            {
                {
                    auto msg = serial_message_buffer.request("raw string");
                }

                REQUIRE(std::memcmp(reinterpret_cast<char *>(
                                        custom_allocator::get_mem_ptr()),
                                    "raw string", 10)
                        == 0);
            }
        }

        WHEN("Check span and iterator overloads")
        {
            std::array<std::byte, 4> source{std::byte{0x01}, std::byte{0x02},
                                            std::byte{0x03}, std::byte{0x04}};

            THEN("request from iterators allows mutation")
            {
                auto msg = serial_message_buffer.request(source.cbegin(),
                                                         source.cend());
                REQUIRE(msg);
                msg.operator->()[0] = std::byte{0xAB};
                msg.operator->()[1] = std::byte{0xCD};
                msg.operator->()[2] = std::byte{0xEF};
                msg.operator->()[3] = std::byte{0x00};
            }

            THEN("request from span allows mutation")
            {
                auto msg = serial_message_buffer.request(
                    std::span<const std::byte>{source});
                REQUIRE(msg);
                msg.operator->()[0] = std::byte{0xAB};
                msg.operator->()[1] = std::byte{0xCD};
                msg.operator->()[2] = std::byte{0xEF};
                msg.operator->()[3] = std::byte{0x00};
            }
        }
    }

    GIVEN("Serial message buffer with route only")
    {
        queue_type queue{};
        auto       serial_message_buffer =
            make_serial_message_buffer(queue, stv::head_route{});

        const stv::head_route::head_route_setup_t route_setup{.dst_id  = 111,
                                                              .pack_id = 222};

        const char                               *pload{"char string"};
        const std::string_view                    pload_view{"char string"};

        constexpr std::size_t pload_offset{stv::head_route::header_size()};
        constexpr std::size_t route_offset{0};

        WHEN("Check strings")
        {
            THEN("string view")
            {
                {
                    auto msg =
                        serial_message_buffer.request(pload_view, route_setup);
                }

                REQUIRE(std::memcmp(
                            reinterpret_cast<const char *>(
                                custom_allocator::get_mem_ptr() + pload_offset),
                            pload, pload_view.size())
                        == 0);
            }

            THEN("char *")
            {
                {
                    auto msg =
                        serial_message_buffer.request(pload, route_setup);
                }

                REQUIRE(std::memcmp(
                            reinterpret_cast<const char *>(
                                custom_allocator::get_mem_ptr() + pload_offset),
                            pload, std::strlen(pload))
                        == 0);
            }

            const auto *route = reinterpret_cast<
                const stv::head_route::head_route_setup_with_pload_t *>(
                custom_allocator::get_mem_ptr() + route_offset);
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

                REQUIRE(std::memcmp(
                            container_pload.data(),
                            custom_allocator::get_mem_ptr() + pload_offset,
                            container_pload.size()
                                * sizeof(decltype(container_pload)::value_type))
                        == 0);

                const auto *route = reinterpret_cast<
                    const stv::head_route::head_route_setup_with_pload_t *>(
                    custom_allocator::get_mem_ptr() + route_offset);
                REQUIRE(route->pload_size
                        == container_pload.size()
                               * sizeof(decltype(container_pload)::value_type));
            }

            const auto *route = reinterpret_cast<
                const stv::head_route::head_route_setup_with_pload_t *>(
                custom_allocator::get_mem_ptr() + route_offset);
            REQUIRE(route->dst_id == 111);
            REQUIRE(route->pack_id == 222);
        }
    }
    GIVEN("Serial message buffer with route")
    {
        queue_type queue{};
        auto       serial_message_buffer = make_serial_message_buffer(
            queue, start_frame_and_crc_16{}, stv::head_route{});

        THEN("Send message without pload")
        {
            auto msg = serial_message_buffer.request(
                static_cast<std::size_t>(0),
                stv::head_route::head_route_setup_t{.dst_id  = 111,
                                                    .pack_id = 222});
            REQUIRE(msg);
        }

        // Проверка маршрутизации. ---------------------------------------------
        const auto *route =
            // NOLINTNEXTLINE(*-reinterpret-cast)
            reinterpret_cast<stv::head_route::head_route_setup_with_pload_t *>(
                custom_allocator::get_mem_ptr()
                + start_frame_and_crc_16::header_size());

        REQUIRE(route->dst_id == 111);
        REQUIRE(route->pack_id == 222);
        REQUIRE(route->pload_size == 0);
    }

    GIVEN("Serial message buffer with route")
    {
        queue_type queue{};
        auto       serial_message_buffer = make_serial_message_buffer(
            queue, start_frame_and_crc_16{}, stv::head_route{});

        THEN("Create custom message with route")
        {
            auto msg = serial_message_buffer.request<user_data_t>(
                stv::head_route::head_route_setup_t{.dst_id  = 111,
                                                    .pack_id = 222});
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
                custom_allocator::get_mem_ptr()
                + start_frame_and_crc_16::header_size());

        REQUIRE(route->dst_id == 111);
        REQUIRE(route->pack_id == 222);
        REQUIRE(route->pload_size == sizeof(user_data_t));

        // Проверка полезной нагрузки.
        // -----------------------------------------
        auto *pload = reinterpret_cast<user_data_t *>(
            custom_allocator::get_mem_ptr()
            + start_frame_and_crc_16::header_size()
            + stv::head_route::header_size());

        REQUIRE(pload->i == 11);
        REQUIRE(pload->j == 22);
        REQUIRE(pload->k == 33);
        REQUIRE(pload->z == 44);

        // Проверка заголовка. -------------------------------------------------
        auto *head =
            reinterpret_cast<stv::start_frame_and_crc_16::start_frame_t *>(
                custom_allocator::get_mem_ptr());

        REQUIRE(head->start_frame_first
                == stv::start_frame_and_crc_16::first_byte);

        REQUIRE(head->start_frame_second
                == stv::start_frame_and_crc_16::second_byte);

        REQUIRE(head->frame_size
                == sizeof(user_data_t) + start_frame_and_crc_16::trailer_size()
                       + stv::head_route::trailer_size()
                       + stv::head_route::header_size());
    }

    REQUIRE(custom_allocator::get_allocator_cnt() == 0);
}

TEMPLATE_TEST_CASE(
    "Serial container request preserves value type mutability", "[stv][serial]",
    std::byte, std::int16_t, std::uint32_t, float)
{
    using namespace stv;

    using custom_allocator = custom_allocator<std::byte>;
    using sim_buffer_type  = stv::sim_buff<stv::empty_mutex, custom_allocator>;
    using queue_type       = etl::queue<sim_buffer_type, 10>;

    queue_type queue{};
    auto       serial_message_buffer =
        make_serial_message_buffer(queue, stv::empty_serial_decorator{});

    std::array<TestType, 4> source{
        static_cast<TestType>(1), static_cast<TestType>(2),
        static_cast<TestType>(3), static_cast<TestType>(4)};

    std::array<TestType, 4> expected{
        static_cast<TestType>(0xAB), static_cast<TestType>(0xCD),
        static_cast<TestType>(0xEF), static_cast<TestType>(0)};

    SECTION("from container")
    {
        auto msg = serial_message_buffer.request(source);
        REQUIRE(msg);
        REQUIRE(msg.pload_data().size_bytes()
                == sizeof(TestType) * source.size());

        msg.operator->()[0] = expected.at(0);
        msg.operator->()[1] = expected.at(1);
        msg.operator->()[2] = expected.at(2);
        msg.operator->()[3] = expected.at(3);
    }

    SECTION("from span")
    {
        auto msg =
            serial_message_buffer.request(std::span<const TestType>{source});
        REQUIRE(msg);
        REQUIRE(msg.pload_data().size_bytes()
                == sizeof(TestType) * source.size());

        msg.operator->()[0] = expected.at(0);
        msg.operator->()[1] = expected.at(1);
        msg.operator->()[2] = expected.at(2);
        msg.operator->()[3] = expected.at(3);
    }

    REQUIRE(std::memcmp(expected.data(),
                        queue.front().data()
                            + stv::empty_serial_decorator::header_size(),
                        sizeof(source))
            == 0);
}

// NOLINTNEXTLINE(readability-avoid-unconditional-preprocessor-if)
#if 0
TEST_CASE(
    "RND", "[stv][serial]")

{
    using namespace stv;

    using sim_buffer_type =
        stv::sim_buff<stv::empty_mutex, custom_allocator<std::byte>>;
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
        auto serial_message_buffer = make_serial_message_buffer(
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
