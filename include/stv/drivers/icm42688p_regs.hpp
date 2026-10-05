/// @file icm42688p_regs.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.

#ifndef ICM42688P_REGS_HPP
#define ICM42688P_REGS_HPP

#include "icm42688p_types.hpp"
#include "stv/register_field.hpp"
#include <cstdint>

namespace stv {

// NOLINTBEGIN(*hicpp-signed-bitwise, *-member*)

/// @brief Частота дискретизации (Output Data Rate) акселерометра и
/// гироскопа ICM-42688-P.
///
/// @details Общая кодировка 4-битных полей GYRO_ODR (bank 0, регистр 0x4F,
///          биты 3:0) и ACCEL_ODR (bank 0, регистр 0x50, биты 3:0).
///          Значения сверены с разделами 14.37 и 14.38 даташита DS-000347
///          rev 1.6: код 0x0F соответствует 500 Гц (не 0x07!), код 0x07 —
///          200 Гц.
enum struct icm42688p_odr_t : std::uint8_t {
    odr_32k  = 0x01, ///< 32 кГц.
    odr_16k  = 0x02, ///< 16 кГц.
    odr_8k   = 0x03, ///< 8 кГц.
    odr_4k   = 0x04, ///< 4 кГц.
    odr_2k   = 0x05, ///< 2 кГц.
    odr_1k   = 0x06, ///< 1 кГц (значение после сброса).
    odr_200  = 0x07, ///< 200 Гц.
    odr_100  = 0x08, ///< 100 Гц.
    odr_50   = 0x09, ///< 50 Гц.
    odr_25   = 0x0A, ///< 25 Гц.
    odr_12p5 = 0x0B, ///< 12.5 Гц.
    odr_500  = 0x0F, ///< 500 Гц.
};

/// @brief Переводит кодировку ODR в частоту, Гц.
/// @param[in] odr Код ODR.
/// @return Частота дискретизации в Гц.
[[nodiscard]] constexpr auto icm42688p_odr_to_hz(
    icm42688p_odr_t odr) -> float
{
    switch(odr)
    {
        case icm42688p_odr_t::odr_32k:
            return 32000.0F;
        case icm42688p_odr_t::odr_16k:
            return 16000.0F;
        case icm42688p_odr_t::odr_8k:
            return 8000.0F;
        case icm42688p_odr_t::odr_4k:
            return 4000.0F;
        case icm42688p_odr_t::odr_2k:
            return 2000.0F;
        case icm42688p_odr_t::odr_1k:
            return 1000.0F;
        case icm42688p_odr_t::odr_500:
            return 500.0F;
        case icm42688p_odr_t::odr_200:
            return 200.0F;
        case icm42688p_odr_t::odr_100:
            return 100.0F;
        case icm42688p_odr_t::odr_50:
            return 50.0F;
        case icm42688p_odr_t::odr_25:
            return 25.0F;
        case icm42688p_odr_t::odr_12p5:
            return 12.5F;
    }

    return 0.0F;
}

/// @brief Диапазон измерений акселерометра (Full Scale Range) ICM-42688-P.
///
/// @details Кодировка 3-битного поля ACCEL_UI_FS_SEL (bank 0, регистр 0x50,
///          биты 7:5), см. раздел 14.38 даташита DS-000347 rev 1.6.
enum struct icm42688p_accel_fsr_t : std::uint8_t {
    g_16 = 0, ///< ±16 g (значение после сброса).
    g_8  = 1, ///< ±8 g.
    g_4  = 2, ///< ±4 g.
    g_2  = 3, ///< ±2 g.
};

/// @brief Диапазон измерений гироскопа (Full Scale Range) ICM-42688-P.
///
/// @details Кодировка 3-битного поля GYRO_UI_FS_SEL (bank 0, регистр 0x4F,
///          биты 7:5), см. раздел 14.37 даташита DS-000347 rev 1.6.
enum struct icm42688p_gyro_fsr_t : std::uint8_t {
    dps_2000   = 0, ///< ±2000 dps (значение после сброса).
    dps_1000   = 1, ///< ±1000 dps.
    dps_500    = 2, ///< ±500 dps.
    dps_250    = 3, ///< ±250 dps.
    dps_125    = 4, ///< ±125 dps.
    dps_62p5   = 5, ///< ±62.5 dps.
    dps_31p25  = 6, ///< ±31.25 dps.
    dps_15p625 = 7, ///< ±15.625 dps.
};

/// @brief Регистр DEVICE_CONFIG (bank 0, адрес 0x11) для управления
/// конфигурацией устройства ICM-42688-P.
///
/// @details Содержит бит программного сброса SOFT_RESET_CONFIG (бит 0).
///          После записи 1 в этот бит необходимо выждать 1 мс до доступа к
///          остальным регистрам (см. раздел 14.1 даташита DS-000347 rev 1.6).
class icm42688p_device_config_reg
{
    static constexpr int soft_reset_config_offset{0U};

  public:
    /// @brief Адрес регистра DEVICE_CONFIG в памяти устройства (bank 0).
    static constexpr icm42688p_reg_type addr{0x11};

    explicit icm42688p_device_config_reg(
        icm42688p_reg_type value = icm42688p_reg_type{0})
    {
        parse(value);
    }

    /// @brief Оператор преобразования в сырое значение регистра.
    explicit operator icm42688p_reg_type() const
    {
        return static_cast<icm42688p_reg_type>(
            field_to_raw(soft_reset_config, soft_reset_config_offset));
    }

    /// @brief Оператор присваивания, обновляющий значение регистра.
    auto operator=(
        icm42688p_reg_type value) -> icm42688p_device_config_reg &
    {
        parse(value);
        return *this;
    }

    /// @brief Оператор сравнения двух экземпляров регистра на равенство.
    bool operator==(
        const icm42688p_device_config_reg &other) const
    {
        return static_cast<icm42688p_reg_type>(*this)
               == static_cast<icm42688p_reg_type>(other);
    }

    /// @brief Перечисление состояния программного сброса.
    enum struct soft_reset_t : std::uint8_t {
        normal = 0, ///< Обычный режим работы (по умолчанию).
        enable = 1, ///< Запуск программного сброса.
    };

    /// @brief Программный сброс устройства.
    soft_reset_t soft_reset_config{soft_reset_t::normal};

  private:
    void parse(
        icm42688p_reg_type reg)
    {
        soft_reset_config =
            parse_flag(reg, soft_reset_config_offset, soft_reset_t::enable,
                       soft_reset_t::normal);
    }
};

/// @brief Регистр DRIVE_CONFIG (bank 0, адрес 0x13) для настройки скорости
/// нарастания фронтов выводов ICM-42688-P.
///
/// @details Содержит поля I2C_SLEW_RATE (биты 5:3) и SPI_SLEW_RATE (биты
///          2:0), см. раздел 14.2 даташита DS-000347 rev 1.6. Заметка 12.3
///          даташита рекомендует для режима I2C значение 1 для обоих полей.
class icm42688p_drive_config_reg
{
    static constexpr int i2c_slew_rate_offset{3U};
    static constexpr int spi_slew_rate_offset{0U};

  public:
    /// @brief Адрес регистра DRIVE_CONFIG в памяти устройства (bank 0).
    static constexpr icm42688p_reg_type addr{0x13};

    explicit icm42688p_drive_config_reg(
        icm42688p_reg_type value = icm42688p_reg_type{0x05})
    {
        parse(value);
    }

    /// @brief Оператор преобразования в сырое значение регистра.
    explicit operator icm42688p_reg_type() const
    {
        return static_cast<icm42688p_reg_type>(
            field_to_raw(i2c_slew_rate, i2c_slew_rate_offset)
            | field_to_raw(spi_slew_rate, spi_slew_rate_offset));
    }

