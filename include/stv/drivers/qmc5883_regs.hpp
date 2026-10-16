/// @file qmc5883_regs.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.

#ifndef QMC5883_REGS_HPP
#define QMC5883_REGS_HPP

#include "qmc5883_types.hpp"
#include "stv/register_field.hpp"
#include <cstdint>

namespace stv {

// NOLINTBEGIN(*hicpp-signed-bitwise, *-member*)

/// @brief Класс для работы с регистром управления 1 QMC5883.
///
/// @details Регистр 0x09 (CTRL1) управляет коэффициентом переподписки (OSR),
/// диапазоном измерений (RNG), частотой дискретизации (ODR) и режимом работы.
class qmc5883_ctrl1_reg
{
    static constexpr int osr_offset{6U};
    static constexpr int rng_offset{4U};
    static constexpr int odr_offset{2U};
    static constexpr int mode_offset{0U};

  public:
    /// @brief Адрес регистра CTRL1 в памяти устройства.
    static constexpr qmc5883_reg_type addr{0x09};

    explicit qmc5883_ctrl1_reg(
        qmc5883_reg_type value = qmc5883_reg_type{0})
    { parse(value); }

    /// @brief Оператор преобразования в сырое значение регистра.
    explicit operator qmc5883_reg_type() const
    {
        return static_cast<qmc5883_reg_type>(
            field_to_raw(osr, osr_offset) | field_to_raw(rng, rng_offset)
            | field_to_raw(odr, odr_offset) | field_to_raw(mode, mode_offset));
    }

    /// @brief Оператор присваивания, обновляющий значение регистра.
    auto operator=(
        qmc5883_reg_type value) -> qmc5883_ctrl1_reg &
    {
        parse(value);
        return *this;
    }

    /// @brief Оператор сравнения двух экземпляров регистра на равенство.
    bool operator==(
        const qmc5883_ctrl1_reg &other) const
    {
        return static_cast<qmc5883_reg_type>(*this)
               == static_cast<qmc5883_reg_type>(other);
    }

    /// @brief Перечисление значений коэффициента переподписки (OSR).
    enum struct osr_t : std::uint8_t {
        oversampling_512 = 0, ///< Over sample Rate 512.
        oversampling_256 = 1, ///< Over sample Rate 256.
        oversampling_128 = 2, ///< Over sample Rate 128.
        oversampling_64  = 3, ///< Over sample Rate 64.
    };

    /// @brief Коэффициент переподписки (OSR).
    osr_t osr{osr_t::oversampling_512};

    /// @brief Перечисление значений полного диапазона (RNG).
    enum struct rng_t : std::uint8_t {
        full_scale_2g = 0, ///< Полный диапазон ±2 Гс.
        full_scale_8g = 1, ///< Полный диапазон ±8 Гс.
    };

    /// @brief Диапазон измерений.
    rng_t rng{rng_t::full_scale_2g};

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
        standby    = 0, ///< Режим ожидания.
        continuous = 1, ///< Непрерывный режим измерений.
        reserve_1  = 2,
        reserve_2  = 3,
    };

    /// @brief Режим работы.
    mode_t mode{mode_t::standby};

  private:
    void parse(
        qmc5883_reg_type reg)
    {
        osr  = extract_field<osr_t>(reg, osr_offset, 0x03U);
        rng  = extract_field<rng_t>(reg, rng_offset, 0x03U);
        odr  = extract_field<odr_t>(reg, odr_offset, 0x03U);
        mode = extract_field<mode_t>(reg, mode_offset, 0x03U);
    }
};

/// @brief Класс для работы с регистром управления 2 QMC5883.
///
/// @details Регистр 0x0A (CTRL2) управляет мягким сбросом, прокруткой
/// указателя и прерываниями.
class qmc5883_ctrl2_reg
{
    static constexpr int soft_reset_offset{7U};
    static constexpr int rol_pnt_offset{6U};
    static constexpr int int_enb_offset{0U};

