/// @file lwrb.cpp
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
    REQUIRE(buff.get_free() == buffer_size - 1);

    SECTION("Check write with overflow")
    {
        constexpr std::string_view str{"Hello World! ABCDEFGHIJF"};
        REQUIRE(buff.write(str) == 0);
        REQUIRE(buff.get_full() == 0);

        REQUIRE(buff.write(str, false) == buff.capacity());
        REQUIRE(buff.get_full() == buff.capacity());
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
            std::array<std::byte, buffer_size> dst{};
            REQUIRE(buff.read(std::span<std::byte>{dst.data(), dst.size()})
                    == str.size());
            REQUIRE(std::strcmp(reinterpret_cast<const char *>(dst.data()),
                                str.data())
                    == 0);
        }

        SECTION("Read like span in zero buffer")
        {
            std::array<std::byte, 0> dst{};
            REQUIRE(buff.read(std::span<std::byte>{dst.data(), dst.size()})
                    == 0);
        }
    }
}

// NOLINTEND(*-magic-numbers, google-build-using-namespace,
// readability-function-cognitive-complexity,
// cppcoreguidelines-avoid-non-const-global-variables)