    /// @brief Оператор присваивания, обновляющий значение регистра.
    auto operator=(
        icm42688p_reg_type value) -> icm42688p_drive_config_reg &
    {
        parse(value);
        return *this;
    }

    /// @brief Оператор сравнения двух экземпляров регистра на равенство.
    bool operator==(
        const icm42688p_drive_config_reg &other) const
    {
        return static_cast<icm42688p_reg_type>(*this)
               == static_cast<icm42688p_reg_type>(other);
    }

    /// @brief Перечисление значений скорости нарастания фронтов.
    enum struct slew_rate_t : std::uint8_t {
        ns_20_60 = 0, ///< 20–60 нс (по умолчанию для I2C).
        ns_12_36 = 1, ///< 12–36 нс.
        ns_6_18  = 2, ///< 6–18 нс.
        ns_4_12  = 3, ///< 4–12 нс.
        ns_2_6   = 4, ///< 2–6 нс.
        less_2ns = 5, ///< Менее 2 нс (по умолчанию для SPI после сброса).
    };

    /// @brief Скорость нарастания фронтов вывода 14 в режиме I2C.
    slew_rate_t i2c_slew_rate{slew_rate_t::ns_20_60};

    /// @brief Скорость нарастания фронтов вывода 14 в режиме SPI/I3C и всех
    /// остальных выводов.
    slew_rate_t spi_slew_rate{slew_rate_t::less_2ns};

  private:
    void parse(
        icm42688p_reg_type reg)
    {
        constexpr icm42688p_reg_type slew_mask{0x07};
        i2c_slew_rate =
            extract_field<slew_rate_t>(reg, i2c_slew_rate_offset, slew_mask);
        spi_slew_rate =
            extract_field<slew_rate_t>(reg, spi_slew_rate_offset, slew_mask);
    }
};

/// @brief Регистр INT_CONFIG (bank 0, адрес 0x14) для настройки электрики
/// выводов прерываний INT1/INT2 ICM-42688-P.
///
/// @details Содержит поля режима (импульсный/защёлкнутый), типа выхода
///          (открытый сток/двухтактный) и полярности (активный ноль/единица)
///          для INT1 (биты 2:0) и INT2 (биты 5:3), см. раздел 14.3 даташита
///          DS-000347 rev 1.6.
class icm42688p_int_config_reg
{
    static constexpr int int2_mode_offset{5U};
    static constexpr int int2_drive_circuit_offset{4U};
    static constexpr int int2_polarity_offset{3U};
    static constexpr int int1_mode_offset{2U};
    static constexpr int int1_drive_circuit_offset{1U};
    static constexpr int int1_polarity_offset{0U};

  public:
    /// @brief Адрес регистра INT_CONFIG в памяти устройства (bank 0).
    static constexpr icm42688p_reg_type addr{0x14};

    explicit icm42688p_int_config_reg(
        icm42688p_reg_type value = icm42688p_reg_type{0})
    {
        parse(value);
    }

    /// @brief Оператор преобразования в сырое значение регистра.
    explicit operator icm42688p_reg_type() const
    {
        return static_cast<icm42688p_reg_type>(
            field_to_raw(int2_mode, int2_mode_offset)
            | field_to_raw(int2_drive_circuit, int2_drive_circuit_offset)
            | field_to_raw(int2_polarity, int2_polarity_offset)
            | field_to_raw(int1_mode, int1_mode_offset)
            | field_to_raw(int1_drive_circuit, int1_drive_circuit_offset)
            | field_to_raw(int1_polarity, int1_polarity_offset));
    }

    /// @brief Оператор присваивания, обновляющий значение регистра.
    auto operator=(
        icm42688p_reg_type value) -> icm42688p_int_config_reg &
    {
        parse(value);
        return *this;
    }

    /// @brief Оператор сравнения двух экземпляров регистра на равенство.
    bool operator==(
        const icm42688p_int_config_reg &other) const
    {
        return static_cast<icm42688p_reg_type>(*this)
               == static_cast<icm42688p_reg_type>(other);
    }

    /// @brief Перечисление режимов прерывания.
    enum struct mode_t : std::uint8_t {
        pulse   = 0, ///< Импульсный режим (100 мкс, по умолчанию).
        latched = 1, ///< Защёлкнутый режим.
    };

    /// @brief Перечисление типа выходного каскада прерывания.
    enum struct drive_circuit_t : std::uint8_t {
        open_drain = 0, ///< Открытый сток (по умолчанию).
        push_pull  = 1, ///< Двухтактный выход (push-pull).
    };

    /// @brief Перечисление полярности прерывания.
    enum struct polarity_t : std::uint8_t {
        active_low  = 0, ///< Активный низкий уровень (по умолчанию).
        active_high = 1, ///< Активный высокий уровень.
    };

    /// @brief Режим прерывания INT2.
    mode_t int2_mode{mode_t::pulse};

    /// @brief Тип выходного каскада прерывания INT2.
    drive_circuit_t int2_drive_circuit{drive_circuit_t::open_drain};

    /// @brief Полярность прерывания INT2.
    polarity_t int2_polarity{polarity_t::active_low};

    /// @brief Режим прерывания INT1.
    mode_t int1_mode{mode_t::pulse};

    /// @brief Тип выходного каскада прерывания INT1.
    drive_circuit_t int1_drive_circuit{drive_circuit_t::open_drain};

    /// @brief Полярность прерывания INT1.
    polarity_t int1_polarity{polarity_t::active_low};

  private:
    void parse(
        icm42688p_reg_type reg)
    {
        int2_mode =
            parse_flag(reg, int2_mode_offset, mode_t::latched, mode_t::pulse);
        int2_drive_circuit =
            parse_flag(reg, int2_drive_circuit_offset,
                       drive_circuit_t::push_pull, drive_circuit_t::open_drain);
        int2_polarity =
            parse_flag(reg, int2_polarity_offset, polarity_t::active_high,
                       polarity_t::active_low);
        int1_mode =
            parse_flag(reg, int1_mode_offset, mode_t::latched, mode_t::pulse);
        int1_drive_circuit =
            parse_flag(reg, int1_drive_circuit_offset,
                       drive_circuit_t::push_pull, drive_circuit_t::open_drain);
        int1_polarity =
            parse_flag(reg, int1_polarity_offset, polarity_t::active_high,
                       polarity_t::active_low);
    }
};

/// @brief Регистр FIFO_CONFIG (bank 0, адрес 0x16) для выбора режима FIFO
/// ICM-42688-P.
///
/// @details Содержит поле FIFO_MODE (биты 7:6), см. раздел 14.4 даташита
///          DS-000347 rev 1.6.
class icm42688p_fifo_config_reg
{
    static constexpr int fifo_mode_offset{6U};

  public:
    /// @brief Адрес регистра FIFO_CONFIG в памяти устройства (bank 0).
    static constexpr icm42688p_reg_type addr{0x16};

    explicit icm42688p_fifo_config_reg(
        icm42688p_reg_type value = icm42688p_reg_type{0})
    {
        parse(value);
    }

    /// @brief Оператор преобразования в сырое значение регистра.
    explicit operator icm42688p_reg_type() const
    {
        return static_cast<icm42688p_reg_type>(
            field_to_raw(fifo_mode, fifo_mode_offset));
    }

    /// @brief Оператор присваивания, обновляющий значение регистра.
    auto operator=(
        icm42688p_reg_type value) -> icm42688p_fifo_config_reg &
    {
        parse(value);
        return *this;
    }

    /// @brief Оператор сравнения двух экземпляров регистра на равенство.
    bool operator==(
        const icm42688p_fifo_config_reg &other) const
    {
        return static_cast<icm42688p_reg_type>(*this)
               == static_cast<icm42688p_reg_type>(other);
    }

