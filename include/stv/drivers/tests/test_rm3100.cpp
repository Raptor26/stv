/// @file test_rm3100.cpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.

#include "stv/drivers/rm3100.hpp"
#include "stv/drivers/rm3100_regs.hpp"
#include "stv/gyraccmag_types.hpp"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <fakeit.hpp>

// NOLINTBEGIN(*-magic-numbers, google-build-using-namespace,
// readability-function-cognitive-,
// cppcoreguidelines-avoid-non-const-global-variables)

TEST_CASE(
    "rm3100", "[stv][drivers]")
{
    using namespace fakeit;
    using mag_type          = stv::mag<float, std::uint32_t>;
    using rm3100_setup_type = stv::rm3100_setup<mag_type>;
    using rm3100_type       = stv::rm3100<stv::rm3100_setup<mag_type>>;

    SECTION("Default setup has no I2C")
    {
        const rm3100_setup_type setup;
        REQUIRE_FALSE(rm3100_type{setup});
    }
}

TEST_CASE(
    "rm3100 registers", "[stv][drivers]")
{
    SECTION("CMM default")
    {
        stv::rm3100_cmm_reg reg;
        REQUIRE_FALSE(reg.start);
        REQUIRE(reg.drdm == stv::rm3100_cmm_drdm::after_all_axes);
        REQUIRE_FALSE(reg.cmx);
        REQUIRE_FALSE(reg.cmy);
        REQUIRE_FALSE(reg.cmz);
    }

    SECTION("CMM set all axes")
    {
        stv::rm3100_cmm_reg reg;
        reg.start = true;
        reg.cmx   = true;
        reg.cmy   = true;
        reg.cmz   = true;
        REQUIRE(reg.start);
        REQUIRE(reg.cmx);
        REQUIRE(reg.cmy);
        REQUIRE(reg.cmz);
    }

    SECTION("CC default cycle count")
    {
        REQUIRE(stv::rm3100_ccx_reg{}.cycle_count == 200);
        REQUIRE(stv::rm3100_ccy_reg{}.cycle_count == 200);
        REQUIRE(stv::rm3100_ccz_reg{}.cycle_count == 200);
    }

    SECTION("STATUS drdy")
    {
        stv::rm3100_status_reg reg;
        reg.drdy = true;
        REQUIRE(reg.drdy);
    }

    SECTION("POLL cast to raw value")
    {
        stv::rm3100_poll_reg reg;
        reg.pmx = true;
        reg.pmy = true;
        reg.pmz = true;
        REQUIRE(static_cast<stv::rm3100_reg_type>(reg) == 0x70U);
    }

    SECTION("CMM cast and parse round-trip")
    {
        stv::rm3100_cmm_reg reg;
        reg.start      = true;
        reg.drdm       = stv::rm3100_cmm_drdm::after_all_axes;
        reg.cmx        = true;
        reg.cmy        = true;
        reg.cmz        = true;
        const auto raw = static_cast<stv::rm3100_reg_type>(reg);
        REQUIRE(raw == 0b01110001);

        const stv::rm3100_cmm_reg parsed{raw};
        REQUIRE(parsed.start);
        REQUIRE(parsed.drdm == stv::rm3100_cmm_drdm::after_all_axes);
        REQUIRE(parsed.cmx);
        REQUIRE(parsed.cmy);
        REQUIRE(parsed.cmz);
    }

    SECTION("STATUS parse from raw value")
    {
        const stv::rm3100_status_reg reg{0x80U};
        REQUIRE(reg.drdy);
    }

    SECTION("TMRC equality")
    {
        stv::rm3100_tmrc_reg reg1{0x96U};
        stv::rm3100_tmrc_reg reg2{0x96U};
        stv::rm3100_tmrc_reg reg3{0x92U};
        REQUIRE(reg1 == reg2);
        REQUIRE_FALSE(reg1 == reg3);
    }
}