  public:
    /// @brief Адрес регистра CTRL2 в памяти устройства.
    static constexpr qmc5883_reg_type addr{0x0A};

    explicit qmc5883_ctrl2_reg(
        qmc5883_reg_type value = qmc5883_reg_type{0})
    { parse(value); }

    /// @brief Оператор преобразования в сырое значение регистра.
    explicit operator qmc5883_reg_type() const
    {
        return static_cast<qmc5883_reg_type>(
            field_to_raw(soft_reset, soft_reset_offset)
            | field_to_raw(rol_pnt, rol_pnt_offset)
            | field_to_raw(int_enb, int_enb_offset));
    }

    /// @brief Оператор сравнения двух экземпляров регистра на равенство.
    bool operator==(
        const qmc5883_ctrl2_reg &other) const
    {
        return static_cast<qmc5883_reg_type>(*this)
               == static_cast<qmc5883_reg_type>(other);
    }

    /// @brief Перечисление значений для мягкого сброса.
    enum struct soft_reset_t : std::uint8_t {
        normal = 0, ///< Нормальный режим работы.
        enable = 1, ///< Включить мягкий сброс.
    };

    /// @brief Мягкий сброс.
    soft_reset_t soft_reset{soft_reset_t::normal};

    /// @brief Перечисление значений функции прокрутки указателя.
    enum struct rol_pnt_t : std::uint8_t {
        normal = 0, ///< Нормальный режим работы.
        enable = 1, ///< Включить функцию прокрутки указателя.
    };

    /// @brief Функция прокрутки указателя.
    rol_pnt_t rol_pnt{rol_pnt_t::enable};

    /// @brief Перечисление значений разрешения прерывания.
    enum struct int_enb_t : std::uint8_t {
        enable  = 0, ///< Включить прерывание.
        disable = 1, ///< Отключить прерывание.
    };

    /// @brief Разрешение прерывания.
    int_enb_t int_enb{int_enb_t::enable};

  private:
    void parse(
        qmc5883_reg_type reg)
    {
        soft_reset = parse_flag(reg, soft_reset_offset, soft_reset_t::enable,
                                soft_reset_t::normal);
        rol_pnt    = parse_flag(reg, rol_pnt_offset, rol_pnt_t::enable,
                                rol_pnt_t::normal);
        int_enb    = parse_flag(reg, int_enb_offset, int_enb_t::disable,
                                int_enb_t::enable);
    }
};

/// @brief Класс для работы с регистром периода сброса QMC5883.
///
/// @details Регистр 0x0B (SET/RESET Period) задаёт период сброса.
class qmc5883_set_reset_period_reg
{
  public:
    /// @brief Адрес регистра SET/RESET Period в памяти устройства.
    static constexpr qmc5883_reg_type addr{0x0B};

    /// @brief Конструктор с параметром периода.
    /// @param[in] period Период сброса (по умолчанию 0x01).
    explicit qmc5883_set_reset_period_reg(
        qmc5883_reg_type period = qmc5883_reg_type{0x01}):
        fbr{period}
    {
    }

    /// @brief Оператор присваивания.
    auto operator=(
        qmc5883_reg_type value) -> qmc5883_set_reset_period_reg &
    {
        fbr = value;
        return *this;
    }

    /// @brief Оператор преобразования в сырое значение регистра.
    explicit operator qmc5883_reg_type() const
    { return static_cast<qmc5883_reg_type>(fbr); }

    /// @brief Оператор сравнения двух экземпляров регистра на равенство.
    bool operator==(
        const qmc5883_set_reset_period_reg &other) const
    { return fbr == other.fbr; }

    /// @brief Поле, содержащее значение периода сброса.
    qmc5883_reg_type fbr;
};

