/// @file mmc56xx_regs.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.

#ifndef MMC56XX_REGS_HPP
#define MMC56XX_REGS_HPP

#include "mmc56xx_types.hpp"
#include <cstdint>

namespace stv {

// NOLINTBEGIN(*hicpp-signed-bitwise, *-member*)

/// @brief Регистр статуса 1 для получения информации о состоянии датчика
/// MMC56xx.
class mmc56xx_status_reg
{
    static constexpr int meas_t_done_offset{7U};
    static constexpr int meas_m_done_offset{6U};
    static constexpr int sat_sensor_offset{5U};
    static constexpr int otp_read_done_offset{4U};

  public:
    /// @brief Адрес регистра STATUS в памяти устройства.
    static constexpr mmc56xx_reg_type addr{0x18};

    explicit mmc56xx_status_reg(
        mmc56xx_reg_type value = mmc56xx_reg_type{0})
    { parse(value); }

    /// @brief Оператор преобразования в сырое значение регистра.
    explicit operator mmc56xx_reg_type() const
    {
        return static_cast<mmc56xx_reg_type>(
            (static_cast<std::uint32_t>(meas_t_done)
             << static_cast<unsigned>(meas_t_done_offset))
            | (static_cast<std::uint32_t>(meas_m_done)
               << static_cast<unsigned>(meas_m_done_offset))
            | (static_cast<std::uint32_t>(sat_sensor)
               << static_cast<unsigned>(sat_sensor_offset))
            | (static_cast<std::uint32_t>(otp_read_done)
               << static_cast<unsigned>(otp_read_done_offset)));
    }

    /// @brief Оператор присваивания, обновляющий значение регистра.
    auto operator=(
        mmc56xx_reg_type value) -> mmc56xx_status_reg &
    {
        parse(value);
        return *this;
    }

    /// @brief Оператор сравнения двух экземпляров регистра на равенство.
    bool operator==(
        const mmc56xx_status_reg &other) const
    {
        return static_cast<mmc56xx_reg_type>(*this)
               == static_cast<mmc56xx_reg_type>(other);
    }

    /// @brief Перечисление состояния завершения измерения температуры.
    enum struct meas_t_done_t : std::uint8_t {
        not_ready = 0, ///< Измерение не завершено.
        is_done   = 1, ///< Измерение завершено.
    };

    /// @brief Указывает, что измерение температуры выполнено и данные готовы к
    /// чтению.
    meas_t_done_t meas_t_done{meas_t_done_t::not_ready};

    /// @brief Перечисление состояния завершения измерения магнитного поля.
    enum struct meas_m_done_t : std::uint8_t {
        not_ready = 0, ///< Измерение не завершено.
        is_done   = 1, ///< Измерение завершено.
    };

    /// @brief Указывает, что измерение магнитного поля выполнено и данные
    /// готовы к чтению.
    meas_m_done_t meas_m_done{meas_m_done_t::not_ready};

    /// @brief Перечисление состояния самотестирования сенсора.
    enum struct sat_sensor_t : std::uint8_t {
        pass_ = 0, ///< Самотестирование пройдено.
        fail_ = 1, ///< Самотестирование не пройдено.
    };

    /// @brief Указывает на результат самотестирования устройства.
    sat_sensor_t sat_sensor{sat_sensor_t::pass_};

    /// @brief Перечисление состояния чтения OTP памяти.
    enum struct otp_read_done_t : std::uint8_t {
        error   = 0, ///< Ошибка чтения OTP.
        success = 1, ///< Успешное чтение OTP.
    };

    /// @brief Указывает на успешное чтение OTP памяти.
    otp_read_done_t otp_read_done{otp_read_done_t::error};

  private:
    void parse(
        mmc56xx_reg_type reg)
    {
        meas_t_done = (reg & (1U << static_cast<unsigned>(meas_t_done_offset)))
                          ? meas_t_done_t::is_done
                          : meas_t_done_t::not_ready;
        meas_m_done = (reg & (1U << static_cast<unsigned>(meas_m_done_offset)))
                          ? meas_m_done_t::is_done
                          : meas_m_done_t::not_ready;
        sat_sensor  = (reg & (1U << static_cast<unsigned>(sat_sensor_offset)))
                          ? sat_sensor_t::fail_
                          : sat_sensor_t::pass_;
        otp_read_done =
            (reg & (1U << static_cast<unsigned>(otp_read_done_offset)))
                ? otp_read_done_t::success
                : otp_read_done_t::error;
    }
};

/// @brief Регистр частоты дискретизации (ODR) для настройки частоты измерений
/// MMC56xx.
class mmc56xx_odr_reg
{
  public:
    /// @brief Адрес регистра ODR в памяти устройства.
    static constexpr mmc56xx_reg_type addr{0x1A};