    /// @brief Перечисление режимов работы FIFO.
    enum struct fifo_mode_t : std::uint8_t {
        bypass         = 0, ///< Обход FIFO (по умолчанию).
        stream_to_fifo = 1, ///< Потоковый режим (Stream-to-FIFO).
        stop_on_full   = 2, ///< Остановка при заполнении (STOP-on-FULL).
    };

    /// @brief Режим работы FIFO.
    fifo_mode_t fifo_mode{fifo_mode_t::bypass};

  private:
    void parse(
        icm42688p_reg_type reg)
    {
        constexpr icm42688p_reg_type fifo_mode_mask{0x03};
        fifo_mode =
            extract_field<fifo_mode_t>(reg, fifo_mode_offset, fifo_mode_mask);
    }
};

/// @brief Регистр INT_STATUS (bank 0, адрес 0x2D) для опроса флагов
/// прерываний ICM-42688-P.
///
/// @details Флаги очищаются после чтения регистра (R/C). Бит DATA_RDY_INT —
///          бит 3 (не бит 0!), см. раздел 14.21 даташита DS-000347 rev 1.6.
///          Значение после сброса — 0x10 (RESET_DONE_INT).
class icm42688p_int_status_reg
{
    static constexpr int ui_fsync_int_offset{6U};
    static constexpr int pll_rdy_int_offset{5U};
    static constexpr int reset_done_int_offset{4U};
    static constexpr int data_rdy_int_offset{3U};
    static constexpr int fifo_ths_int_offset{2U};
    static constexpr int fifo_full_int_offset{1U};
    static constexpr int agc_rdy_int_offset{0U};

  public:
    /// @brief Адрес регистра INT_STATUS в памяти устройства (bank 0).
    static constexpr icm42688p_reg_type addr{0x2D};

    explicit icm42688p_int_status_reg(
        icm42688p_reg_type value = icm42688p_reg_type{0x10})
    {
        parse(value);
    }

    /// @brief Оператор преобразования в сырое значение регистра.
    explicit operator icm42688p_reg_type() const
    {
        return static_cast<icm42688p_reg_type>(
            field_to_raw(ui_fsync_int, ui_fsync_int_offset)
            | field_to_raw(pll_rdy_int, pll_rdy_int_offset)
            | field_to_raw(reset_done_int, reset_done_int_offset)
            | field_to_raw(data_rdy_int, data_rdy_int_offset)
            | field_to_raw(fifo_ths_int, fifo_ths_int_offset)
            | field_to_raw(fifo_full_int, fifo_full_int_offset)
            | field_to_raw(agc_rdy_int, agc_rdy_int_offset));
    }

    /// @brief Оператор присваивания, обновляющий значение регистра.
    auto operator=(
        icm42688p_reg_type value) -> icm42688p_int_status_reg &
    {
        parse(value);
        return *this;
    }

    /// @brief Оператор сравнения двух экземпляров регистра на равенство.
    bool operator==(
        const icm42688p_int_status_reg &other) const
    {
        return static_cast<icm42688p_reg_type>(*this)
               == static_cast<icm42688p_reg_type>(other);
    }

    /// @brief Перечисление состояния флага прерывания.
    enum struct switcher_t : std::uint8_t {
        disable = 0, ///< Прерывание не сгенерировано.
        enable  = 1, ///< Прерывание сгенерировано.
    };

    /// @brief Флаг прерывания UI FSYNC.
    switcher_t ui_fsync_int{switcher_t::disable};

    /// @brief Флаг прерывания готовности PLL.
    switcher_t pll_rdy_int{switcher_t::disable};

    /// @brief Флаг завершения программного сброса.
    switcher_t reset_done_int{switcher_t::enable};

    /// @brief Флаг готовности новых данных сенсоров (данные обновлены в
    /// регистрах и/или FIFO).
    switcher_t data_rdy_int{switcher_t::disable};

    /// @brief Флаг прерывания по порогу заполнения FIFO.
    switcher_t fifo_ths_int{switcher_t::disable};

    /// @brief Флаг прерывания переполнения FIFO.
    switcher_t fifo_full_int{switcher_t::disable};

    /// @brief Флаг прерывания готовности AGC.
    switcher_t agc_rdy_int{switcher_t::disable};

  private:
    void parse(
        icm42688p_reg_type reg)
    {
        ui_fsync_int = parse_flag(reg, ui_fsync_int_offset, switcher_t::enable,
                                  switcher_t::disable);
        pll_rdy_int  = parse_flag(reg, pll_rdy_int_offset, switcher_t::enable,
                                  switcher_t::disable);
        reset_done_int = parse_flag(reg, reset_done_int_offset,
                                    switcher_t::enable, switcher_t::disable);
        data_rdy_int  = parse_flag(reg, data_rdy_int_offset, switcher_t::enable,
                                   switcher_t::disable);
        fifo_ths_int  = parse_flag(reg, fifo_ths_int_offset, switcher_t::enable,
                                   switcher_t::disable);
        fifo_full_int = parse_flag(reg, fifo_full_int_offset,
                                   switcher_t::enable, switcher_t::disable);
        agc_rdy_int   = parse_flag(reg, agc_rdy_int_offset, switcher_t::enable,
                                   switcher_t::disable);
    }
};

/// @brief Регистр INTF_CONFIG0 (bank 0, адрес 0x4C) для настройки формата
/// данных FIFO и сенсорных регистров ICM-42688-P.
///
/// @details Содержит поля FIFO_HOLD_LAST_DATA_EN (бит 7), FIFO_COUNT_REC
///          (бит 6), FIFO_COUNT_ENDIAN (бит 5) и SENSOR_DATA_ENDIAN (бит 4),
///          см. раздел 14.34 даташита DS-000347 rev 1.6. Значение после
///          сброса — 0x30 (оба endianness-поля установлены в Big Endian).
class icm42688p_intf_config0_reg
{
    static constexpr int fifo_hold_last_data_en_offset{7U};
    static constexpr int fifo_count_rec_offset{6U};
    static constexpr int fifo_count_endian_offset{5U};
    static constexpr int sensor_data_endian_offset{4U};

  public:
    /// @brief Адрес регистра INTF_CONFIG0 в памяти устройства (bank 0).
    static constexpr icm42688p_reg_type addr{0x4C};

    explicit icm42688p_intf_config0_reg(
        icm42688p_reg_type value = icm42688p_reg_type{0x30})
    {
        parse(value);
    }

    /// @brief Оператор преобразования в сырое значение регистра.
    explicit operator icm42688p_reg_type() const
    {
        return static_cast<icm42688p_reg_type>(
            field_to_raw(fifo_hold_last_data_en, fifo_hold_last_data_en_offset)
            | field_to_raw(fifo_count_rec, fifo_count_rec_offset)
            | field_to_raw(fifo_count_endian, fifo_count_endian_offset)
            | field_to_raw(sensor_data_endian, sensor_data_endian_offset));
    }

    /// @brief Оператор присваивания, обновляющий значение регистра.
    auto operator=(
        icm42688p_reg_type value) -> icm42688p_intf_config0_reg &
    {
        parse(value);
        return *this;
    }

    /// @brief Оператор сравнения двух экземпляров регистра на равенство.
    bool operator==(
        const icm42688p_intf_config0_reg &other) const
    {
        return static_cast<icm42688p_reg_type>(*this)
               == static_cast<icm42688p_reg_type>(other);
    }

    /// @brief Перечисление состояния бита конфигурации.
    enum struct switcher_t : std::uint8_t {
        disable = 0, ///< Отключено.
        enable  = 1, ///< Включено.
    };

    /// @brief Перечисление порядка байт данных.
    enum struct endian_t : std::uint8_t {
        little = 0, ///< Младший байт первым (Little Endian).
        big    = 1, ///< Старший байт первым (Big Endian, по умолчанию).
    };

    /// @brief Обработка некорректных выборок в FIFO (копия последней
    /// валидной вместо кода некорректной выборки).
    switcher_t fifo_hold_last_data_en{switcher_t::disable};

