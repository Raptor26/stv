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
#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <fakeit.hpp>

// NOLINTBEGIN(*-magic-numbers, google-build-using-namespace,
// readability-function-cognitive-,
// cppcoreguidelines-avoid-non-const-global-variables)

TEST_CASE(
    "qmc6100", "[stv][drivers]")
{
    using namespace fakeit;
    using namespace stv;

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
                    *chip_id     = 0xFA;
                    return true;
                });
            REQUIRE(qma6100::is_detected(
                &i2c.get(), qma6100_setup::i2c_addr_connect_to_gnd));
        }

        SECTION("Nothing read")
        {
            Fake(Method(i2c, read));
            REQUIRE_FALSE(qma6100::is_detected(
                &i2c.get(), qma6100_setup::i2c_addr_connect_to_gnd));
        }
    }

    Mock<stv::i2c_interface> i2c;
    Fake(Method(i2c, write));
    Fake(Method(i2c, read));
    qma6100_setup setup{.i2c = &i2c.get()};
    qma6100       driver{setup};
    REQUIRE(driver);

    SECTION("Init")
    {
        qma6100_regs_setup regs_setup;
        regs_setup.bw_reg.bw   = qma6100_bw_reg::bw_t::mclk_1024_488_hz;
        regs_setup.bw_reg.nlpf = qma6100_bw_reg::nlpf_t::average_4;

        // Заглушка первого вызова read
        When(Method(i2c, read))
            .Do([&](auto slave_addr, auto reg_addr, void *dst, auto len) {
                (void)slave_addr;
                (void)reg_addr;
                (void)len;

                auto dst_byte = reinterpret_cast<qma6100_reg_type *>(dst);
                *dst_byte = static_cast<qma6100_reg_type>(regs_setup.bw_reg);
                return true;
            });
        REQUIRE(driver.init(regs_setup));
    }
}

// NOLINTEND(*-magic-numbers, google-build-using-namespace,
// readability-function-cognitive-complexity,
// cppcoreguidelines-avoid-non-const-global-variables)