    /// @brief Конструктор с параметром ODR.
    /// @param[in] odr_val Значение ODR (по умолчанию 0x00).
    explicit mmc56xx_odr_reg(
        mmc56xx_reg_type odr_val = mmc56xx_reg_type{0x00}):
        odr{odr_val}
    {
    }

    /// @brief Оператор преобразования в сырое значение регистра.
    explicit operator mmc56xx_reg_type() const { return odr; }

    /// @brief Оператор присваивания.
    auto operator=(
        mmc56xx_reg_type value) -> mmc56xx_odr_reg &
    {
        odr = value;
        return *this;
    }

    /// @brief Оператор сравнения двух экземпляров регистра на равенство.
    bool operator==(
        const mmc56xx_odr_reg &other) const
    { return odr == other.odr; }

    /// @brief Значение частоты дискретизации.
    mmc56xx_reg_type odr{0x00};
};

/// @brief Внутренний управляющий регистр 0 для MMC56xx.
class mmc56xx_ctrl0_reg
{
    static constexpr int cmm_freq_en_offset{7U};
    static constexpr int auto_st_en_offset{6U};
    static constexpr int auto_sr_en_offset{5U};
    static constexpr int do_reset_offset{4U};
    static constexpr int do_set_offset{3U};
    static constexpr int take_meas_t_offset{1U};
    static constexpr int take_meas_m_offset{0U};

  public:
    /// @brief Адрес регистра CTRL0 в памяти устройства.
    static constexpr mmc56xx_reg_type addr{0x1B};

    explicit mmc56xx_ctrl0_reg(
        mmc56xx_reg_type value = mmc56xx_reg_type{0})
    { parse(value); }

    /// @brief Оператор преобразования в сырое значение регистра.
    explicit operator mmc56xx_reg_type() const
    {
        return static_cast<mmc56xx_reg_type>(
            (static_cast<std::uint32_t>(cmm_freq_en)
             << static_cast<unsigned>(cmm_freq_en_offset))
            | (static_cast<std::uint32_t>(auto_st_en)
               << static_cast<unsigned>(auto_st_en_offset))
            | (static_cast<std::uint32_t>(auto_sr_en)
               << static_cast<unsigned>(auto_sr_en_offset))
            | (static_cast<std::uint32_t>(do_reset)
               << static_cast<unsigned>(do_reset_offset))
            | (static_cast<std::uint32_t>(do_set)
               << static_cast<unsigned>(do_set_offset))
            | (static_cast<std::uint32_t>(take_meas_t)
               << static_cast<unsigned>(take_meas_t_offset))
            | (static_cast<std::uint32_t>(take_meas_m)
               << static_cast<unsigned>(take_meas_m_offset)));
    }

    /// @brief Оператор сравнения двух экземпляров регистра на равенство.
    bool operator==(
        const mmc56xx_ctrl0_reg &other) const
    {
        return static_cast<mmc56xx_reg_type>(*this)
               == static_cast<mmc56xx_reg_type>(other);
    }

    /// @brief Перечисление состояния включения частоты CMM.
    enum struct cmm_freq_en_t : std::uint8_t {
        reset = 0, ///< Сброс.
        set   = 1, ///< Установка.
    };

    /// @brief Запуск вычисления периода измерения в соответствии с ODR.
    cmm_freq_en_t cmm_freq_en{cmm_freq_en_t::reset};

    /// @brief Перечисление состояния включения автоматического
    /// самотестирования.
    enum struct auto_st_en_t : std::uint8_t {
        disable = 0, ///< Отключено.
        enable  = 1, ///< Включено.
    };

    /// @brief Автоматическое самотестирование.
    auto_st_en_t auto_st_en{auto_st_en_t::disable};

    /// @brief Перечисление состояния включения автоматического SET/RESET.
    enum struct auto_sr_en_t : std::uint8_t {
        reset = 0, ///< Сброс.
        set   = 1, ///< Установка.
    };

    /// @brief Автоматический SET/RESET.
    auto_sr_en_t auto_sr_en{auto_sr_en_t::reset};