TEST_CASE(
    "rm3100 detection and init", "[stv][drivers]")
{
    using namespace fakeit;
    using mag_type = stv::mag<float, std::uint32_t>;
    using rm3100   = stv::rm3100<stv::rm3100_setup<mag_type>>;

    Mock<stv::i2c_interface> i2c;
    Fake(Method(i2c, write));
    Fake(Method(i2c, read));

    stv::rm3100_setup<mag_type> setup{{.i2c = &i2c.get()}};
    rm3100                      mag{setup};
    REQUIRE(mag);

    SECTION("is_detected returns true for expected REVID")
    {
        When(Method(i2c, read))
            .AlwaysDo([](auto slave_addr, auto reg_addr, void *dst, auto len) {
                (void)slave_addr;
                (void)reg_addr;
                (void)len;
                *static_cast<stv::rm3100_reg_type *>(dst) =
                    stv::rm3100_revid_reg::expected_value;
                return true;
            });
        REQUIRE(rm3100::is_detected(&i2c.get()));
    }

    SECTION("is_detected returns false for wrong REVID")
    {
        When(Method(i2c, read))
            .AlwaysDo([](auto slave_addr, auto reg_addr, void *dst, auto len) {
                (void)slave_addr;
                (void)reg_addr;
                (void)len;
                *static_cast<stv::rm3100_reg_type *>(dst) = 0xFF;
                return true;
            });
        REQUIRE_FALSE(rm3100::is_detected(&i2c.get()));
    }

    SECTION("init starts CMM")
    {
        stv::rm3100_regs_setup regs{};
        regs.cmm.start = true;
        regs.cmm.cmx   = true;
        regs.cmm.cmy   = true;
        regs.cmm.cmz   = true;

        stv::rm3100_reg_type latest_written_reg_value{0x00};
        When(Method(i2c, write))
            .AlwaysDo([&](auto slave_addr, auto reg_addr, auto value) {
                (void)slave_addr;
                (void)reg_addr;
                latest_written_reg_value =
                    static_cast<stv::rm3100_reg_type>(value);
                return true;
            });
        When(Method(i2c, read))
            .AlwaysDo([&](auto slave_addr, auto reg_addr, void *dst, auto len) {
                (void)slave_addr;
                (void)reg_addr;
                auto *buf = static_cast<stv::rm3100_reg_type *>(dst);
                for(std::size_t i = 0; i < len; ++i)
                {
                    buf[i] = latest_written_reg_value;
                }
                // cycle count 200 == 0x00C8
                if(len == 6)
                {
                    buf[0] = 0x00;
                    buf[1] = 0xC8;
                    buf[2] = 0x00;
                    buf[3] = 0xC8;
                    buf[4] = 0x00;
                    buf[5] = 0xC8;
                }
                return true;
            });

        REQUIRE(mag.init(regs));
        REQUIRE(mag.is_continuous_active());
    }

    SECTION("init polling mode")
    {
        stv::rm3100_regs_setup regs;
        regs.cmm.start = false;

        stv::rm3100_reg_type latest_written_reg_value{0x00};
        When(Method(i2c, write))
            .AlwaysDo([&](auto slave_addr, auto reg_addr, auto value) {
                (void)slave_addr;
                (void)reg_addr;
                latest_written_reg_value =
                    static_cast<stv::rm3100_reg_type>(value);
                return true;
            });
        When(Method(i2c, read))
            .AlwaysDo([&](auto slave_addr, auto reg_addr, void *dst, auto len) {
                (void)slave_addr;
                (void)reg_addr;
                auto *buf = static_cast<stv::rm3100_reg_type *>(dst);
                for(std::size_t i = 0; i < len; ++i)
                {
                    buf[i] = latest_written_reg_value;
                }
                if(len == 6)
                {
                    buf[0] = 0x00;
                    buf[1] = 0xC8;
                    buf[2] = 0x00;
                    buf[3] = 0xC8;
                    buf[4] = 0x00;
                    buf[5] = 0xC8;
                }
                return true;
            });

        REQUIRE(mag.init(regs));
        REQUIRE_FALSE(mag.is_continuous_active());
    }

    SECTION("request_single stops CMM first")
    {
        stv::rm3100_regs_setup regs;
        regs.cmm.start = true;
        regs.cmm.cmx   = true;
        regs.cmm.cmy   = true;
        regs.cmm.cmz   = true;

        stv::rm3100_reg_type latest_written_reg_value{0x00};
        When(Method(i2c, write))
            .AlwaysDo([&](auto slave_addr, auto reg_addr, auto value) {
                (void)slave_addr;
                (void)reg_addr;
                latest_written_reg_value =
                    static_cast<stv::rm3100_reg_type>(value);
                return true;
            });
        When(Method(i2c, read))
            .AlwaysDo([&](auto slave_addr, auto reg_addr, void *dst, auto len) {
                (void)slave_addr;
                (void)reg_addr;
                auto *buf = static_cast<stv::rm3100_reg_type *>(dst);
                for(std::size_t i = 0; i < len; ++i)
                {
                    buf[i] = latest_written_reg_value;
                }
                if(len == 6)
                {
                    buf[0] = 0x00;
                    buf[1] = 0xC8;
                    buf[2] = 0x00;
                    buf[3] = 0xC8;
                    buf[4] = 0x00;
                    buf[5] = 0xC8;
                }
                return true;
            });

        REQUIRE(mag.init(regs));
        REQUIRE(mag.is_continuous_active());

        stv::rm3100_poll_reg poll;
        poll.pmx = true;
        poll.pmy = true;
        poll.pmz = true;
        REQUIRE(mag.request_single(poll));
        REQUIRE_FALSE(mag.is_continuous_active());
    }
}

