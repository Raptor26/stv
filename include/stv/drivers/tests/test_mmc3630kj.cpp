/// @file test_mmc3630kj.cpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.

#include "stv/drivers/mmc3630kj.hpp"
#include "stv/drivers/mmc3630kj_regs.hpp"
#include <array>
#include <bitset>
#include <catch2/catch_test_macros.hpp>
#include <fakeit.hpp>

// NOLINTBEGIN(*-magic-numbers, google-build-using-namespace,
// readability-function-cognitive-,
// cppcoreguidelines-avoid-non-const-global-variables)

TEST_CASE(
    "MMC3630KJ Registers")
{
    using namespace stv;
    using reg_bitset = std::bitset<8>;

    SECTION("Status")
    {
        // Установить значение регистра в нулевое значение (для удобства
        // тестирования).
        mmc3630kj_status_reg reg;
        reg.otp_rd_done = mmc3630kj_status_reg::otp_rd_done_t::not_able_to_read;
        reg.pump_on     = mmc3630kj_status_reg::pump_on_t::charge_pump_complete;
        reg.motion_detect = mmc3630kj_status_reg::motion_detected_t::no_motion;
        reg.meas_t_done   = mmc3630kj_status_reg::meas_t_done_t::not_ready;
        reg.meas_m_done   = mmc3630kj_status_reg::meas_m_done_t::not_ready;

        SECTION("OTP_Rd_Done")
        {
            SECTION("If OTP_Rd_Done == NOT_ABLE_TO_READ")
            {
                reg.otp_rd_done =
                    mmc3630kj_status_reg::otp_rd_done_t::not_able_to_read;
                const reg_bitset expect_reg_val{std::string{"00000000"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }

            SECTION("If OTP_Rd_Done == ABLE_TO_READ")
            {
                reg.otp_rd_done =
                    mmc3630kj_status_reg::otp_rd_done_t::able_to_read;
                const reg_bitset expect_reg_val{std::string{"00010000"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }
        }

        SECTION("Pump_On")
        {
            SECTION("If Pump_On == CHARGE_PUMP_COMPLETE")
            {
                reg.pump_on =
                    mmc3630kj_status_reg::pump_on_t::charge_pump_complete;
                const reg_bitset expect_reg_val{std::string{"00000000"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }

            SECTION("If Pump_On == CHARGE_PUMP_ACTIVE")
            {
                reg.pump_on =
                    mmc3630kj_status_reg::pump_on_t::charge_pump_active;
                const reg_bitset expect_reg_val{std::string{"00001000"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }
        }

        SECTION("Motion_Detected")
        {
            SECTION("If Motion_Detected == NO_MOTION")
            {
                reg.motion_detect =
                    mmc3630kj_status_reg::motion_detected_t::no_motion;
                const reg_bitset expect_reg_val{std::string{"00000000"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }

            SECTION("If Motion_Detected == MOTION_DETECT")
            {
                reg.motion_detect =
                    mmc3630kj_status_reg::motion_detected_t::motion_detect;
                const reg_bitset expect_reg_val{std::string{"00000100"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }
        }

        SECTION("Meas_T_Done")
        {
            SECTION("If Meas_T_Done == NOT_READY")
            {
                reg.meas_t_done =
                    mmc3630kj_status_reg::meas_t_done_t::not_ready;
                const reg_bitset expect_reg_val{std::string{"00000000"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }

            SECTION("If Meas_T_Done == FINISHED")
            {
                reg.meas_t_done = mmc3630kj_status_reg::meas_t_done_t::finished;
                const reg_bitset expect_reg_val{std::string{"00000010"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }
        }

        SECTION("Meas_M_Done")
        {
            SECTION("If Meas_M_Done == NOT_READY")
            {
                reg.meas_m_done =
                    mmc3630kj_status_reg::meas_m_done_t::not_ready;
                const reg_bitset expect_reg_val{std::string{"00000000"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }

            SECTION("If Meas_M_Done == FINISHED")
            {
                reg.meas_m_done = mmc3630kj_status_reg::meas_m_done_t::finished;
                const reg_bitset expect_reg_val{std::string{"00000001"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }
        }
    }

    SECTION("Internal Control 0")
    {
        // Установить значение регистра в нулевое значение (для удобства
        // тестирования).
        mmc3630kj_ctrl0_reg reg;
        reg.otr_read   = mmc3630kj_ctrl0_reg::otp_read_t::reset;
        reg.refill_cap = mmc3630kj_ctrl0_reg::refill_cap_t::reset;
        reg.reset      = mmc3630kj_ctrl0_reg::reset_t::disable;
        reg.set        = mmc3630kj_ctrl0_reg::set_t::disable;
        reg.start_mdt =
            mmc3630kj_ctrl0_reg::start_mdt_t::disable_or_motion_is_detect;
        reg.tm_t = mmc3630kj_ctrl0_reg::tm_t_t::reset;
        reg.tm_m = mmc3630kj_ctrl0_reg::tm_m_t::reset;

        SECTION("OTP_Read")
        {
            SECTION("If OTP_Read == RESET")
            {
                reg.otr_read = mmc3630kj_ctrl0_reg::otp_read_t::reset;
                const reg_bitset expect_reg_val{std::string{"00000000"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }

            SECTION("If OTP_Read == LET_DEVICE_TO_READ_OTP_DATA_AGAIN")
            {
                reg.otr_read = mmc3630kj_ctrl0_reg::otp_read_t::
                    let_device_to_read_otp_data_again;
                const reg_bitset expect_reg_val{std::string{"01000000"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }
        }

        SECTION("Refill_Cap")
        {
            SECTION("If Refill_Cap == RESET")
            {
                reg.refill_cap = mmc3630kj_ctrl0_reg::refill_cap_t::reset;
                const reg_bitset expect_reg_val{std::string{"00000000"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }

            SECTION("If Refill_Cap == REQUEST_RECHARGE_CAPACITY")
            {
                reg.refill_cap = mmc3630kj_ctrl0_reg::refill_cap_t::
                    request_recharge_capacity;
                const reg_bitset expect_reg_val{std::string{"00100000"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }
        }

        SECTION("Reset")
        {
            SECTION("If Reset == DISABLE")
            {
                reg.reset = mmc3630kj_ctrl0_reg::reset_t::disable;
                const reg_bitset expect_reg_val{std::string{"00000000"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }

            SECTION("If Reset == ENABLE")
            {
                reg.reset = mmc3630kj_ctrl0_reg::reset_t::enable;
                const reg_bitset expect_reg_val{std::string{"00010000"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }
        }

        SECTION("Set")
        {
            SECTION("If Set == DISABLE")
            {
                reg.set = mmc3630kj_ctrl0_reg::set_t::disable;
                const reg_bitset expect_reg_val{std::string{"00000000"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }

            SECTION("If Set == ENABLE")
            {
                reg.set = mmc3630kj_ctrl0_reg::set_t::enable;
                const reg_bitset expect_reg_val{std::string{"00001000"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }
        }

        SECTION("Start_MDT")
        {
            SECTION("If Start_MDT == DISABLE_OR_MOTION_IS_DETECT")
            {
                reg.start_mdt = mmc3630kj_ctrl0_reg::start_mdt_t::
                    disable_or_motion_is_detect;
                const reg_bitset expect_reg_val{std::string{"00000000"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }

            SECTION("If Start_MDT == ENABLE")
            {
                reg.start_mdt = mmc3630kj_ctrl0_reg::start_mdt_t::enable;
                const reg_bitset expect_reg_val{std::string{"00000100"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }
        }

        SECTION("TM_T")
        {
            SECTION("If TM_T == RESET")
            {
                reg.tm_t = mmc3630kj_ctrl0_reg::tm_t_t::reset;
                const reg_bitset expect_reg_val{std::string{"00000000"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }

            SECTION("If TM_T == INITIATE_MEASUREMENT")
            {
                reg.tm_t = mmc3630kj_ctrl0_reg::tm_t_t::initiate_measurement;
                const reg_bitset expect_reg_val{std::string{"00000010"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }
        }

        SECTION("TM_M")
        {
            SECTION("If TM_M == RESET")
            {
                reg.tm_m = mmc3630kj_ctrl0_reg::tm_m_t::reset;
                const reg_bitset expect_reg_val{std::string{"00000000"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }

            SECTION("If TM_M == INITIATE_MEASUREMENT")
            {
                reg.tm_m = mmc3630kj_ctrl0_reg::tm_m_t::initiate_measurement;
                const reg_bitset expect_reg_val{std::string{"00000001"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }
        }
    }

    SECTION("Internal Control 1")
    {
        mmc3630kj_ctrl1_reg reg;
        SECTION("SW_RST")
        {
            SECTION("If SW_RST == DISABLE")
            {
                reg.sw_reset = mmc3630kj_ctrl1_reg::sw_rst_t::disable;
                const reg_bitset expect_reg_val{std::string{"00000000"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }

            SECTION("If SW_RST == ENABLE")
            {
                reg.sw_reset = mmc3630kj_ctrl1_reg::sw_rst_t::enable;
                const reg_bitset expect_reg_val{std::string{"10000000"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }
        }

        SECTION("Z-inhibit")
        {
            SECTION("If Z_INHIBIT == ENABLE")
            {
                reg.z_inhibit = mmc3630kj_ctrl1_reg::z_inhibit_t::enable;
                const reg_bitset expect_reg_val{std::string{"00000000"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }

            SECTION("If Z_INHIBIT == DISABLE")
            {
                reg.z_inhibit = mmc3630kj_ctrl1_reg::z_inhibit_t::disable;
                const reg_bitset expect_reg_val{std::string{"00010000"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }
        }

        SECTION("Y-inhibit")
        {
            SECTION("If Y_INHIBIT == ENABLE")
            {
                reg.y_inhibit = mmc3630kj_ctrl1_reg::y_inhibit_t::enable;
                const reg_bitset expect_reg_val{std::string{"00000000"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }

            SECTION("If Y_INHIBIT == DISABLE")
            {
                reg.y_inhibit = mmc3630kj_ctrl1_reg::y_inhibit_t::disable;
                const reg_bitset expect_reg_val{std::string{"00001000"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }
        }

        SECTION("X-inhibit")
        {
            SECTION("If X_INHIBIT == ENABLE")
            {
                reg.x_inhibit = mmc3630kj_ctrl1_reg::x_inhibit_t::enable;
                const reg_bitset expect_reg_val{std::string{"00000000"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }

            SECTION("If X_INHIBIT == DISABLE")
            {
                reg.x_inhibit = mmc3630kj_ctrl1_reg::x_inhibit_t::disable;
                const reg_bitset expect_reg_val{std::string{"00000100"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }
        }

        SECTION("BW")
        {
            SECTION("If BW == ODR_100HZ")
            {
                reg.bw = mmc3630kj_ctrl1_reg::bw_t::odr_100hz;
                const reg_bitset expect_reg_val{std::string{"00000000"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }

            SECTION("If BW == ODR_200HZ")
            {
                reg.bw = mmc3630kj_ctrl1_reg::bw_t::odr_200hz;
                const reg_bitset expect_reg_val{std::string{"00000001"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }

            SECTION("If BW == ODR_400HZ")
            {
                reg.bw = mmc3630kj_ctrl1_reg::bw_t::odr_400hz;
                const reg_bitset expect_reg_val{std::string{"00000010"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }

            SECTION("If BW == ODR_600HZ")
            {
                reg.bw = mmc3630kj_ctrl1_reg::bw_t::odr_600hz;
                const reg_bitset expect_reg_val{std::string{"00000011"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }
        }
    }

    SECTION("Internal Control 2")
    {
        mmc3630kj_ctrl2_reg reg{};

        SECTION("INT_Meas_Done_EN")
        {
            SECTION("If INT_Meas_Done_EN == DISABLE")
            {
                reg.int_meas_done_en =
                    mmc3630kj_ctrl2_reg::int_meas_done_en_t::disable;
                const reg_bitset expect_reg_val{std::string{"00000000"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }

            SECTION("If INT_Meas_Done_EN == ENABLE")
            {
                reg.int_meas_done_en =
                    mmc3630kj_ctrl2_reg::int_meas_done_en_t::enable;
                const reg_bitset expect_reg_val{std::string{"01000000"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }
        }

        SECTION("INT_MDT_EN")
        {
            SECTION("If INT_MDT_EN == DISABLE")
            {
                reg.int_mdt_en = mmc3630kj_ctrl2_reg::int_mdt_en_t::disable;
                const reg_bitset expect_reg_val{std::string{"00000000"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }

            SECTION("If INT_MDT_EN == ENABLE")
            {
                reg.int_mdt_en = mmc3630kj_ctrl2_reg::int_mdt_en_t::enable;
                const reg_bitset expect_reg_val{std::string{"00100000"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }
        }

        SECTION("CM_Freq")
        {
            SECTION("If CM_Freq == CONTINUOUS_MODE_IS_OFF")
            {
                reg.cm_freq =
                    mmc3630kj_ctrl2_reg::cm_freq_t::continuous_mode_is_off;
                const reg_bitset expect_reg_val{std::string{"00000000"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }

            SECTION("If CM_Freq == FREQ_14HZ")
            {
                reg.cm_freq = mmc3630kj_ctrl2_reg::cm_freq_t::freq_14hz;
                const reg_bitset expect_reg_val{std::string{"00000001"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }

            SECTION("If CM_Freq == FREQ_5HZ")
            {
                reg.cm_freq = mmc3630kj_ctrl2_reg::cm_freq_t::freq_5hz;
                const reg_bitset expect_reg_val{std::string{"00000010"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }

            SECTION("If CM_Freq == FREQ_2_2HZ")
            {
                reg.cm_freq = mmc3630kj_ctrl2_reg::cm_freq_t::freq_2_2hz;
                const reg_bitset expect_reg_val{std::string{"00000011"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }

            SECTION("If CM_Freq == FREQ_1HZ")
            {
                reg.cm_freq = mmc3630kj_ctrl2_reg::cm_freq_t::freq_1hz;
                const reg_bitset expect_reg_val{std::string{"00000100"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }

            SECTION("If CM_Freq == FREQ_0_5HZ")
            {
                reg.cm_freq = mmc3630kj_ctrl2_reg::cm_freq_t::freq_0_5hz;
                const reg_bitset expect_reg_val{std::string{"00000101"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }

            SECTION("If CM_Freq == FREQ_0_25HZ")
            {
                reg.cm_freq = mmc3630kj_ctrl2_reg::cm_freq_t::freq_0_25hz;
                const reg_bitset expect_reg_val{std::string{"00000110"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }

            SECTION("If CM_Freq == FREQ_0_125HZ")
            {
                reg.cm_freq = mmc3630kj_ctrl2_reg::cm_freq_t::freq_0_125hz;
                const reg_bitset expect_reg_val{std::string{"00000111"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }

            SECTION("If CM_Freq == FREQ_0_0625HZ")
            {
                reg.cm_freq = mmc3630kj_ctrl2_reg::cm_freq_t::freq_0_0625hz;
                const reg_bitset expect_reg_val{std::string{"00001000"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }

            SECTION("If CM_Freq == FREQ_0_03125HZ")
            {
                reg.cm_freq = mmc3630kj_ctrl2_reg::cm_freq_t::freq_0_03125hz;
                const reg_bitset expect_reg_val{std::string{"00001001"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }

            SECTION("If CM_Freq == FREQ_0_015625HZ")
            {
                reg.cm_freq = mmc3630kj_ctrl2_reg::cm_freq_t::freq_0_015625hz;
                const reg_bitset expect_reg_val{std::string{"00001010"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }
        }
    }
}

SCENARIO(
    "MMC3630KJ Driver")
{
    using namespace fakeit;
    using namespace stv;

    GIVEN("Default initialize setup")
    {
        mmc3630kj_setup setup;

        WHEN("Initialize with default setup")
        {
            mmc3630kj<stv::mag<float>> driver{setup};

            THEN("Driver is not valid object") { REQUIRE_FALSE(driver); }
        }
    }

    GIVEN("Initialize setup with valid i2c_interface")
    {
        Mock<stv::i2c_interface> i2c_mock;
        mmc3630kj_setup          setup;
        setup.i2c = &i2c_mock.get();

        WHEN("Initialize without read/write errors")
        {
            When(Method(i2c_mock, write)).AlwaysReturn(true);
            When(Method(i2c_mock, read)).AlwaysReturn(true);

            mmc3630kj<stv::mag<float>> driver{setup};
            const auto                 init_result = driver.init(setup);

            THEN("Driver is valid object")
            {
                REQUIRE(driver);
                REQUIRE(init_result);
            }

            AND_WHEN("Read raw measurement data")
            {
                std::array<std::uint8_t, 6> meas_data{
                    0x00, 0x80, // X = 0x8000 = 32768 (null field)
                    0x00, 0x80, // Y = 0x8000 = 32768 (null field)
                    0x00, 0x80, // Z = 0x8000 = 32768 (null field)
                };

                When(Method(i2c_mock, read))
                    .AlwaysDo([&](stv::i2c_interface::byte_type slave_addr,
                                  stv::i2c_interface::byte_type reg_addr,
                                  void *dst, std::size_t len) -> bool {
                        (void)slave_addr;
                        (void)reg_addr;
                        std::memcpy(dst, meas_data.data(),
                                    std::min(len, meas_data.size()));
                        return true;
                    });

                const auto raw = driver.read_meas_raw();

                THEN("Raw data is valid and converted correctly")
                {
                    REQUIRE(raw);
                    REQUIRE(raw.x == 0x8000);
                    REQUIRE(raw.y == 0x8000);
                    REQUIRE(raw.z == 0x8000);
                }

                THEN("Normalized data is zero at null field")
                {
                    const auto mag = driver.read_normalized();
                    REQUIRE(mag.give_x() == 0.0F);
                    REQUIRE(mag.give_y() == 0.0F);
                    REQUIRE(mag.give_z() == 0.0F);
                }
            }
        }

        WHEN("Initialize with write error on first register")
        {
            // Simulate I2C write failure on the first write call
            std::size_t write_cnt{0};
            When(Method(i2c_mock, write))
                .AlwaysDo([&](stv::i2c_interface::byte_type slave_addr,
                              stv::i2c_interface::byte_type reg_addr,
                              stv::i2c_interface::byte_type value) -> bool {
                    (void)slave_addr;
                    (void)reg_addr;
                    (void)value;
                    ++write_cnt;
                    // Fail on the first write (ctrl0_reg)
                    return write_cnt > 1;
                });
            When(Method(i2c_mock, read)).AlwaysReturn(true);

            mmc3630kj<stv::mag<float>> driver{setup};
            const auto                 init_result = driver.init(setup);

            THEN("Driver init fails")
            {
                REQUIRE_FALSE(init_result);
                REQUIRE_FALSE(driver);
            }
        }
    }

    GIVEN("i2c_interface for chip ID detection")
    {
        Mock<stv::i2c_interface> i2c_mock;

        WHEN("Chip ID matches expected value")
        {
            When(Method(i2c_mock, read))
                .AlwaysDo([&](stv::i2c_interface::byte_type slave_addr,
                              stv::i2c_interface::byte_type reg_addr, void *dst,
                              std::size_t len) -> bool {
                    (void)slave_addr;
                    (void)reg_addr;
                    (void)len;
                    *static_cast<std::uint8_t *>(dst) =
                        0x0A; // Expected chip ID
                    return true;
                });

            THEN("is_detected returns true")
            {
                REQUIRE(
                    mmc3630kj<stv::mag<float>>::is_detected(&i2c_mock.get()));
            }
        }

        WHEN("Chip ID does not match expected value")
        {
            When(Method(i2c_mock, read))
                .AlwaysDo([&](stv::i2c_interface::byte_type slave_addr,
                              stv::i2c_interface::byte_type reg_addr, void *dst,
                              std::size_t len) -> bool {
                    (void)slave_addr;
                    (void)reg_addr;
                    (void)len;
                    *static_cast<std::uint8_t *>(dst) = 0xFF; // Wrong chip ID
                    return true;
                });

            THEN("is_detected returns false")
            {
                REQUIRE_FALSE(
                    mmc3630kj<stv::mag<float>>::is_detected(&i2c_mock.get()));
            }
        }

        WHEN("I2C read fails")
        {
            When(Method(i2c_mock, read)).AlwaysReturn(false);

            THEN("is_detected returns false")
            {
                REQUIRE_FALSE(
                    mmc3630kj<stv::mag<float>>::is_detected(&i2c_mock.get()));
            }
        }
    }
}

// NOLINTEND(*-magic-numbers, google-build-using-namespace,
// readability-function-cognitive-complexity,
// cppcoreguidelines-avoid-non-const-global-variables)
