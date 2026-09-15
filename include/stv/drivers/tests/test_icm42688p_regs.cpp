/// @file test_icm42688p_regs.cpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.

#include <bitset>
#include <catch2/catch_test_macros.hpp>
#include <string>

#include "stv/drivers/icm42688p_regs.hpp"
#include "stv/drivers/icm42688p_types.hpp"

// NOLINTBEGIN(*-magic-numbers, google-build-using-namespace,
// readability-function-cognitive-,
// cppcoreguidelines-avoid-non-const-global-variables)

TEST_CASE(
    "icm42688p regs", "[icm42688p][drivers]")
{
    using namespace stv;
    using reg_bitset = std::bitset<8>;

    SECTION("DEVICE_CONFIG")
    {
        stv::icm42688p_device_config_reg reg;

        SECTION("default is 0x00")
        {
            constexpr reg_bitset expect_reg_val{std::string{"00000000"}};
            REQUIRE(
                static_cast<icm42688p_reg_type>(reg)
                == static_cast<icm42688p_reg_type>(expect_reg_val.to_ulong()));
        }

        SECTION("SOFT_RESET_CONFIG == enable")
        {
            reg.soft_reset_config =
                stv::icm42688p_device_config_reg::soft_reset_t::enable;
            constexpr reg_bitset expect_reg_val{std::string{"00000001"}};
            REQUIRE(
                static_cast<icm42688p_reg_type>(reg)
                == static_cast<icm42688p_reg_type>(expect_reg_val.to_ulong()));
        }
    }

    SECTION("DRIVE_CONFIG")
    {
        stv::icm42688p_drive_config_reg reg;

        SECTION("default is reset value 0x05")
        {
            constexpr reg_bitset expect_reg_val{std::string{"00000101"}};
            REQUIRE(
                static_cast<icm42688p_reg_type>(reg)
                == static_cast<icm42688p_reg_type>(expect_reg_val.to_ulong()));
        }

        SECTION("I2C and SPI slew rate 1 (I2C driver setting)")
        {
            reg.i2c_slew_rate =
                stv::icm42688p_drive_config_reg::slew_rate_t::ns_12_36;
            reg.spi_slew_rate =
                stv::icm42688p_drive_config_reg::slew_rate_t::ns_12_36;
            constexpr reg_bitset expect_reg_val{std::string{"00001001"}};
            REQUIRE(
                static_cast<icm42688p_reg_type>(reg)
                == static_cast<icm42688p_reg_type>(expect_reg_val.to_ulong()));
        }

        SECTION("Ctor")
        {
            stv::icm42688p_drive_config_reg reg_ctor{icm42688p_reg_type{0x28}};
            REQUIRE(reg_ctor.i2c_slew_rate
                    == stv::icm42688p_drive_config_reg::slew_rate_t::less_2ns);
            REQUIRE(reg_ctor.spi_slew_rate
                    == stv::icm42688p_drive_config_reg::slew_rate_t::ns_20_60);
        }
    }

    SECTION("INT_CONFIG")
    {
        stv::icm42688p_int_config_reg reg;

        SECTION("default is 0x00 (active low, open drain, pulsed)")
        {
            constexpr reg_bitset expect_reg_val{std::string{"00000000"}};
            REQUIRE(
                static_cast<icm42688p_reg_type>(reg)
                == static_cast<icm42688p_reg_type>(expect_reg_val.to_ulong()));
        }

        SECTION("INT1 push-pull active-high pulse")
        {
            reg.int1_polarity =
                stv::icm42688p_int_config_reg::polarity_t::active_high;
            reg.int1_drive_circuit =
                stv::icm42688p_int_config_reg::drive_circuit_t::push_pull;
            reg.int1_mode = stv::icm42688p_int_config_reg::mode_t::pulse;
            constexpr reg_bitset expect_reg_val{std::string{"00000011"}};
            REQUIRE(
                static_cast<icm42688p_reg_type>(reg)
                == static_cast<icm42688p_reg_type>(expect_reg_val.to_ulong()));
        }

        SECTION("INT1 latched open-drain active-low")
        {
            reg.int1_mode = stv::icm42688p_int_config_reg::mode_t::latched;
            constexpr reg_bitset expect_reg_val{std::string{"00000100"}};
            REQUIRE(
                static_cast<icm42688p_reg_type>(reg)
                == static_cast<icm42688p_reg_type>(expect_reg_val.to_ulong()));
        }

        SECTION("INT2 push-pull active-high")
        {
            reg.int2_polarity =
                stv::icm42688p_int_config_reg::polarity_t::active_high;
            reg.int2_drive_circuit =
                stv::icm42688p_int_config_reg::drive_circuit_t::push_pull;
            constexpr reg_bitset expect_reg_val{std::string{"00011000"}};
            REQUIRE(
                static_cast<icm42688p_reg_type>(reg)
                == static_cast<icm42688p_reg_type>(expect_reg_val.to_ulong()));
        }
    }

    SECTION("FIFO_CONFIG")
    {
        stv::icm42688p_fifo_config_reg reg;

        SECTION("default bypass")
        {
            constexpr reg_bitset expect_reg_val{std::string{"00000000"}};
            REQUIRE(
                static_cast<icm42688p_reg_type>(reg)
                == static_cast<icm42688p_reg_type>(expect_reg_val.to_ulong()));
        }

        SECTION("stream-to-FIFO")
        {
            reg.fifo_mode =
                stv::icm42688p_fifo_config_reg::fifo_mode_t::stream_to_fifo;
            constexpr reg_bitset expect_reg_val{std::string{"01000000"}};
            REQUIRE(
                static_cast<icm42688p_reg_type>(reg)
                == static_cast<icm42688p_reg_type>(expect_reg_val.to_ulong()));
        }
    }

    SECTION("INTF_CONFIG0")
    {
        stv::icm42688p_intf_config0_reg reg;

        SECTION("default is reset value 0x30")
        {
            constexpr reg_bitset expect_reg_val{std::string{"00110000"}};
            REQUIRE(
                static_cast<icm42688p_reg_type>(reg)
                == static_cast<icm42688p_reg_type>(expect_reg_val.to_ulong()));
        }

        SECTION("FIFO_COUNT_REC")
        {
            reg.fifo_count_rec =
                stv::icm42688p_intf_config0_reg::switcher_t::enable;
            constexpr reg_bitset expect_reg_val{std::string{"01110000"}};
            REQUIRE(
                static_cast<icm42688p_reg_type>(reg)
                == static_cast<icm42688p_reg_type>(expect_reg_val.to_ulong()));
        }
    }

    SECTION("PWR_MGMT0")
    {
        stv::icm42688p_pwr_mgmt0_reg reg;

        SECTION("default is 0x00 (all off)")
        {
            constexpr reg_bitset expect_reg_val{std::string{"00000000"}};
            REQUIRE(
                static_cast<icm42688p_reg_type>(reg)
                == static_cast<icm42688p_reg_type>(expect_reg_val.to_ulong()));
        }

        SECTION("gyro LN + accel LN")
        {
            reg.gyro_mode =
                stv::icm42688p_pwr_mgmt0_reg::gyro_mode_t::low_noise;
            reg.accel_mode =
                stv::icm42688p_pwr_mgmt0_reg::accel_mode_t::low_noise;
            constexpr reg_bitset expect_reg_val{std::string{"00001111"}};
            REQUIRE(
                static_cast<icm42688p_reg_type>(reg)
                == static_cast<icm42688p_reg_type>(expect_reg_val.to_ulong()));
        }

        SECTION("TEMP_DIS")
        {
            reg.temp_dis = stv::icm42688p_pwr_mgmt0_reg::switcher_t::enable;
            constexpr reg_bitset expect_reg_val{std::string{"00100000"}};
            REQUIRE(
                static_cast<icm42688p_reg_type>(reg)
                == static_cast<icm42688p_reg_type>(expect_reg_val.to_ulong()));
        }
    }

    SECTION("GYRO_CONFIG0")
    {
        stv::icm42688p_gyro_config0_reg reg;

        SECTION("default is reset value 0x06 (2000dps, 1kHz)")
        {
            constexpr reg_bitset expect_reg_val{std::string{"00000110"}};
            REQUIRE(
                static_cast<icm42688p_reg_type>(reg)
                == static_cast<icm42688p_reg_type>(expect_reg_val.to_ulong()));
        }

        SECTION("FS_SEL 500dps")
        {
            reg.fs_sel = stv::icm42688p_gyro_fsr_t::dps_500;
            constexpr reg_bitset expect_reg_val{std::string{"01000110"}};
            REQUIRE(
                static_cast<icm42688p_reg_type>(reg)
                == static_cast<icm42688p_reg_type>(expect_reg_val.to_ulong()));
        }

        SECTION("ODR 50 Hz")
        {
            reg.odr = stv::icm42688p_odr_t::odr_50;
            constexpr reg_bitset expect_reg_val{std::string{"00001001"}};
            REQUIRE(
                static_cast<icm42688p_reg_type>(reg)
                == static_cast<icm42688p_reg_type>(expect_reg_val.to_ulong()));
        }

        SECTION("FS_SEL 2000dps + ODR 50 Hz")
        {
            reg.fs_sel = stv::icm42688p_gyro_fsr_t::dps_2000;
            reg.odr    = stv::icm42688p_odr_t::odr_50;
            constexpr reg_bitset expect_reg_val{std::string{"00001001"}};
            REQUIRE(
                static_cast<icm42688p_reg_type>(reg)
                == static_cast<icm42688p_reg_type>(expect_reg_val.to_ulong()));
        }

        SECTION("FS_SEL 250dps + ODR 500 Hz")
        {
            reg.fs_sel = stv::icm42688p_gyro_fsr_t::dps_250;
            reg.odr    = stv::icm42688p_odr_t::odr_500;
            constexpr reg_bitset expect_reg_val{std::string{"01101111"}};
            REQUIRE(
                static_cast<icm42688p_reg_type>(reg)
                == static_cast<icm42688p_reg_type>(expect_reg_val.to_ulong()));
        }

        SECTION("Ctor")
        {
            stv::icm42688p_gyro_config0_reg reg_ctor{icm42688p_reg_type{0x49}};
            REQUIRE(reg_ctor.fs_sel == stv::icm42688p_gyro_fsr_t::dps_500);
            REQUIRE(reg_ctor.odr == stv::icm42688p_odr_t::odr_50);
        }
    }

    SECTION("ACCEL_CONFIG0")
    {
        stv::icm42688p_accel_config0_reg reg;

        SECTION("default is reset value 0x06 (16g, 1kHz)")
        {
            constexpr reg_bitset expect_reg_val{std::string{"00000110"}};
            REQUIRE(
                static_cast<icm42688p_reg_type>(reg)
                == static_cast<icm42688p_reg_type>(expect_reg_val.to_ulong()));
        }

        SECTION("FS_SEL 8g + ODR 50 Hz")
        {
            reg.fs_sel = stv::icm42688p_accel_fsr_t::g_8;
            reg.odr    = stv::icm42688p_odr_t::odr_50;
            constexpr reg_bitset expect_reg_val{std::string{"00101001"}};
            REQUIRE(
                static_cast<icm42688p_reg_type>(reg)
                == static_cast<icm42688p_reg_type>(expect_reg_val.to_ulong()));
        }
    }

    SECTION("FIFO_CONFIG1")
    {
        stv::icm42688p_fifo_config1_reg reg;

        SECTION("default is 0x00")
        {
            constexpr reg_bitset expect_reg_val{std::string{"00000000"}};
            REQUIRE(
                static_cast<icm42688p_reg_type>(reg)
                == static_cast<icm42688p_reg_type>(expect_reg_val.to_ulong()));
        }

        SECTION("HIRES + TMST_FSYNC + TEMP + GYRO + ACCEL (20-byte packet)")
        {
            reg.hires_en = stv::icm42688p_fifo_config1_reg::switcher_t::enable;
            reg.tmst_fsync_en =
                stv::icm42688p_fifo_config1_reg::switcher_t::enable;
            reg.temp_en  = stv::icm42688p_fifo_config1_reg::switcher_t::enable;
            reg.gyro_en  = stv::icm42688p_fifo_config1_reg::switcher_t::enable;
            reg.accel_en = stv::icm42688p_fifo_config1_reg::switcher_t::enable;
            constexpr reg_bitset expect_reg_val{std::string{"00011111"}};
            REQUIRE(
                static_cast<icm42688p_reg_type>(reg)
                == static_cast<icm42688p_reg_type>(expect_reg_val.to_ulong()));
        }
    }

    SECTION("INT_CONFIG0")
    {
        stv::icm42688p_int_config0_reg reg;

        SECTION("default is 0x00 (clear on status read)")
        {
            constexpr reg_bitset expect_reg_val{std::string{"00000000"}};
            REQUIRE(
                static_cast<icm42688p_reg_type>(reg)
                == static_cast<icm42688p_reg_type>(expect_reg_val.to_ulong()));
        }

        SECTION("UI_DRDY_INT_CLEAR == clear on sensor register read")
        {
            reg.ui_drdy_int_clear = stv::icm42688p_int_config0_reg::
                int_clear_t::sensor_register_read;
            constexpr reg_bitset expect_reg_val{std::string{"00100000"}};
            REQUIRE(
                static_cast<icm42688p_reg_type>(reg)
                == static_cast<icm42688p_reg_type>(expect_reg_val.to_ulong()));
        }
    }

    SECTION("INT_CONFIG1")
    {
        stv::icm42688p_int_config1_reg reg;

        SECTION("default is reset value 0x10 (INT_ASYNC_RESET == 1)")
        {
            constexpr reg_bitset expect_reg_val{std::string{"00010000"}};
            REQUIRE(
                static_cast<icm42688p_reg_type>(reg)
                == static_cast<icm42688p_reg_type>(expect_reg_val.to_ulong()));
            REQUIRE(reg.int_async_reset
                    == stv::icm42688p_int_config1_reg::async_reset_t::reset);
        }

        SECTION("INT_ASYNC_RESET == no_reset")
        {
            reg.int_async_reset =
                stv::icm42688p_int_config1_reg::async_reset_t::no_reset;
            constexpr reg_bitset expect_reg_val{std::string{"00000000"}};
            REQUIRE(
                static_cast<icm42688p_reg_type>(reg)
                == static_cast<icm42688p_reg_type>(expect_reg_val.to_ulong()));
        }
    }

    SECTION("INT_SOURCE0")
    {
        stv::icm42688p_int_source0_reg reg;

        SECTION("default is reset value 0x10 (UI_DRDY routed to INT1)")
        {
            constexpr reg_bitset expect_reg_val{std::string{"00010000"}};
            REQUIRE(
                static_cast<icm42688p_reg_type>(reg)
                == static_cast<icm42688p_reg_type>(expect_reg_val.to_ulong()));
            REQUIRE(reg.ui_drdy_int1_en
                    == stv::icm42688p_int_source0_reg::switcher_t::enable);
        }

        SECTION("UI_DRDY_INT1_EN == disable")
        {
            reg.ui_drdy_int1_en =
                stv::icm42688p_int_source0_reg::switcher_t::disable;
            constexpr reg_bitset expect_reg_val{std::string{"00000000"}};
            REQUIRE(
                static_cast<icm42688p_reg_type>(reg)
                == static_cast<icm42688p_reg_type>(expect_reg_val.to_ulong()));
        }
    }

    SECTION("SELF_TEST_CONFIG")
    {
        stv::icm42688p_self_test_config_reg reg;

        SECTION("default is 0x00")
        {
            constexpr reg_bitset expect_reg_val{std::string{"00000000"}};
            REQUIRE(
                static_cast<icm42688p_reg_type>(reg)
                == static_cast<icm42688p_reg_type>(expect_reg_val.to_ulong()));
        }

        SECTION("all gyro axes")
        {
            reg.en_gx_st =
                stv::icm42688p_self_test_config_reg::switcher_t::enable;
            reg.en_gy_st =
                stv::icm42688p_self_test_config_reg::switcher_t::enable;
            reg.en_gz_st =
                stv::icm42688p_self_test_config_reg::switcher_t::enable;
            constexpr reg_bitset expect_reg_val{std::string{"00000111"}};
            REQUIRE(
                static_cast<icm42688p_reg_type>(reg)
                == static_cast<icm42688p_reg_type>(expect_reg_val.to_ulong()));
        }

        SECTION("all accel axes + accel st power")
        {
            reg.accel_st_power =
                stv::icm42688p_self_test_config_reg::switcher_t::enable;
            reg.en_ax_st =
                stv::icm42688p_self_test_config_reg::switcher_t::enable;
            reg.en_ay_st =
                stv::icm42688p_self_test_config_reg::switcher_t::enable;
            reg.en_az_st =
                stv::icm42688p_self_test_config_reg::switcher_t::enable;
            constexpr reg_bitset expect_reg_val{std::string{"01111000"}};
            REQUIRE(
                static_cast<icm42688p_reg_type>(reg)
                == static_cast<icm42688p_reg_type>(expect_reg_val.to_ulong()));
        }

        SECTION("all six axes + accel st power")
        {
            reg.accel_st_power =
                stv::icm42688p_self_test_config_reg::switcher_t::enable;
            reg.en_ax_st =
                stv::icm42688p_self_test_config_reg::switcher_t::enable;
            reg.en_ay_st =
                stv::icm42688p_self_test_config_reg::switcher_t::enable;
            reg.en_az_st =
                stv::icm42688p_self_test_config_reg::switcher_t::enable;
            reg.en_gx_st =
                stv::icm42688p_self_test_config_reg::switcher_t::enable;
            reg.en_gy_st =
                stv::icm42688p_self_test_config_reg::switcher_t::enable;
            reg.en_gz_st =
                stv::icm42688p_self_test_config_reg::switcher_t::enable;
            constexpr reg_bitset expect_reg_val{std::string{"01111111"}};
            REQUIRE(
                static_cast<icm42688p_reg_type>(reg)
                == static_cast<icm42688p_reg_type>(expect_reg_val.to_ulong()));
        }
    }

    SECTION("REG_BANK_SEL")
    {
        stv::icm42688p_reg_bank_sel_reg reg;

        SECTION("default bank 0")
        {
            constexpr reg_bitset expect_reg_val{std::string{"00000000"}};
            REQUIRE(
                static_cast<icm42688p_reg_type>(reg)
                == static_cast<icm42688p_reg_type>(expect_reg_val.to_ulong()));
        }

        SECTION("bank 2")
        {
            reg.bank_sel = stv::icm42688p_reg_bank_sel_reg::bank_t::bank_2;
            constexpr reg_bitset expect_reg_val{std::string{"00000010"}};
            REQUIRE(
                static_cast<icm42688p_reg_type>(reg)
                == static_cast<icm42688p_reg_type>(expect_reg_val.to_ulong()));
        }
    }

    SECTION("INTF_CONFIG6")
    {
        stv::icm42688p_intf_config6_reg reg;

        SECTION("default keeps functional bits at reset state (0x1F)")
        {
            // Конструктор по умолчанию принимает полный reset value 0x5F;
            // резервные биты 6:5 не моделируются и при сериализации
            // обнуляются.
            constexpr reg_bitset expect_reg_val{std::string{"00011111"}};
            REQUIRE(
                static_cast<icm42688p_reg_type>(reg)
                == static_cast<icm42688p_reg_type>(expect_reg_val.to_ulong()));
        }

        SECTION("Ctor from full reset value 0x5F drops reserved bits 6:5")
        {
            stv::icm42688p_intf_config6_reg reg_ctor{icm42688p_reg_type{0x5F}};
            constexpr reg_bitset expect_reg_val{std::string{"00011111"}};
            REQUIRE(
                static_cast<icm42688p_reg_type>(reg_ctor)
                == static_cast<icm42688p_reg_type>(expect_reg_val.to_ulong()));
        }

        SECTION("I2C driver setting (I3C_EN == 1, SDR/DDR disabled)")
        {
            reg.i3c_en = stv::icm42688p_intf_config6_reg::switcher_t::enable;
            reg.i3c_sdr_en =
                stv::icm42688p_intf_config6_reg::switcher_t::disable;
            reg.i3c_ddr_en =
                stv::icm42688p_intf_config6_reg::switcher_t::disable;
            reg.i3c_ibi_en =
                stv::icm42688p_intf_config6_reg::switcher_t::disable;
            reg.i3c_ibi_byte_en =
                stv::icm42688p_intf_config6_reg::switcher_t::disable;
            constexpr reg_bitset expect_reg_val{std::string{"00010000"}};
            REQUIRE(
                static_cast<icm42688p_reg_type>(reg)
                == static_cast<icm42688p_reg_type>(expect_reg_val.to_ulong()));
        }
    }

    SECTION("INT_STATUS")
    {
        SECTION("default is reset value 0x10 (RESET_DONE_INT)")
        {
            constexpr reg_bitset          reg_val{std::string{"00010000"}};
            stv::icm42688p_int_status_reg reg{
                icm42688p_reg_type{reg_val.to_ulong()}};
            REQUIRE(reg.reset_done_int
                    == stv::icm42688p_int_status_reg::switcher_t::enable);
            REQUIRE(reg.data_rdy_int
                    == stv::icm42688p_int_status_reg::switcher_t::disable);
        }

        SECTION("DATA_RDY_INT is bit 3")
        {
            constexpr reg_bitset          reg_val{std::string{"00001000"}};
            stv::icm42688p_int_status_reg reg{
                icm42688p_reg_type{reg_val.to_ulong()}};
            REQUIRE(reg.data_rdy_int
                    == stv::icm42688p_int_status_reg::switcher_t::enable);
            REQUIRE(reg.fifo_ths_int
                    == stv::icm42688p_int_status_reg::switcher_t::disable);
        }
    }

    SECTION("WHO_AM_I")
    {
        stv::icm42688p_who_am_i_reg reg;
        REQUIRE(stv::icm42688p_who_am_i_reg::expected_value
                == icm42688p_reg_type{0x47});
        reg.who_am_i = stv::icm42688p_who_am_i_reg::expected_value;
        REQUIRE(static_cast<icm42688p_reg_type>(reg)
                == stv::icm42688p_who_am_i_reg::expected_value);
    }

    SECTION("icm42688p_odr_to_hz")
    {
        REQUIRE(stv::icm42688p_odr_to_hz(stv::icm42688p_odr_t::odr_50)
                == 50.0F);
        REQUIRE(stv::icm42688p_odr_to_hz(stv::icm42688p_odr_t::odr_1k)
                == 1000.0F);
        REQUIRE(stv::icm42688p_odr_to_hz(stv::icm42688p_odr_t::odr_12p5)
                == 12.5F);
        REQUIRE(stv::icm42688p_odr_to_hz(stv::icm42688p_odr_t::odr_500)
                == 500.0F);
    }

    SECTION("Round-trip")
    {
        stv::icm42688p_gyro_config0_reg reg_out;
        reg_out.fs_sel = stv::icm42688p_gyro_fsr_t::dps_250;
        reg_out.odr    = stv::icm42688p_odr_t::odr_500;

        const stv::icm42688p_gyro_config0_reg reg_in{
            static_cast<icm42688p_reg_type>(reg_out)};

        REQUIRE(reg_in.fs_sel == reg_out.fs_sel);
        REQUIRE(reg_in.odr == reg_out.odr);
    }
}

// NOLINTEND(*-magic-numbers, google-build-using-namespace,
// readability-function-cognitive-complexity,
// cppcoreguidelines-avoid-non-const-global-variables)