    /// @brief Перечисление состояния выполнения сброса.
    enum struct do_reset_t : std::uint8_t {
        reset = 0, ///< Сброс.
        set   = 1, ///< Установка.
    };

    /// @brief Выполнить операцию RESET.
    do_reset_t do_reset{do_reset_t::reset};

    /// @brief Перечисление состояния выполнения SET операции.
    enum struct do_set_t : std::uint8_t {
        reset = 0, ///< Сброс.
        set   = 1, ///< Установка.
    };

    /// @brief Выполнить операцию SET.
    do_set_t do_set{do_set_t::reset};

    /// @brief Перечисление состояния измерения температуры.
    enum struct take_meas_t_t : std::uint8_t {
        disable = 0, ///< Отключено.
        enable  = 1, ///< Включено.
    };

    /// @brief Измерение температуры.
    take_meas_t_t take_meas_t{take_meas_t_t::disable};

    /// @brief Перечисление состояния измерения магнитного поля.
    enum struct take_meas_m_t : std::uint8_t {
        disable = 0, ///< Отключено.
        enable  = 1, ///< Включено.
    };

    /// @brief Измерение магнитного поля.
    take_meas_m_t take_meas_m{take_meas_m_t::disable};

  private:
    void parse(
        mmc56xx_reg_type reg)
    {
        cmm_freq_en = (reg & (1U << static_cast<unsigned>(cmm_freq_en_offset)))
                          ? cmm_freq_en_t::set
                          : cmm_freq_en_t::reset;
        auto_st_en  = (reg & (1U << static_cast<unsigned>(auto_st_en_offset)))
                          ? auto_st_en_t::enable
                          : auto_st_en_t::disable;
        auto_sr_en  = (reg & (1U << static_cast<unsigned>(auto_sr_en_offset)))
                          ? auto_sr_en_t::set
                          : auto_sr_en_t::reset;
        do_reset    = (reg & (1U << static_cast<unsigned>(do_reset_offset)))
                          ? do_reset_t::set
                          : do_reset_t::reset;
        do_set      = (reg & (1U << static_cast<unsigned>(do_set_offset)))
                          ? do_set_t::set
                          : do_set_t::reset;
        take_meas_t = (reg & (1U << static_cast<unsigned>(take_meas_t_offset)))
                          ? take_meas_t_t::enable
                          : take_meas_t_t::disable;
        take_meas_m = (reg & (1U << static_cast<unsigned>(take_meas_m_offset)))
                          ? take_meas_m_t::enable
                          : take_meas_m_t::disable;
    }
};

/// @brief Внутренний управляющий регистр 1 для MMC56xx.
class mmc56xx_ctrl1_reg
{
    static constexpr int sw_reset_offset{7U};
    static constexpr int st_enm_offset{6U};
    static constexpr int st_enp_offset{5U};
    static constexpr int z_inhibit_offset{4U};
    static constexpr int y_inhibit_offset{3U};
    static constexpr int x_inhibit_offset{2U};
    static constexpr int bw_offset{0U};

  public:
    /// @brief Адрес регистра CTRL1 в памяти устройства.
    static constexpr mmc56xx_reg_type addr{0x1C};

    explicit mmc56xx_ctrl1_reg(
        mmc56xx_reg_type value = mmc56xx_reg_type{0})
    { parse(value); }

    /// @brief Оператор преобразования в сырое значение регистра.
    explicit operator mmc56xx_reg_type() const
    {
        return static_cast<mmc56xx_reg_type>(
            (static_cast<std::uint32_t>(sw_reset)
             << static_cast<unsigned>(sw_reset_offset))
            | (static_cast<std::uint32_t>(st_enm)
               << static_cast<unsigned>(st_enm_offset))
            | (static_cast<std::uint32_t>(st_enp)
               << static_cast<unsigned>(st_enp_offset))
            | (static_cast<std::uint32_t>(z_inhibit)
               << static_cast<unsigned>(z_inhibit_offset))
            | (static_cast<std::uint32_t>(y_inhibit)
               << static_cast<unsigned>(y_inhibit_offset))
            | (static_cast<std::uint32_t>(x_inhibit)
               << static_cast<unsigned>(x_inhibit_offset))
            | (static_cast<std::uint32_t>(bw)
               << static_cast<unsigned>(bw_offset)));
    }