TEST_CASE(
    "rm3100 measurement reading", "[stv][drivers]")
{
    using namespace fakeit;
    using mag_type = stv::mag<float, std::uint32_t>;
    using rm3100   = stv::rm3100<stv::rm3100_setup<mag_type>>;

    Mock<stv::i2c_interface> i2c;
    Fake(Method(i2c, write));
    Fake(Method(i2c, read));

    stv::rm3100_setup<mag_type> setup{{.i2c = &i2c.get()}};
    rm3100                      mag{setup};

    stv::rm3100_regs_setup      regs;
    regs.ccx.cycle_count = 200;
    regs.ccy.cycle_count = 200;
    regs.ccz.cycle_count = 200;

    stv::rm3100_reg_type latest_written_reg_value{0x00};
    When(Method(i2c, write))
        .AlwaysDo([&](auto slave_addr, auto reg_addr, auto value) {
            (void)slave_addr;
            (void)reg_addr;
            latest_written_reg_value = static_cast<stv::rm3100_reg_type>(value);
            return true;
        });
    When(Method(i2c, read))
        .AlwaysDo([&](auto slave_addr, auto reg_addr, void *dst, auto len) {
            (void)slave_addr;
            (void)reg_addr;
            auto *buf = static_cast<stv::rm3100_reg_type *>(dst);

            if(len == 6)
            {
                buf[0] = 0x00;
                buf[1] = 0xC8;
                buf[2] = 0x00;
                buf[3] = 0xC8;
                buf[4] = 0x00;
                buf[5] = 0xC8;
                return true;
            }

            if(len == 9)
            {
                // X = +75 LSB, Y = -75 LSB, Z = 0
                // 75 = 0x00004B
                buf[0] = 0x00;
                buf[1] = 0x00;
                buf[2] = 0x4B;
                // -75 = 0xFFFFB5 (24-bit two's complement)
                buf[3] = 0xFF;
                buf[4] = 0xFF;
                buf[5] = 0xB5;
                buf[6] = 0x00;
                buf[7] = 0x00;
                buf[8] = 0x00;
                return true;
            }

            buf[0] = latest_written_reg_value;
            for(std::size_t i = 1; i < len; ++i)
            {
                buf[i] = 0x00;
            }
            return true;
        });

    REQUIRE(mag.init(regs));

    const auto field = mag.read_normalized();
    REQUIRE(field);
    REQUIRE(field.packstamp == 1);
    // 75 LSB / (75 LSB/uT * 100 uT/Gs) = 0.01 Gs
    REQUIRE(field.give_x() == Catch::Approx(0.01F));
    REQUIRE(field.give_y() == Catch::Approx(-0.01F));
    REQUIRE(field.give_z() == Catch::Approx(0.0F));

    REQUIRE(mag.get_mag().packstamp == 1);
}

