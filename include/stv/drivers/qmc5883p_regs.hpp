/// @file qmc5883p_regs.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.

#ifndef QMC5883P_REGS_HPP
#define QMC5883P_REGS_HPP

#include "qmc5883p_types.hpp"
#include "stv/register_field.hpp"
#include <cstdint>

namespace stv {

// NOLINTBEGIN(*hicpp-signed-bitwise, *-member*)

/// @brief Класс для работы с регистром идентификатора чипа QMC5883P.
///
/// @details Регистр 0x00 (CHIP_ID) содержит идентификатор чипа.
class qmc5883p_chip_id_reg
{
  public:
    /// @brief Адрес регистра CHIP_ID в памяти устройства.
    static constexpr qmc5883p_reg_type addr{0x00};

    /// @brief Ожидаемое значение идентификатора чипа.
    static constexpr qmc5883p_reg_type expected_value{0x80};

    explicit qmc5883p_chip_id_reg(
        qmc5883p_reg_type value = qmc5883p_reg_type{0}):
        chip_id{value}
    {
    }

    /// @brief Оператор преобразования в сырое значение регистра.
    explicit operator qmc5883p_reg_type() const { return chip_id; }

    /// @brief Оператор присваивания, обновляющий значение регистра.
    auto operator=(
        qmc5883p_reg_type value) -> qmc5883p_chip_id_reg &
    {
        chip_id = value;
        return *this;
    }

    /// @brief Оператор сравнения двух экземпляров регистра на равенство.
    bool operator==(
        const qmc5883p_chip_id_reg &other) const
    { return chip_id == other.chip_id; }

    /// @brief Значение идентификатора чипа.
    qmc5883p_reg_type chip_id{0};
};

/// @brief Класс для работы с регистром определения знаков осей QMC5883P.
///
/// @details Регистр 0x29 задаёт знаки осей X, Y и Z. Во всех примерах
/// инициализации из даташита (7.1 Normal Mode, 7.2 Continuous Mode,
/// 7.3 Self-test) первым шагом выполняется запись значения 0x06
/// («Define the sign for X Y and Z axis»). Регистр не входит в Register
/// Map (Table 14) и в даташите не описан как доступный для чтения.
class qmc5883p_axis_sign_reg
{
  public:
    /// @brief Адрес регистра знаков осей в памяти устройства.
    static constexpr qmc5883p_reg_type addr{0x29};

    /// @brief Значение из примеров инициализации даташита.
    static constexpr qmc5883p_reg_type default_value{0x06};

    explicit qmc5883p_axis_sign_reg(
        qmc5883p_reg_type value = default_value):
        sign{value}
    {
    }

    /// @brief Оператор преобразования в сырое значение регистра.
    explicit operator qmc5883p_reg_type() const { return sign; }

    /// @brief Значение регистра знаков осей.
    qmc5883p_reg_type sign{default_value};
};

/// @brief Класс для работы с регистром статуса QMC5883P.
///
/// @details Регистр 0x09 (STATUS) содержит флаги состояния устройства.
class qmc5883p_status_reg
{
    static constexpr int drdy_offset{0U};
    static constexpr int ovfl_offset{1U};

  public:
    /// @brief Адрес регистра STATUS в памяти устройства.
    static constexpr qmc5883p_reg_type addr{0x09};

    explicit qmc5883p_status_reg(
        qmc5883p_reg_type value = qmc5883p_reg_type{0})
    { parse(value); }

    /// @brief Оператор преобразования в сырое значение регистра.
    explicit operator qmc5883p_reg_type() const
    {
        return static_cast<qmc5883p_reg_type>(
            field_to_raw(ovfl, ovfl_offset) | field_to_raw(drdy, drdy_offset));
    }

    /// @brief Оператор присваивания, обновляющий значение регистра.
    auto operator=(
        qmc5883p_reg_type value) -> qmc5883p_status_reg &
    {
        parse(value);
        return *this;
    }

    /// @brief Оператор сравнения двух экземпляров регистра на равенство.
    bool operator==(
        const qmc5883p_status_reg &other) const
    {
        return static_cast<qmc5883p_reg_type>(*this)
               == static_cast<qmc5883p_reg_type>(other);
    }

    /// @brief Перечисление значений для флага переполнения данных.
    enum struct ovfl_t : std::uint8_t {
        normal        = 0, ///< Переполнения нет.
        data_overflow = 1, ///< Данные переполнены.
    };

    /// @brief Флаг переполнения данных.
    ovfl_t ovfl{ovfl_t::normal};