    /// @brief Оператор сравнения двух экземпляров регистра на равенство.
    bool operator==(
        const mmc56xx_ctrl1_reg &other) const
    {
        return static_cast<mmc56xx_reg_type>(*this)
               == static_cast<mmc56xx_reg_type>(other);
    }

    /// @brief Перечисление состояния программного сброса.
    enum struct sw_reset_t : std::uint8_t {
        disable = 0, ///< Отключено.
        enable  = 1, ///< Включено.
    };

    /// @brief Программный сброс.
    sw_reset_t sw_reset{sw_reset_t::disable};

    /// @brief Перечисление состояния включения самотестирования (отрицательная
    /// полярность).
    enum struct st_enm_t : std::uint8_t {
        disable = 0, ///< Отключено.
        enable  = 1, ///< Включено.
    };

    /// @brief Самотестирование отрицательной полярности.
    st_enm_t st_enm{st_enm_t::disable};

    /// @brief Перечисление состояния включения самотестирования (положительная
    /// полярность).
    enum struct st_enp_t : std::uint8_t {
        disable = 0, ///< Отключено.
        enable  = 1, ///< Включено.
    };

    /// @brief Самотестирование положительной полярности.
    st_enp_t st_enp{st_enp_t::disable};

    /// @brief Перечисление состояния подавления оси.
    enum struct axis_inhibit_t : std::uint8_t {
        enable  = 0, ///< Включено.
        disable = 1, ///< Отключено.
    };

    /// @brief Подавление оси Z.
    axis_inhibit_t z_inhibit{axis_inhibit_t::enable};

    /// @brief Подавление оси Y.
    axis_inhibit_t y_inhibit{axis_inhibit_t::enable};

    /// @brief Подавление оси X.
    axis_inhibit_t x_inhibit{axis_inhibit_t::enable};

    /// @brief Перечисление значений ширины полосы.
    enum struct bw_t : std::uint8_t {
        bw_6_6ms = 0, ///< 6.6 мс.
        bw_3_5ms = 1, ///< 3.5 мс.
        bw_2_0ms = 2, ///< 2.0 мс.
        bw_1_2ms = 3, ///< 1.2 мс.
    };

    /// @brief Ширина полосы.
    bw_t bw{bw_t::bw_6_6ms};

  private:
    void parse(
        mmc56xx_reg_type reg)
    {
        sw_reset  = (reg & (1U << static_cast<unsigned>(sw_reset_offset)))
                        ? sw_reset_t::enable
                        : sw_reset_t::disable;
        st_enm    = (reg & (1U << static_cast<unsigned>(st_enm_offset)))
                        ? st_enm_t::enable
                        : st_enm_t::disable;
        st_enp    = (reg & (1U << static_cast<unsigned>(st_enp_offset)))
                        ? st_enp_t::enable
                        : st_enp_t::disable;
        z_inhibit = (reg & (1U << static_cast<unsigned>(z_inhibit_offset)))
                        ? axis_inhibit_t::disable
                        : axis_inhibit_t::enable;
        y_inhibit = (reg & (1U << static_cast<unsigned>(y_inhibit_offset)))
                        ? axis_inhibit_t::disable
                        : axis_inhibit_t::enable;
        x_inhibit = (reg & (1U << static_cast<unsigned>(x_inhibit_offset)))
                        ? axis_inhibit_t::disable
                        : axis_inhibit_t::enable;
        constexpr mmc56xx_reg_type bw_mask{0x03};
        bw = static_cast<bw_t>(reg & bw_mask);
    }
};

/// @brief Внутренний управляющий регистр 2 для MMC56xx.
class mmc56xx_ctrl2_reg
{
    static constexpr int hpower_offset{7U};
    static constexpr int cmm_en_offset{4U};
    static constexpr int en_prd_set_offset{3U};
    static constexpr int prd_set_offset{0U};

  public:
    /// @brief Адрес регистра CTRL2 в памяти устройства.
    static constexpr mmc56xx_reg_type addr{0x1D};

    explicit mmc56xx_ctrl2_reg(
        mmc56xx_reg_type value = mmc56xx_reg_type{0})
    { parse(value); }