    /// @brief Единицы счётчика FIFO: 0 — байты, 1 — записи (records).
    switcher_t fifo_count_rec{switcher_t::disable};

    /// @brief Порядок байт счётчика FIFO.
    endian_t fifo_count_endian{endian_t::big};

    /// @brief Порядок байт данных сенсоров.
    endian_t sensor_data_endian{endian_t::big};

  private:
    void parse(
        icm42688p_reg_type reg)
    {
        fifo_hold_last_data_en =
            parse_flag(reg, fifo_hold_last_data_en_offset, switcher_t::enable,
                       switcher_t::disable);
        fifo_count_rec     = parse_flag(reg, fifo_count_rec_offset,
                                        switcher_t::enable, switcher_t::disable);
        fifo_count_endian  = parse_flag(reg, fifo_count_endian_offset,
                                        endian_t::big, endian_t::little);
        sensor_data_endian = parse_flag(reg, sensor_data_endian_offset,
                                        endian_t::big, endian_t::little);
    }
};

/// @brief Регистр PWR_MGMT0 (bank 0, адрес 0x4E) для управления питанием
/// сенсоров ICM-42688-P.
///
/// @details Содержит поля GYRO_MODE (биты 3:2) и ACCEL_MODE (биты 1:0), а
///          также TEMP_DIS (бит 5) и IDLE (бит 4), см. раздел 14.36 даташита
///          DS-000347 rev 1.6. После выхода сенсора из состояния OFF нельзя
///          выполнять запись в регистры в течение 200 мкс, гироскопу
///          требуется не менее 45 мс для выхода на валидные данные.
class icm42688p_pwr_mgmt0_reg
{
    static constexpr int temp_dis_offset{5U};
    static constexpr int gyro_mode_offset{2U};
    static constexpr int accel_mode_offset{0U};

  public:
    /// @brief Адрес регистра PWR_MGMT0 в памяти устройства (bank 0).
    static constexpr icm42688p_reg_type addr{0x4E};

    explicit icm42688p_pwr_mgmt0_reg(
        icm42688p_reg_type value = icm42688p_reg_type{0})
    {
        parse(value);
    }

    /// @brief Оператор преобразования в сырое значение регистра.
    explicit operator icm42688p_reg_type() const
    {
        return static_cast<icm42688p_reg_type>(
            field_to_raw(temp_dis, temp_dis_offset)
            | field_to_raw(gyro_mode, gyro_mode_offset)
            | field_to_raw(accel_mode, accel_mode_offset));
    }

    /// @brief Оператор присваивания, обновляющий значение регистра.
    auto operator=(
        icm42688p_reg_type value) -> icm42688p_pwr_mgmt0_reg &
    {
        parse(value);
        return *this;
    }

    /// @brief Оператор сравнения двух экземпляров регистра на равенство.
    bool operator==(
        const icm42688p_pwr_mgmt0_reg &other) const
    {
        return static_cast<icm42688p_reg_type>(*this)
               == static_cast<icm42688p_reg_type>(other);
    }

    /// @brief Перечисление состояния бита питания.
    enum struct switcher_t : std::uint8_t {
        disable = 0, ///< Отключено.
        enable  = 1, ///< Включено.
    };

    /// @brief Перечисление режимов работы гироскопа.
    enum struct gyro_mode_t : std::uint8_t {
        off       = 0, ///< Выключен (по умолчанию).
        standby   = 1, ///< Режим ожидания (Standby).
        reserved  = 2, ///< Зарезервировано.
        low_noise = 3, ///< Режим малого шума (Low Noise).
    };

    /// @brief Перечисление режимов работы акселерометра.
    enum struct accel_mode_t : std::uint8_t {
        off          = 0, ///< Выключен (по умолчанию).
        off_repeated = 1, ///< Выключен (дублирующая кодировка).
        low_power    = 2, ///< Режим малого энергопотребления (Low Power).
        low_noise    = 3, ///< Режим малого шума (Low Noise).
    };

    /// @brief Отключение датчика температуры.
    switcher_t temp_dis{switcher_t::disable};

    /// @brief Режим работы гироскопа.
    gyro_mode_t gyro_mode{gyro_mode_t::off};

    /// @brief Режим работы акселерометра.
    accel_mode_t accel_mode{accel_mode_t::off};

  private:
    void parse(
        icm42688p_reg_type reg)
    {
        temp_dis = parse_flag(reg, temp_dis_offset, switcher_t::enable,
                              switcher_t::disable);
        constexpr icm42688p_reg_type mode_mask{0x03};
        gyro_mode =
            extract_field<gyro_mode_t>(reg, gyro_mode_offset, mode_mask);
        accel_mode =
            extract_field<accel_mode_t>(reg, accel_mode_offset, mode_mask);
    }
};

/// @brief Регистр GYRO_CONFIG0 (bank 0, адрес 0x4F) для настройки диапазона
/// измерений и частоты дискретизации гироскопа ICM-42688-P.
///
/// @details Содержит поля GYRO_UI_FS_SEL (биты 7:5) и GYRO_ODR (биты 3:0),
///          см. раздел 14.37 даташита DS-000347 rev 1.6. Значение после
///          сброса — 0x06 (±2000 dps, 1 кГц).
class icm42688p_gyro_config0_reg
{
    static constexpr int fs_sel_offset{5U};
    static constexpr int odr_offset{0U};

  public:
    /// @brief Адрес регистра GYRO_CONFIG0 в памяти устройства (bank 0).
    static constexpr icm42688p_reg_type addr{0x4F};

    explicit icm42688p_gyro_config0_reg(
        icm42688p_reg_type value = icm42688p_reg_type{0x06})
    {
        parse(value);
    }

    /// @brief Оператор преобразования в сырое значение регистра.
    explicit operator icm42688p_reg_type() const
    {
        return static_cast<icm42688p_reg_type>(
            field_to_raw(fs_sel, fs_sel_offset)
            | field_to_raw(odr, odr_offset));
    }

    /// @brief Оператор присваивания, обновляющий значение регистра.
    auto operator=(
        icm42688p_reg_type value) -> icm42688p_gyro_config0_reg &
    {
        parse(value);
        return *this;
    }

    /// @brief Оператор сравнения двух экземпляров регистра на равенство.
    bool operator==(
        const icm42688p_gyro_config0_reg &other) const
    {
        return static_cast<icm42688p_reg_type>(*this)
               == static_cast<icm42688p_reg_type>(other);
    }

    /// @brief Диапазон измерений гироскопа.
    icm42688p_gyro_fsr_t fs_sel{icm42688p_gyro_fsr_t::dps_2000};

    /// @brief Частота дискретизации гироскопа.
    icm42688p_odr_t odr{icm42688p_odr_t::odr_1k};

  private:
    void parse(
        icm42688p_reg_type reg)
    {
        constexpr icm42688p_reg_type fs_mask{0x07};
        fs_sel =
            extract_field<icm42688p_gyro_fsr_t>(reg, fs_sel_offset, fs_mask);

        constexpr icm42688p_reg_type odr_mask{0x0F};
        odr = extract_field<icm42688p_odr_t>(reg, odr_offset, odr_mask);
    }
};

/// @brief Регистр ACCEL_CONFIG0 (bank 0, адрес 0x50) для настройки диапазона
/// измерений и частоты дискретизации акселерометра ICM-42688-P.
///
/// @details Содержит поля ACCEL_UI_FS_SEL (биты 7:5) и ACCEL_ODR (биты
///          3:0), см. раздел 14.38 даташита DS-000347 rev 1.6. Значение
///          после сброса — 0x06 (±16 g, 1 кГц).
class icm42688p_accel_config0_reg
{
    static constexpr int fs_sel_offset{5U};
    static constexpr int odr_offset{0U};