    /// @brief Перечисление значений для флага готовности данных.
    enum struct drdy_t : std::uint8_t {
        no_new_data       = 0, ///< Новых данных нет.
        new_data_is_ready = 1, ///< Новые данные готовы.
    };

    /// @brief Флаг готовности данных.
    drdy_t drdy{drdy_t::no_new_data};

  private:
    void parse(
        qmc5883p_reg_type reg)
    {
        drdy = parse_flag(reg, drdy_offset, drdy_t::new_data_is_ready,
                          drdy_t::no_new_data);
        ovfl =
            parse_flag(reg, ovfl_offset, ovfl_t::data_overflow, ovfl_t::normal);
    }
};

/// @brief Класс для работы с регистром управления 1 QMC5883P.
///
/// @details Регистр 0x0A (CTRL1) управляет режимом работы, частотой
/// дискретизации (ODR) и коэффициентами переподписки (OSR1, OSR2).
class qmc5883p_ctrl1_reg
{
    static constexpr int osr2_offset{6U};
    static constexpr int osr1_offset{4U};
    static constexpr int odr_offset{2U};
    static constexpr int mode_offset{0U};

  public:
    /// @brief Адрес регистра CTRL1 в памяти устройства.
    static constexpr qmc5883p_reg_type addr{0x0A};

    explicit qmc5883p_ctrl1_reg(
        qmc5883p_reg_type value = qmc5883p_reg_type{0})
    { parse(value); }

    /// @brief Оператор преобразования в сырое значение регистра.
    explicit operator qmc5883p_reg_type() const
    {
        return static_cast<qmc5883p_reg_type>(
            field_to_raw(osr2, osr2_offset) | field_to_raw(osr1, osr1_offset)
            | field_to_raw(odr, odr_offset) | field_to_raw(mode, mode_offset));
    }

    /// @brief Оператор присваивания, обновляющий значение регистра.
    auto operator=(
        qmc5883p_reg_type value) -> qmc5883p_ctrl1_reg &
    {
        parse(value);
        return *this;
    }

    /// @brief Оператор сравнения двух экземпляров регистра на равенство.
    bool operator==(
        const qmc5883p_ctrl1_reg &other) const
    {
        return static_cast<qmc5883p_reg_type>(*this)
               == static_cast<qmc5883p_reg_type>(other);
    }

    /// @brief Перечисление значений второго коэффициента переподписки (OSR2).
    enum struct osr2_t : std::uint8_t {
        oversampling_1 = 0, ///< Коэффициент переподписки 1.
        oversampling_2 = 1, ///< Коэффициент переподписки 2.
        oversampling_4 = 2, ///< Коэффициент переподписки 4.
        oversampling_8 = 3, ///< Коэффициент переподписки 8.
    };

    /// @brief Второй коэффициент переподписки (OSR2).
    osr2_t osr2{osr2_t::oversampling_1};

    /// @brief Перечисление значений первого коэффициента переподписки (OSR1).
    enum struct osr1_t : std::uint8_t {
        oversampling_8 = 0, ///< Коэффициент переподписки 8.
        oversampling_4 = 1, ///< Коэффициент переподписки 4.
        oversampling_2 = 2, ///< Коэффициент переподписки 2.
        oversampling_1 = 3, ///< Коэффициент переподписки 1.
    };

    /// @brief Первый коэффициент переподписки (OSR1).
    osr1_t osr1{osr1_t::oversampling_8};

    /// @brief Перечисление значений частоты дискретизации (ODR).
    enum struct odr_t : std::uint8_t {
        output_data_rate_10hz  = 0, ///< Частота дискретизации 10 Гц.
        output_data_rate_50hz  = 1, ///< Частота дискретизации 50 Гц.
        output_data_rate_100hz = 2, ///< Частота дискретизации 100 Гц.
        output_data_rate_200hz = 3, ///< Частота дискретизации 200 Гц.
    };

    /// @brief Частота дискретизации.
    odr_t odr{odr_t::output_data_rate_10hz};

    /// @brief Перечисление значений режима работы.
    enum struct mode_t : std::uint8_t {
        suspend    = 0, ///< Режим ожидания.
        normal     = 1, ///< Нормальный режим.
        single     = 2, ///< Одиночный режим измерения.
        continuous = 3, ///< Непрерывный режим.
    };

    /// @brief Режим работы.
    mode_t mode{mode_t::suspend};