/// @brief Класс для работы с регистром статуса QMC5883.
///
/// @details Регистр 0x06 (STATUS) содержит флаги состояния устройства.
class qmc5883_status_reg
{
    static constexpr int drdy_offset{0U};
    static constexpr int ovl_offset{1U};
    static constexpr int dor_offset{2U};

  public:
    /// @brief Адрес регистра STATUS в памяти устройства.
    static constexpr qmc5883_reg_type addr{0x06};

    explicit qmc5883_status_reg(
        qmc5883_reg_type value = qmc5883_reg_type{0})
    { parse(value); }

    /// @brief Оператор преобразования в сырое значение регистра.
    explicit operator qmc5883_reg_type() const
    {
        return static_cast<qmc5883_reg_type>(field_to_raw(dor, dor_offset)
                                             | field_to_raw(ovl, ovl_offset)
                                             | field_to_raw(drdy, drdy_offset));
    }

    /// @brief Оператор сравнения двух экземпляров регистра на равенство.
    bool operator==(
        const qmc5883_status_reg &other) const
    {
        return static_cast<qmc5883_reg_type>(*this)
               == static_cast<qmc5883_reg_type>(other);
    }

    /// @brief Оператор присваивания.
    auto operator=(
        qmc5883_reg_type value) -> qmc5883_status_reg &
    {
        parse(value);
        return *this;
    }

    /// @brief Перечисление значений для флага пропуска данных.
    enum struct dor_t : std::uint8_t {
        normal                   = 0, ///< Нормальное состояние.
        data_skipped_for_reading = 1, ///< Данные пропущены для чтения.
    };

    /// @brief Флаг пропуска данных.
    dor_t dor{dor_t::normal};

    /// @brief Перечисление значений для флага переполнения.
    enum struct ovl_t : std::uint8_t {
        normal        = 0, ///< Нормальное состояние.
        data_overflow = 1, ///< Данные переполнены.
    };

    /// @brief Флаг переполнения данных.
    ovl_t ovl{ovl_t::normal};

    /// @brief Перечисление значений для флага готовности данных.
    enum struct drdy_t : std::uint8_t {
        no_new_data       = 0, ///< Нет новых данных.
        new_data_is_ready = 1, ///< Новые данные готовы.
    };

    /// @brief Флаг готовности данных.
    drdy_t drdy{drdy_t::no_new_data};

  private:
    void parse(
        qmc5883_reg_type reg)
    {
        drdy = parse_flag(reg, drdy_offset, drdy_t::new_data_is_ready,
                          drdy_t::no_new_data);
        ovl  = parse_flag(reg, ovl_offset, ovl_t::data_overflow, ovl_t::normal);
        dor  = parse_flag(reg, dor_offset, dor_t::data_skipped_for_reading,
                          dor_t::normal);
    }
};

/// @brief Класс для работы с регистром идентификатора продукта QMC5883.
///
/// @details Регистр 0x0D (CHIP_ID) содержит идентификатор чипа.
class qmc5883_chip_id_reg
{
  public:
    /// @brief Адрес регистра CHIP_ID в памяти устройства.
    static constexpr qmc5883_reg_type addr{0x0D};

    /// @brief Ожидаемое значение идентификатора продукта.
    static constexpr qmc5883_reg_type expected_value{0xFF};

    explicit qmc5883_chip_id_reg(
        qmc5883_reg_type value = qmc5883_reg_type{0}):
        chip_id{value}
    {
    }

    /// @brief Оператор преобразования в сырое значение регистра.
    explicit operator qmc5883_reg_type() const { return chip_id; }

    /// @brief Оператор сравнения двух экземпляров регистра на равенство.
    bool operator==(
        const qmc5883_chip_id_reg &other) const
    { return chip_id == other.chip_id; }

    /// @brief Значение идентификатора продукта.
    qmc5883_reg_type chip_id{0};
};

// NOLINTEND(*hicpp-signed-bitwise, *-member*)

} // namespace stv

#endif /* QMC5883_REGS_HPP */