TEST_CASE(
    "rm3100 measurement edge cases", "[stv][drivers]")
{
    using namespace fakeit;
    using mag_type = stv::mag<float, std::uint32_t>;
    using rm3100   = stv::rm3100<stv::rm3100_setup<mag_type>>;

    SECTION("raw_t rejects all-zero and INT24 boundary values")
    {
        using raw_t = rm3100::raw_t;

        raw_t all_zero{};
        REQUIRE_FALSE(all_zero);

        raw_t valid{1, 1, 1};
        REQUIRE(valid);

        raw_t min_x{-0x800000, 1, 1};
        REQUIRE_FALSE(min_x);

        raw_t max_x{0x7FFFFF, 1, 1};
        REQUIRE_FALSE(max_x);

        raw_t min_y{1, -0x800000, 1};
        REQUIRE_FALSE(min_y);

        raw_t max_y{1, 0x7FFFFF, 1};
        REQUIRE_FALSE(max_y);

        raw_t min_z{1, 1, -0x800000};
        REQUIRE_FALSE(min_z);

        raw_t max_z{1, 1, 0x7FFFFF};
        REQUIRE_FALSE(max_z);
    }

    SECTION("normalization scales with cycle count 100")
    {
        Mock<stv::i2c_interface> i2c;
        Fake(Method(i2c, write));
        Fake(Method(i2c, read));

        stv::rm3100_setup<mag_type> setup{{.i2c = &i2c.get()}};
        rm3100                      mag{setup};

        stv::rm3100_regs_setup      regs;
        regs.ccx.cycle_count = 100;
        regs.ccy.cycle_count = 100;
        regs.ccz.cycle_count = 100;

        stv::rm3100_reg_type latest_written_reg_value{0x00};
        When(Method(i2c, write))
            .AlwaysDo([&](auto slave_addr, auto reg_addr, auto value) {
                (void)slave_addr;
                (void)reg_addr;
                latest_written_reg_value =
                    static_cast<stv::rm3100_reg_type>(value);
                return true;
            });
        When(Method(i2c, read))
            .AlwaysDo([&](auto slave_addr, auto reg_addr, void *dst, auto len) {
                (void)slave_addr;
                (void)reg_addr;
                auto *buf = static_cast<stv::rm3100_reg_type *>(dst);

                if(len == 6)
                {
                    buf[0] = 0x00;
                    buf[1] = 0x64;
                    buf[2] = 0x00;
                    buf[3] = 0x64;
                    buf[4] = 0x00;
                    buf[5] = 0x64;
                    return true;
                }

                if(len == 9)
                {
                    // Cycle count 100 -> 38 LSB/uT.
                    // 38 LSB / (38 LSB/uT * 100 uT/Gs) = 0.01 Gs.
                    buf[0] = 0x00;
                    buf[1] = 0x00;
                    buf[2] = 0x26;
                    buf[3] = 0xFF;
                    buf[4] = 0xFF;
                    buf[5] = 0xDA;
                    buf[6] = 0x00;
                    buf[7] = 0x00;
                    buf[8] = 0x00;
                    return true;
                }

                buf[0] = latest_written_reg_value;
                for(std::size_t i = 1; i < len; ++i)
                {
                    buf[i] = 0x00;
                }
                return true;
            });

        REQUIRE(mag.init(regs));

        const auto field = mag.read_normalized();
        REQUIRE(field);
        REQUIRE(field.give_x() == Catch::Approx(0.01F));
        REQUIRE(field.give_y() == Catch::Approx(-0.01F));
        REQUIRE(field.give_z() == Catch::Approx(0.0F));
    }

    SECTION("normalization scales with non-table cycle count 400")
    {
        Mock<stv::i2c_interface> i2c;
        Fake(Method(i2c, write));
        Fake(Method(i2c, read));

        stv::rm3100_setup<mag_type> setup{{.i2c = &i2c.get()}};
        rm3100                      mag{setup};

        stv::rm3100_regs_setup      regs;
        regs.ccx.cycle_count = 400;
        regs.ccy.cycle_count = 400;
        regs.ccz.cycle_count = 400;

        stv::rm3100_reg_type latest_written_reg_value{0x00};
        When(Method(i2c, write))
            .AlwaysDo([&](auto slave_addr, auto reg_addr, auto value) {
                (void)slave_addr;
                (void)reg_addr;
                latest_written_reg_value =
                    static_cast<stv::rm3100_reg_type>(value);
                return true;
            });
        When(Method(i2c, read))
            .AlwaysDo([&](auto slave_addr, auto reg_addr, void *dst, auto len) {
                (void)slave_addr;
                (void)reg_addr;
                auto *buf = static_cast<stv::rm3100_reg_type *>(dst);

                if(len == 6)
                {
                    buf[0] = 0x01;
                    buf[1] = 0x90;
                    buf[2] = 0x01;
                    buf[3] = 0x90;
                    buf[4] = 0x01;
                    buf[5] = 0x90;
                    return true;
                }

                if(len == 9)
                {
                    // Cycle count 400 -> 75 * 200 / 400 = 150 LSB/uT.
                    // 150 LSB / (150 LSB/uT * 100 uT/Gs) = 0.01 Gs.
                    buf[0] = 0x00;
                    buf[1] = 0x00;
                    buf[2] = 0x96;
                    buf[3] = 0xFF;
                    buf[4] = 0xFF;
                    buf[5] = 0x6A;
                    buf[6] = 0x00;
                    buf[7] = 0x00;
                    buf[8] = 0x00;
                    return true;
                }

                buf[0] = latest_written_reg_value;
                for(std::size_t i = 1; i < len; ++i)
                {
                    buf[i] = 0x00;
                }
                return true;
            });

        REQUIRE(mag.init(regs));

        const auto field = mag.read_normalized();
        REQUIRE(field);
        REQUIRE(field.give_x() == Catch::Approx(0.01F));
        REQUIRE(field.give_y() == Catch::Approx(-0.01F));
        REQUIRE(field.give_z() == Catch::Approx(0.0F));
    }
}