  private:
    void parse(
        qmc5883p_reg_type reg)
    {
        osr2 = extract_field<osr2_t>(reg, osr2_offset, 0x03U);
        osr1 = extract_field<osr1_t>(reg, osr1_offset, 0x03U);
        odr  = extract_field<odr_t>(reg, odr_offset, 0x03U);
        mode = extract_field<mode_t>(reg, mode_offset, 0x03U);
    }
};

/// @brief Класс для работы с регистром управления 2 QMC5883P.
///
/// @details Регистр 0x0B (CTRL2) управляет мягким сбросом, самотестированием,
/// диапазоном измерений (RNG) и режимом установки/сброса (SET/RESET).
class qmc5883p_ctrl2_reg
{
    static constexpr int soft_reset_offset{7U};
    static constexpr int self_test_offset{6U};
    static constexpr int rng_offset{2U};
    static constexpr int set_reset_mode_offset{0U};

  public:
    /// @brief Адрес регистра CTRL2 в памяти устройства.
    static constexpr qmc5883p_reg_type addr{0x0B};

    explicit qmc5883p_ctrl2_reg(
        qmc5883p_reg_type value = qmc5883p_reg_type{0})
    { parse(value); }

    /// @brief Оператор преобразования в сырое значение регистра.
    explicit operator qmc5883p_reg_type() const
    {
        return static_cast<qmc5883p_reg_type>(
            field_to_raw(soft_reset, soft_reset_offset)
            | field_to_raw(self_test, self_test_offset)
            | field_to_raw(rng, rng_offset)
            | field_to_raw(set_reset_mode, set_reset_mode_offset));
    }

    /// @brief Оператор присваивания, обновляющий значение регистра.
    auto operator=(
        qmc5883p_reg_type value) -> qmc5883p_ctrl2_reg &
    {
        parse(value);
        return *this;
    }

    /// @brief Оператор сравнения двух экземпляров регистра на равенство.
    bool operator==(
        const qmc5883p_ctrl2_reg &other) const
    {
        return static_cast<qmc5883p_reg_type>(*this)
               == static_cast<qmc5883p_reg_type>(other);
    }

    /// @brief Перечисление значений для мягкого сброса.
    enum struct soft_reset_t : std::uint8_t {
        normal = 0, ///< Нормальный режим работы.
        enable = 1, ///< Включить мягкий сброс.
    };

    /// @brief Мягкий сброс.
    soft_reset_t soft_reset{soft_reset_t::normal};

    /// @brief Перечисление значений для самотестирования.
    enum struct self_test_t : std::uint8_t {
        normal = 0, ///< Нормальный режим работы.
        enable = 1, ///< Включить самотестирование.
    };

    /// @brief Самотестирование.
    self_test_t self_test{self_test_t::normal};

    /// @brief Перечисление значений полного диапазона (RNG).
    ///
    /// @note Значения соответствуют даташиту QMC5883P (Table 18):
    ///       00 = ±30 Гс, 01 = ±12 Гс, 10 = ±8 Гс, 11 = ±2 Гс.
    enum struct rng_t : std::uint8_t {
        full_scale_30g = 0, ///< Полный диапазон ±30 Гс.
        full_scale_12g = 1, ///< Полный диапазон ±12 Гс.
        full_scale_8g  = 2, ///< Полный диапазон ±8 Гс.
        full_scale_2g  = 3, ///< Полный диапазон ±2 Гс.
    };

    /// @brief Диапазон измерений.
    rng_t rng{rng_t::full_scale_30g};

    /// @brief Перечисление значений режима установки/сброса.
    ///
    /// @note Значения соответствуют даташиту QMC5883P (Table 18):
    ///       00 = Set and reset on, 01 = Set only on, 10/11 = Set and reset
    ///       off.
    enum struct set_reset_mode_t : std::uint8_t {
        set_and_reset_on  = 0, ///< Установка и сброс включены.
        set_only_on       = 1, ///< Только установка включена.
        set_and_reset_off = 2, ///< Установка и сброс отключены.
    };

    /// @brief Режим установки/сброса.
    set_reset_mode_t set_reset_mode{set_reset_mode_t::set_and_reset_on};

  private:
    void parse(
        qmc5883p_reg_type reg)
    {
        soft_reset = parse_flag(reg, soft_reset_offset, soft_reset_t::enable,
                                soft_reset_t::normal);
        self_test  = parse_flag(reg, self_test_offset, self_test_t::enable,
                                self_test_t::normal);
        rng        = extract_field<rng_t>(reg, rng_offset, 0x03U);
        set_reset_mode =
            extract_field<set_reset_mode_t>(reg, set_reset_mode_offset, 0x03U);
    }
};

// NOLINTEND(*hicpp-signed-bitwise, *-member*)

} // namespace stv

#endif /* QMC5883P_REGS_HPP */
