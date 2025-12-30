/// @file test_qma6100_regs.cpp
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

#include "stv/drivers/qma6100_regs.hpp"
#include <bitset>
#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

// NOLINTBEGIN(*-magic-numbers, google-build-using-namespace,
// readability-function-cognitive-,
// cppcoreguidelines-avoid-non-const-global-variables)

TEST_CASE(
    "qma6100 regs", "[qma6100][drivers]")
{
    using namespace stv;
    using reg_bitset = std::bitset<8>;

    SECTION("BW")
    {
        stv::qma6100_bw_reg reg;
        SECTION("BW")
        {
            SECTION("BW == mclk_512_977_hz")
            {
                reg.bw = stv::qma6100_bw_reg::bw_t::mclk_512_977_hz;
                constexpr reg_bitset expect_reg_val{std::string{"00000000"}};
                REQUIRE(static_cast<qma6100_reg_type>(reg)
                        == static_cast<qma6100_reg_type>(
                            expect_reg_val.to_ulong()));
            }

            SECTION("BW == mclk_32_15625_hz")
            {
                reg.bw = stv::qma6100_bw_reg::bw_t::mclk_32_15625_hz;
                constexpr reg_bitset expect_reg_val{std::string{"00000100"}};
                REQUIRE(static_cast<qma6100_reg_type>(reg)
                        == static_cast<qma6100_reg_type>(
                            expect_reg_val.to_ulong()));
            }

            SECTION("BW == mclk_4096_122_hz")
            {
                reg.bw = stv::qma6100_bw_reg::bw_t::mclk_4096_122_hz;
                constexpr reg_bitset expect_reg_val{std::string{"00000111"}};
                REQUIRE(static_cast<qma6100_reg_type>(reg)
                        == static_cast<qma6100_reg_type>(
                            expect_reg_val.to_ulong()));
            }
        }

        SECTION("NLPF")
        {
            SECTION("no_average")
            {
                reg.nlpf = stv::qma6100_bw_reg::nlpf_t::no_average;
                constexpr reg_bitset expect_reg_val{std::string{"00000000"}};
                REQUIRE(static_cast<qma6100_reg_type>(reg)
                        == static_cast<qma6100_reg_type>(
                            expect_reg_val.to_ulong()));
            }

            SECTION("average_16")
            {
                reg.nlpf = stv::qma6100_bw_reg::nlpf_t::average_16;
                constexpr reg_bitset expect_reg_val{std::string{"01100000"}};
                REQUIRE(static_cast<qma6100_reg_type>(reg)
                        == static_cast<qma6100_reg_type>(
                            expect_reg_val.to_ulong()));
            }
        }

        SECTION("Ctor")
        {
            constexpr reg_bitset expect_reg_val{std::string{"01000110"}};
            stv::qma6100_bw_reg  reg_ctor{
                stv::qma6100_reg_type{expect_reg_val.to_ulong()}};
            REQUIRE(reg_ctor.nlpf == qma6100_bw_reg::nlpf_t::average_4);
            REQUIRE(reg_ctor.bw == qma6100_bw_reg::bw_t::mclk_2048_244_hz);
        }
    }

    SECTION("FSR")
    {
        stv::qma6100_fsr_reg reg;

        SECTION("RANGE")
        {
            SECTION("RANGE == g_2")
            {
                reg.range = stv::qma6100_fsr_reg::range_t::g_2;
                constexpr reg_bitset expect_reg_val{std::string{"00000001"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }

            SECTION("RANGE == g_4")
            {
                reg.range = stv::qma6100_fsr_reg::range_t::g_4;
                constexpr reg_bitset expect_reg_val{std::string{"00000010"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }

            SECTION("RANGE == g_8")
            {
                reg.range = stv::qma6100_fsr_reg::range_t::g_8;
                constexpr reg_bitset expect_reg_val{std::string{"00000100"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }

            SECTION("RANGE == g_16")
            {
                reg.range = stv::qma6100_fsr_reg::range_t::g_16;
                constexpr reg_bitset expect_reg_val{std::string{"00001000"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }

            SECTION("RANGE == g_32")
            {
                reg.range = stv::qma6100_fsr_reg::range_t::g_32;
                constexpr reg_bitset expect_reg_val{std::string{"00001111"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }
        }
    }
}

// NOLINTEND(*-magic-numbers, google-build-using-namespace,
// readability-function-cognitive-complexity,
// cppcoreguidelines-avoid-non-const-global-variables)
