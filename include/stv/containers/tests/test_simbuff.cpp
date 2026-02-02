/// @file test_simbuff.cpp
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

#include "stv/containers/lwrb.hpp"
#include "stv/containers/simbuff.hpp"
#include <algorithm>
#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <cstring>
#include <mutex>
#include <string>
#include <string_view>

TEST_CASE(
    "sumbuff", "[stv]")
{
    SECTION("Default Ctor")
    {
        REQUIRE_FALSE(stv::sim_buff{});

        stv::sim_buff buffer;
        REQUIRE_FALSE(buffer.data());
    }

    SECTION("Trim")
    {
        std::string_view test_msg{"Hello World!"};

        stv::sim_buff    buffer{test_msg.size()};
        std::copy(test_msg.begin(), test_msg.end(),
                  reinterpret_cast<char *>(buffer.begin()));

        REQUIRE(test_msg.size() == buffer.size());
        REQUIRE(std::memcmp(buffer.data(), test_msg.data(), buffer.size())
                == 0);

        SECTION("Head")
        {
            std::string_view test_msg_after_first_trim_head{"ello World!"};
            buffer.trim_head(1);
            REQUIRE(test_msg_after_first_trim_head.size() == buffer.size());
            REQUIRE(std::memcmp(buffer.data(),
                                test_msg_after_first_trim_head.data(),
                                buffer.size())
                    == 0);

            std::string_view test_msg_after_second_trim_head{"World!"};
            buffer.trim_head(5);
            REQUIRE(test_msg_after_second_trim_head.size() == buffer.size());
            REQUIRE(std::memcmp(buffer.data(),
                                test_msg_after_second_trim_head.data(),
                                buffer.size())
                    == 0);
        }

        SECTION("Tail")
        {
            std::string_view test_msg_after_first_trim_tail{"Hello World"};
            buffer.trim_tail(1);
            REQUIRE(test_msg_after_first_trim_tail.size() == buffer.size());
            REQUIRE(std::memcmp(buffer.data(),
                                test_msg_after_first_trim_tail.data(),
                                buffer.size())
                    == 0);

            std::string_view test_msg_after_second_trim_tail{"Hello"};
            buffer.trim_tail(6);
            REQUIRE(test_msg_after_second_trim_tail.size() == buffer.size());
            REQUIRE(std::memcmp(buffer.data(),
                                test_msg_after_second_trim_tail.data(),
                                buffer.size())
                    == 0);
        }
    }
}
