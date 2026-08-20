/// @file test_qmc5883p.cpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.

#include "stv/drivers/qmc5883p.hpp"
#include "stv/drivers/qmc5883p_regs.hpp"
#include <array>
#include <bitset>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cstring>
#include <fakeit.hpp>
#include <map>

// NOLINTBEGIN(*-magic-numbers, google-build-using-namespace,
// readability-function-cognitive-complexity,
// cppcoreguidelines-avoid-non-const-global-variables)

TEST_CASE(
    "QMC5883P Regs")
{
    using namespace stv;
    using reg_bitset = std::bitset<8>;

    SECTION("qmc5883p_ctrl1_reg")
    {
        // Установка регистра в нулевые значения. Далее, в каждом тестовом
        // случае будет модифицироваться только нужная часть регистра.
        qmc5883p_ctrl1_reg reg;
        reg.osr2 = qmc5883p_ctrl1_reg::osr2_t::oversampling_1;
        reg.osr1 = qmc5883p_ctrl1_reg::osr1_t::oversampling_8;
        reg.odr  = qmc5883p_ctrl1_reg::odr_t::output_data_rate_10hz;
        reg.mode = qmc5883p_ctrl1_reg::mode_t::suspend;

        SECTION("OSR2")
        {
            SECTION("If OSR2 == 1")
            {
                reg.osr2 = qmc5883p_ctrl1_reg::osr2_t::oversampling_1;
                const reg_bitset expect_reg_val{std::string{"00000000"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }

            SECTION("If OSR2 == 2")
            {
                reg.osr2 = qmc5883p_ctrl1_reg::osr2_t::oversampling_2;
                const reg_bitset expect_reg_val{std::string{"01000000"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }

            SECTION("If OSR2 == 4")
            {
                reg.osr2 = qmc5883p_ctrl1_reg::osr2_t::oversampling_4;
                const reg_bitset expect_reg_val{std::string{"10000000"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }

            SECTION("If OSR2 == 8")
            {
                reg.osr2 = qmc5883p_ctrl1_reg::osr2_t::oversampling_8;
                const reg_bitset expect_reg_val{std::string{"11000000"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }
        }

        SECTION("OSR1")
        {
            SECTION("If OSR1 == 8")
            {
                reg.osr1 = qmc5883p_ctrl1_reg::osr1_t::oversampling_8;
                const reg_bitset expect_reg_val{std::string{"00000000"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }

            SECTION("If OSR1 == 4")
            {
                reg.osr1 = qmc5883p_ctrl1_reg::osr1_t::oversampling_4;
                const reg_bitset expect_reg_val{std::string{"00010000"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }

            SECTION("If OSR1 == 2")
            {
                reg.osr1 = qmc5883p_ctrl1_reg::osr1_t::oversampling_2;
                const reg_bitset expect_reg_val{std::string{"00100000"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }

            SECTION("If OSR1 == 1")
            {
                reg.osr1 = qmc5883p_ctrl1_reg::osr1_t::oversampling_1;
                const reg_bitset expect_reg_val{std::string{"00110000"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }
        }

        SECTION("ODR")
        {
            SECTION("If ODR == 10Hz")
            {
                reg.odr = qmc5883p_ctrl1_reg::odr_t::output_data_rate_10hz;
                const reg_bitset expect_reg_val{std::string{"00000000"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }

            SECTION("If ODR == 50Hz")
            {
                reg.odr = qmc5883p_ctrl1_reg::odr_t::output_data_rate_50hz;
                const reg_bitset expect_reg_val{std::string{"00000100"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }

            SECTION("If ODR == 100Hz")
            {
                reg.odr = qmc5883p_ctrl1_reg::odr_t::output_data_rate_100hz;
                const reg_bitset expect_reg_val{std::string{"00001000"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }

            SECTION("If ODR == 200Hz")
            {
                reg.odr = qmc5883p_ctrl1_reg::odr_t::output_data_rate_200hz;
                const reg_bitset expect_reg_val{std::string{"00001100"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }
        }

        SECTION("MODE")
        {
            SECTION("If MODE == Suspend")
            {
                reg.mode = qmc5883p_ctrl1_reg::mode_t::suspend;
                const reg_bitset expect_reg_val{std::string{"00000000"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }

            SECTION("If MODE == Normal")
            {
                reg.mode = qmc5883p_ctrl1_reg::mode_t::normal;
                const reg_bitset expect_reg_val{std::string{"00000001"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }

            SECTION("If MODE == Single")
            {
                reg.mode = qmc5883p_ctrl1_reg::mode_t::single;
                const reg_bitset expect_reg_val{std::string{"00000010"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }

            SECTION("If MODE == Continuous")
            {
                reg.mode = qmc5883p_ctrl1_reg::mode_t::continuous;
                const reg_bitset expect_reg_val{std::string{"00000011"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }
        }
    }

    SECTION("qmc5883p_ctrl2_reg")
    {
        // Установка регистра в нулевые значения.
        qmc5883p_ctrl2_reg reg;
        reg.soft_reset     = qmc5883p_ctrl2_reg::soft_reset_t::normal;
        reg.self_test      = qmc5883p_ctrl2_reg::self_test_t::normal;
        reg.rng            = qmc5883p_ctrl2_reg::rng_t::full_scale_30g;
        reg.set_reset_mode =
            qmc5883p_ctrl2_reg::set_reset_mode_t::set_and_reset_on;

        SECTION("Soft Reset")
        {
            SECTION("Soft reset == NORMAL")
            {
                reg.soft_reset = qmc5883p_ctrl2_reg::soft_reset_t::normal;
                const reg_bitset expect_reg_val{std::string{"00000000"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }

            SECTION("Soft reset == ENABLE")
            {
                reg.soft_reset = qmc5883p_ctrl2_reg::soft_reset_t::enable;
                const reg_bitset expect_reg_val{std::string{"10000000"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }
        }

        SECTION("Self Test")
        {
            SECTION("Self test == NORMAL")
            {
                reg.self_test = qmc5883p_ctrl2_reg::self_test_t::normal;
                const reg_bitset expect_reg_val{std::string{"00000000"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }

            SECTION("Self test == ENABLE")
            {
                reg.self_test = qmc5883p_ctrl2_reg::self_test_t::enable;
                const reg_bitset expect_reg_val{std::string{"01000000"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }
        }

        SECTION("RNG")
        {
            SECTION("If RNG == 30G")
            {
                reg.rng = qmc5883p_ctrl2_reg::rng_t::full_scale_30g;
                const reg_bitset expect_reg_val{std::string{"00000000"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }

            SECTION("If RNG == 12G")
            {
                reg.rng = qmc5883p_ctrl2_reg::rng_t::full_scale_12g;
                const reg_bitset expect_reg_val{std::string{"00000100"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }

            SECTION("If RNG == 8G")
            {
                reg.rng = qmc5883p_ctrl2_reg::rng_t::full_scale_8g;
                const reg_bitset expect_reg_val{std::string{"00001000"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }

            SECTION("If RNG == 2G")
            {
                reg.rng = qmc5883p_ctrl2_reg::rng_t::full_scale_2g;
                const reg_bitset expect_reg_val{std::string{"00001100"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }
        }

        SECTION("SET/RESET MODE")
        {
            SECTION("If SET/RESET MODE == set and reset on")
            {
                reg.set_reset_mode =
                    qmc5883p_ctrl2_reg::set_reset_mode_t::set_and_reset_on;
                const reg_bitset expect_reg_val{std::string{"00000000"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }

            SECTION("If SET/RESET MODE == set only on")
            {
                reg.set_reset_mode =
                    qmc5883p_ctrl2_reg::set_reset_mode_t::set_only_on;
                const reg_bitset expect_reg_val{std::string{"00000001"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }

            SECTION("If SET/RESET MODE == set and reset off")
            {
                reg.set_reset_mode =
                    qmc5883p_ctrl2_reg::set_reset_mode_t::set_and_reset_off;
                const reg_bitset expect_reg_val{std::string{"00000010"}};
                REQUIRE(static_cast<uint8_t>(reg) == expect_reg_val);
            }
        }
    }

    SECTION("qmc5883p_status_reg")
    {
        SECTION("All bits is reset")
        {
            const reg_bitset    expect_reg_val{std::string{"00000000"}};

            qmc5883p_status_reg status_reg{
                static_cast<std::uint8_t>(expect_reg_val.to_ulong())};
            REQUIRE(status_reg.ovfl == qmc5883p_status_reg::ovfl_t::normal);
            REQUIRE(status_reg.drdy
                    == qmc5883p_status_reg::drdy_t::no_new_data);
        }

        SECTION("All bits is reset (check operator =)")
        {
            const reg_bitset    expect_reg_val{std::string{"00000000"}};

            qmc5883p_status_reg status_reg{0xFF};
            REQUIRE_FALSE(status_reg.ovfl
                          == qmc5883p_status_reg::ovfl_t::normal);
            REQUIRE_FALSE(status_reg.drdy
                          == qmc5883p_status_reg::drdy_t::no_new_data);

            status_reg = static_cast<std::uint8_t>(expect_reg_val.to_ulong());
            REQUIRE(status_reg.ovfl == qmc5883p_status_reg::ovfl_t::normal);
            REQUIRE(status_reg.drdy
                    == qmc5883p_status_reg::drdy_t::no_new_data);
        }

        SECTION("Set only DRDY bit")
        {
            const reg_bitset    expect_reg_val{std::string{"00000001"}};

            qmc5883p_status_reg status_reg{
                static_cast<std::uint8_t>(expect_reg_val.to_ulong())};
            REQUIRE(status_reg.ovfl == qmc5883p_status_reg::ovfl_t::normal);
            REQUIRE(status_reg.drdy
                    == qmc5883p_status_reg::drdy_t::new_data_is_ready);
        }

        SECTION("Set only OVFL bit")
        {
            const reg_bitset    expect_reg_val{std::string{"00000010"}};

            qmc5883p_status_reg status_reg{
                static_cast<std::uint8_t>(expect_reg_val.to_ulong())};
            REQUIRE(status_reg.ovfl
                    == qmc5883p_status_reg::ovfl_t::data_overflow);
            REQUIRE(status_reg.drdy
                    == qmc5883p_status_reg::drdy_t::no_new_data);
        }

        SECTION("Set DRDY and OVFL bits")
        {
            const reg_bitset    expect_reg_val{std::string{"00000011"}};

            qmc5883p_status_reg status_reg{
                static_cast<std::uint8_t>(expect_reg_val.to_ulong())};
            REQUIRE(status_reg.ovfl
                    == qmc5883p_status_reg::ovfl_t::data_overflow);
            REQUIRE(status_reg.drdy
                    == qmc5883p_status_reg::drdy_t::new_data_is_ready);
        }
    }

    SECTION("qmc5883p_chip_id_reg")
    {
        SECTION("Check address")
        { REQUIRE(qmc5883p_chip_id_reg::addr == 0x00); }

        SECTION("Check expected value")
        { REQUIRE(qmc5883p_chip_id_reg::expected_value == 0x80); }

        SECTION("Check default value")
        {
            qmc5883p_chip_id_reg reg;
            REQUIRE(static_cast<std::uint8_t>(reg) == 0x00);
        }

        SECTION("Check explicit value")
        {
            qmc5883p_chip_id_reg reg{0x80};
            REQUIRE(static_cast<std::uint8_t>(reg) == 0x80);
            REQUIRE(reg == qmc5883p_chip_id_reg{0x80});
        }

        SECTION("Check operator =")
        {
            qmc5883p_chip_id_reg reg;
            reg = 0x80;
            REQUIRE(static_cast<std::uint8_t>(reg) == 0x80);
        }
    }
}

SCENARIO(
    "QMC5883P Driver")
{
    using namespace fakeit;
    using namespace stv;

    GIVEN("Default initialize setup")
    {
        qmc5883p_setup setup;

        WHEN("Initialize with default setup")
        {
            qmc5883p<stv::mag<float>> driver{setup};

            THEN("Driver is not valid object") { REQUIRE_FALSE(driver); }
        }
    }

    GIVEN("Initialize setup with valid i2c_interface")
    {
        using reg_bitset = std::bitset<8>;

        Mock<stv::i2c_interface> i2c_mock;
        qmc5883p_setup           setup;
        setup.i2c = &i2c_mock.get();

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

        std::size_t                  meas_read_cnt{0};
        std::array<std::uint8_t, 6U> data1{0x64, 0x00, 0x64, 0x00, 0x64, 0x00};
        std::array<std::uint8_t, 6U> data2{0xC8, 0x00, 0xC8, 0x00, 0xC8, 0x00};

        When(Method(i2c_mock, read))
            .AlwaysDo([&](stv::i2c_interface::byte_type slave_addr,
                          stv::i2c_interface::byte_type reg_addr, void *dst,
                          std::size_t len) -> bool {
                (void)slave_addr;
                const auto addr = static_cast<std::uint8_t>(reg_addr);

                if((addr == static_cast<std::uint8_t>(0x01)) && (len == 6U))
                {
                    const auto &src =
                        (meas_read_cnt % 2U == 0U) ? data1 : data2;
                    std::memcpy(dst, src.data(), src.size());
                    ++meas_read_cnt;
                }
                else if(addr
                        == static_cast<std::uint8_t>(qmc5883p_status_reg::addr))
                {
                    *static_cast<std::uint8_t *>(dst) = 0x01;
                }
                else
                {
                    const auto it = reg_map.find(addr);
                    *static_cast<std::uint8_t *>(dst) =
                        (it != reg_map.end()) ? it->second : 0x00;
                }
                return true;
            });

        WHEN("Initialize without self-test")
        {
            qmc5883p<stv::mag<float>> driver{setup};
            const auto                init_result = driver.init(setup);

            THEN("Driver is valid object")
            {
                REQUIRE(driver);
                REQUIRE(driver.is_initialized());
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

        WHEN("Initialize with self-test")
        {
            std::uint32_t             delay_ms{0};
            qmc5883p<stv::mag<float>> driver{setup};
            const auto                init_result = driver.init(
                setup, true, [&delay_ms](std::uint32_t ms) { delay_ms = ms; });

            THEN("Init succeeds, self-test is valid and delay callback is "
                 "called with 5 ms")
            {
                REQUIRE(init_result);
                REQUIRE(driver.is_self_test_valid());
                REQUIRE(delay_ms == 5);
            }
        }

        WHEN("Check self-test with delay callback")
        {
            std::uint32_t             delay_ms{0};
            qmc5883p<stv::mag<float>> driver{setup};
            driver.init(setup);

            const auto self_test_result = driver.check_self_test(
                [&delay_ms](std::uint32_t ms) { delay_ms = ms; });

            THEN("Delay callback is called with 5 ms")
            { REQUIRE(delay_ms == 5); }

            THEN("Self-test passes") { REQUIRE(self_test_result); }
        }

        WHEN("Self-test fails due to zero delta")
        {
            data2 = data1;
            qmc5883p<stv::mag<float>> driver{setup};
            const auto                init_result =
                driver.init(setup, true, [](std::uint32_t) {});

            THEN("Self-test fails, I2C interface stays valid")
            {
                REQUIRE_FALSE(init_result);
                REQUIRE(driver);
                REQUIRE_FALSE(driver.is_self_test_valid());
            }
        }

        WHEN("Self-test restores CTRL2")
        {
            qmc5883p<stv::mag<float>> driver{setup};
            driver.init(setup);
            driver.check_self_test([](std::uint32_t) {});

            THEN("CTRL2 self-test bit is cleared")
            {
                const qmc5883p_ctrl2_reg final_ctrl2{
                    reg_map[static_cast<std::uint8_t>(
                        qmc5883p_ctrl2_reg::addr)]};
                REQUIRE(final_ctrl2.self_test
                        == qmc5883p_ctrl2_reg::self_test_t::normal);
            }
        }

        WHEN("Read measurement data")
        {
            qmc5883p<stv::mag<float>> driver{setup};
            driver.init(setup);

            THEN("read_meas_raw returns valid data")
            {
                const auto raw = driver.read_meas_raw();
                REQUIRE(raw);
                REQUIRE(raw.x == 100);
                REQUIRE(raw.y == 100);
                REQUIRE(raw.z == 100);
            }

            AND_WHEN("Status register has overflow bit set")
            {
                When(Method(i2c_mock, read))
                    .AlwaysDo([&](stv::i2c_interface::byte_type slave_addr,
                                  stv::i2c_interface::byte_type reg_addr,
                                  void *dst, std::size_t len) -> bool {
                        (void)slave_addr;
                        const auto addr = static_cast<std::uint8_t>(reg_addr);

                        if((addr == static_cast<std::uint8_t>(0x01))
                           && (len == 6U))
                        {
                            constexpr std::array<std::uint8_t, 6U> data{
                                0x64, 0x00, 0x64, 0x00, 0x64, 0x00};
                            std::memcpy(dst, data.data(), data.size());
                        }
                        else if(addr
                                == static_cast<std::uint8_t>(
                                    qmc5883p_status_reg::addr))
                        {
                            *static_cast<std::uint8_t *>(dst) = 0x02;
                        }
                        return true;
                    });

                THEN("read_meas_raw returns invalid data")
                {
                    const auto raw = driver.read_meas_raw();
                    REQUIRE_FALSE(raw);
                }
            }

            AND_WHEN("All axes are zero")
            {
                When(Method(i2c_mock, read))
                    .AlwaysDo([&](stv::i2c_interface::byte_type slave_addr,
                                  stv::i2c_interface::byte_type reg_addr,
                                  void *dst, std::size_t len) -> bool {
                        (void)slave_addr;
                        const auto addr = static_cast<std::uint8_t>(reg_addr);

                        if((addr == static_cast<std::uint8_t>(0x01))
                           && (len == 6U))
                        {
                            constexpr std::array<std::uint8_t, 6U> data{};
                            std::memcpy(dst, data.data(), data.size());
                        }
                        else if(addr
                                == static_cast<std::uint8_t>(
                                    qmc5883p_status_reg::addr))
                        {
                            *static_cast<std::uint8_t *>(dst) = 0x01;
                        }
                        return true;
                    });

                THEN("read_meas_raw returns invalid data")
                {
                    const auto raw = driver.read_meas_raw();
                    REQUIRE_FALSE(raw);
                }
            }

            AND_WHEN("I2C read of axis data fails")
            {
                When(Method(i2c_mock, read))
                    .AlwaysDo([&](stv::i2c_interface::byte_type slave_addr,
                                  stv::i2c_interface::byte_type reg_addr,
                                  void *dst, std::size_t len) -> bool {
                        (void)slave_addr;
                        const auto addr = static_cast<std::uint8_t>(reg_addr);

                        if((addr == static_cast<std::uint8_t>(0x01))
                           && (len == 6U))
                        {
                            return false;
                        }

                        if(addr
                           == static_cast<std::uint8_t>(
                               qmc5883p_status_reg::addr))
                        {
                            *static_cast<std::uint8_t *>(dst) = 0x01;
                        }
                        return true;
                    });

                THEN("read_meas_raw returns invalid data")
                {
                    const auto raw = driver.read_meas_raw();
                    REQUIRE_FALSE(raw);
                }
            }
        }

        WHEN("Normalize measurement for ±30 Gs range")
        {
            qmc5883p<stv::mag<float>> driver{setup};
            driver.init(setup);

            const qmc5883p<stv::mag<float>>::raw_t raw_meas{100, 200, -300,
                                                            0x01};
            const auto normalized = driver.normalize(raw_meas);

            THEN("Normalized values are multiplied by g_per_lsb")
            {
                constexpr float g_per_lsb = 1.0F / 1000.0F;
                REQUIRE(normalized.give_x()
                        == Catch::Approx(100.0F * g_per_lsb));
                REQUIRE(normalized.give_y()
                        == Catch::Approx(200.0F * g_per_lsb));
                REQUIRE(normalized.give_z()
                        == Catch::Approx(-300.0F * g_per_lsb));
            }
        }
    }

    GIVEN("Initialize setup with i2c_interface that fails")
    {
        Mock<stv::i2c_interface> i2c_mock;
        qmc5883p_setup           setup;
        setup.i2c = &i2c_mock.get();

        WHEN("I2C write fails")
        {
            When(Method(i2c_mock, write))
                .AlwaysDo([&](stv::i2c_interface::byte_type slave_addr,
                              stv::i2c_interface::byte_type reg_addr,
                              stv::i2c_interface::byte_type value) -> bool {
                    (void)slave_addr;
                    (void)value;
                    const auto addr = static_cast<std::uint8_t>(reg_addr);

                    if(addr
                       == static_cast<std::uint8_t>(qmc5883p_ctrl1_reg::addr))
                    {
                        return false;
                    }
                    return true;
                });

            When(Method(i2c_mock, read))
                .AlwaysDo([&](stv::i2c_interface::byte_type slave_addr,
                              stv::i2c_interface::byte_type reg_addr, void *dst,
                              std::size_t len) -> bool {
                    (void)slave_addr;
                    (void)reg_addr;
                    (void)len;
                    *static_cast<std::uint8_t *>(dst) = 0x00;
                    return true;
                });

            qmc5883p<stv::mag<float>> driver{setup};

            THEN("Init fails, I2C interface stays valid")
            {
                REQUIRE_FALSE(driver.init(setup));
                REQUIRE(driver);
                REQUIRE_FALSE(driver.is_initialized());
            }
        }

        WHEN("Read-back mismatch")
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
                    const auto addr = static_cast<std::uint8_t>(reg_addr);

                    if(addr
                       == static_cast<std::uint8_t>(qmc5883p_ctrl1_reg::addr))
                    {
                        *static_cast<std::uint8_t *>(dst) = 0xFF;
                        return true;
                    }

                    const auto it = reg_map.find(addr);
                    *static_cast<std::uint8_t *>(dst) =
                        (it != reg_map.end()) ? it->second : 0x00;
                    return true;
                });

            qmc5883p<stv::mag<float>> driver{setup};

            THEN("Init fails, I2C interface stays valid")
            {
                REQUIRE_FALSE(driver.init(setup));
                REQUIRE(driver);
                REQUIRE_FALSE(driver.is_initialized());
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
                        0x80; // Expected chip ID
                    return true;
                });

            THEN("is_detected returns true")
            {
                REQUIRE(
                    qmc5883p<stv::mag<float>>::is_detected(&i2c_mock.get()));
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
                    *static_cast<std::uint8_t *>(dst) = 0x00; // Wrong chip ID
                    return true;
                });

            THEN("is_detected returns false")
            {
                REQUIRE_FALSE(
                    qmc5883p<stv::mag<float>>::is_detected(&i2c_mock.get()));
            }
        }

        WHEN("I2C read fails")
        {
            When(Method(i2c_mock, read)).AlwaysReturn(false);

            THEN("is_detected returns false")
            {
                REQUIRE_FALSE(
                    qmc5883p<stv::mag<float>>::is_detected(&i2c_mock.get()));
            }
        }
    }
}

// NOLINTEND(*-magic-numbers, google-build-using-namespace,
// readability-function-cognitive-complexity,
// cppcoreguidelines-avoid-non-const-global-variables)
