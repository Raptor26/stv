/// @file test_qma6100_regs.cpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.

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
                REQUIRE(static_cast<qma6100_reg_type>(reg)
                        == static_cast<qma6100_reg_type>(
                            expect_reg_val.to_ulong()));
            }

            SECTION("RANGE == g_4")
            {
                reg.range = stv::qma6100_fsr_reg::range_t::g_4;
                constexpr reg_bitset expect_reg_val{std::string{"00000010"}};
                REQUIRE(static_cast<qma6100_reg_type>(reg)
                        == static_cast<qma6100_reg_type>(
                            expect_reg_val.to_ulong()));
            }

            SECTION("RANGE == g_8")
            {
                reg.range = stv::qma6100_fsr_reg::range_t::g_8;
                constexpr reg_bitset expect_reg_val{std::string{"00000100"}};
                REQUIRE(static_cast<qma6100_reg_type>(reg)
                        == static_cast<qma6100_reg_type>(
                            expect_reg_val.to_ulong()));
            }

            SECTION("RANGE == g_16")
            {
                reg.range = stv::qma6100_fsr_reg::range_t::g_16;
                constexpr reg_bitset expect_reg_val{std::string{"00001000"}};
                REQUIRE(static_cast<qma6100_reg_type>(reg)
                        == static_cast<qma6100_reg_type>(
                            expect_reg_val.to_ulong()));
            }

            SECTION("RANGE == g_32")
            {
                reg.range = stv::qma6100_fsr_reg::range_t::g_32;
                constexpr reg_bitset expect_reg_val{std::string{"00001111"}};
                REQUIRE(static_cast<qma6100_reg_type>(reg)
                        == static_cast<qma6100_reg_type>(
                            expect_reg_val.to_ulong()));
            }
        }
    }

    SECTION("INT_EN1")
    {
        stv::qma6100_int_en1_reg reg;

        SECTION("NT_FWM_EN")
        {
            SECTION("NT_FWM_EN == disable")
            {
                reg.int_fwm_en = stv::qma6100_int_en1_reg::switcher_t::disable;
                constexpr reg_bitset expect_reg_val{std::string{"00000000"}};
                REQUIRE(static_cast<qma6100_reg_type>(reg)
                        == static_cast<qma6100_reg_type>(
                            expect_reg_val.to_ulong()));
            }

            SECTION("NT_FWM_EN == enable")
            {
                reg.int_fwm_en = stv::qma6100_int_en1_reg::switcher_t::enable;
                constexpr reg_bitset expect_reg_val{std::string{"01000000"}};
                REQUIRE(static_cast<qma6100_reg_type>(reg)
                        == static_cast<qma6100_reg_type>(
                            expect_reg_val.to_ulong()));
            }
        }

        SECTION("INT_FFULL_EN")
        {
            SECTION("INT_FFULL_EN == disable")
            {
                reg.int_ffull_en =
                    stv::qma6100_int_en1_reg::switcher_t::disable;
                constexpr reg_bitset expect_reg_val{std::string{"00000000"}};
                REQUIRE(static_cast<qma6100_reg_type>(reg)
                        == static_cast<qma6100_reg_type>(
                            expect_reg_val.to_ulong()));
            }

            SECTION("INT_FFULL_EN == enable")
            {
                reg.int_ffull_en = stv::qma6100_int_en1_reg::switcher_t::enable;
                constexpr reg_bitset expect_reg_val{std::string{"00100000"}};
                REQUIRE(static_cast<qma6100_reg_type>(reg)
                        == static_cast<qma6100_reg_type>(
                            expect_reg_val.to_ulong()));
            }
        }

        SECTION("INT_DATA_EN")
        {
            SECTION("INT_DATA_EN == disable")
            {
                reg.int_data_en = stv::qma6100_int_en1_reg::switcher_t::disable;
                constexpr reg_bitset expect_reg_val{std::string{"00000000"}};
                REQUIRE(static_cast<qma6100_reg_type>(reg)
                        == static_cast<qma6100_reg_type>(
                            expect_reg_val.to_ulong()));
            }

            SECTION("INT_DATA_EN == enable")
            {
                reg.int_data_en = stv::qma6100_int_en1_reg::switcher_t::enable;
                constexpr reg_bitset expect_reg_val{std::string{"00010000"}};
                REQUIRE(static_cast<qma6100_reg_type>(reg)
                        == static_cast<qma6100_reg_type>(
                            expect_reg_val.to_ulong()));
            }
        }
    }

    SECTION("INT_MAP1")
    {
        stv::qma6100_int_map1_reg reg;
        SECTION("INT1_NO_MOT")
        {
            SECTION("INT1_NO_MOT == disable")
            {
                reg.int1_no_mot = stv::qma6100_int_map1_reg::mapper_t::disable;
                constexpr reg_bitset expect_reg_val{std::string{"00000000"}};
                REQUIRE(static_cast<qma6100_reg_type>(reg)
                        == static_cast<qma6100_reg_type>(
                            expect_reg_val.to_ulong()));
            }

            SECTION("INT1_NO_MOT == enable")
            {
                reg.int1_no_mot = stv::qma6100_int_map1_reg::mapper_t::enable;
                constexpr reg_bitset expect_reg_val{std::string{"10000000"}};
                REQUIRE(static_cast<qma6100_reg_type>(reg)
                        == static_cast<qma6100_reg_type>(
                            expect_reg_val.to_ulong()));
            }
        }

        SECTION("INT1_ANY_MOT")
        {
            SECTION("INT1_ANY_MOT == disable")
            {
                reg.int1_any_mot = stv::qma6100_int_map1_reg::mapper_t::disable;
                constexpr reg_bitset expect_reg_val{std::string{"00000000"}};
                REQUIRE(static_cast<qma6100_reg_type>(reg)
                        == static_cast<qma6100_reg_type>(
                            expect_reg_val.to_ulong()));
            }

            SECTION("INT1_ANY_MOT == enable")
            {
                reg.int1_any_mot = stv::qma6100_int_map1_reg::mapper_t::enable;
                constexpr reg_bitset expect_reg_val{std::string{"00000001"}};
                REQUIRE(static_cast<qma6100_reg_type>(reg)
                        == static_cast<qma6100_reg_type>(
                            expect_reg_val.to_ulong()));
            }
        }
    }

    SECTION("INT_MAP3")
    {
        stv::qma6100_int_map3_reg reg;
        SECTION("INT2_NO_MOT")
        {
            SECTION("INT2_NO_MOT == disable")
            {
                reg.int2_no_mot = stv::qma6100_int_map3_reg::mapper_t::disable;
                constexpr reg_bitset expect_reg_val{std::string{"00000000"}};
                REQUIRE(static_cast<qma6100_reg_type>(reg)
                        == static_cast<qma6100_reg_type>(
                            expect_reg_val.to_ulong()));
            }
            SECTION("INT2_NO_MOT == enable")
            {
                reg.int2_no_mot = stv::qma6100_int_map3_reg::mapper_t::enable;
                constexpr reg_bitset expect_reg_val{std::string{"10000000"}};
                REQUIRE(static_cast<qma6100_reg_type>(reg)
                        == static_cast<qma6100_reg_type>(
                            expect_reg_val.to_ulong()));
            }
        }

        SECTION("INT2_ANY_MOT")
        {
            SECTION("INT2_ANY_MOT == disable")
            {
                reg.int2_any_mot = stv::qma6100_int_map3_reg::mapper_t::disable;
                constexpr reg_bitset expect_reg_val{std::string{"00000000"}};
                REQUIRE(static_cast<qma6100_reg_type>(reg)
                        == static_cast<qma6100_reg_type>(
                            expect_reg_val.to_ulong()));
            }
            SECTION("INT2_ANY_MOT == enable")
            {
                reg.int2_any_mot = stv::qma6100_int_map3_reg::mapper_t::enable;
                constexpr reg_bitset expect_reg_val{std::string{"00000001"}};
                REQUIRE(static_cast<qma6100_reg_type>(reg)
                        == static_cast<qma6100_reg_type>(
                            expect_reg_val.to_ulong()));
            }
        }
    }

    SECTION("INTPIN_CONF")
    {
        stv::qma6100_intpin_conf_reg reg;

        // Set all fields to zero.
        reg.int2_lvl =
            stv::qma6100_intpin_conf_reg::int2_lvl_t::logic_low_as_active;
        reg.int1_lvl =
            stv::qma6100_intpin_conf_reg::int1_lvl_t::logic_low_as_active;

        SECTION("DIS_PU_SENB")
        {
            SECTION("DIS_PU_SENB == enable")
            {
                reg.dis_pu_senb =
                    stv::qma6100_intpin_conf_reg::dis_pu_senb_t::enable;
                constexpr reg_bitset expect_reg_val{std::string{"00000000"}};
                REQUIRE(static_cast<qma6100_reg_type>(reg)
                        == static_cast<qma6100_reg_type>(
                            expect_reg_val.to_ulong()));
            }

            SECTION("DIS_PU_SENB == disable")
            {
                reg.dis_pu_senb =
                    stv::qma6100_intpin_conf_reg::dis_pu_senb_t::disable;
                constexpr reg_bitset expect_reg_val{std::string{"10000000"}};
                REQUIRE(static_cast<qma6100_reg_type>(reg)
                        == static_cast<qma6100_reg_type>(
                            expect_reg_val.to_ulong()));
            }
        }

        SECTION("DIS_IE_AD0")
        {
            SECTION("DIS_IE_AD0: == enable")
            {
                reg.dis_ie_ad0 =
                    stv::qma6100_intpin_conf_reg::dis_ie_ad0_t::enable;
                constexpr reg_bitset expect_reg_val{std::string{"00000000"}};
                REQUIRE(static_cast<qma6100_reg_type>(reg)
                        == static_cast<qma6100_reg_type>(
                            expect_reg_val.to_ulong()));
            }

            SECTION("DIS_IE_AD0: == disable")
            {
                reg.dis_ie_ad0 =
                    stv::qma6100_intpin_conf_reg::dis_ie_ad0_t::disable;
                constexpr reg_bitset expect_reg_val{std::string{"01000000"}};
                REQUIRE(static_cast<qma6100_reg_type>(reg)
                        == static_cast<qma6100_reg_type>(
                            expect_reg_val.to_ulong()));
            }
        }

        SECTION("EN_SPI3W")
        {
            SECTION("EN_SPI3W == enable")
            {
                reg.en_spi3w = stv::qma6100_intpin_conf_reg::en_spi3w_t::enable;
                constexpr reg_bitset expect_reg_val{std::string{"00100000"}};
                REQUIRE(static_cast<qma6100_reg_type>(reg)
                        == static_cast<qma6100_reg_type>(
                            expect_reg_val.to_ulong()));
            }

            SECTION("EN_SPI3W == disable")
            {
                reg.en_spi3w =
                    stv::qma6100_intpin_conf_reg::en_spi3w_t::disable;
                constexpr reg_bitset expect_reg_val{std::string{"00000000"}};
                REQUIRE(static_cast<qma6100_reg_type>(reg)
                        == static_cast<qma6100_reg_type>(
                            expect_reg_val.to_ulong()));
            }
        }

        SECTION("INT2_OD")
        {
            SECTION("INT2_OD == push_pull")
            {
                reg.int2_od =
                    stv::qma6100_intpin_conf_reg::int2_od_t::push_pull;
                constexpr reg_bitset expect_reg_val{std::string{"00000000"}};
                REQUIRE(static_cast<qma6100_reg_type>(reg)
                        == static_cast<qma6100_reg_type>(
                            expect_reg_val.to_ulong()));
            }

            SECTION("INT2_OD == disable")
            {
                reg.int2_od =
                    stv::qma6100_intpin_conf_reg::int2_od_t::open_drain;
                constexpr reg_bitset expect_reg_val{std::string{"00001000"}};
                REQUIRE(static_cast<qma6100_reg_type>(reg)
                        == static_cast<qma6100_reg_type>(
                            expect_reg_val.to_ulong()));
            }
        }

        SECTION("INT2_LVL")
        {
            SECTION("INT2_LVL == push_pull")
            {
                reg.int2_lvl = stv::qma6100_intpin_conf_reg::int2_lvl_t::
                    logic_low_as_active;
                constexpr reg_bitset expect_reg_val{std::string{"00000000"}};
                REQUIRE(static_cast<qma6100_reg_type>(reg)
                        == static_cast<qma6100_reg_type>(
                            expect_reg_val.to_ulong()));
            }

            SECTION("INT2_LVL == disable")
            {
                reg.int2_lvl = stv::qma6100_intpin_conf_reg::int2_lvl_t::
                    logic_high_as_active;
                constexpr reg_bitset expect_reg_val{std::string{"00000100"}};
                REQUIRE(static_cast<qma6100_reg_type>(reg)
                        == static_cast<qma6100_reg_type>(
                            expect_reg_val.to_ulong()));
            }
        }

        SECTION("INT1_OD")
        {
            SECTION("INT1_OD == push_pull")
            {
                reg.int1_od =
                    stv::qma6100_intpin_conf_reg::int1_od_t::push_pull;
                constexpr reg_bitset expect_reg_val{std::string{"00000000"}};
                REQUIRE(static_cast<qma6100_reg_type>(reg)
                        == static_cast<qma6100_reg_type>(
                            expect_reg_val.to_ulong()));
            }

            SECTION("INT1_OD == disable")
            {
                reg.int1_od =
                    stv::qma6100_intpin_conf_reg::int1_od_t::open_drain;
                constexpr reg_bitset expect_reg_val{std::string{"00000010"}};
                REQUIRE(static_cast<qma6100_reg_type>(reg)
                        == static_cast<qma6100_reg_type>(
                            expect_reg_val.to_ulong()));
            }
        }

        SECTION("INT1_LVL")
        {
            SECTION("INT1_LVL == push_pull")
            {
                reg.int1_lvl = stv::qma6100_intpin_conf_reg::int1_lvl_t::
                    logic_low_as_active;
                constexpr reg_bitset expect_reg_val{std::string{"00000000"}};
                REQUIRE(static_cast<qma6100_reg_type>(reg)
                        == static_cast<qma6100_reg_type>(
                            expect_reg_val.to_ulong()));
            }

            SECTION("INT1_LVL == disable")
            {
                reg.int1_lvl = stv::qma6100_intpin_conf_reg::int1_lvl_t::
                    logic_high_as_active;
                constexpr reg_bitset expect_reg_val{std::string{"00000001"}};
                REQUIRE(static_cast<qma6100_reg_type>(reg)
                        == static_cast<qma6100_reg_type>(
                            expect_reg_val.to_ulong()));
            }
        }
    }

    SECTION("INT_CFG")
    {
        stv::qma6100_int_cfg_reg reg;
        SECTION("NT_RD_CLR")
        {
            SECTION("NT_RD_CLR == clear_related")
            {
                reg.int_rd_clr =
                    stv::qma6100_int_cfg_reg::int_rd_clr_t::clear_related;
                constexpr reg_bitset expect_reg_val{std::string{"00000000"}};
                REQUIRE(static_cast<qma6100_reg_type>(reg)
                        == static_cast<qma6100_reg_type>(
                            expect_reg_val.to_ulong()));
            }

            SECTION("NT_RD_CLR == clear_all")
            {
                reg.int_rd_clr =
                    stv::qma6100_int_cfg_reg::int_rd_clr_t::clear_all;
                constexpr reg_bitset expect_reg_val{std::string{"10000000"}};
                REQUIRE(static_cast<qma6100_reg_type>(reg)
                        == static_cast<qma6100_reg_type>(
                            expect_reg_val.to_ulong()));
            }
        }

        SECTION("SHADOW_DIS")
        {
            SECTION("SHADOW_DIS == enable")
            {
                reg.shadow_dis = stv::qma6100_int_cfg_reg::shadow_dis_t::enable;
                constexpr reg_bitset expect_reg_val{std::string{"00000000"}};
                REQUIRE(static_cast<qma6100_reg_type>(reg)
                        == static_cast<qma6100_reg_type>(
                            expect_reg_val.to_ulong()));
            }

            SECTION("SHADOW_DIS == disable")
            {
                reg.shadow_dis =
                    stv::qma6100_int_cfg_reg::shadow_dis_t::disable;
                constexpr reg_bitset expect_reg_val{std::string{"01000000"}};
                REQUIRE(static_cast<qma6100_reg_type>(reg)
                        == static_cast<qma6100_reg_type>(
                            expect_reg_val.to_ulong()));
            }
        }

        SECTION("DIS_I2C")
        {
            SECTION("DIS_I2C == enable")
            {
                reg.dis_i2c = stv::qma6100_int_cfg_reg::dis_i2c_t::enable;
                constexpr reg_bitset expect_reg_val{std::string{"00000000"}};
                REQUIRE(static_cast<qma6100_reg_type>(reg)
                        == static_cast<qma6100_reg_type>(
                            expect_reg_val.to_ulong()));
            }

            SECTION("DIS_I2C == disable")
            {
                reg.dis_i2c = stv::qma6100_int_cfg_reg::dis_i2c_t::disable;
                constexpr reg_bitset expect_reg_val{std::string{"00100000"}};
                REQUIRE(static_cast<qma6100_reg_type>(reg)
                        == static_cast<qma6100_reg_type>(
                            expect_reg_val.to_ulong()));
            }
        }

        SECTION("LATCH_INT_STEP")
        {
            SECTION("LATCH_INT_STEP == non_latch_mode")
            {
                reg.latch_int_step =
                    stv::qma6100_int_cfg_reg::latch_int_step_t::non_latch_mode;
                constexpr reg_bitset expect_reg_val{std::string{"00000000"}};
                REQUIRE(static_cast<qma6100_reg_type>(reg)
                        == static_cast<qma6100_reg_type>(
                            expect_reg_val.to_ulong()));
            }

            SECTION("LATCH_INT_STEP == latch_mode")
            {
                reg.latch_int_step =
                    stv::qma6100_int_cfg_reg::latch_int_step_t::latch_mode;
                constexpr reg_bitset expect_reg_val{std::string{"00000010"}};
                REQUIRE(static_cast<qma6100_reg_type>(reg)
                        == static_cast<qma6100_reg_type>(
                            expect_reg_val.to_ulong()));
            }
        }

        SECTION("LATCH_INT")
        {
            SECTION("LATCH_INT == non_latch_mode")
            {
                reg.latch_int =
                    stv::qma6100_int_cfg_reg::latch_int_t::non_latch_mode;
                constexpr reg_bitset expect_reg_val{std::string{"00000000"}};
                REQUIRE(static_cast<qma6100_reg_type>(reg)
                        == static_cast<qma6100_reg_type>(
                            expect_reg_val.to_ulong()));
            }

            SECTION("LATCH_INT == latch_mode")
            {
                reg.latch_int =
                    stv::qma6100_int_cfg_reg::latch_int_t::latch_mode;
                constexpr reg_bitset expect_reg_val{std::string{"00000001"}};
                REQUIRE(static_cast<qma6100_reg_type>(reg)
                        == static_cast<qma6100_reg_type>(
                            expect_reg_val.to_ulong()));
            }
        }
    }

    SECTION("PM")
    {
        stv::qma6100_pm_reg reg;
        reg.mode_bit = stv::qma6100_pm_reg::mode_bit_t::standby;

        SECTION("MODE_BIT ")
        {
            SECTION("Standby")
            {
                reg.mode_bit = stv::qma6100_pm_reg::mode_bit_t::standby;
                constexpr reg_bitset expect_reg_val{std::string{"00000000"}};
                REQUIRE(static_cast<qma6100_reg_type>(reg)
                        == static_cast<qma6100_reg_type>(
                            expect_reg_val.to_ulong()));
            }

            SECTION("Active")
            {
                reg.mode_bit = stv::qma6100_pm_reg::mode_bit_t::active;
                constexpr reg_bitset expect_reg_val{std::string{"10000000"}};
                REQUIRE(static_cast<qma6100_reg_type>(reg)
                        == static_cast<qma6100_reg_type>(
                            expect_reg_val.to_ulong()));
            }
        }

        SECTION("T_RSTB_SINC_SEL ")
        {
            SECTION("T_RSTB_SINC=3*MCLK")
            {
                reg.t_rstb_sinc_sel =
                    stv::qma6100_pm_reg::t_rstb_sinc_sel_t::k_3_mult_mckl;
                constexpr reg_bitset expect_reg_val{std::string{"00000000"}};
                REQUIRE(static_cast<qma6100_reg_type>(reg)
                        == static_cast<qma6100_reg_type>(
                            expect_reg_val.to_ulong()));
            }

            SECTION("T_RSTB_SINC=4*MCLK")
            {
                reg.t_rstb_sinc_sel =
                    stv::qma6100_pm_reg::t_rstb_sinc_sel_t::k_4_mult_mckl;
                constexpr reg_bitset expect_reg_val{std::string{"00010000"}};
                REQUIRE(static_cast<qma6100_reg_type>(reg)
                        == static_cast<qma6100_reg_type>(
                            expect_reg_val.to_ulong()));
            }

            SECTION("T_RSTB_SINC=6*MCLK")
            {
                reg.t_rstb_sinc_sel =
                    stv::qma6100_pm_reg::t_rstb_sinc_sel_t::k_6_mult_mckl;
                constexpr reg_bitset expect_reg_val{std::string{"00100000"}};
                REQUIRE(static_cast<qma6100_reg_type>(reg)
                        == static_cast<qma6100_reg_type>(
                            expect_reg_val.to_ulong()));
            }

            SECTION("T_RSTB_SINC=8*MCLK")
            {
                reg.t_rstb_sinc_sel =
                    stv::qma6100_pm_reg::t_rstb_sinc_sel_t::k_8_mult_mckl;
                constexpr reg_bitset expect_reg_val{std::string{"00110000"}};
                REQUIRE(static_cast<qma6100_reg_type>(reg)
                        == static_cast<qma6100_reg_type>(
                            expect_reg_val.to_ulong()));
            }
        }

        SECTION("MCLK_SEL")
        {
            SECTION("500k")
            {
                reg.mclk_sel = stv::qma6100_pm_reg::mclk_sel_t::freq_500k;

                constexpr reg_bitset expect_reg_val{std::string{"00000000"}};
                REQUIRE(static_cast<qma6100_reg_type>(reg)
                        == static_cast<qma6100_reg_type>(
                            expect_reg_val.to_ulong()));
            }

            SECTION("333k")
            {
                reg.mclk_sel = stv::qma6100_pm_reg::mclk_sel_t::freq_333k;

                constexpr reg_bitset expect_reg_val{std::string{"00000001"}};
                REQUIRE(static_cast<qma6100_reg_type>(reg)
                        == static_cast<qma6100_reg_type>(
                            expect_reg_val.to_ulong()));
            }

            SECTION("200k")
            {
                reg.mclk_sel = stv::qma6100_pm_reg::mclk_sel_t::freq_200k;

                constexpr reg_bitset expect_reg_val{std::string{"00000010"}};
                REQUIRE(static_cast<qma6100_reg_type>(reg)
                        == static_cast<qma6100_reg_type>(
                            expect_reg_val.to_ulong()));
            }

            SECTION("100k")
            {
                reg.mclk_sel = stv::qma6100_pm_reg::mclk_sel_t::freq_100k;

                constexpr reg_bitset expect_reg_val{std::string{"00000011"}};
                REQUIRE(static_cast<qma6100_reg_type>(reg)
                        == static_cast<qma6100_reg_type>(
                            expect_reg_val.to_ulong()));
            }

            SECTION("50k")
            {
                reg.mclk_sel = stv::qma6100_pm_reg::mclk_sel_t::freq_50k;

                constexpr reg_bitset expect_reg_val{std::string{"00000100"}};
                REQUIRE(static_cast<qma6100_reg_type>(reg)
                        == static_cast<qma6100_reg_type>(
                            expect_reg_val.to_ulong()));
            }

            SECTION("20k")
            {
                reg.mclk_sel = stv::qma6100_pm_reg::mclk_sel_t::freq_20k;

                constexpr reg_bitset expect_reg_val{std::string{"00000101"}};
                REQUIRE(static_cast<qma6100_reg_type>(reg)
                        == static_cast<qma6100_reg_type>(
                            expect_reg_val.to_ulong()));
            }

            SECTION("10k")
            {
                reg.mclk_sel = stv::qma6100_pm_reg::mclk_sel_t::freq_10k;

                constexpr reg_bitset expect_reg_val{std::string{"00000110"}};
                REQUIRE(static_cast<qma6100_reg_type>(reg)
                        == static_cast<qma6100_reg_type>(
                            expect_reg_val.to_ulong()));
            }

            SECTION("5k")
            {
                reg.mclk_sel = stv::qma6100_pm_reg::mclk_sel_t::freq_5k;

                constexpr reg_bitset expect_reg_val{std::string{"00000111"}};
                REQUIRE(static_cast<qma6100_reg_type>(reg)
                        == static_cast<qma6100_reg_type>(
                            expect_reg_val.to_ulong()));
            }
        }
    }
}

// NOLINTEND(*-magic-numbers, google-build-using-namespace,
// readability-function-cognitive-complexity,
// cppcoreguidelines-avoid-non-const-global-variables)
