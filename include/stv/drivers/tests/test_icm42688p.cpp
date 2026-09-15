/// @file test_icm42688p.cpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.

#include <algorithm>
#include <array>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cstddef>
#include <cstdint>
#include <fakeit.hpp>
#include <span>
#include <utility>
#include <vector>

#include "stv/drivers/icm42688p.hpp"
#include "stv/drivers/icm42688p_fifo.hpp"
#include "stv/drivers/icm42688p_types.hpp"

// NOLINTBEGIN(*-magic-numbers, google-build-using-namespace,
// readability-function-cognitive-,
// cppcoreguidelines-avoid-non-const-global-variables)

TEST_CASE(
    "icm42688p fifo packet", "[icm42688p][drivers]")
{
    using namespace stv;

    SECTION("parse 20-bit packet")
    {
        // Header: ACCEL + GYRO + HEADER_20, без MSG.
        constexpr std::array<std::uint8_t, icm42688p_fifo_packet_size> data{
            0x70,       ///< Header.
            0x12, 0x34, ///< Accel X [19:12], [11:4].
            0x67, 0x89, ///< Accel Y.
            0xFF, 0xFF, ///< Accel Z (20-битное -1).
            0x02, 0x46, ///< Gyro X.
            0x0A, 0xBC, ///< Gyro Y.
            0x00, 0x00, ///< Gyro Z.
            0x12, 0x34, ///< Temperature [15:8], [7:0].
            0xAB, 0xCD, ///< Timestamp [15:8], [7:0].
            0x5A,       ///< Accel X [3:0] (старший ниббл), Accel Y [3:0].
            0xF8,       ///< Accel Z [3:0], Gyro X [3:0].
            0xD0,       ///< Gyro Y [3:0], Gyro Z [3:0].
        };

        const auto packet = stv::icm42688p_parse_fifo_packet(
            std::span<const std::uint8_t, icm42688p_fifo_packet_size>{data});

        REQUIRE(packet);
        REQUIRE(packet.accel_x == 0x12345);
        REQUIRE(packet.accel_y == 0x6789A);
        REQUIRE(packet.accel_z == -1);
        REQUIRE(packet.gyro_x == 0x02468);
        REQUIRE(packet.gyro_y == 0x0ABCD);
        REQUIRE(packet.gyro_z == 0);
        REQUIRE(packet.temp == 0x1234);
        REQUIRE(packet.timestamp == 0xABCD);
    }

    SECTION("negative 20-bit values sign extension")
    {
        constexpr std::array<std::uint8_t, icm42688p_fifo_packet_size> data{
            0x70, 0x80, 0x00, 0x80, 0x00, 0x80, 0x00, ///< Оси = 0x80000.
            0x80, 0x00, 0x80, 0x00, 0x80, 0x00,       ///< Gyro = 0x80000.
            0x00, 0x00,                               ///< Temp.
            0x00, 0x00,                               ///< Timestamp.
            0x00, 0x00, 0x00,
        };

        const auto packet = stv::icm42688p_parse_fifo_packet(
            std::span<const std::uint8_t, icm42688p_fifo_packet_size>{data});

        REQUIRE(packet);
        // 0x80000 с расширением знака с бита 19 равен -524288.
        REQUIRE(packet.accel_x == -524288);
        REQUIRE(packet.accel_y == -524288);
        REQUIRE(packet.accel_z == -524288);
        REQUIRE(packet.gyro_x == -524288);
        REQUIRE(packet.gyro_y == -524288);
        REQUIRE(packet.gyro_z == -524288);
    }

    SECTION("header without 20-bit flag is invalid")
    {
        constexpr std::array<std::uint8_t, icm42688p_fifo_packet_size> data{
            0x60, ///< HEADER_20 не установлен.
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        };

        const auto packet = stv::icm42688p_parse_fifo_packet(
            std::span<const std::uint8_t, icm42688p_fifo_packet_size>{data});

        REQUIRE_FALSE(packet);
        REQUIRE_FALSE(packet.valid);
    }

    SECTION("empty FIFO header (MSG bit) is invalid")
    {
        constexpr std::array<std::uint8_t, icm42688p_fifo_packet_size> data{
            static_cast<std::uint8_t>(0x80 | 0x70), ///< HEADER_MSG установлен.
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
        };

        const auto packet = stv::icm42688p_parse_fifo_packet(
            std::span<const std::uint8_t, icm42688p_fifo_packet_size>{data});

        REQUIRE_FALSE(packet);
    }
}

