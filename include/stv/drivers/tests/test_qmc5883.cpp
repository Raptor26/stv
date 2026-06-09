/// @file test_qmc5883.cpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.

#include "stv/drivers/qmc5883.hpp"
#include "stv/drivers/qmc5883_regs.hpp"
#include <array>
#include <bitset>
#include <catch2/catch_test_macros.hpp>
#include <fakeit.hpp>
#include <map>

// NOLINTBEGIN(*-magic-numbers, google-build-using-namespace,
// readability-function-cognitive-,
// cppcoreguidelines-avoid-non-const-global-variables)

TEST_CASE(
    "QMC5883 Regs")
{
    using namespace stv;
    using reg_bitset = std::bitset<8>;

    SECTION("qmc5883_ctrl1_reg")
    {
        // Установка регистра в нулевые значения. Далее, в каждом тестовом
        // случае будет модифицироваться только нужная часть регистра.
        qmc5883_ctrl1_reg reg;
        reg.osr  = qmc5883_ctrl1_reg::osr_t::oversampling_512;
        reg.rng  = qmc5883_ctrl1_reg::rng_t::full_scale_2g;
        reg.odr  = qmc5883_ctrl1_reg::odr_t::output_data_rate_10hz;
        reg.mode = qmc5883_ctrl1_reg::mode_t::standby;

        SECTION("OSR")
        {
            SECTION("If OSR == 64")
            {
                reg.osr = qmc5883_ctrl1_reg::osr_t::oversampling_64;
                const reg_bitset expect_reg_val{std::string{"11000000"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }

            SECTION("If OSR == 128")
            {
                reg.osr = qmc5883_ctrl1_reg::osr_t::oversampling_128;
                const reg_bitset expect_reg_val{std::string{"10000000"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }

            SECTION("If OSR == 256")
            {
                reg.osr = qmc5883_ctrl1_reg::osr_t::oversampling_256;
                const reg_bitset expect_reg_val{std::string{"01000000"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }

            SECTION("If OSR == 512")
            {
                reg.osr = qmc5883_ctrl1_reg::osr_t::oversampling_512;
                const reg_bitset expect_reg_val{std::string{"00000000"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }
        }

        SECTION("RNG")
        {
            SECTION("If RNG == 2G")
            {
                reg.rng = qmc5883_ctrl1_reg::rng_t::full_scale_2g;
                const reg_bitset expect_reg_val{std::string{"00000000"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }

            SECTION("If RNG == 8G")
            {
                reg.rng = qmc5883_ctrl1_reg::rng_t::full_scale_8g;
                const reg_bitset expect_reg_val{std::string{"00010000"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }

            SECTION("Write binary")
            {
                SECTION("Write with FULL_SCALE_2G")
                {
                    constexpr reg_bitset expect_reg_val{
                        std::string{"00000000"}};
                    reg = static_cast<std::uint8_t>(expect_reg_val.to_ullong());
                    REQUIRE(reg.rng == qmc5883_ctrl1_reg::rng_t::full_scale_2g);
                }

                SECTION("Write with FULL_SCALE_8G")
                {
                    constexpr reg_bitset expect_reg_val{
                        std::string{"00010000"}};
                    reg = static_cast<std::uint8_t>(expect_reg_val.to_ullong());
                    REQUIRE(reg.rng == qmc5883_ctrl1_reg::rng_t::full_scale_8g);
                }
            }
        }

        SECTION("ODR")
        {
            SECTION("If ODR == 10Hz")
            {
                reg.odr = qmc5883_ctrl1_reg::odr_t::output_data_rate_10hz;
                const reg_bitset expect_reg_val{std::string{"00000000"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }

            SECTION("If ODR == 50Hz")
            {
                reg.odr = qmc5883_ctrl1_reg::odr_t::output_data_rate_50hz;
                const reg_bitset expect_reg_val{std::string{"00000100"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }

            SECTION("If ODR == 100Hz")
            {
                reg.odr = qmc5883_ctrl1_reg::odr_t::output_data_rate_100hz;
                const reg_bitset expect_reg_val{std::string{"00001000"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }

            SECTION("If ODR == 200Hz")
            {
                reg.odr = qmc5883_ctrl1_reg::odr_t::output_data_rate_200hz;
                const reg_bitset expect_reg_val{std::string{"00001100"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }
        }

        SECTION("Mode")
        {
            SECTION("If MODE == Standby")
            {
                reg.mode = qmc5883_ctrl1_reg::mode_t::standby;
                const reg_bitset expect_reg_val{std::string{"00000000"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }
            SECTION("If MODE == Continuous")
            {
                reg.mode = qmc5883_ctrl1_reg::mode_t::continuous;
                const reg_bitset expect_reg_val{std::string{"00000001"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }
        }
    }

    SECTION("qmc5883_ctrl2_reg")
    {
        // Установка регистра в нулевые значения.
        qmc5883_ctrl2_reg reg;
        reg.soft_reset = qmc5883_ctrl2_reg::soft_reset_t::normal;
        reg.rol_pnt    = qmc5883_ctrl2_reg::rol_pnt_t::normal;
        reg.int_enb    = qmc5883_ctrl2_reg::int_enb_t::enable;

        SECTION("Soft Reset")
        {
            SECTION("Soft reset == NORMAL")
            {
                reg.soft_reset = qmc5883_ctrl2_reg::soft_reset_t::normal;
                const reg_bitset expect_reg_val{std::string{"00000000"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }

            SECTION("Soft reset == ENABLE")
            {
                reg.soft_reset = qmc5883_ctrl2_reg::soft_reset_t::enable;
                const reg_bitset expect_reg_val{std::string{"10000000"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }
        }

        SECTION("ROL_PNT")
        {
            SECTION("ROL_PNT == NORMAL")
            {
                reg.rol_pnt = qmc5883_ctrl2_reg::rol_pnt_t::normal;
                const reg_bitset expect_reg_val{std::string{"00000000"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }

            SECTION("ROL_PNT == ENABLE")
            {
                reg.rol_pnt = qmc5883_ctrl2_reg::rol_pnt_t::enable;
                const reg_bitset expect_reg_val{std::string{"01000000"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }
        }

        SECTION("INT_ENB")
        {
            SECTION("INT_ENB == ENABLE")
            {
                reg.int_enb = qmc5883_ctrl2_reg::int_enb_t::enable;
                const reg_bitset expect_reg_val{std::string{"00000000"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }

            SECTION("INT_ENB == DISABLE")
            {
                reg.int_enb = qmc5883_ctrl2_reg::int_enb_t::disable;
                const reg_bitset expect_reg_val{std::string{"00000001"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }
        }
    }

    SECTION("qmc5883_set_reset_period_reg")
    {
        qmc5883_set_reset_period_reg reg;
        SECTION("Check default value")
        {
            constexpr std::uint8_t expect_reg_val{0x01};
            REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
        }

        SECTION("Check zero value")
        {
            constexpr std::uint8_t expect_reg_val{0x00};
            reg.fbr = expect_reg_val;
            REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
        }

        SECTION("Check non zero value")
        {
            constexpr std::uint8_t expect_reg_val{0xFF};
            reg.fbr = expect_reg_val;
            REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
        }

        SECTION("Check non zero with Ctor")
        {
            constexpr std::uint8_t       expect_reg_val{0xFF};
            qmc5883_set_reset_period_reg reg_with_ctor{expect_reg_val};
            REQUIRE(static_cast<uint8_t>(reg_with_ctor) == expect_reg_val);
        }
    }

    SECTION("qmc5883_status_reg")
    {
        SECTION("All bits is reset")
        {
            const reg_bitset   expect_reg_val{std::string{"00000000"}};

            qmc5883_status_reg status_reg{
                static_cast<std::uint8_t>(expect_reg_val.to_ulong())};
            REQUIRE(status_reg.dor == qmc5883_status_reg::dor_t::normal);
            REQUIRE(status_reg.ovl == qmc5883_status_reg::ovl_t::normal);
            REQUIRE(status_reg.drdy == qmc5883_status_reg::drdy_t::no_new_data);
        }

        SECTION("All bits is reset (check operator =)")
        {
            const reg_bitset   expect_reg_val{std::string{"00000000"}};

            qmc5883_status_reg status_reg{0xFF};
            REQUIRE_FALSE(status_reg.dor == qmc5883_status_reg::dor_t::normal);
            REQUIRE_FALSE(status_reg.ovl == qmc5883_status_reg::ovl_t::normal);
            REQUIRE_FALSE(status_reg.drdy
                          == qmc5883_status_reg::drdy_t::no_new_data);

            status_reg = static_cast<std::uint8_t>(expect_reg_val.to_ulong());
            REQUIRE(status_reg.dor == qmc5883_status_reg::dor_t::normal);
            REQUIRE(status_reg.ovl == qmc5883_status_reg::ovl_t::normal);
            REQUIRE(status_reg.drdy == qmc5883_status_reg::drdy_t::no_new_data);
        }

        SECTION("Set only DRDY bit")
        {
            const reg_bitset   expect_reg_val{std::string{"00000001"}};

            qmc5883_status_reg status_reg{
                static_cast<std::uint8_t>(expect_reg_val.to_ulong())};
            REQUIRE(status_reg.dor == qmc5883_status_reg::dor_t::normal);
            REQUIRE(status_reg.ovl == qmc5883_status_reg::ovl_t::normal);
            REQUIRE(status_reg.drdy
                    == qmc5883_status_reg::drdy_t::new_data_is_ready);
        }

        SECTION("Set only OVL bit")
        {
            const reg_bitset   expect_reg_val{std::string{"00000010"}};

            qmc5883_status_reg status_reg{
                static_cast<std::uint8_t>(expect_reg_val.to_ulong())};
            REQUIRE(status_reg.dor == qmc5883_status_reg::dor_t::normal);
            REQUIRE(status_reg.ovl == qmc5883_status_reg::ovl_t::data_overflow);
            REQUIRE(status_reg.drdy == qmc5883_status_reg::drdy_t::no_new_data);
        }

        SECTION("Set only DOR bit")
        {
            const reg_bitset   expect_reg_val{std::string{"00000100"}};

            qmc5883_status_reg status_reg{
                static_cast<std::uint8_t>(expect_reg_val.to_ulong())};
            REQUIRE(status_reg.dor
                    == qmc5883_status_reg::dor_t::data_skipped_for_reading);
            REQUIRE(status_reg.ovl == qmc5883_status_reg::ovl_t::normal);
            REQUIRE(status_reg.drdy == qmc5883_status_reg::drdy_t::no_new_data);
        }
    }
}

SCENARIO(
    "QMC5883 Driver")
{
    using namespace fakeit;
    using namespace stv;

    GIVEN("Default initialize setup")
    {
        qmc5883_setup setup;

        WHEN("Initialize with default setup")
        {
            qmc5883<stv::mag<float>> driver{setup};

            THEN("Driver is not valid object") { REQUIRE_FALSE(driver); }
        }
    }

    GIVEN("Initialize setup with valid i2c_interface")
    {
        Mock<stv::i2c_interface> i2c_mock;
        qmc5883_setup            setup;
        setup.i2c        = &i2c_mock.get();
        using reg_bitset = std::bitset<8>;

        WHEN("Initialize without read/write errors")
        {
            std::map<std::uint8_t, std::uint8_t> reg_map;

            When(Method(i2c_mock, write))
                .AlwaysDo([&](stv::i2c_interface::byte_type slave_addr,
                              stv::i2c_interface::byte_type reg_addr,
                              stv::i2c_interface::byte_type value) -> bool {
                    (void)slave_addr;
                    reg_map[static_cast<std::uint8_t>(reg_addr)] =
                        static_cast<std::uint8_t>(value);
                    return true;
                });

            When(Method(i2c_mock, read))
                .AlwaysDo([&](stv::i2c_interface::byte_type slave_addr,
                              stv::i2c_interface::byte_type reg_addr, void *dst,
                              std::size_t len) -> bool {
                    (void)slave_addr;
                    (void)len;
                    const auto it =
                        reg_map.find(static_cast<std::uint8_t>(reg_addr));
                    *static_cast<std::uint8_t *>(dst) =
                        (it != reg_map.end()) ? it->second : 0x00;
                    return true;
                });

            qmc5883<stv::mag<float>> driver{setup};
            const auto               init_result = driver.init(setup);

            THEN("Driver is valid object")
            {
                REQUIRE(driver);
                REQUIRE(init_result);
            }

            AND_WHEN("New data ready bit set")
            {
                const reg_bitset expect_reg_val{std::string{"00000001"}};

                When(Method(i2c_mock, read))
                    .AlwaysDo([&](stv::i2c_interface::byte_type slave_addr,
                                  stv::i2c_interface::byte_type reg_addr,
                                  void *dst, std::size_t len) -> bool {
                        (void)slave_addr;
                        (void)reg_addr;
                        (void)len;
                        *static_cast<std::uint8_t *>(dst) =
                            static_cast<uint8_t>(expect_reg_val.to_ulong());
                        return true;
                    });

                THEN("is_data_ready returns true")
                { REQUIRE(driver.is_data_ready()); }
            }

            AND_WHEN("New data ready bit reset")
            {
                const reg_bitset expect_reg_val{std::string{"00000000"}};

                When(Method(i2c_mock, read))
                    .AlwaysDo([&](stv::i2c_interface::byte_type slave_addr,
                                  stv::i2c_interface::byte_type reg_addr,
                                  void *dst, std::size_t len) -> bool {
                        (void)slave_addr;
                        (void)reg_addr;
                        (void)len;
                        *static_cast<std::uint8_t *>(dst) =
                            static_cast<uint8_t>(expect_reg_val.to_ulong());
                        return true;
                    });

                THEN("is_data_ready returns false")
                { REQUIRE_FALSE(driver.is_data_ready()); }
            }
        }

        WHEN("Initialize with read/write errors")
        {
            When(Method(i2c_mock, write)).AlwaysReturn(true);

            std::size_t                 ret_val_cnt{0};
            std::array<std::uint8_t, 4> reg_vals{
                static_cast<std::uint8_t>(setup.set_reset_period_reg),
                static_cast<std::uint8_t>(setup.ctrl1_reg),
                static_cast<std::uint8_t>(setup.ctrl2_reg),
                static_cast<std::uint8_t>(setup.ctrl1_reg)};

            When(Method(i2c_mock, read))
                .AlwaysDo([&](stv::i2c_interface::byte_type slave_addr,
                              stv::i2c_interface::byte_type reg_addr, void *dst,
                              std::size_t len) -> bool {
                    (void)slave_addr;
                    (void)reg_addr;
                    (void)len;
                    *static_cast<std::uint8_t *>(dst) = reg_vals[ret_val_cnt];
                    ++ret_val_cnt;
                    return true;
                });

            AND_WHEN("Invalid set/reset only") { reg_vals.at(0) = 0xFF; }

            AND_WHEN("Invalid ctrl reg 1 only") { reg_vals.at(1) = 0xFF; }

            AND_WHEN("Invalid ctrl reg 2 only") { reg_vals.at(2) = 0xFF; }

            qmc5883<stv::mag<float>> driver{setup};
            REQUIRE_FALSE(driver.init(setup));
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
                        0xFF; // Expected chip ID
                    return true;
                });

            THEN("is_detected returns true")
            { REQUIRE(qmc5883<stv::mag<float>>::is_detected(&i2c_mock.get())); }
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
                    *static_cast<std::uint8_t *>(dst) = 0x00; // Wrong chip ID
                    return true;
                });

            THEN("is_detected returns false")
            {
                REQUIRE_FALSE(
                    qmc5883<stv::mag<float>>::is_detected(&i2c_mock.get()));
            }
        }

        WHEN("I2C read fails")
        {
            When(Method(i2c_mock, read)).AlwaysReturn(false);

            THEN("is_detected returns false")
            {
                REQUIRE_FALSE(
                    qmc5883<stv::mag<float>>::is_detected(&i2c_mock.get()));
            }
        }
    }
}

TEST_CASE(
    "QMC5883 Raw meas bool operator")
{
    using namespace stv;
    using reg_bitset = std::bitset<8>;
    using raw_t      = qmc5883<stv::mag<float>>::raw_t;

    GIVEN("Default raw meas struct")
    {
        const reg_bitset status_reg_with_ovf_bit{std::string{"00000010"}};

        raw_t            raw_meas;

        WHEN("Don't modify anything")
        {
            THEN("Meas is not valid") { REQUIRE_FALSE(raw_meas); }
        }

        WHEN("Write one axis valid data")
        {
            AND_WHEN("Only X")
            {
                raw_meas.x = 1;

                THEN("Meas is valid") { REQUIRE(raw_meas); }
            }

            AND_WHEN("Only Y")
            {
                raw_meas.y = 1;

                THEN("Meas is valid") { REQUIRE(raw_meas); }
            }

            AND_WHEN("Only Z")
            {
                raw_meas.z = 1;

                THEN("Meas is valid") { REQUIRE(raw_meas); }
            }
        }

        WHEN("Write overflow bit")
        {
            raw_meas.status_reg =
                static_cast<std::uint8_t>(status_reg_with_ovf_bit.to_ulong());
            THEN("Meas is not valid") { REQUIRE_FALSE(raw_meas); }

            AND_WHEN("Set all axis with valid values")
            {
                raw_meas.x = 1;
                raw_meas.y = 1;
                raw_meas.z = 1;

                THEN("Meas is not valid") { REQUIRE_FALSE(raw_meas); }
            }
        }
    }
}

// NOLINTEND(*-magic-numbers, google-build-using-namespace,
// readability-function-cognitive-complexity,
// cppcoreguidelines-avoid-non-const-global-variables)