TEST_CASE(
    "rm3100 error handling", "[stv][drivers]")
{
    using namespace fakeit;
    using mag_type = stv::mag<float, std::uint32_t>;
    using rm3100   = stv::rm3100<stv::rm3100_setup<mag_type>>;

    Mock<stv::i2c_interface> i2c;
    Fake(Method(i2c, write));
    Fake(Method(i2c, read));

    stv::rm3100_setup<mag_type> setup{{.i2c = &i2c.get()}};
    rm3100                      mag{setup};

    SECTION("read returns invalid raw")
    {
        stv::rm3100_regs_setup regs;
        When(Method(i2c, write)).AlwaysReturn(true);
        When(Method(i2c, read))
            .AlwaysDo([](auto slave_addr, auto reg_addr, void *dst, auto len) {
                (void)slave_addr;
                (void)reg_addr;
                auto *buf = static_cast<stv::rm3100_reg_type *>(dst);
                for(std::size_t i = 0; i < len; ++i)
                {
                    buf[i] = 0xFF;
                }
                if(len == 6)
                {
                    buf[0] = 0x00;
                    buf[1] = 0xC8;
                    buf[2] = 0x00;
                    buf[3] = 0xC8;
                    buf[4] = 0x00;
                    buf[5] = 0xC8;
                }
                if(len == 1)
                {
                    // Default TMRC value so init() read-back succeeds.
                    buf[0] = 0x96;
                }
                if(len == 9)
                {
                    // INT24_MIN on all axes -> raw_t rejects the reading.
                    for(std::size_t i = 0; i < 9; i += 3)
                    {
                        buf[i]     = 0x80;
                        buf[i + 1] = 0x00;
                        buf[i + 2] = 0x00;
                    }
                }
                return true;
            });

        REQUIRE(mag.init(regs));
        const auto field = mag.read_normalized();
        REQUIRE_FALSE(field);
    }

    SECTION("I2C read error returns invalid field")
    {
        stv::rm3100_regs_setup regs;
        When(Method(i2c, write)).AlwaysReturn(true);
        When(Method(i2c, read)).AlwaysReturn(false);
        REQUIRE_FALSE(mag.init(regs));
    }
}