TEST_CASE(
    "icm42688p driver", "[icm42688p][drivers]")
{
    using namespace fakeit;
    using namespace stv;

    using acc_type    = stv::acc<float, std::uint32_t>;
    using gyr_type    = stv::gyr<float, std::uint32_t>;
    using driver_type = stv::icm42688p<acc_type, gyr_type>;
    using setup_type  = stv::icm42688p_setup<>;

    SECTION("is_detected checks who am i")
    {
        Mock<stv::i2c_interface> detect_i2c;
        Fake(Method(detect_i2c, write));

        SECTION("detected on valid id")
        {
            When(Method(detect_i2c, read)).Do([](auto, auto, void *dst, auto) {
                *static_cast<stv::icm42688p_reg_type *>(dst) =
                    stv::icm42688p_who_am_i_reg::expected_value;
                return true;
            });
            REQUIRE(driver_type::is_detected(&detect_i2c.get()));
        }

        SECTION("not detected on wrong id")
        {
            When(Method(detect_i2c, read)).Do([](auto, auto, void *dst, auto) {
                *static_cast<stv::icm42688p_reg_type *>(dst) = 0x00;
                return true;
            });
            REQUIRE_FALSE(driver_type::is_detected(&detect_i2c.get()));
        }

        SECTION("not detected when read fails")
        {
            Fake(Method(detect_i2c, read));
            REQUIRE_FALSE(driver_type::is_detected(&detect_i2c.get()));
        }
    }

    Mock<stv::i2c_interface> i2c;
    Fake(Method(i2c, write));
    Fake(Method(i2c, read));

    /// @brief Модель памяти регистров датчика (bank 0 вид сверху).
    std::array<stv::icm42688p_reg_type, 256> reg_model{};

    /// @brief Журнал записей (адрес регистра, значение).
    std::vector<std::pair<std::uint16_t, std::uint16_t>> write_log;

    /// @brief Счётчик вызовов функтора задержки.
    std::uint32_t delay_call_count{0};
    auto delay_fn = [&delay_call_count](std::uint32_t) { ++delay_call_count; };

    When(Method(i2c, write))
        .AlwaysDo([&](std::byte, std::byte reg_addr, std::byte value) {
            const auto reg = std::to_integer<std::uint16_t>(reg_addr);
            write_log.emplace_back(reg, std::to_integer<std::uint16_t>(value));
            reg_model.at(static_cast<std::size_t>(reg)) =
                static_cast<stv::icm42688p_reg_type>(value);
            return true;
        });

    /// @brief Данные блока сенсорных регистров 0x1D–0x2A.
    std::array<std::uint8_t, 14> burst_data{};

    When(Method(i2c, read))
        .AlwaysDo(
            [&](std::byte, std::byte reg_addr, void *dst, std::size_t len) {
                const auto reg = std::to_integer<std::size_t>(reg_addr);
                auto      *out = static_cast<std::uint8_t *>(dst);

                if((reg == 0x1DU) && (len == burst_data.size()))
                {
                    std::copy(burst_data.begin(), burst_data.end(), out);
                    return true;
                }

                for(std::size_t i{0}; i < len; ++i)
                {
                    out[i] = reg_model.at(reg + i);
                }

                return true;
            });

    auto values_written_to = [&write_log](std::uint16_t reg) {
        std::vector<std::uint16_t> values;
        for(const auto &[written_reg, value]: write_log)
        {
            if(written_reg == reg)
            {
                values.push_back(value);
            }
        }
        return values;
    };

    auto last_write_position = [&write_log](std::uint16_t reg) {
        auto position = write_log.size();
        for(std::size_t i{0}; i < write_log.size(); ++i)
        {
            if(write_log.at(i).first == reg)
            {
                position = i;
            }
        }
        return position;
    };

    setup_type  setup{{.i2c = &i2c.get()}};
    driver_type driver{setup};
    // До первого успешного init() датчик не считается инициализированным.
    REQUIRE_FALSE(driver);

    SECTION("init writes configuration sequence")
    {
        REQUIRE(driver.init(setup, delay_fn));
        REQUIRE(driver);
        REQUIRE(delay_call_count == 2U);

        // Программный сброс.
        REQUIRE(values_written_to(0x11U) == std::vector<std::uint16_t>{0x01U});
        // Переход в bank 1 и обратно, разрешение I2C (I3C_EN = 1).
        REQUIRE(values_written_to(0x76U)
                == std::vector<std::uint16_t>{0x01U, 0x00U});
        REQUIRE(values_written_to(0x7CU) == std::vector<std::uint16_t>{0x1CU});
        // Скорости фронтов по заметке 12.3 (I2C/SPI slew rate = 1).
        REQUIRE(values_written_to(0x13U) == std::vector<std::uint16_t>{0x09U});
        // INT1: push-pull, active-high, pulse.
        REQUIRE(values_written_to(0x14U) == std::vector<std::uint16_t>{0x03U});
        // INT_ASYNC_RESET = 0.
        REQUIRE(values_written_to(0x64U) == std::vector<std::uint16_t>{0x00U});
        // UI_DRDY_INT_CLEAR = 0b10 (очистка при чтении сенсорных регистров).
        REQUIRE(values_written_to(0x63U) == std::vector<std::uint16_t>{0x20U});
        // UI_DRDY маршрутизирован на INT1 (reset-значение регистра 0x10).
        REQUIRE(values_written_to(0x65U) == std::vector<std::uint16_t>{0x10U});
        // GYRO_CONFIG0/ACCEL_CONFIG0: ±2000 dps/±16 g, ODR 50 Гц.
        REQUIRE(values_written_to(0x4FU) == std::vector<std::uint16_t>{0x09U});
        REQUIRE(values_written_to(0x50U) == std::vector<std::uint16_t>{0x09U});
        // В режиме registers_16bit FIFO не настраивается.
        REQUIRE(values_written_to(0x16U).empty());
        REQUIRE(values_written_to(0x5FU).empty());
        REQUIRE(values_written_to(0x4CU).empty());
        // PWR_MGMT0: оба сенсора в Low Noise, регистр записан последним.
        REQUIRE(values_written_to(0x4EU) == std::vector<std::uint16_t>{0x0FU});
        REQUIRE(last_write_position(0x4EU) == (write_log.size() - 1U));

        // Повторная инициализация повторяет последовательность.
        write_log.clear();
        REQUIRE(driver.reinit(delay_fn));
        REQUIRE(values_written_to(0x11U) == std::vector<std::uint16_t>{0x01U});
        REQUIRE(last_write_position(0x4EU) == (write_log.size() - 1U));
    }

    SECTION("init in fifo 20bit mode configures fifo")
    {
        setup.mode = stv::icm42688p_data_mode::fifo_20bit;
        REQUIRE(driver.init(setup, delay_fn));

        // FIFO_CONFIG: Stream-to-FIFO (биты 7:6 = 01).
        REQUIRE(values_written_to(0x16U) == std::vector<std::uint16_t>{0x40U});
        // FIFO_CONFIG1: HIRES + TMST_FSYNC + TEMP + GYRO + ACCEL.
        REQUIRE(values_written_to(0x5FU) == std::vector<std::uint16_t>{0x1FU});
        // INTF_CONFIG0: FIFO_COUNT_REC = 1 и big-endian по умолчанию.
        REQUIRE(values_written_to(0x4CU) == std::vector<std::uint16_t>{0x70U});
        // PWR_MGMT0 всё ещё последний.
        REQUIRE(last_write_position(0x4EU) == (write_log.size() - 1U));
        REQUIRE(last_write_position(0x4CU) < last_write_position(0x4EU));
    }

    SECTION("init fails when register readback fails")
    {
        When(Method(i2c, read)).AlwaysDo([](auto, auto, void *, auto) {
            return false;
        });
        REQUIRE_FALSE(driver.init(setup, delay_fn));
        REQUIRE_FALSE(driver);
    }

    SECTION("init succeeds when reserved bits read back as reset value")
    {
        // Модель кристалла: резервные биты 6:5 INTF_CONFIG6 (bank 1, 0x7C)
        // читаются как 0b10 после сброса независимо от записи (reset value
        // 0x5F → readback 0x5C вместо записанных 0x1C).
        When(Method(i2c, write))
            .AlwaysDo([&](std::byte, std::byte reg_addr, std::byte value) {
                const auto reg    = std::to_integer<std::uint16_t>(reg_addr);
                auto       stored = std::to_integer<std::uint16_t>(value);
                if(reg == 0x7CU)
                {
                    stored |= 0x40U;
                }
                write_log.emplace_back(reg, stored);
                reg_model.at(static_cast<std::size_t>(reg)) =
                    static_cast<stv::icm42688p_reg_type>(stored);
                return true;
            });

        REQUIRE(driver.init(setup, delay_fn));
        REQUIRE(driver);
        REQUIRE(values_written_to(0x7CU) == std::vector<std::uint16_t>{0x5CU});
    }

    SECTION("read raw and normalize")
    {
        REQUIRE(driver.init(setup, delay_fn));

        burst_data = {
            0x00, 0x00, ///< temp = 0.
            0x08, 0x00, ///< accel x = 2048.
            0xF0, 0x00, ///< accel y = -4096.
            0x20, 0x00, ///< accel z = 8192.
            0x00, 0xA4, ///< gyro x = 164.
            0xFE, 0xB8, ///< gyro y = -328.
            0x00, 0x32  ///< gyro z = 50.
        };

        const auto raw = driver.read_raw();
        REQUIRE(raw);
        REQUIRE(raw.temp == 0);
        REQUIRE(raw.accel_x == 2048);
        REQUIRE(raw.accel_y == -4096);
        REQUIRE(raw.accel_z == 8192);
        REQUIRE(raw.gyro_x == 164);
        REQUIRE(raw.gyro_y == -328);
        REQUIRE(raw.gyro_z == 50);

        const auto meas = driver.read_normalized();
        REQUIRE(meas);
        // Акселерометр: 2048/2048 = 1 g, -4096/2048 = -2 g, 8192/2048 = 4 g.
        REQUIRE(meas.acc.give_x() == Catch::Approx(1.0F));
        REQUIRE(meas.acc.give_y() == Catch::Approx(-2.0F));
        REQUIRE(meas.acc.give_z() == Catch::Approx(4.0F));
        // Гироскоп: LSB / 16.4 LSB-per-dps, перевод в рад/с.
        constexpr float deg_to_rad{0.017453292F};
        REQUIRE(meas.gyr.give_x()
                == Catch::Approx(164.0F / 16.4F * deg_to_rad).margin(1e-5F));
        REQUIRE(meas.gyr.give_y()
                == Catch::Approx(-328.0F / 16.4F * deg_to_rad).margin(1e-4F));
        REQUIRE(meas.gyr.give_z()
                == Catch::Approx(50.0F / 16.4F * deg_to_rad).margin(1e-4F));
        // Период интегрирования = 1/50 Гц.
        REQUIRE(meas.gyr.dT_sec == Catch::Approx(0.02F).margin(1e-6F));
        REQUIRE(meas.acc.packstamp == 2U);
        REQUIRE(meas.gyr.packstamp == 2U);

        // Температура: 0 LSB = 25 °C.
        REQUIRE(driver.get_temperature() == Catch::Approx(25.0F));

        burst_data.at(0) = 0x33;
        burst_data.at(1) = 0xC0; ///< temp = 13248 LSB.
        (void)driver.read_raw();
        REQUIRE(driver.get_temperature()
                == Catch::Approx(125.0F).margin(0.01F));
    }

    SECTION("invalid burst resets raw and timestamp")
    {
        REQUIRE(driver.init(setup, delay_fn));

        burst_data.fill(0x00);
        const auto invalid = driver.read_raw();
        REQUIRE_FALSE(invalid);
        REQUIRE(invalid.accel_x == 0);
        REQUIRE(invalid.gyro_z == 0);

        burst_data       = {0x00, 0x00, 0x08, 0x00, 0x00, 0x01, 0x00,
                            0x02, 0x00, 0x03, 0x00, 0x04, 0x00, 0x05};
        const auto valid = driver.read_raw();
        REQUIRE(valid);
        REQUIRE(valid.accel_x == 2048);

        const auto meas = driver.read_normalized();
        REQUIRE(meas.acc.packstamp == 2U);
    }

    SECTION("read failure returns zeroed raw")
    {
        REQUIRE(driver.init(setup, delay_fn));

        When(Method(i2c, read)).AlwaysDo([](auto, auto, void *, auto) {
            return false;
        });

        const auto raw = driver.read_raw();
        REQUIRE_FALSE(raw);
        REQUIRE(raw.temp == 0);
        // Валидных данных ещё не было — буферы недоступны.
        REQUIRE_FALSE(static_cast<bool>(driver.get_acc()));
        REQUIRE(driver.get_temperature() == Catch::Approx(25.0F));
    }

    SECTION("set_odr updates registers and gyro period")
    {
        REQUIRE(driver.init(setup, delay_fn));
        write_log.clear();

        REQUIRE(driver.set_odr(stv::icm42688p_odr_t::odr_100,
                               stv::icm42688p_odr_t::odr_200));
        REQUIRE(values_written_to(0x4FU) == std::vector<std::uint16_t>{0x07U});
        REQUIRE(values_written_to(0x50U) == std::vector<std::uint16_t>{0x08U});

        burst_data      = {0x00, 0x00, 0x08, 0x00, 0x10, 0x00, 0x18,
                           0x00, 0x00, 0xA4, 0x00, 0x00, 0x00, 0x00};
        const auto meas = driver.read_normalized();
        REQUIRE(meas.gyr.dT_sec == Catch::Approx(0.005F).margin(1e-6F));
    }

    SECTION("set_full_scale updates sensitivity")
    {
        REQUIRE(driver.init(setup, delay_fn));
        write_log.clear();

        REQUIRE(driver.set_full_scale(stv::icm42688p_accel_fsr_t::g_8,
                                      stv::icm42688p_gyro_fsr_t::dps_500));
        REQUIRE(values_written_to(0x50U) == std::vector<std::uint16_t>{0x29U});
        REQUIRE(values_written_to(0x4FU) == std::vector<std::uint16_t>{0x49U});

        burst_data      = {0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00,
                           0x00, 0x02, 0x8F, 0x00, 0x00, 0x00, 0x00};
        const auto meas = driver.read_normalized();
        // 4096 LSB при ±8 g (4096 LSB/g) = 1 g.
        REQUIRE(meas.acc.give_x() == Catch::Approx(1.0F));
        // 655 LSB при ±500 dps (65.5 LSB/dps) = 10 dps в рад/с.
        constexpr float deg_to_rad{0.017453292F};
        REQUIRE(meas.gyr.give_x()
                == Catch::Approx(10.0F * deg_to_rad).margin(1e-5F));
    }

    SECTION("run_self_test computes axis responses and reads trims")
    {
        REQUIRE(driver.init(setup, delay_fn));

        reg_model.at(0x2DU) = 0x08; ///< DATA_RDY_INT всегда готов.
        reg_model.at(0x5FU) = 0x11;
        reg_model.at(0x60U) = 0x22;
        reg_model.at(0x61U) = 0x33;
        reg_model.at(0x3BU) = 0x44;
        reg_model.at(0x3CU) = 0x55;
        reg_model.at(0x3DU) = 0x66;

        When(Method(i2c, read))
            .AlwaysDo(
                [&](std::byte, std::byte reg_addr, void *dst, std::size_t len) {
                    const auto reg = std::to_integer<std::size_t>(reg_addr);
                    auto      *out = static_cast<std::uint8_t *>(dst);

                    if((reg == 0x1DU) && (len == burst_data.size()))
                    {
                        const auto base = static_cast<std::uint16_t>(
                            (reg_model.at(0x70U) != 0) ? 500 : 100);
                        out[0] = 0x10;
                        out[1] = 0x00;
                        for(std::size_t i{0}; i < 6; ++i)
                        {
                            const auto value = static_cast<std::uint16_t>(
                                base + static_cast<std::uint16_t>(i));
                            out[2 + i * 2] = static_cast<std::uint8_t>(
                                static_cast<std::uint32_t>(value) >> 8U);
                            out[3 + i * 2] = static_cast<std::uint8_t>(value);
                        }
                        return true;
                    }

                    for(std::size_t i{0}; i < len; ++i)
                    {
                        out[i] = reg_model.at(reg + i);
                    }
                    return true;
                });

        const auto result = driver.run_self_test(delay_fn);
        REQUIRE(result.has_value());

        constexpr float expected_response{400.0F};
        REQUIRE(result->accel_x == Catch::Approx(expected_response));
        REQUIRE(result->accel_y == Catch::Approx(expected_response));
        REQUIRE(result->accel_z == Catch::Approx(expected_response));
        REQUIRE(result->gyro_x == Catch::Approx(expected_response));
        REQUIRE(result->gyro_y == Catch::Approx(expected_response));
        REQUIRE(result->gyro_z == Catch::Approx(expected_response));

        REQUIRE(result->gyro_trim_x == 0x11);
        REQUIRE(result->gyro_trim_y == 0x22);
        REQUIRE(result->gyro_trim_z == 0x33);
        REQUIRE(result->accel_trim_x == 0x44);
        REQUIRE(result->accel_trim_y == 0x55);
        REQUIRE(result->accel_trim_z == 0x66);

        // Self-test включён на всех осях и выключен после завершения.
        REQUIRE(values_written_to(0x70U)
                == std::vector<std::uint16_t>{0x7FU, 0x00U});
        // Банк 0 восстановлен после чтения заводских тримов.
        REQUIRE(reg_model.at(0x76U) == 0x00);
    }

    SECTION("run_self_test fails when data ready flag never sets")
    {
        REQUIRE(driver.init(setup, delay_fn));

        // DATA_RDY_INT (бит 3 регистра 0x2D) в модели никогда не
        // устанавливается — ожидание готовности данных выходит за лимит
        // попыток.
        const auto result = driver.run_self_test(delay_fn);
        REQUIRE_FALSE(result.has_value());

        // Self-test не включался и не выключался.
        REQUIRE(values_written_to(0x70U).empty());
    }

    SECTION("read_fifo_packet reads 20 byte packet")
    {
        setup.mode = stv::icm42688p_data_mode::fifo_20bit;
        REQUIRE(driver.init(setup, delay_fn));

        const std::array<std::uint8_t, stv::icm42688p_fifo_packet_size>
            packet_bytes{0x70, 0x12, 0x34, 0x67, 0x89, 0xFF, 0xFF,
                         0x02, 0x46, 0x0A, 0xBC, 0x00, 0x00, 0x12,
                         0x34, 0xAB, 0xCD, 0x5A, 0xF8, 0xD0};

        reg_model.at(0x2EU) = 0x00;
        reg_model.at(0x2FU) = 0x01; ///< Одна запись в FIFO.
        std::copy(packet_bytes.begin(), packet_bytes.end(),
                  reg_model.begin() + 0x30);

        const auto packet = driver.read_fifo_packet();
        REQUIRE(packet);
        REQUIRE(packet.accel_x == 0x12345);
        REQUIRE(packet.accel_y == 0x6789A);
        REQUIRE(packet.accel_z == -1);
        REQUIRE(packet.gyro_x == 0x02468);
        REQUIRE(packet.gyro_y == 0x0ABCD);
        REQUIRE(packet.gyro_z == 0);
        REQUIRE(packet.temp == 0x1234);
        REQUIRE(packet.timestamp == 0xABCD);

        reg_model.at(0x2FU) = 0x00; ///< FIFO пуст.
        REQUIRE_FALSE(driver.read_fifo_packet());
    }
}

// NOLINTEND(*-magic-numbers, google-build-using-namespace,
// readability-function-cognitive-complexity,
// cppcoreguidelines-avoid-non-const-global-variables)
