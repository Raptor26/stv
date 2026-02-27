/// @file test_mc3479.cpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.

#include "stv/drivers/mc3479.hpp"
#include "stv/gyraccmag_types.hpp"
#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <fakeit.hpp>

// NOLINTBEGIN(*-magic-numbers, google-build-using-namespace,
// readability-function-cognitive-,
// cppcoreguidelines-avoid-non-const-global-variables)

TEST_CASE(
    "mc3479", "[stv][drivers]")
{
    using namespace fakeit;
    using acc_type          = stv::acc<float, std::uint32_t>;
    using mc3478_setup_type = stv::mc3479_setup;
    using mc3478_type       = stv::mc3479<acc_type>;

    SECTION("Default Setup")
    {
        const mc3478_setup_type setup;
        REQUIRE_FALSE(mc3478_type{setup});
    }

    SECTION("Who am i")
    {
        Mock<stv::i2c_interface> i2c;
        Fake(Method(i2c, write));

        SECTION("Return valid chip id")
        {
            When(Method(i2c, read))
                .Do([](auto slave_addr, auto reg_addr, void *dst, auto len) {
                    (void)slave_addr;
                    (void)reg_addr;
                    (void)len;

                    auto *chip_id = reinterpret_cast<std::uint8_t *>(dst);
                    *chip_id      = 0xA4; ///< see datasheet
                    return true;
                });
            REQUIRE(mc3478_type::is_detected(
                &i2c.get(), mc3478_setup_type::i2c_addr_connect_to_gnd));
        }

        SECTION("Return invalid chip id")
        {
            When(Method(i2c, read))
                .Do([](auto slave_addr, auto reg_addr, void *dst, auto len) {
                    (void)slave_addr;
                    (void)reg_addr;
                    (void)len;

                    auto *chip_id = reinterpret_cast<std::uint8_t *>(dst);
                    *chip_id      = 0xAB; ///< invalid chip id
                    return true;
                });
            REQUIRE_FALSE(mc3478_type::is_detected(
                &i2c.get(), mc3478_setup_type::i2c_addr_connect_to_gnd));
        }

        SECTION("Read Error with valid chip_id")
        {
            When(Method(i2c, read))
                .Do([](auto slave_addr, auto reg_addr, void *dst, auto len) {
                    (void)slave_addr;
                    (void)reg_addr;
                    (void)len;

                    auto *chip_id = reinterpret_cast<std::uint8_t *>(dst);
                    *chip_id      = 0xA4; ///< see datasheet
                    return false;
                });
            REQUIRE_FALSE(mc3478_type::is_detected(
                &i2c.get(), mc3478_setup_type::i2c_addr_connect_to_gnd));
        }
    }

    SECTION("Valid Setup")
    {
        Mock<stv::i2c_interface> i2c;
        Fake(Method(i2c, write));
        Fake(Method(i2c, read));
        const mc3478_setup_type setup{{.i2c = &i2c.get()}};
        mc3478_type             driver{setup};
        REQUIRE(driver);

        SECTION("Init")
        {
            stv::mc3479_regs_setup regs_setup;
            regs_setup.mode.state   = stv::mc3479_state::wake;
            regs_setup.mode.wtd_neg = true;

            regs_setup.interrupt_enable.anym_int_enable = true;

            regs_setup.range_and_scale_control.lpf_enable = true;
            regs_setup.range_and_scale_control.lpf = stv::mc3479_lpf::IDRdev6;

            /// @brief Крайнее записанное при вызове i2c::write() значение
            /// регистра.
            stv::mc3479_reg_type latest_written_reg_value{0xAA};

            When(Method(i2c, write))
                .AlwaysDo(
                    [&](auto slave_addr, auto reg_addr, auto write_reg_value) {
                        (void)slave_addr;
                        (void)reg_addr;
                        latest_written_reg_value = write_reg_value;
                        return true;
                    });

            When(Method(i2c, read))
                .AlwaysDo(
                    [&](auto slave_addr, auto reg_addr, void *dst, auto len) {
                        (void)slave_addr;
                        (void)reg_addr;
                        (void)len;
                        auto *dst_byte =
                            reinterpret_cast<stv::mc3479_reg_type *>(dst);

                        // Считывается то значение, которое было записано при
                        // крайнем вызове 2c::write()ю
                        *dst_byte = latest_written_reg_value;

                        // Сброс значения записанного регистра чтобы избежать
                        // ошибки, при которой результат теста может быть ложно
                        // положительным если значения последовательно
                        // расположенных регистров совпадают.
                        latest_written_reg_value = stv::mc3479_reg_type{0xAA};

                        return true;
                    });
            REQUIRE(driver.init(regs_setup));

            // Ожидается, что количество вызовов write() совпадает с
            // количеством поддерживаемых конфигурационных регистров + операции
            // stop() и start().
            Verify(Method(i2c, write)).Exactly(5 + 2);
        }
    }
}

// NOLINTEND(*-magic-numbers, google-build-using-namespace,
// readability-function-cognitive-,
// cppcoreguidelines-avoid-non-const-global-variables)