TEST_CASE(
    "rm3100 self test", "[stv][drivers]")
{
    using namespace fakeit;
    using mag_type = stv::mag<float, std::uint32_t>;
    using rm3100   = stv::rm3100<stv::rm3100_setup<mag_type>>;

    Mock<stv::i2c_interface> i2c;
    Fake(Method(i2c, write));
    Fake(Method(i2c, read));

    stv::rm3100_setup<mag_type> setup{{.i2c = &i2c.get()}};
    rm3100                      mag{setup};

    stv::rm3100_reg_type        latest_written_reg_value{0x00};
    std::size_t                 status_read_count{0};

    When(Method(i2c, write))
        .AlwaysDo([&](auto slave_addr, auto reg_addr, auto value) {
            (void)slave_addr;
            (void)reg_addr;
            latest_written_reg_value = static_cast<stv::rm3100_reg_type>(value);
            return true;
        });

    SECTION("run_self_test succeeds when all axes report OK")
    {
        When(Method(i2c, read))
            .AlwaysDo([&](auto slave_addr, auto reg_addr, void *dst, auto len) {
                (void)slave_addr;
                auto *buf = static_cast<stv::rm3100_reg_type *>(dst);

                if(len == 6)
                {
                    buf[0] = 0x00;
                    buf[1] = 0xC8;
                    buf[2] = 0x00;
                    buf[3] = 0xC8;
                    buf[4] = 0x00;
                    buf[5] = 0xC8;
                    return true;
                }

                if(reg_addr
                   == static_cast<stv::i2c_interface::byte_type>(
                       stv::rm3100_status_reg::addr))
                {
                    ++status_read_count;
                    buf[0] = (status_read_count >= 2) ? 0x80U : 0x00U;
                    return true;
                }

                if(reg_addr
                   == static_cast<stv::i2c_interface::byte_type>(
                       stv::rm3100_bist_reg::addr))
                {
                    // STE=1, XOK=YOK=ZOK=1.
                    buf[0] = 0xF0U;
                    return true;
                }

                buf[0] = latest_written_reg_value;
                return true;
            });

        REQUIRE(mag.init(stv::rm3100_regs_setup{}));
        const auto no_delay = [](std::uint16_t) {};
        REQUIRE(mag.run_self_test(no_delay));
        // Последняя запись — сброс STE в BIST.
        REQUIRE(latest_written_reg_value == 0x00U);
    }

    SECTION("run_self_test fails when one axis reports not OK")
    {
        When(Method(i2c, read))
            .AlwaysDo([&](auto slave_addr, auto reg_addr, void *dst, auto len) {
                (void)slave_addr;
                auto *buf = static_cast<stv::rm3100_reg_type *>(dst);

                if(len == 6)
                {
                    buf[0] = 0x00;
                    buf[1] = 0xC8;
                    buf[2] = 0x00;
                    buf[3] = 0xC8;
                    buf[4] = 0x00;
                    buf[5] = 0xC8;
                    return true;
                }

                if(reg_addr
                   == static_cast<stv::i2c_interface::byte_type>(
                       stv::rm3100_status_reg::addr))
                {
                    buf[0] = 0x80U;
                    return true;
                }

                if(reg_addr
                   == static_cast<stv::i2c_interface::byte_type>(
                       stv::rm3100_bist_reg::addr))
                {
                    // STE=1, XOK=0, YOK=1, ZOK=1.
                    buf[0] = 0xE0U;
                    return true;
                }

                buf[0] = latest_written_reg_value;
                return true;
            });

        REQUIRE(mag.init(stv::rm3100_regs_setup{}));
        const auto no_delay = [](std::uint16_t) {};
        REQUIRE_FALSE(mag.run_self_test(no_delay));
    }

    SECTION("run_self_test fails on DRDY timeout")
    {
        When(Method(i2c, read))
            .AlwaysDo([&](auto slave_addr, auto reg_addr, void *dst, auto len) {
                (void)slave_addr;
                auto *buf = static_cast<stv::rm3100_reg_type *>(dst);

                if(len == 6)
                {
                    buf[0] = 0x00;
                    buf[1] = 0xC8;
                    buf[2] = 0x00;
                    buf[3] = 0xC8;
                    buf[4] = 0x00;
                    buf[5] = 0xC8;
                    return true;
                }

                if(reg_addr
                   == static_cast<stv::i2c_interface::byte_type>(
                       stv::rm3100_status_reg::addr))
                {
                    buf[0] = 0x00U;
                    return true;
                }

                if(reg_addr
                   == static_cast<stv::i2c_interface::byte_type>(
                       stv::rm3100_bist_reg::addr))
                {
                    buf[0] = 0xF0U;
                    return true;
                }

                buf[0] = latest_written_reg_value;
                return true;
            });

        REQUIRE(mag.init(stv::rm3100_regs_setup{}));
        const auto no_delay = [](std::uint16_t) {};
        REQUIRE_FALSE(mag.run_self_test(std::uint16_t{2}, no_delay));
    }
}