    /// @brief Оператор преобразования в сырое значение регистра.
    explicit operator mmc56xx_reg_type() const
    {
        return static_cast<mmc56xx_reg_type>(
            (static_cast<std::uint32_t>(hpower)
             << static_cast<unsigned>(hpower_offset))
            | (static_cast<std::uint32_t>(cmm_en)
               << static_cast<unsigned>(cmm_en_offset))
            | (static_cast<std::uint32_t>(en_prd_set)
               << static_cast<unsigned>(en_prd_set_offset))
            | (static_cast<std::uint32_t>(prd_set)
               << static_cast<unsigned>(prd_set_offset)));
    }

    /// @brief Оператор сравнения двух экземпляров регистра на равенство.
    bool operator==(
        const mmc56xx_ctrl2_reg &other) const
    {
        return static_cast<mmc56xx_reg_type>(*this)
               == static_cast<mmc56xx_reg_type>(other);
    }

    /// @brief Перечисление состояния высокой мощности.
    enum struct hpower_t : std::uint8_t {
        disable         = 0, ///< Отключено.
        achieve_1000_hz = 1, ///< Достижение 1000 Гц.
    };

    /// @brief Высокая мощность.
    hpower_t hpower{hpower_t::disable};

    /// @brief Перечисление состояния включения непрерывного режима.
    enum struct cmm_en_t : std::uint8_t {
        disable = 0, ///< Отключено.
        enable  = 1, ///< Включено.
    };

    /// @brief Непрерывный режим.
    cmm_en_t cmm_en{cmm_en_t::disable};

    /// @brief Перечисление состояния включения периодического SET.
    enum struct en_prd_set_t : std::uint8_t {
        disable = 0, ///< Отключено.
        enable  = 1, ///< Включено.
    };

    /// @brief Периодический SET.
    en_prd_set_t en_prd_set{en_prd_set_t::disable};

    /// @brief Перечисление периодов SET.
    enum struct prd_set_t : std::uint8_t {
        every_1    = 0, ///< Каждое 1 измерение.
        every_25   = 1, ///< Каждое 25 измерений.
        every_75   = 2, ///< Каждое 75 измерений.
        every_100  = 3, ///< Каждое 100 измерений.
        every_250  = 4, ///< Каждое 250 измерений.
        every_500  = 5, ///< Каждое 500 измерений.
        every_1000 = 6, ///< Каждое 1000 измерений.
        every_2000 = 7, ///< Каждое 2000 измерений.
    };

    /// @brief Период SET операций.
    prd_set_t prd_set{prd_set_t::every_1};

  private:
    void parse(
        mmc56xx_reg_type reg)
    {
        hpower     = (reg & (1U << static_cast<unsigned>(hpower_offset)))
                         ? hpower_t::achieve_1000_hz
                         : hpower_t::disable;
        cmm_en     = (reg & (1U << static_cast<unsigned>(cmm_en_offset)))
                         ? cmm_en_t::enable
                         : cmm_en_t::disable;
        en_prd_set = (reg & (1U << static_cast<unsigned>(en_prd_set_offset)))
                         ? en_prd_set_t::enable
                         : en_prd_set_t::disable;
        constexpr mmc56xx_reg_type prd_set_mask{0x07};
        prd_set = static_cast<prd_set_t>(reg & prd_set_mask);
    }
};

/// @brief Класс для работы с регистром идентификатора продукта MMC56xx.
class mmc56xx_product_id_reg
{
  public:
    /// @brief Адрес регистра PRODUCT_ID в памяти устройства.
    static constexpr mmc56xx_reg_type addr{0x39};

    /// @brief Ожидаемое значение идентификатора продукта для MMC5603NJ.
    static constexpr mmc56xx_reg_type expected_value_mmc5603nj{0x10};

    /// @brief Ожидаемое значение идентификатора продукта для MMC5616WA.
    static constexpr mmc56xx_reg_type expected_value_mmc5616wa{0x11};

    explicit mmc56xx_product_id_reg(
        mmc56xx_reg_type value = mmc56xx_reg_type{0}):
        product_id{value}
    {
    }

    /// @brief Оператор преобразования в сырое значение регистра.
    explicit operator mmc56xx_reg_type() const { return product_id; }

    /// @brief Оператор сравнения двух экземпляров регистра на равенство.
    bool operator==(
        const mmc56xx_product_id_reg &other) const
    { return product_id == other.product_id; }

    /// @brief Значение идентификатора продукта.
    mmc56xx_reg_type product_id{0};
};

// NOLINTEND(*hicpp-signed-bitwise, *-member*)

} // namespace stv

#endif /* MMC56XX_REGS_HPP */