  public:
    /// @brief Адрес регистра ACCEL_CONFIG0 в памяти устройства (bank 0).
    static constexpr icm42688p_reg_type addr{0x50};

    explicit icm42688p_accel_config0_reg(
        icm42688p_reg_type value = icm42688p_reg_type{0x06})
    {
        parse(value);
    }

    /// @brief Оператор преобразования в сырое значение регистра.
    explicit operator icm42688p_reg_type() const
    {
        return static_cast<icm42688p_reg_type>(
            field_to_raw(fs_sel, fs_sel_offset)
            | field_to_raw(odr, odr_offset));
    }

    /// @brief Оператор присваивания, обновляющий значение регистра.
    auto operator=(
        icm42688p_reg_type value) -> icm42688p_accel_config0_reg &
    {
        parse(value);
        return *this;
    }

    /// @brief Оператор сравнения двух экземпляров регистра на равенство.
    bool operator==(
        const icm42688p_accel_config0_reg &other) const
    {
        return static_cast<icm42688p_reg_type>(*this)
               == static_cast<icm42688p_reg_type>(other);
    }

    /// @brief Диапазон измерений акселерометра.
    icm42688p_accel_fsr_t fs_sel{icm42688p_accel_fsr_t::g_16};

    /// @brief Частота дискретизации акселерометра.
    icm42688p_odr_t odr{icm42688p_odr_t::odr_1k};

  private:
    void parse(
        icm42688p_reg_type reg)
    {
        constexpr icm42688p_reg_type fs_mask{0x07};
        fs_sel =
            extract_field<icm42688p_accel_fsr_t>(reg, fs_sel_offset, fs_mask);

        constexpr icm42688p_reg_type odr_mask{0x0F};
        odr = extract_field<icm42688p_odr_t>(reg, odr_offset, odr_mask);
    }
};

/// @brief Регистр FIFO_CONFIG1 (bank 0, адрес 0x5F) для выбора типа данных,
/// помещаемых в FIFO ICM-42688-P.
///
/// @details Содержит поля FIFO_HIRES_EN (бит 4), FIFO_TMST_FSYNC_EN (бит 3),
///          FIFO_TEMP_EN (бит 2), FIFO_GYRO_EN (бит 1) и FIFO_ACCEL_EN
///          (бит 0), см. раздел 14.45 даташита DS-000347 rev 1.6.
///          Установка FIFO_HIRES_EN переводит FIFO в режим 20-байтных
///          пакетов с расширенным разрешением (20 бит).
class icm42688p_fifo_config1_reg
{
    static constexpr int resume_partial_rd_offset{7U};
    static constexpr int wm_gt_th_offset{6U};
    static constexpr int hires_en_offset{4U};
    static constexpr int tmst_fsync_en_offset{3U};
    static constexpr int temp_en_offset{2U};
    static constexpr int gyro_en_offset{1U};
    static constexpr int accel_en_offset{0U};

  public:
    /// @brief Адрес регистра FIFO_CONFIG1 в памяти устройства (bank 0).
    static constexpr icm42688p_reg_type addr{0x5F};

    explicit icm42688p_fifo_config1_reg(
        icm42688p_reg_type value = icm42688p_reg_type{0})
    {
        parse(value);
    }

    /// @brief Оператор преобразования в сырое значение регистра.
    explicit operator icm42688p_reg_type() const
    {
        return static_cast<icm42688p_reg_type>(
            field_to_raw(resume_partial_rd, resume_partial_rd_offset)
            | field_to_raw(wm_gt_th, wm_gt_th_offset)
            | field_to_raw(hires_en, hires_en_offset)
            | field_to_raw(tmst_fsync_en, tmst_fsync_en_offset)
            | field_to_raw(temp_en, temp_en_offset)
            | field_to_raw(gyro_en, gyro_en_offset)
            | field_to_raw(accel_en, accel_en_offset));
    }

    /// @brief Оператор присваивания, обновляющий значение регистра.
    auto operator=(
        icm42688p_reg_type value) -> icm42688p_fifo_config1_reg &
    {
        parse(value);
        return *this;
    }

    /// @brief Оператор сравнения двух экземпляров регистра на равенство.
    bool operator==(
        const icm42688p_fifo_config1_reg &other) const
    {
        return static_cast<icm42688p_reg_type>(*this)
               == static_cast<icm42688p_reg_type>(other);
    }

    /// @brief Перечисление состояния бита включения.
    enum struct switcher_t : std::uint8_t {
        disable = 0, ///< Отключено.
        enable  = 1, ///< Включено.
    };

    /// @brief Разрешение частичного чтения FIFO с продолжением.
    switcher_t resume_partial_rd{switcher_t::disable};

    /// @brief Генерация прерывания watermark каждый ODR при FIFO_COUNT ≥ WM.
    switcher_t wm_gt_th{switcher_t::disable};

    /// @brief Расширенное разрешение данных (20-битный формат пакетов).
    switcher_t hires_en{switcher_t::disable};

    /// @brief Запись времени FSYNC в FIFO.
    switcher_t tmst_fsync_en{switcher_t::disable};

    /// @brief Запись данных температуры в FIFO.
    switcher_t temp_en{switcher_t::disable};

    /// @brief Запись данных гироскопа в FIFO.
    switcher_t gyro_en{switcher_t::disable};

    /// @brief Запись данных акселерометра в FIFO.
    switcher_t accel_en{switcher_t::disable};

  private:
    void parse(
        icm42688p_reg_type reg)
    {
        resume_partial_rd = parse_flag(reg, resume_partial_rd_offset,
                                       switcher_t::enable, switcher_t::disable);
        wm_gt_th          = parse_flag(reg, wm_gt_th_offset, switcher_t::enable,
                                       switcher_t::disable);
        hires_en          = parse_flag(reg, hires_en_offset, switcher_t::enable,
                                       switcher_t::disable);
        tmst_fsync_en     = parse_flag(reg, tmst_fsync_en_offset,
                                       switcher_t::enable, switcher_t::disable);
        temp_en           = parse_flag(reg, temp_en_offset, switcher_t::enable,
                                       switcher_t::disable);
        gyro_en           = parse_flag(reg, gyro_en_offset, switcher_t::enable,
                                       switcher_t::disable);
        accel_en          = parse_flag(reg, accel_en_offset, switcher_t::enable,
                                       switcher_t::disable);
    }
};

/// @brief Регистр INT_CONFIG0 (bank 0, адрес 0x63) для настройки условий
/// сброса прерываний ICM-42688-P.
///
/// @details Содержит поле UI_DRDY_INT_CLEAR (биты 5:4), см. раздел 14.49
///          даташита DS-000347 rev 1.6. Значение 0b10 очищает прерывание
///          готовности данных при чтении сенсорных регистров.
class icm42688p_int_config0_reg
{
    static constexpr int ui_drdy_int_clear_offset{4U};
    static constexpr int fifo_ths_int_clear_offset{2U};
    static constexpr int fifo_full_int_clear_offset{0U};

  public:
    /// @brief Адрес регистра INT_CONFIG0 в памяти устройства (bank 0).
    static constexpr icm42688p_reg_type addr{0x63};

    explicit icm42688p_int_config0_reg(
        icm42688p_reg_type value = icm42688p_reg_type{0})
    {
        parse(value);
    }

    /// @brief Оператор преобразования в сырое значение регистра.
    explicit operator icm42688p_reg_type() const
    {
        return static_cast<icm42688p_reg_type>(
            field_to_raw(ui_drdy_int_clear, ui_drdy_int_clear_offset)
            | field_to_raw(fifo_ths_int_clear, fifo_ths_int_clear_offset)
            | field_to_raw(fifo_full_int_clear, fifo_full_int_clear_offset));
    }

    /// @brief Оператор присваивания, обновляющий значение регистра.
    auto operator=(
        icm42688p_reg_type value) -> icm42688p_int_config0_reg &
    {
        parse(value);
        return *this;
    }

