/// @file test_qma6100.cpp
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

#include "stv/drivers/qma6100.hpp"
#include "stv/drivers/qma6100_regs.hpp"
#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <fakeit.hpp>
#include <type_traits>

// NOLINTBEGIN(*-magic-numbers, google-build-using-namespace,
// readability-function-cognitive-,
// cppcoreguidelines-avoid-non-const-global-variables)

TEST_CASE(
    "qmc6100", "[stv][drivers]")
{
    using namespace fakeit;
    using namespace stv;

    using qma6100_type = stv::qma6100<float, std::uint32_t>;

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

                    auto chip_id = reinterpret_cast<std::uint8_t *>(dst);
                    *chip_id     = 0x90;
                    return true;
                });
            REQUIRE(qma6100_type::is_detected(
                &i2c.get(), qma6100_setup::i2c_addr_connect_to_gnd));
        }

        SECTION("Nothing read")
        {
            Fake(Method(i2c, read));
            REQUIRE_FALSE(qma6100_type::is_detected(
                &i2c.get(), qma6100_setup::i2c_addr_connect_to_gnd));
        }
    }

    Mock<stv::i2c_interface> i2c;
    Fake(Method(i2c, write));
    Fake(Method(i2c, read));
    qma6100_setup setup{{.i2c = &i2c.get()}};
    qma6100       driver{setup};
    REQUIRE(driver);

    SECTION("Init")
    {
        qma6100_regs_setup regs_setup;
        regs_setup.bw_reg.bw   = qma6100_bw_reg::bw_t::mclk_1024_488_hz;
        regs_setup.bw_reg.nlpf = qma6100_bw_reg::nlpf_t::average_4;

        regs_setup.fsr_reg.range = qma6100_fsr_reg::range_t::g_4;

        regs_setup.int_en1_reg.int_data_en =
            qma6100_int_en1_reg::switcher_t::enable;
        regs_setup.int_en1_reg.int_ffull_en =
            qma6100_int_en1_reg::switcher_t::enable;
        regs_setup.int_en1_reg.int_fwm_en =
            qma6100_int_en1_reg::switcher_t::enable;

        regs_setup.int_map1_reg.int1_any_mot =
            qma6100_int_map1_reg::mapper_t::enable;
        regs_setup.int_map1_reg.int1_no_mot =
            qma6100_int_map1_reg::mapper_t::enable;

        regs_setup.int_map3_reg.int2_any_mot =
            qma6100_int_map3_reg::mapper_t::enable;

        regs_setup.intpin_conf_reg.dis_pu_senb =
            qma6100_intpin_conf_reg::dis_pu_senb_t::disable;
        regs_setup.intpin_conf_reg.dis_ie_ad0 =
            qma6100_intpin_conf_reg::dis_ie_ad0_t::disable;
        regs_setup.intpin_conf_reg.en_spi3w =
            qma6100_intpin_conf_reg::en_spi3w_t::enable;
        regs_setup.intpin_conf_reg.int2_od =
            qma6100_intpin_conf_reg::int2_od_t::open_drain;
        regs_setup.intpin_conf_reg.int2_lvl =
            qma6100_intpin_conf_reg::int2_lvl_t::logic_low_as_active;
        regs_setup.intpin_conf_reg.int1_od =
            qma6100_intpin_conf_reg::int1_od_t::open_drain;
        regs_setup.intpin_conf_reg.int1_lvl =
            qma6100_intpin_conf_reg::int1_lvl_t::logic_low_as_active;

        regs_setup.int_cfg_reg.int_rd_clr =
            stv::qma6100_int_cfg_reg::int_rd_clr_t::clear_all;
        regs_setup.int_cfg_reg.shadow_dis =
            stv::qma6100_int_cfg_reg::shadow_dis_t::disable;
        regs_setup.int_cfg_reg.dis_i2c =
            stv::qma6100_int_cfg_reg::dis_i2c_t::disable;
        regs_setup.int_cfg_reg.latch_int_step =
            stv::qma6100_int_cfg_reg::latch_int_step_t::latch_mode;
        regs_setup.int_cfg_reg.latch_int =
            stv::qma6100_int_cfg_reg::latch_int_t::latch_mode;

        regs_setup.pm_reg.mode_bit = stv::qma6100_pm_reg::mode_bit_t::active;
        regs_setup.pm_reg.mclk_sel = stv::qma6100_pm_reg::mclk_sel_t::freq_200k;
        regs_setup.pm_reg.t_rstb_sinc_sel =
            stv::qma6100_pm_reg::t_rstb_sinc_sel_t::k_6_mult_mckl;

        /// @brief Крайнее записанное при вызове i2c::write() значение регистра.
        qma6100_reg_type latest_written_reg_value{0xAA};

        When(Method(i2c, write))
            .AlwaysDo(
                [&](auto slave_addr, auto reg_addr, auto write_reg_value) {
                    (void)slave_addr;
                    (void)reg_addr;
                    latest_written_reg_value = write_reg_value;
                    return true;
                });

        When(Method(i2c, read))
            .AlwaysDo([&](auto slave_addr, auto reg_addr, void *dst, auto len) {
                (void)slave_addr;
                (void)reg_addr;
                (void)len;
                auto dst_byte = reinterpret_cast<qma6100_reg_type *>(dst);

                // Считывается то значение, которое было записано при крайнем
                // вызове 2c::write()ю
                *dst_byte = latest_written_reg_value;

                // Сброс значения записанного регистра чтобы избежать ошибки,
                // при которой результат теста может быть ложно положительным
                // если значения последовательно расположенных регистров
                // совпадают.
                latest_written_reg_value = qma6100_reg_type{0xAA};

                return true;
            });
        REQUIRE(driver.init(regs_setup));
    }
}

// NOLINTEND(*-magic-numbers, google-build-using-namespace,
// readability-function-cognitive-complexity,
// cppcoreguidelines-avoid-non-const-global-variables)
