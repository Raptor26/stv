/// @file test_simbuff.cpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.

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

        const stv::sim_buff buffer;
        REQUIRE_FALSE(buffer.data());
    }

    SECTION("Trim")
    {
        const std::string_view test_msg{"Hello World!"};

        stv::sim_buff          buffer{test_msg.size()};
        std::copy(test_msg.begin(), test_msg.end(),
                  reinterpret_cast<char *>(buffer.begin()));

        REQUIRE(test_msg.size() == buffer.size());
        REQUIRE(std::memcmp(buffer.data(), test_msg.data(), buffer.size())
                == 0);

        SECTION("Head")
        {
            const std::string_view test_msg_after_first_trim_head{
                "ello World!"};
            buffer.trim_head(1);
            REQUIRE(test_msg_after_first_trim_head.size() == buffer.size());
            REQUIRE(std::memcmp(buffer.data(),
                                test_msg_after_first_trim_head.data(),
                                buffer.size())
                    == 0);

            const std::string_view test_msg_after_second_trim_head{"World!"};
            buffer.trim_head(5);
            REQUIRE(test_msg_after_second_trim_head.size() == buffer.size());
            REQUIRE(std::memcmp(buffer.data(),
                                test_msg_after_second_trim_head.data(),
                                buffer.size())
                    == 0);
        }

        SECTION("Tail")
        {
            const std::string_view test_msg_after_first_trim_tail{
                "Hello World"};
            buffer.trim_tail(1);
            REQUIRE(test_msg_after_first_trim_tail.size() == buffer.size());
            REQUIRE(std::memcmp(buffer.data(),
                                test_msg_after_first_trim_tail.data(),
                                buffer.size())
                    == 0);

            const std::string_view test_msg_after_second_trim_tail{"Hello"};
            buffer.trim_tail(6);
            REQUIRE(test_msg_after_second_trim_tail.size() == buffer.size());
            REQUIRE(std::memcmp(buffer.data(),
                                test_msg_after_second_trim_tail.data(),
                                buffer.size())
                    == 0);
        }
    }
}