    /// @brief Оператор сравнения двух экземпляров регистра на равенство.
    bool operator==(
        const icm42688p_int_config0_reg &other) const
    {
        return static_cast<icm42688p_reg_type>(*this)
               == static_cast<icm42688p_reg_type>(other);
    }

    /// @brief Перечисление условий сброса прерывания.
    enum struct int_clear_t : std::uint8_t {
        status_read = 0, ///< Сброс при чтении регистра статуса (по умолчанию).
        status_read_also     = 1, ///< Сброс при чтении регистра статуса.
        sensor_register_read = 2, ///< Сброс при чтении сенсорных регистров.
        status_and_sensor_register_read = 3, ///< Сброс при обоих событиях.
    };

    /// @brief Условие сброса прерывания готовности данных UI.
    int_clear_t ui_drdy_int_clear{int_clear_t::status_read};

    /// @brief Условие сброса прерывания порога FIFO.
    int_clear_t fifo_ths_int_clear{int_clear_t::status_read};

    /// @brief Условие сброса прерывания переполнения FIFO.
    int_clear_t fifo_full_int_clear{int_clear_t::status_read};

  private:
    void parse(
        icm42688p_reg_type reg)
    {
        constexpr icm42688p_reg_type clear_mask{0x03};
        ui_drdy_int_clear = extract_field<int_clear_t>(
            reg, ui_drdy_int_clear_offset, clear_mask);
        fifo_ths_int_clear = extract_field<int_clear_t>(
            reg, fifo_ths_int_clear_offset, clear_mask);
        fifo_full_int_clear = extract_field<int_clear_t>(
            reg, fifo_full_int_clear_offset, clear_mask);
    }
};

/// @brief Регистр INT_CONFIG1 (bank 0, адрес 0x64) для настройки таймингов
/// прерываний ICM-42688-P.
///
/// @details Содержит поля INT_TPULSE_DURATION (бит 6), INT_TDEASSERT_DISABLE
///          (бит 5) и INT_ASYNC_RESET (бит 4), см. раздел 14.50 даташита
///          DS-000347 rev 1.6. Значение после сброса — 0x10
///          (INT_ASYNC_RESET = 1); по заметке 12.6 даташита этот бит нужно
///          сбросить в 0 для корректной работы выводов INT1/INT2.
class icm42688p_int_config1_reg
{
    static constexpr int int_pulse_duration_offset{6U};
    static constexpr int int_deassert_disable_offset{5U};
    static constexpr int int_async_reset_offset{4U};

  public:
    /// @brief Адрес регистра INT_CONFIG1 в памяти устройства (bank 0).
    static constexpr icm42688p_reg_type addr{0x64};

    explicit icm42688p_int_config1_reg(
        icm42688p_reg_type value = icm42688p_reg_type{0x10})
    {
        parse(value);
    }

    /// @brief Оператор преобразования в сырое значение регистра.
    explicit operator icm42688p_reg_type() const
    {
        return static_cast<icm42688p_reg_type>(
            field_to_raw(int_pulse_duration, int_pulse_duration_offset)
            | field_to_raw(int_deassert_disable, int_deassert_disable_offset)
            | field_to_raw(int_async_reset, int_async_reset_offset));
    }

    /// @brief Оператор присваивания, обновляющий значение регистра.
    auto operator=(
        icm42688p_reg_type value) -> icm42688p_int_config1_reg &
    {
        parse(value);
        return *this;
    }

    /// @brief Оператор сравнения двух экземпляров регистра на равенство.
    bool operator==(
        const icm42688p_int_config1_reg &other) const
    {
        return static_cast<icm42688p_reg_type>(*this)
               == static_cast<icm42688p_reg_type>(other);
    }

    /// @brief Перечисление длительности импульса прерывания.
    enum struct pulse_duration_t : std::uint8_t {
        us_100 = 0, ///< 100 мкс (по умолчанию, только при ODR < 4 кГц).
        us_8   = 1, ///< 8 мкс (обязательно при ODR ≥ 4 кГц).
    };

    /// @brief Перечисление длительности деактивации прерывания.
    enum struct deassert_t : std::uint8_t {
        us_100_minimum = 0, ///< Минимум 100 мкс (по умолчанию).
        disabled       = 1, ///< Деактивация отключена.
    };

    /// @brief Перечисление режима асинхронного сброса прерывания.
    enum struct async_reset_t : std::uint8_t {
        reset    = 1, ///< Асинхронный сброс (значение после сброса).
        no_reset = 0, ///< Асинхронный сброс отключен (рекомендуется).
    };

    /// @brief Длительность импульса прерывания.
    pulse_duration_t int_pulse_duration{pulse_duration_t::us_100};

    /// @brief Длительность деактивации прерывания.
    deassert_t int_deassert_disable{deassert_t::us_100_minimum};

    /// @brief Режим асинхронного сброса прерывания.
    async_reset_t int_async_reset{async_reset_t::reset};

  private:
    void parse(
        icm42688p_reg_type reg)
    {
        int_pulse_duration =
            parse_flag(reg, int_pulse_duration_offset, pulse_duration_t::us_8,
                       pulse_duration_t::us_100);
        int_deassert_disable =
            parse_flag(reg, int_deassert_disable_offset, deassert_t::disabled,
                       deassert_t::us_100_minimum);
        int_async_reset =
            parse_flag(reg, int_async_reset_offset, async_reset_t::reset,
                       async_reset_t::no_reset);
    }
};

/// @brief Регистр INT_SOURCE0 (bank 0, адрес 0x65) для маршрутизации
/// прерываний на вывод INT1 ICM-42688-P.
///
/// @details Содержит поле UI_DRDY_INT1_EN — бит 4 (не бит 0!), см. раздел
///          14.51 даташита DS-000347 rev 1.6. Значение после сброса — 0x10,
///          то есть прерывание готовности данных уже замаршрутизировано на
///          INT1.
class icm42688p_int_source0_reg
{
    static constexpr int ui_fsync_int1_en_offset{7U};
    static constexpr int pll_rdy_int1_en_offset{6U};
    static constexpr int reset_done_int1_en_offset{5U};
    static constexpr int ui_drdy_int1_en_offset{4U};
    static constexpr int fifo_ths_int1_en_offset{3U};
    static constexpr int fifo_full_int1_en_offset{2U};
    static constexpr int ui_agc_rdy_int1_en_offset{1U};

  public:
    /// @brief Адрес регистра INT_SOURCE0 в памяти устройства (bank 0).
    static constexpr icm42688p_reg_type addr{0x65};

    explicit icm42688p_int_source0_reg(
        icm42688p_reg_type value = icm42688p_reg_type{0x10})
    {
        parse(value);
    }

    /// @brief Оператор преобразования в сырое значение регистра.
    explicit operator icm42688p_reg_type() const
    {
        return static_cast<icm42688p_reg_type>(
            field_to_raw(ui_fsync_int1_en, ui_fsync_int1_en_offset)
            | field_to_raw(pll_rdy_int1_en, pll_rdy_int1_en_offset)
            | field_to_raw(reset_done_int1_en, reset_done_int1_en_offset)
            | field_to_raw(ui_drdy_int1_en, ui_drdy_int1_en_offset)
            | field_to_raw(fifo_ths_int1_en, fifo_ths_int1_en_offset)
            | field_to_raw(fifo_full_int1_en, fifo_full_int1_en_offset)
            | field_to_raw(ui_agc_rdy_int1_en, ui_agc_rdy_int1_en_offset));
    }

    /// @brief Оператор присваивания, обновляющий значение регистра.
    auto operator=(
        icm42688p_reg_type value) -> icm42688p_int_source0_reg &
    {
        parse(value);
        return *this;
    }