TEST_CASE(
    "rm3100 public API methods", "[stv][drivers]")
{
    using namespace fakeit;
    using mag_type = stv::mag<float, std::uint32_t>;
    using rm3100   = stv::rm3100<stv::rm3100_setup<mag_type>>;

    Mock<stv::i2c_interface> i2c;
    Fake(Method(i2c, write));
    Fake(Method(i2c, read));

    stv::rm3100_setup<mag_type> setup{{.i2c = &i2c.get()}};
    rm3100                      mag{setup};

    stv::rm3100_reg_type        latest_written_reg_value{0x00};
    When(Method(i2c, write))
        .AlwaysDo([&](auto slave_addr, auto reg_addr, auto value) {
            (void)slave_addr;
            (void)reg_addr;
            latest_written_reg_value = static_cast<stv::rm3100_reg_type>(value);
            return true;
        });
    When(Method(i2c, read))
        .AlwaysDo([&](auto slave_addr, auto reg_addr, void *dst, auto len) {
            (void)slave_addr;
            (void)reg_addr;
            auto *buf = static_cast<stv::rm3100_reg_type *>(dst);
            for(std::size_t i = 0; i < len; ++i)
            {
                buf[i] = latest_written_reg_value;
            }
            if(len == 6)
            {
                buf[0] = 0x00;
                buf[1] = 0xC8;
                buf[2] = 0x00;
                buf[3] = 0xC8;
                buf[4] = 0x00;
                buf[5] = 0xC8;
            }
            return true;
        });

    SECTION("is_data_ready returns true when STATUS DRDY bit is set")
    {
        stv::rm3100_regs_setup regs;
        REQUIRE(mag.init(regs));

        When(Method(i2c, read))
            .AlwaysDo([&](auto slave_addr, auto reg_addr, void *dst, auto len) {
                (void)slave_addr;
                auto *buf = static_cast<stv::rm3100_reg_type *>(dst);
                if(len == 6)
                {
                    buf[0] = 0x00;
                    buf[1] = 0xC8;
                    buf[2] = 0x00;
                    buf[3] = 0xC8;
                    buf[4] = 0x00;
                    buf[5] = 0xC8;
                    return true;
                }
                if(reg_addr
                   == static_cast<stv::i2c_interface::byte_type>(
                       stv::rm3100_status_reg::addr))
                {
                    buf[0] = 0x80U;
                    return true;
                }
                buf[0] = latest_written_reg_value;
                return true;
            });
        REQUIRE(mag.is_data_ready());
    }

    SECTION("is_data_ready returns false when STATUS DRDY bit is cleared")
    {
        stv::rm3100_regs_setup regs;
        REQUIRE(mag.init(regs));

        When(Method(i2c, read))
            .AlwaysDo([&](auto slave_addr, auto reg_addr, void *dst, auto len) {
                (void)slave_addr;
                auto *buf = static_cast<stv::rm3100_reg_type *>(dst);
                if(len == 6)
                {
                    buf[0] = 0x00;
                    buf[1] = 0xC8;
                    buf[2] = 0x00;
                    buf[3] = 0xC8;
                    buf[4] = 0x00;
                    buf[5] = 0xC8;
                    return true;
                }
                if(reg_addr
                   == static_cast<stv::i2c_interface::byte_type>(
                       stv::rm3100_status_reg::addr))
                {
                    buf[0] = 0x00U;
                    return true;
                }
                buf[0] = latest_written_reg_value;
                return true;
            });
        REQUIRE_FALSE(mag.is_data_ready());
    }

    SECTION("read_status_reg returns the STATUS register value")
    {
        stv::rm3100_regs_setup regs;
        REQUIRE(mag.init(regs));

        When(Method(i2c, read))
            .AlwaysDo([&](auto slave_addr, auto reg_addr, void *dst, auto len) {
                (void)slave_addr;
                auto *buf = static_cast<stv::rm3100_reg_type *>(dst);
                if(len == 6)
                {
                    buf[0] = 0x00;
                    buf[1] = 0xC8;
                    buf[2] = 0x00;
                    buf[3] = 0xC8;
                    buf[4] = 0x00;
                    buf[5] = 0xC8;
                    return true;
                }
                if(reg_addr
                   == static_cast<stv::i2c_interface::byte_type>(
                       stv::rm3100_status_reg::addr))
                {
                    buf[0] = 0x80U;
                    return true;
                }
                buf[0] = latest_written_reg_value;
                return true;
            });
        const auto status = mag.read_status_reg();
        REQUIRE(status.drdy);
        REQUIRE(static_cast<stv::rm3100_reg_type>(status) == 0x80U);
    }

    SECTION("read_revid_reg returns the REVID register value")
    {
        stv::rm3100_regs_setup regs;
        REQUIRE(mag.init(regs));

        When(Method(i2c, read))
            .AlwaysDo([&](auto slave_addr, auto reg_addr, void *dst, auto len) {
                (void)slave_addr;
                auto *buf = static_cast<stv::rm3100_reg_type *>(dst);
                if(len == 6)
                {
                    buf[0] = 0x00;
                    buf[1] = 0xC8;
                    buf[2] = 0x00;
                    buf[3] = 0xC8;
                    buf[4] = 0x00;
                    buf[5] = 0xC8;
                    return true;
                }
                if(reg_addr
                   == static_cast<stv::i2c_interface::byte_type>(
                       stv::rm3100_revid_reg::addr))
                {
                    buf[0] = 0x05U;
                    return true;
                }
                buf[0] = latest_written_reg_value;
                return true;
            });
        const auto revid = mag.read_revid_reg();
        REQUIRE(revid.revid == 0x05U);
    }

    SECTION("start_continuous after polling init enables CMM")
    {
        stv::rm3100_regs_setup regs;
        regs.cmm.start = false;
        REQUIRE(mag.init(regs));
        REQUIRE_FALSE(mag.is_continuous_active());

        REQUIRE(mag.start_continuous());
        REQUIRE(mag.is_continuous_active());
        REQUIRE(latest_written_reg_value == 0b00000001);
    }

    SECTION("stop_continuous after CMM init disables CMM")
    {
        stv::rm3100_regs_setup regs;
        regs.cmm.start = true;
        REQUIRE(mag.init(regs));
        REQUIRE(mag.is_continuous_active());

        REQUIRE(mag.stop_continuous());
        REQUIRE_FALSE(mag.is_continuous_active());
        REQUIRE(latest_written_reg_value == 0x00U);
    }

    SECTION("reinit writes the same configuration registers again")
    {
        stv::rm3100_regs_setup regs;
        regs.cmm.start = true;
        regs.cmm.cmx   = true;
        regs.cmm.cmy   = true;
        regs.cmm.cmz   = true;

        std::size_t write_count{0};
        When(Method(i2c, write))
            .AlwaysDo([&](auto slave_addr, auto reg_addr, auto value) {
                (void)slave_addr;
                (void)reg_addr;
                latest_written_reg_value =
                    static_cast<stv::rm3100_reg_type>(value);
                ++write_count;
                return true;
            });

        REQUIRE(mag.init(regs));
        const auto writes_after_init = write_count;

        REQUIRE(mag.reinit());
        REQUIRE(write_count == (writes_after_init * 2));
        REQUIRE(latest_written_reg_value
                == static_cast<stv::rm3100_reg_type>(regs.cmm));
    }
}

// NOLINTEND(*-magic-numbers, google-build-using-namespace,
// readability-function-cognitive-,
// cppcoreguidelines-avoid-non-const-global-variables)
