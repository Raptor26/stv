/// @file test_lwrb.cpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.

#include "stv/containers/lwrb.hpp"
#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <mutex>
#include <string_view>

// NOLINTBEGIN(*-magic-numbers, google-build-using-namespace,
// readability-function-cognitive-,
// cppcoreguidelines-avoid-non-const-global-variables)

TEMPLATE_PRODUCT_TEST_CASE(
    "lwrb", "[stv][containers]", (stv::lwrb_setup),
    ((stv::empty_mutex), (stv::empty_mutex *), (std::recursive_mutex),
     (std::recursive_mutex *)))
{
    using setup_type     = TestType;
    using lwrb_base_type = stv::lwrb_base<setup_type>;
    constexpr std::size_t buffer_size{20};

    std::recursive_mutex  std_mutex{};
    (void)std_mutex;

    stv::empty_mutex empty_mutex;
    (void)empty_mutex;

    // NOLINTNEXTLINE(misc-const-correctness)
    setup_type attr;

    if constexpr(std::is_same_v<typename setup_type::mutex_type,
                                std::recursive_mutex *>)
    {
        attr.mutex = &std_mutex;
    }

    if constexpr(std::is_same_v<typename setup_type::mutex_type,
                                stv::empty_mutex *>)
    {
        attr.mutex = &empty_mutex;
    }

    stv::lwrb<lwrb_base_type, buffer_size> buff{attr};
    REQUIRE(buff.get_free() == buffer_size);

    SECTION("Check write with overflow")
    {
        constexpr std::string_view str{"Hello World! ABCDEFGHIJF"};
        REQUIRE(buff.write(str) == 0);
        REQUIRE(buff.get_full() == 0);

        const auto     real_capacity = buff.capacity() - 1;
        constexpr auto write_all_or_nothing{false};
        REQUIRE(buff.write(str, write_all_or_nothing) == real_capacity);
        REQUIRE(buff.get_full() == real_capacity);
    }

    SECTION("Check write/read")
    {
        constexpr std::string_view str{"Hello World"};

        SECTION("Write as_ c-style string")
        {
            REQUIRE(buff.write(str.data()) == str.size());
            REQUIRE(buff.get_full() == str.size());
            buff.reset();
        }

        REQUIRE(buff.write(str) == str.size());
        REQUIRE(buff.get_full() == str.size());
        REQUIRE_FALSE(buff.is_empty());

        SECTION("Read linear addr")
        {
            {
                const auto linear_addr_in_buff = buff.read_linear_addr();
                REQUIRE(std::strcmp(reinterpret_cast<const char *>(
                                        linear_addr_in_buff.data()),
                                    str.data())
                        == 0);
            }
            REQUIRE(buff.is_empty());
        }

        SECTION("Skip")
        {
            const auto linear_addr_in_buff = buff.get_linear_addr();
            REQUIRE(std::strcmp(reinterpret_cast<const char *>(
                                    linear_addr_in_buff.data()),
                                str.data())
                    == 0);

            REQUIRE_FALSE(buff.is_empty());

            REQUIRE(buff.skip(linear_addr_in_buff)
                    == linear_addr_in_buff.size_bytes());
            REQUIRE(buff.is_empty());
        }

        SECTION("Read like std::array")
        {
            std::array<std::byte, buffer_size> dst{};
            REQUIRE(buff.read(dst) == str.size());
            REQUIRE(std::strcmp(reinterpret_cast<const char *>(dst.data()),
                                str.data())
                    == 0);
        }

        SECTION("Read like std::span")
        {
            std::array<typename lwrb_base_type::value_type, buffer_size> dst{};
            REQUIRE(buff.read(std::span<typename lwrb_base_type::value_type>{
                        dst.data(), dst.size()})
                    == str.size());
            REQUIRE(std::strcmp(reinterpret_cast<const char *>(dst.data()),
                                str.data())
                    == 0);
        }

        SECTION("Read like span in zero buffer")
        {
            std::array<typename lwrb_base_type::value_type, 0> dst{};
            REQUIRE(buff.read(std::span<typename lwrb_base_type::value_type>{
                        dst.data(), dst.size()})
                    == 0);
        }
    }
}

// NOLINTEND(*-magic-numbers, google-build-using-namespace,
// readability-function-cognitive-complexity,
// cppcoreguidelines-avoid-non-const-global-variables)