    /// @brief Оператор сравнения двух экземпляров регистра на равенство.
    bool operator==(
        const icm42688p_int_source0_reg &other) const
    {
        return static_cast<icm42688p_reg_type>(*this)
               == static_cast<icm42688p_reg_type>(other);
    }

    /// @brief Перечисление состояния маршрутизации прерывания.
    enum struct switcher_t : std::uint8_t {
        disable = 0, ///< Прерывание не маршрутизировано на INT1.
        enable  = 1, ///< Прерывание маршрутизировано на INT1.
    };

    /// @brief Маршрутизация прерывания UI FSYNC на INT1.
    switcher_t ui_fsync_int1_en{switcher_t::disable};

    /// @brief Маршрутизация прерывания готовности PLL на INT1.
    switcher_t pll_rdy_int1_en{switcher_t::disable};

    /// @brief Маршрутизация прерывания завершения сброса на INT1.
    switcher_t reset_done_int1_en{switcher_t::disable};

    /// @brief Маршрутизация прерывания готовности данных UI на INT1.
    switcher_t ui_drdy_int1_en{switcher_t::enable};

    /// @brief Маршрутизация прерывания порога FIFO на INT1.
    switcher_t fifo_ths_int1_en{switcher_t::disable};

    /// @brief Маршрутизация прерывания переполнения FIFO на INT1.
    switcher_t fifo_full_int1_en{switcher_t::disable};

    /// @brief Маршрутизация прерывания готовности AGC на INT1.
    switcher_t ui_agc_rdy_int1_en{switcher_t::disable};

  private:
    void parse(
        icm42688p_reg_type reg)
    {
        ui_fsync_int1_en = parse_flag(reg, ui_fsync_int1_en_offset,
                                      switcher_t::enable, switcher_t::disable);
        pll_rdy_int1_en  = parse_flag(reg, pll_rdy_int1_en_offset,
                                      switcher_t::enable, switcher_t::disable);
        reset_done_int1_en =
            parse_flag(reg, reset_done_int1_en_offset, switcher_t::enable,
                       switcher_t::disable);
        ui_drdy_int1_en   = parse_flag(reg, ui_drdy_int1_en_offset,
                                       switcher_t::enable, switcher_t::disable);
        fifo_ths_int1_en  = parse_flag(reg, fifo_ths_int1_en_offset,
                                       switcher_t::enable, switcher_t::disable);
        fifo_full_int1_en = parse_flag(reg, fifo_full_int1_en_offset,
                                       switcher_t::enable, switcher_t::disable);
        ui_agc_rdy_int1_en =
            parse_flag(reg, ui_agc_rdy_int1_en_offset, switcher_t::enable,
                       switcher_t::disable);
    }
};

/// @brief Регистр SELF_TEST_CONFIG (bank 0, адрес 0x70) для управления
/// самотестированием сенсоров ICM-42688-P.
///
/// @details Содержит поля ACCEL_ST_POWER (бит 6), EN_AZ/AY/AX_ST (биты
///          5:3) и EN_GZ/GY/GX_ST (биты 2:0), см. раздел 14.57 даташита
///          DS-000347 rev 1.6. Бит ACCEL_ST_POWER должен быть установлен на
///          время самотестирования акселерометра и сброшен после его
///          завершения.
class icm42688p_self_test_config_reg
{
    static constexpr int accel_st_power_offset{6U};
    static constexpr int en_az_st_offset{5U};
    static constexpr int en_ay_st_offset{4U};
    static constexpr int en_ax_st_offset{3U};
    static constexpr int en_gz_st_offset{2U};
    static constexpr int en_gy_st_offset{1U};
    static constexpr int en_gx_st_offset{0U};

  public:
    /// @brief Адрес регистра SELF_TEST_CONFIG в памяти устройства (bank 0).
    static constexpr icm42688p_reg_type addr{0x70};

    explicit icm42688p_self_test_config_reg(
        icm42688p_reg_type value = icm42688p_reg_type{0})
    {
        parse(value);
    }

    /// @brief Оператор преобразования в сырое значение регистра.
    explicit operator icm42688p_reg_type() const
    {
        return static_cast<icm42688p_reg_type>(
            field_to_raw(accel_st_power, accel_st_power_offset)
            | field_to_raw(en_az_st, en_az_st_offset)
            | field_to_raw(en_ay_st, en_ay_st_offset)
            | field_to_raw(en_ax_st, en_ax_st_offset)
            | field_to_raw(en_gz_st, en_gz_st_offset)
            | field_to_raw(en_gy_st, en_gy_st_offset)
            | field_to_raw(en_gx_st, en_gx_st_offset));
    }

    /// @brief Оператор присваивания, обновляющий значение регистра.
    auto operator=(
        icm42688p_reg_type value) -> icm42688p_self_test_config_reg &
    {
        parse(value);
        return *this;
    }

    /// @brief Оператор сравнения двух экземпляров регистра на равенство.
    bool operator==(
        const icm42688p_self_test_config_reg &other) const
    {
        return static_cast<icm42688p_reg_type>(*this)
               == static_cast<icm42688p_reg_type>(other);
    }

    /// @brief Перечисление состояния бита самотестирования.
    enum struct switcher_t : std::uint8_t {
        disable = 0, ///< Самотестирование отключено.
        enable  = 1, ///< Самотестирование включено.
    };

    /// @brief Питание актуаторов самотестирования акселерометра.
    switcher_t accel_st_power{switcher_t::disable};

    /// @brief Включение самотестирования оси Z акселерометра.
    switcher_t en_az_st{switcher_t::disable};

    /// @brief Включение самотестирования оси Y акселерометра.
    switcher_t en_ay_st{switcher_t::disable};

    /// @brief Включение самотестирования оси X акселерометра.
    switcher_t en_ax_st{switcher_t::disable};

    /// @brief Включение самотестирования оси Z гироскопа.
    switcher_t en_gz_st{switcher_t::disable};

    /// @brief Включение самотестирования оси Y гироскопа.
    switcher_t en_gy_st{switcher_t::disable};

    /// @brief Включение самотестирования оси X гироскопа.
    switcher_t en_gx_st{switcher_t::disable};

  private:
    void parse(
        icm42688p_reg_type reg)
    {
        accel_st_power = parse_flag(reg, accel_st_power_offset,
                                    switcher_t::enable, switcher_t::disable);
        en_az_st       = parse_flag(reg, en_az_st_offset, switcher_t::enable,
                                    switcher_t::disable);
        en_ay_st       = parse_flag(reg, en_ay_st_offset, switcher_t::enable,
                                    switcher_t::disable);
        en_ax_st       = parse_flag(reg, en_ax_st_offset, switcher_t::enable,
                                    switcher_t::disable);
        en_gz_st       = parse_flag(reg, en_gz_st_offset, switcher_t::enable,
                                    switcher_t::disable);
        en_gy_st       = parse_flag(reg, en_gy_st_offset, switcher_t::enable,
                                    switcher_t::disable);
        en_gx_st       = parse_flag(reg, en_gx_st_offset, switcher_t::enable,
                                    switcher_t::disable);
    }
};

/// @brief Регистр WHO_AM_I (bank 0, адрес 0x75) для идентификации устройства
/// ICM-42688-P.
///
/// @details Содержит 8-битный идентификатор устройства, см. раздел 14.58
///          даташита DS-000347 rev 1.6. Значение после сброса — 0x47.
class icm42688p_who_am_i_reg
{
  public:
    /// @brief Адрес регистра WHO_AM_I в памяти устройства (bank 0).
    static constexpr icm42688p_reg_type addr{0x75};

    /// @brief Ожидаемое значение идентификатора устройства ICM-42688-P.
    static constexpr icm42688p_reg_type expected_value{0x47};

    explicit icm42688p_who_am_i_reg(
        icm42688p_reg_type value = icm42688p_reg_type{0})
    {
        parse(value);
    }

