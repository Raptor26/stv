/// @file test_mmc56xx.cpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.

#include "stv/drivers/mmc56xx_regs.hpp"
#include <bitset>
#include <catch2/catch_test_macros.hpp>
#include <fakeit.hpp>

// NOLINTBEGIN(*-magic-numbers, google-build-using-namespace,
// readability-function-cognitive-,
// cppcoreguidelines-avoid-non-const-global-variables)

TEST_CASE(
    "MMC56xx Registers")
{
    using namespace stv;
    using reg_bitset = std::bitset<8>;

    SECTION("Status1")
    {
        mmc56xx_status_reg reg{};

        SECTION("Meas_t_done")
        {
            SECTION("Meas_t_done == MEAS_NOT_READY")
            {
                reg.meas_t_done = mmc56xx_status_reg::meas_t_done_t::not_ready;
                const reg_bitset expect_reg_val{std::string{"00000000"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }

            SECTION("Meas_t_done == MEAS_IS_DONE")
            {
                reg.meas_t_done = mmc56xx_status_reg::meas_t_done_t::is_done;
                const reg_bitset expect_reg_val{std::string{"10000000"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }
        }

        SECTION("Meas_m_done")
        {
            SECTION("Meas_m_done == MEAS_NOT_READY")
            {
                reg.meas_m_done = mmc56xx_status_reg::meas_m_done_t::not_ready;
                const reg_bitset expect_reg_val{std::string{"00000000"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }

            SECTION("Meas_m_done == MEAS_IS_DONE")
            {
                reg.meas_m_done = mmc56xx_status_reg::meas_m_done_t::is_done;
                const reg_bitset expect_reg_val{std::string{"01000000"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }
        }

        SECTION("Sat_sensor")
        {
            SECTION("Sat_sensor == PASS")
            {
                reg.sat_sensor = mmc56xx_status_reg::sat_sensor_t::pass_;
                const reg_bitset expect_reg_val{std::string{"00000000"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }

            SECTION("Sat_sensor == FAIL")
            {
                reg.sat_sensor = mmc56xx_status_reg::sat_sensor_t::fail_;
                const reg_bitset expect_reg_val{std::string{"00100000"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }
        }

        SECTION("OTP_read_done")
        {
            SECTION("OTP_read_done == ERROR")
            {
                reg.otp_read_done = mmc56xx_status_reg::otp_read_done_t::error;
                const reg_bitset expect_reg_val{std::string{"00000000"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }

            SECTION("OTP_read_done == SUCCESS")
            {
                reg.otp_read_done =
                    mmc56xx_status_reg::otp_read_done_t::success;
                const reg_bitset expect_reg_val{std::string{"00010000"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }
        }

        SECTION("Convert binary to flags")
        {
            SECTION("All bits is reset")
            {
                const reg_bitset expect_reg_val{std::string{"00000000"}};
                reg = static_cast<mmc56xx_reg_type>(expect_reg_val.to_ulong());
                REQUIRE(reg.meas_t_done
                        == mmc56xx_status_reg::meas_t_done_t::not_ready);
                REQUIRE(reg.meas_m_done
                        == mmc56xx_status_reg::meas_m_done_t::not_ready);
                REQUIRE(reg.sat_sensor
                        == mmc56xx_status_reg::sat_sensor_t::pass_);
                REQUIRE(reg.otp_read_done
                        == mmc56xx_status_reg::otp_read_done_t::error);
            }

            SECTION("Meas_t_done only is set")
            {
                const reg_bitset expect_reg_val{std::string{"10000000"}};
                reg = static_cast<mmc56xx_reg_type>(expect_reg_val.to_ulong());
                REQUIRE(reg.meas_t_done
                        == mmc56xx_status_reg::meas_t_done_t::is_done);
                REQUIRE(reg.meas_m_done
                        == mmc56xx_status_reg::meas_m_done_t::not_ready);
                REQUIRE(reg.sat_sensor
                        == mmc56xx_status_reg::sat_sensor_t::pass_);
                REQUIRE(reg.otp_read_done
                        == mmc56xx_status_reg::otp_read_done_t::error);
            }

            SECTION("Meas_m_done only is set")
            {
                const reg_bitset expect_reg_val{std::string{"01000000"}};
                reg = static_cast<mmc56xx_reg_type>(expect_reg_val.to_ulong());
                REQUIRE(reg.meas_t_done
                        == mmc56xx_status_reg::meas_t_done_t::not_ready);
                REQUIRE(reg.meas_m_done
                        == mmc56xx_status_reg::meas_m_done_t::is_done);
                REQUIRE(reg.sat_sensor
                        == mmc56xx_status_reg::sat_sensor_t::pass_);
                REQUIRE(reg.otp_read_done
                        == mmc56xx_status_reg::otp_read_done_t::error);
            }

            SECTION("Sat_sensor only is set")
            {
                const reg_bitset expect_reg_val{std::string{"00100000"}};
                reg = static_cast<mmc56xx_reg_type>(expect_reg_val.to_ulong());
                REQUIRE(reg.meas_t_done
                        == mmc56xx_status_reg::meas_t_done_t::not_ready);
                REQUIRE(reg.meas_m_done
                        == mmc56xx_status_reg::meas_m_done_t::not_ready);
                REQUIRE(reg.sat_sensor
                        == mmc56xx_status_reg::sat_sensor_t::fail_);
                REQUIRE(reg.otp_read_done
                        == mmc56xx_status_reg::otp_read_done_t::error);
            }

            SECTION("OTP_read_done only is set")
            {
                const reg_bitset expect_reg_val{std::string{"00010000"}};
                reg = static_cast<mmc56xx_reg_type>(expect_reg_val.to_ulong());
                REQUIRE(reg.meas_t_done
                        == mmc56xx_status_reg::meas_t_done_t::not_ready);
                REQUIRE(reg.meas_m_done
                        == mmc56xx_status_reg::meas_m_done_t::not_ready);
                REQUIRE(reg.sat_sensor
                        == mmc56xx_status_reg::sat_sensor_t::pass_);
                REQUIRE(reg.otp_read_done
                        == mmc56xx_status_reg::otp_read_done_t::success);
            }

            SECTION("Ctor")
            {
                SECTION("All bits is set")
                {
                    // Все биты установлены кроме тех которые предназначены для
                    // использования изготовителем.
                    const reg_bitset expect_reg_val{std::string{"11110000"}};
                    const mmc56xx_status_reg status_reg{
                        static_cast<mmc56xx_reg_type>(
                            expect_reg_val.to_ulong())};
                    REQUIRE(status_reg.meas_t_done
                            == mmc56xx_status_reg::meas_t_done_t::is_done);
                    REQUIRE(status_reg.meas_m_done
                            == mmc56xx_status_reg::meas_m_done_t::is_done);
                    REQUIRE(status_reg.sat_sensor
                            == mmc56xx_status_reg::sat_sensor_t::fail_);
                    REQUIRE(status_reg.otp_read_done
                            == mmc56xx_status_reg::otp_read_done_t::success);
                }
            }
        }
    }

    SECTION("ODR")
    {
        constexpr std::uint8_t expected_val{0x10};
        mmc56xx_odr_reg        reg{expected_val};
        REQUIRE(static_cast<std::uint8_t>(reg) == expected_val);
    }

    SECTION("Internal Control 0")
    {
        mmc56xx_ctrl0_reg reg{};
        SECTION("Cmm_freq_en")
        {
            SECTION("Cmm_freq_en")
            {
                SECTION("Cmm_freq_en == RESET")
                {
                    reg.cmm_freq_en = mmc56xx_ctrl0_reg::cmm_freq_en_t::reset;
                    const reg_bitset expect_reg_val{std::string{"00000000"}};
                    REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
                }

                SECTION("Cmm_freq_en == SET")
                {
                    reg.cmm_freq_en = mmc56xx_ctrl0_reg::cmm_freq_en_t::set;
                    const reg_bitset expect_reg_val{std::string{"10000000"}};
                    REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
                }
            }

            SECTION("Auto_st_en")
            {
                SECTION("Auto_st_en == DISABLE")
                {
                    reg.auto_st_en = mmc56xx_ctrl0_reg::auto_st_en_t::disable;
                    const reg_bitset expect_reg_val{std::string{"00000000"}};
                    REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
                }

                SECTION("Auto_st_en == ENABLE")
                {
                    reg.auto_st_en = mmc56xx_ctrl0_reg::auto_st_en_t::enable;
                    const reg_bitset expect_reg_val{std::string{"01000000"}};
                    REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
                }
            }

            SECTION("Auto_SR_en")
            {
                SECTION("Auto_SR_en == RESET")
                {
                    reg.auto_sr_en = mmc56xx_ctrl0_reg::auto_sr_en_t::reset;
                    const reg_bitset expect_reg_val{std::string{"00000000"}};
                    REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
                }

                SECTION("Auto_SR_en == SET")
                {
                    reg.auto_sr_en = mmc56xx_ctrl0_reg::auto_sr_en_t::set;
                    const reg_bitset expect_reg_val{std::string{"00100000"}};
                    REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
                }
            }

            SECTION("Do_Reset")
            {
                SECTION("Do_Reset == RESET")
                {
                    reg.do_reset = mmc56xx_ctrl0_reg::do_reset_t::reset;
                    const reg_bitset expect_reg_val{std::string{"00000000"}};
                    REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
                }

                SECTION("Do_Reset == SET")
                {
                    reg.do_reset = mmc56xx_ctrl0_reg::do_reset_t::set;
                    const reg_bitset expect_reg_val{std::string{"00010000"}};
                    REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
                }
            }

            SECTION("Do_Set")
            {
                SECTION("Do_Set == RESET")
                {
                    reg.do_set = mmc56xx_ctrl0_reg::do_set_t::reset;
                    const reg_bitset expect_reg_val{std::string{"00000000"}};
                    REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
                }

                SECTION("Do_Set == SET")
                {
                    reg.do_set = mmc56xx_ctrl0_reg::do_set_t::set;
                    const reg_bitset expect_reg_val{std::string{"00001000"}};
                    REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
                }
            }

            SECTION("Take_meas_T")
            {
                SECTION("Take_meas_T == DISABLE")
                {
                    reg.take_meas_t = mmc56xx_ctrl0_reg::take_meas_t_t::disable;
                    const reg_bitset expect_reg_val{std::string{"00000000"}};
                    REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
                }

                SECTION("Take_meas_T == ENABLE")
                {
                    reg.take_meas_t = mmc56xx_ctrl0_reg::take_meas_t_t::enable;
                    const reg_bitset expect_reg_val{std::string{"00000010"}};
                    REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
                }
            }

            SECTION("Take_meas_M")
            {
                SECTION("Take_meas_M == DISABLE")
                {
                    reg.take_meas_m = mmc56xx_ctrl0_reg::take_meas_m_t::disable;
                    const reg_bitset expect_reg_val{std::string{"00000000"}};
                    REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
                }

                SECTION("Take_meas_M == ENABLE")
                {
                    reg.take_meas_m = mmc56xx_ctrl0_reg::take_meas_m_t::enable;
                    const reg_bitset expect_reg_val{std::string{"00000001"}};
                    REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
                }
            }
        }
    }

    SECTION("Internal Control 1")
    {
        mmc56xx_ctrl1_reg reg{};

        SECTION("Sw_reset")
        {
            SECTION("Sw_reset == DISABLE")
            {
                reg.sw_reset = mmc56xx_ctrl1_reg::sw_reset_t::disable;
                const reg_bitset expect_reg_val{std::string{"00000000"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }

            SECTION("Sw_reset == ENABLE")
            {
                reg.sw_reset = mmc56xx_ctrl1_reg::sw_reset_t::enable;
                const reg_bitset expect_reg_val{std::string{"10000000"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }
        }

        SECTION("St_enm")
        {
            SECTION("St_enm == DISABLE")
            {
                reg.st_enm = mmc56xx_ctrl1_reg::st_enm_t::disable;
                const reg_bitset expect_reg_val{std::string{"00000000"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }

            SECTION("St_enm == ENABLE")
            {
                reg.st_enm = mmc56xx_ctrl1_reg::st_enm_t::enable;
                const reg_bitset expect_reg_val{std::string{"01000000"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }
        }

        SECTION("St_enp")
        {
            SECTION("St_enp == DISABLE")
            {
                reg.st_enp = mmc56xx_ctrl1_reg::st_enp_t::disable;
                const reg_bitset expect_reg_val{std::string{"00000000"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }

            SECTION("St_enp == ENABLE")
            {
                reg.st_enp = mmc56xx_ctrl1_reg::st_enp_t::enable;
                const reg_bitset expect_reg_val{std::string{"00100000"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }
        }

        SECTION("z_inhibit")
        {
            SECTION("z_inhibit == DISABLE")
            {
                reg.z_inhibit = mmc56xx_ctrl1_reg::axis_inhibit_t::enable;
                const reg_bitset expect_reg_val{std::string{"00000000"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }

            SECTION("z_inhibit == ENABLE")
            {
                reg.z_inhibit = mmc56xx_ctrl1_reg::axis_inhibit_t::disable;
                const reg_bitset expect_reg_val{std::string{"00010000"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }
        }

        SECTION("y_inhibit")
        {
            SECTION("y_inhibit == DISABLE")
            {
                reg.y_inhibit = mmc56xx_ctrl1_reg::axis_inhibit_t::enable;
                const reg_bitset expect_reg_val{std::string{"00000000"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }

            SECTION("y_inhibit == ENABLE")
            {
                reg.y_inhibit = mmc56xx_ctrl1_reg::axis_inhibit_t::disable;
                const reg_bitset expect_reg_val{std::string{"00001000"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }
        }

        SECTION("x_inhibit")
        {
            SECTION("x_inhibit == DISABLE")
            {
                reg.x_inhibit = mmc56xx_ctrl1_reg::axis_inhibit_t::enable;
                const reg_bitset expect_reg_val{std::string{"00000000"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }

            SECTION("x_inhibit == ENABLE")
            {
                reg.x_inhibit = mmc56xx_ctrl1_reg::axis_inhibit_t::disable;
                const reg_bitset expect_reg_val{std::string{"00000100"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }
        }

        SECTION("BW")
        {
            SECTION("BW == BW_6_6MS")
            {
                reg.bw = mmc56xx_ctrl1_reg::bw_t::bw_6_6ms;
                const reg_bitset expect_reg_val{std::string{"00000000"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }

            SECTION("BW == BW_3_5MS")
            {
                reg.bw = mmc56xx_ctrl1_reg::bw_t::bw_3_5ms;
                const reg_bitset expect_reg_val{std::string{"00000001"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }

            SECTION("BW == BW_2_0MS")
            {
                reg.bw = mmc56xx_ctrl1_reg::bw_t::bw_2_0ms;
                const reg_bitset expect_reg_val{std::string{"00000010"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }

            SECTION("BW == BW_1_2MS")
            {
                reg.bw = mmc56xx_ctrl1_reg::bw_t::bw_1_2ms;
                const reg_bitset expect_reg_val{std::string{"00000011"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }
        }
    }

    SECTION("Internal Control 2")
    {
        mmc56xx_ctrl2_reg reg{};

        SECTION("Hpower")
        {
            SECTION("Hpower == DISABLE")
            {
                reg.hpower = mmc56xx_ctrl2_reg::hpower_t::disable;
                const reg_bitset expect_reg_val{std::string{"00000000"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }

            SECTION("Hpower == ACHIEVE_1000_HZ")
            {
                reg.hpower = mmc56xx_ctrl2_reg::hpower_t::achieve_1000_hz;
                const reg_bitset expect_reg_val{std::string{"10000000"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }
        }

        SECTION("Cmm_en")
        {
            SECTION("Cmm_en == DISABLE")
            {
                reg.cmm_en = mmc56xx_ctrl2_reg::cmm_en_t::disable;
                const reg_bitset expect_reg_val{std::string{"00000000"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }

            SECTION("Cmm_en == ENABLE")
            {
                reg.cmm_en = mmc56xx_ctrl2_reg::cmm_en_t::enable;
                const reg_bitset expect_reg_val{std::string{"00010000"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }
        }

        SECTION("En_prd_set")
        {
            SECTION("En_prd_set == DISABLE")
            {
                reg.en_prd_set = mmc56xx_ctrl2_reg::en_prd_set_t::disable;
                const reg_bitset expect_reg_val{std::string{"00000000"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }

            SECTION("En_prd_set == ENABLE")
            {
                reg.en_prd_set = mmc56xx_ctrl2_reg::en_prd_set_t::enable;
                const reg_bitset expect_reg_val{std::string{"00001000"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }
        }

        SECTION("Prd_set")
        {
            SECTION("Prd_set == EVERY_1")
            {
                reg.prd_set = mmc56xx_ctrl2_reg::prd_set_t::every_1;
                const reg_bitset expect_reg_val{std::string{"00000000"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }

            SECTION("Prd_set == EVERY_25")
            {
                reg.prd_set = mmc56xx_ctrl2_reg::prd_set_t::every_25;
                const reg_bitset expect_reg_val{std::string{"00000001"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }

            SECTION("Prd_set == EVERY_2000")
            {
                reg.prd_set = mmc56xx_ctrl2_reg::prd_set_t::every_2000;
                const reg_bitset expect_reg_val{std::string{"00000111"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }
        }
    }
}

// NOLINTEND(*-magic-numbers, google-build-using-namespace,
// readability-function-cognitive-complexity,
// cppcoreguidelines-avoid-non-const-global-variables)