    /// @brief Оператор преобразования в сырое значение регистра.
    explicit operator icm42688p_reg_type() const { return who_am_i; }

    /// @brief Оператор присваивания, обновляющий значение регистра.
    auto operator=(
        icm42688p_reg_type value) -> icm42688p_who_am_i_reg &
    {
        parse(value);
        return *this;
    }

    /// @brief Оператор сравнения двух экземпляров регистра на равенство.
    bool operator==(
        const icm42688p_who_am_i_reg &other) const
    {
        return who_am_i == other.who_am_i;
    }

    /// @brief Значение идентификатора устройства.
    icm42688p_reg_type who_am_i{0};

  private:
    void parse(
        icm42688p_reg_type reg)
    {
        who_am_i = reg;
    }
};

/// @brief Регистр REG_BANK_SEL (адрес 0x76, доступен из всех банков) для
/// выбора банка регистров ICM-42688-P.
///
/// @details Содержит поле BANK_SEL (биты 2:0), см. раздел 14.59 даташита
///          DS-000347 rev 1.6.
class icm42688p_reg_bank_sel_reg
{
    static constexpr int bank_sel_offset{0U};

  public:
    /// @brief Адрес регистра REG_BANK_SEL в памяти устройства (все банки).
    static constexpr icm42688p_reg_type addr{0x76};

    explicit icm42688p_reg_bank_sel_reg(
        icm42688p_reg_type value = icm42688p_reg_type{0})
    {
        parse(value);
    }

    /// @brief Оператор преобразования в сырое значение регистра.
    explicit operator icm42688p_reg_type() const
    {
        return static_cast<icm42688p_reg_type>(
            field_to_raw(bank_sel, bank_sel_offset));
    }

    /// @brief Оператор присваивания, обновляющий значение регистра.
    auto operator=(
        icm42688p_reg_type value) -> icm42688p_reg_bank_sel_reg &
    {
        parse(value);
        return *this;
    }

    /// @brief Оператор сравнения двух экземпляров регистра на равенство.
    bool operator==(
        const icm42688p_reg_bank_sel_reg &other) const
    {
        return static_cast<icm42688p_reg_type>(*this)
               == static_cast<icm42688p_reg_type>(other);
    }

    /// @brief Перечисление банков регистров устройства.
    enum struct bank_t : std::uint8_t {
        bank_0 = 0, ///< Банк 0 (по умолчанию).
        bank_1 = 1, ///< Банк 1.
        bank_2 = 2, ///< Банк 2.
        bank_3 = 3, ///< Банк 3.
        bank_4 = 4, ///< Банк 4.
    };

    /// @brief Выбранный банк регистров.
    bank_t bank_sel{bank_t::bank_0};

  private:
    void parse(
        icm42688p_reg_type reg)
    {
        constexpr icm42688p_reg_type bank_mask{0x07};
        bank_sel = extract_field<bank_t>(reg, bank_sel_offset, bank_mask);
    }
};

/// @brief Регистр INTF_CONFIG6 (bank 1, адрес 0x7C) для настройки
/// последовательного интерфейса ICM-42688-P.
///
/// @details Содержит поле I3C_EN (бит 4), а также биты I3C_IBI_BYTE_EN
///          (бит 3), I3C_IBI_EN (бит 2), I3C_DDR_EN (бит 1) и I3C_SDR_EN
///          (бит 0), см. раздел 15.15 даташита DS-000347 rev 1.6. По заметке
///          12.3 даташита устройство стартует в режиме SPI, а для работы по
///          I2C необходимо установить I3C_EN = 1 и сбросить I3C_SDR_EN и
///          I3C_DDR_EN. Значение после сброса — 0x5F. Резервные биты 6:5
///          не моделируются: при разборе игнорируются, при сериализации
///          записываются как 0 (на кристалле после сброса они читаются как
///          0b10, поэтому смоделированные биты регистра читаются как 0x1F).
class icm42688p_intf_config6_reg
{
    static constexpr int asynctime0_dis_offset{7U};
    static constexpr int i3c_en_offset{4U};
    static constexpr int i3c_ibi_byte_en_offset{3U};
    static constexpr int i3c_ibi_en_offset{2U};
    static constexpr int i3c_ddr_en_offset{1U};
    static constexpr int i3c_sdr_en_offset{0U};

  public:
    /// @brief Адрес регистра INTF_CONFIG6 в памяти устройства (bank 1).
    static constexpr icm42688p_reg_type addr{0x7C};

    explicit icm42688p_intf_config6_reg(
        icm42688p_reg_type value = icm42688p_reg_type{0x5F})
    {
        parse(value);
    }

    /// @brief Оператор преобразования в сырое значение регистра.
    explicit operator icm42688p_reg_type() const
    {
        return static_cast<icm42688p_reg_type>(
            field_to_raw(asynctime0_dis, asynctime0_dis_offset)
            | field_to_raw(i3c_en, i3c_en_offset)
            | field_to_raw(i3c_ibi_byte_en, i3c_ibi_byte_en_offset)
            | field_to_raw(i3c_ibi_en, i3c_ibi_en_offset)
            | field_to_raw(i3c_ddr_en, i3c_ddr_en_offset)
            | field_to_raw(i3c_sdr_en, i3c_sdr_en_offset));
    }

    /// @brief Оператор присваивания, обновляющий значение регистра.
    auto operator=(
        icm42688p_reg_type value) -> icm42688p_intf_config6_reg &
    {
        parse(value);
        return *this;
    }

    /// @brief Оператор сравнения двух экземпляров регистра на равенство.
    bool operator==(
        const icm42688p_intf_config6_reg &other) const
    {
        return static_cast<icm42688p_reg_type>(*this)
               == static_cast<icm42688p_reg_type>(other);
    }

    /// @brief Перечисление состояния бита включения.
    enum struct switcher_t : std::uint8_t {
        disable = 0, ///< Отключено.
        enable  = 1, ///< Включено.
    };

    /// @brief Отключение контроля тайминга I3CSM Asynchronous Mode 0.
    switcher_t asynctime0_dis{switcher_t::disable};

    /// @brief Разрешение работы интерфейса (обязательно 1 для режима I2C).
    switcher_t i3c_en{switcher_t::enable};

    /// @brief Разрешение полезной нагрузки IBI в режиме I3CSM.
    switcher_t i3c_ibi_byte_en{switcher_t::enable};

    /// @brief Разрешение функции IBI в режиме I3CSM.
    switcher_t i3c_ibi_en{switcher_t::enable};

    /// @brief Разрешение режима DDR в режиме I3CSM.
    switcher_t i3c_ddr_en{switcher_t::enable};

    /// @brief Разрешение режима SDR в режиме I3CSM.
    switcher_t i3c_sdr_en{switcher_t::enable};

  private:
    void parse(
        icm42688p_reg_type reg)
    {
        asynctime0_dis  = parse_flag(reg, asynctime0_dis_offset,
                                     switcher_t::enable, switcher_t::disable);
        i3c_en          = parse_flag(reg, i3c_en_offset, switcher_t::enable,
                                     switcher_t::disable);
        i3c_ibi_byte_en = parse_flag(reg, i3c_ibi_byte_en_offset,
                                     switcher_t::enable, switcher_t::disable);
        i3c_ibi_en      = parse_flag(reg, i3c_ibi_en_offset, switcher_t::enable,
                                     switcher_t::disable);
        i3c_ddr_en      = parse_flag(reg, i3c_ddr_en_offset, switcher_t::enable,
                                     switcher_t::disable);
        i3c_sdr_en      = parse_flag(reg, i3c_sdr_en_offset, switcher_t::enable,
                                     switcher_t::disable);
    }
};

// NOLINTEND(*hicpp-signed-bitwise, *-member*)

} // namespace stv

#endif /* ICM42688P_REGS_HPP */
