/// @file mmc3630kj_regs.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.

#ifndef MMC3630KJ_REGS_HPP
#define MMC3630KJ_REGS_HPP

#include "mmc3630kj_types.hpp"
#include <cstdint>

namespace stv {

// NOLINTBEGIN(*hicpp-signed-bitwise, *-member*)

/// @brief Регистр статуса устройства MMC3630KJ (адрес 0x07).
///
/// @details Содержит флаги состояния устройства: готовность измерений
/// магнитного поля и температуры, статус charge pump, обнаружение движения,
/// статус чтения OTP. Все флаги сбрасываются записью логической 1.
///
/// @note Перед чтением выходных данных необходимо проверить флаг
/// `meas_m_done` — он указывает, что измерение завершено и данные готовы.
///
/// Регистр STATUS (0x07):
/// | Бит | Имя | Описание |
/// |-----|-----|----------|
/// | 7–5 | Reserved | Зарезервировано |
/// | 4 | OTP_Rd_Done | Успешное чтение OTP |
/// | 3 | Pump_On | Статус charge pump |
/// | 2 | Motion_Detected | Обнаружение движения |
/// | 1 | Meas_T_Done | Измерение температуры завершено |
/// | 0 | Meas_M_Done | Измерение магнитного поля завершено |
class mmc3630kj_status_reg
{
    static constexpr int otp_rd_done_offset{4U};
    static constexpr int pump_on_offset{3U};
    static constexpr int motion_detect_offset{2U};
    static constexpr int meas_t_done_offset{1U};
    static constexpr int meas_m_done_offset{0U};

  public:
    /// @brief Адрес регистра STATUS в памяти устройства.
    static constexpr mmc3630kj_reg_type addr{0x07};

    explicit mmc3630kj_status_reg(
        mmc3630kj_reg_type value = mmc3630kj_reg_type{0})
    { parse(value); }

    /// @brief Оператор преобразования в сырое значение регистра.
    explicit operator mmc3630kj_reg_type() const
    {
        return static_cast<mmc3630kj_reg_type>(
            (static_cast<std::uint8_t>(otp_rd_done) << otp_rd_done_offset)
            | (static_cast<std::uint8_t>(pump_on) << pump_on_offset)
            | (static_cast<std::uint8_t>(motion_detect) << motion_detect_offset)
            | (static_cast<std::uint8_t>(meas_t_done) << meas_t_done_offset)
            | (static_cast<std::uint8_t>(meas_m_done) << meas_m_done_offset));
    }

    /// @brief Оператор сравнения двух экземпляров регистра на равенство.
    bool operator==(
        const mmc3630kj_status_reg &other) const
    {
        return static_cast<mmc3630kj_reg_type>(*this)
               == static_cast<mmc3630kj_reg_type>(other);
    }

    /// @brief Перечисление состояний готовности чтения OTP.
    enum struct otp_rd_done_t : std::uint8_t {
        not_able_to_read = 0, ///< OTP не прочитано.
        able_to_read     = 1  ///< OTP успешно прочитано.
    };

    /// @brief Указывает, смог ли чип успешно прочитать свою OTP память.
    ///
    /// @details Флаг устанавливается при успешном чтении OTP памяти чипом
    /// в процессе старта. Если флаг сброшен — OTP данные недоступны.
    otp_rd_done_t otp_rd_done{otp_rd_done_t::not_able_to_read};

    /// @brief Перечисление состояний зарядного насоса.
    enum struct pump_on_t : std::uint8_t {
        charge_pump_complete = 0, ///< Зарядный насос завершил работу.
        charge_pump_active   = 1  ///< Зарядный насос активен.
    };

    /// @brief Указывает состояние зарядного насоса.
    ///
    /// @details После команды `Refill Cap` charge pump запускается и
    /// этот флаг устанавливается в `charge_pump_active`. Когда конденсатор
    /// на выводе VCAP достигает целевого напряжения, charge pump
    /// отключается и флаг сбрасывается в `charge_pump_complete`.
    pump_on_t pump_on{pump_on_t::charge_pump_complete};

    /// @brief Перечисление состояний обнаружения движения.
    enum struct motion_detected_t : std::uint8_t {
        no_motion     = 0, ///< Движение не обнаружено.
        motion_detect = 1, ///< Движение обнаружено.
    };

    /// @brief Указывает, обнаружено ли движение.
    ///
    /// @details Устанавливается в `motion_detect`, когда детектор движения
    /// обнаруживает изменение магнитного поля, превышающее пороговое
    /// значение, заданное в регистрах X/Y/Z Threshold (0x0B–0x0D).
    /// Запись 1 очищает флаг.
    motion_detected_t motion_detect{motion_detected_t::no_motion};

    /// @brief Перечисление состояний завершения измерения температуры.
    enum struct meas_t_done_t : std::uint8_t {
        not_ready = 0, ///< Измерение не завершено.
        finished  = 1, ///< Измерение завершено.
    };

    /// @brief Указывает, завершено ли измерение температуры.
    ///
    /// @details Сбрасывается в `not_ready` при новой команде измерения
    /// температуры (установка `tm_t` в CTRL0). Когда измерение завершено,
    /// флаг устанавливается в `finished` и остаётся в этом состоянии до
    /// следующего измерения. Запись 1 очищает флаг и соответствующее
    /// прерывание.
    meas_t_done_t meas_t_done{meas_t_done_t::not_ready};

    /// @brief Перечисление состояний завершения измерения магнитного поля.
    enum struct meas_m_done_t : std::uint8_t {
        not_ready = 0, ///< Измерение не завершено.
        finished  = 1, ///< Измерение завершено.
    };

    /// @brief Указывает, завершено ли измерение магнитного поля.
    ///
    /// @details Сбрасывается в `not_ready` при новой команде измерения
    /// магнитного поля (установка `tm_m` в CTRL0). Когда измерение
    /// завершено, флаг устанавливается в `finished` и остаётся в этом
    /// состоянии до следующего измерения. Запись 1 очищает флаг и
    /// соответствующее прерывание. Необходимо проверять этот флаг перед
    /// чтением выходных данных Xout/Yout/Zout.
    meas_m_done_t meas_m_done{meas_m_done_t::not_ready};

  private:
    void parse(
        mmc3630kj_reg_type reg)
    {
        otp_rd_done   = (reg & (1 << otp_rd_done_offset))
                            ? otp_rd_done_t::able_to_read
                            : otp_rd_done_t::not_able_to_read;
        pump_on       = (reg & (1 << pump_on_offset))
                            ? pump_on_t::charge_pump_active
                            : pump_on_t::charge_pump_complete;
        motion_detect = (reg & (1 << motion_detect_offset))
                            ? motion_detected_t::motion_detect
                            : motion_detected_t::no_motion;
        meas_t_done   = (reg & (1 << meas_t_done_offset))
                            ? meas_t_done_t::finished
                            : meas_t_done_t::not_ready;
        meas_m_done   = (reg & (1 << meas_m_done_offset))
                            ? meas_m_done_t::finished
                            : meas_m_done_t::not_ready;
    }
};

/// @brief Управляющий регистр 0 MMC3630KJ (адрес 0x08).
///
/// @details Регистр CTRL0 управляет измерениями магнитного поля и
/// температуры, SET/RESET катушкой, перезарядкой конденсатора на VCAP,
/// детектором движения и повторным чтением OTP. Все биты этого регистра
/// write-only (только для записи).
///
/// Регистр CTRL0 (0x08):
/// | Бит | Имя | Описание |
/// |-----|-----|----------|
/// | 7 | Test_Pin_Sel | Заводская настройка |
/// | 6 | OTP_Read | Повторное чтение OTP |
/// | 5 | Refill_Cap | Перезарядка конденсатора |
/// | 4 | Reset | RESET катушки |
/// | 3 | Set | SET катушки |
/// | 2 | Start_MDT | Запуск детектора движения |
/// | 1 | TM_T | Измерение температуры |
/// | 0 | TM_M | Измерение магнитного поля |
///
/// @note Перед операцией SET/RESET при single supply необходимо сначала
/// выполнить `Refill Cap` и подождать не менее 50 мс.
/// При dual supply бит `Refill_Cap` зарезервирован — писать 0.
class mmc3630kj_ctrl0_reg
{
    static constexpr int otr_read_offset{6U};
    static constexpr int refill_cap_offset{5U};
    static constexpr int reset_offset{4U};
    static constexpr int set_offset{3U};
    static constexpr int start_mdt_offset{2U};
    static constexpr int tm_t_offset{1U};
    static constexpr int tm_m_offset{0U};

  public:
    /// @brief Адрес регистра CTRL0 в памяти устройства.
    static constexpr mmc3630kj_reg_type addr{0x08};

    explicit mmc3630kj_ctrl0_reg(
        mmc3630kj_reg_type value = mmc3630kj_reg_type{0})
    { parse(value); }

    /// @brief Оператор преобразования в сырое значение регистра.
    explicit operator mmc3630kj_reg_type() const
    {
        return static_cast<mmc3630kj_reg_type>(
            (static_cast<std::uint8_t>(otr_read) << otr_read_offset)
            | (static_cast<std::uint8_t>(refill_cap) << refill_cap_offset)
            | (static_cast<std::uint8_t>(reset) << reset_offset)
            | (static_cast<std::uint8_t>(set) << set_offset)
            | (static_cast<std::uint8_t>(start_mdt) << start_mdt_offset)
            | (static_cast<std::uint8_t>(tm_t) << tm_t_offset)
            | (static_cast<std::uint8_t>(tm_m) << tm_m_offset));
    }

    /// @brief Оператор сравнения двух экземпляров регистра на равенство.
    bool operator==(
        const mmc3630kj_ctrl0_reg &other) const
    {
        return static_cast<mmc3630kj_reg_type>(*this)
               == static_cast<mmc3630kj_reg_type>(other);
    }

    /// @brief Перечисление состояний чтения OTP.
    enum struct otp_read_t : std::uint8_t {
        reset = 0, ///< Сброс.
        let_device_to_read_otp_data_again =
            1,     ///< Позволить устройству снова прочитать OTP данные.
    };

    /// @brief Повторное чтение OTP данных.
    ///
    /// @details Установка в 1 позволяет устройству снова прочитать OTP
    /// данные. Используется для восстановления заводских калибровочных
    /// данных. Бит автоматически сбрасывается в 0 после выполнения.
    otp_read_t otr_read{otp_read_t::reset};

    /// @brief Перечисление состояний перезарядки конденсатора.
    enum struct refill_cap_t : std::uint8_t {
        reset                     = 0, ///< Сброс.
        request_recharge_capacity = 1, ///< Запросить перезарядку конденсатора.
    };

    /// @brief Перезарядка конденсатора на выводе VCAP.
    ///
    /// @details Установка в 1 инициирует перезарядку конденсатора на
    /// выводе VCAP. Требуется перед операциями SET/RESET при использовании
    /// single supply (внутренний charge pump). После команды необходимо
    /// ожидать завершения charge pump (проверка флага `pump_on` в STATUS).
    ///
    /// @note При dual supply (внешнее питание VCAP) этот бит зарезервирован
    /// — необходимо писать 0.
    ///
    /// @note Время подготовки: single supply — 50 мс, dual supply — 1 мс.
    refill_cap_t refill_cap{refill_cap_t::reset};

    /// @brief Перечисление состояний сброса катушки.
    enum struct reset_t : std::uint8_t {
        disable = 0, ///< Отключено.
        enable  = 1, ///< Включено.
    };

    /// @brief RESET катушки.
    ///
    /// @details Установка в 1 пропускает большой ток через SET/RESET coil
    /// в направлении, противоположном SET (180°). Сбрасывает намагниченность
    /// AMR-сенсоров. Используется в паре с SET для устранения температурного
    /// дрейфа Null Field Output и остаточной намагниченности от сильных
    /// внешних полей (>36 Г).
    ///
    /// @note Минимальный интервал между SET и RESET (t_SR) — 1 мс.
    reset_t reset{reset_t::disable};

    /// @brief Перечисление состояний установки катушки.
    enum struct set_t : std::uint8_t {
        disable = 0, ///< Отключено.
        enable  = 1, ///< Включено.
    };

    /// @brief SET катушки.
    ///
    /// @details Установка в 1 пропускает большой ток через SET/RESET coil
    /// в одном направлении. Устанавливает намагниченность AMR-сенсоров
    /// (тонкоплёночных резисторов из пермаллоя в конфигурации Уитстона)
    /// в направлении SET поля. Устраняет температурный дрейф Null Field
    /// Output и очищает остаточную намагниченность от сильных внешних
    /// полей (>36 Г).
    ///
    /// @note Минимальный интервал между SET и RESET (t_SR) — 1 мс.
    set_t set{set_t::disable};

    /// @brief Перечисление состояний запуска детектора движения.
    enum struct start_mdt_t : std::uint8_t {
        disable_or_motion_is_detect = 0, ///< Отключено или движение обнаружено.
        enable                      = 1, ///< Включено.
    };

    /// @brief Запуск детектора движения.
    ///
    /// @details Установка в 1 запускает детектор движения. Устройство
    /// выполняет начальное измерение магнитного поля и использует его
    /// как baseline (базовое значение). Затем измерения выполняются
    /// периодически с частотой, заданной полем `cm_freq` в CTRL2 (0x0A),
    /// и новые данные сравниваются с baseline. Если разница превышает
    /// пороговое значение, заданное в регистрах X/Y/Z Threshold
    /// (0x0B–0x0D, шаг 4 мГ, максимум 1 Г), флаг `motion_detect` в
    /// STATUS (0x07) устанавливается в 1, а бит `start_mdt` сбрасывается
    /// в 0.
    ///
    /// @note Детектор движения не работает, если `cm_freq` в CTRL2
    /// установлен в `continuous_mode_is_off` (0000).
    start_mdt_t start_mdt{start_mdt_t::disable_or_motion_is_detect};

    /// @brief Перечисление состояний измерения температуры.
    enum struct tm_t_t : std::uint8_t {
        reset                = 0, ///< Сброс.
        initiate_measurement = 1, ///< Инициировать измерение.
    };

    /// @brief Измерение температуры.
    ///
    /// @details Установка в 1 инициирует измерение температуры.
    /// Бит автоматически сбрасывается в 0. Работает совместно с `tm_m`.
    /// Результат измерения доступен в регистре Temperature (0x06).
    /// Диапазон: -75°C … +125°C, разрешение ~0.8°C/LSB, значение 0x00
    /// соответствует -75°C.
    tm_t_t tm_t{tm_t_t::reset};

    /// @brief Перечисление состояний измерения магнитного поля.
    enum struct tm_m_t : std::uint8_t {
        reset                = 0, ///< Сброс.
        initiate_measurement = 1, ///< Инициировать измерение.
    };

    /// @brief Измерение магнитного поля.
    ///
    /// @details Установка в 1 инициирует измерение магнитного поля по
    /// всем трём осям (X, Y, Z). Измерения выполняются последовательно
    /// с задержкой 1/3 от Measurement Time между осями. Бит автоматически
    /// сбрасывается в 0. Результат доступен в регистрах Xout/Yout/Zout
    /// (0x00–0x05) после установки флага `meas_m_done` в STATUS.
    tm_m_t tm_m{tm_m_t::reset};

  private:
    void parse(
        mmc3630kj_reg_type reg)
    {
        otr_read   = (reg & (1 << otr_read_offset))
                         ? otp_read_t::let_device_to_read_otp_data_again
                         : otp_read_t::reset;
        refill_cap = (reg & (1 << refill_cap_offset))
                         ? refill_cap_t::request_recharge_capacity
                         : refill_cap_t::reset;
        reset =
            (reg & (1 << reset_offset)) ? reset_t::enable : reset_t::disable;
        set       = (reg & (1 << set_offset)) ? set_t::enable : set_t::disable;
        start_mdt = (reg & (1 << start_mdt_offset))
                        ? start_mdt_t::enable
                        : start_mdt_t::disable_or_motion_is_detect;
        tm_t      = (reg & (1 << tm_t_offset)) ? tm_t_t::initiate_measurement
                                               : tm_t_t::reset;
        tm_m      = (reg & (1 << tm_m_offset)) ? tm_m_t::initiate_measurement
                                               : tm_m_t::reset;
    }
};

/// @brief Управляющий регистр 1 MMC3630KJ (адрес 0x09).
///
/// @details Регистр CTRL1 управляет программным сбросом, отключением
/// отдельных каналов (X, Y, Z) и частотой дискретизации (Bandwidth).
///
/// Регистр CTRL1 (0x09):
/// | Бит | Имя | Описание |
/// |-----|-----|----------|
/// | 7 | SW_RST | Программный сброс |
/// | 6–5 | OTP_BL_SEL | Заводская настройка |
/// | 4 | Z-inhibit | Отключение канала Z |
/// | 3 | Y-inhibit | Отключение канала Y |
/// | 2 | X-inhibit | Отключение канала X |
/// | 1–0 | BW[1:0] | Частота дискретизации |
///
/// Таблица BW (Output Resolution / ODR):
/// | BW1 | BW0 | Разрешение | Время изм. | ODR |
/// |-----|-----|------------|------------|-----|
/// | 0 | 0 | 16 бит | 10 мс | 100 Гц |
/// | 0 | 1 | 16 бит | 5 мс | 200 Гц |
/// | 1 | 0 | 16 бит | 2.5 мс | 400 Гц |
/// | 1 | 1 | 16 бит | 1.6 мс | 600 Гц |
///
/// @note Задержка между измерениями X/Y/Z = 1/3 от Measurement Time.
/// @note Запись 1 в X/Y/Z-inhibit отключает соответствующий канал,
/// уменьшая время измерения и общий заряд.
class mmc3630kj_ctrl1_reg
{
    static constexpr int sw_reset_offset{7U};
    static constexpr int z_inhibit_offset{4U};
    static constexpr int y_inhibit_offset{3U};
    static constexpr int x_inhibit_offset{2U};
    static constexpr int bw_offset{0U};

  public:
    /// @brief Адрес регистра CTRL1 в памяти устройства.
    static constexpr mmc3630kj_reg_type addr{0x09};

    explicit mmc3630kj_ctrl1_reg(
        mmc3630kj_reg_type value = mmc3630kj_reg_type{0})
    { parse(value); }

    /// @brief Оператор преобразования в сырое значение регистра.
    explicit operator mmc3630kj_reg_type() const
    {
        return static_cast<mmc3630kj_reg_type>(
            (static_cast<std::uint8_t>(sw_reset) << sw_reset_offset)
            | (static_cast<std::uint8_t>(z_inhibit) << z_inhibit_offset)
            | (static_cast<std::uint8_t>(y_inhibit) << y_inhibit_offset)
            | (static_cast<std::uint8_t>(x_inhibit) << x_inhibit_offset)
            | (static_cast<std::uint8_t>(bw) << bw_offset));
    }

    /// @brief Оператор сравнения двух экземпляров регистра на равенство.
    bool operator==(
        const mmc3630kj_ctrl1_reg &other) const
    {
        return static_cast<mmc3630kj_reg_type>(*this)
               == static_cast<mmc3630kj_reg_type>(other);
    }

    /// @brief Перечисление состояний программного сброса.
    enum struct sw_rst_t : std::uint8_t {
        disable = 0, ///< Отключено.
        enable  = 1, ///< Включено.
    };

    /// @brief Программный сброс.
    ///
    /// @details Установка в 1 вызывает программный сброс, аналогичный
    /// power-up. Очищает все регистры, перечитывает OTP данные.
    /// Время включения (t_Op) — 5 мс. После сброса устройство готово
    /// к работе.
    sw_rst_t sw_reset{sw_rst_t::disable};

    /// @brief Перечисление состояний отключения канала Z.
    enum struct z_inhibit_t : std::uint8_t {
        enable  = 0, ///< Канал Z включен.
        disable = 1, ///< Канал Z отключен.
    };

    /// @brief Отключение канала Z.
    ///
    /// @details Запись 1 отключает канал Z, уменьшая время измерения
    /// и общий заряд, потребляемый при измерении.
    z_inhibit_t z_inhibit{z_inhibit_t::enable};

    /// @brief Перечисление состояний отключения канала Y.
    enum struct y_inhibit_t : std::uint8_t {
        enable  = 0, ///< Канал Y включен.
        disable = 1, ///< Канал Y отключен.
    };

    /// @brief Отключение канала Y.
    ///
    /// @details Запись 1 отключает канал Y, уменьшая время измерения
    /// и общий заряд, потребляемый при измерении.
    y_inhibit_t y_inhibit{y_inhibit_t::enable};

    /// @brief Перечисление состояний отключения канала X.
    enum struct x_inhibit_t : std::uint8_t {
        enable  = 0, ///< Канал X включен.
        disable = 1, ///< Канал X отключен.
    };

    /// @brief Отключение канала X.
    ///
    /// @details Запись 1 отключает канал X, уменьшая время измерения
    /// и общий заряд, потребляемый при измерении.
    x_inhibit_t x_inhibit{x_inhibit_t::enable};

    /// @brief Перечисление значений частоты дискретизации (Bandwidth).
    enum struct bw_t : std::uint8_t {
        odr_100hz = 0, ///< 16 бит, 10 мс, 100 Гц.
        odr_200hz = 1, ///< 16 бит, 5 мс, 200 Гц.
        odr_400hz = 2, ///< 16 бит, 2.5 мс, 400 Гц.
        odr_600hz = 3, ///< 16 бит, 1.6 мс, 600 Гц.
    };

    /// @brief Частота дискретизации (Bandwidth).
    ///
    /// @details Определяет выходное разрешение, время измерения и
    /// максимальную частоту выдачи данных (ODR):
    /// - `odr_100hz` (00): 16 бит, 10 мс, 100 Гц
    /// - `odr_200hz` (01): 16 бит, 5 мс, 200 Гц
    /// - `odr_400hz` (10): 16 бит, 2.5 мс, 400 Гц
    /// - `odr_600hz` (11): 16 бит, 1.6 мс, 600 Гц
    ///
    /// @note Задержка между последовательными измерениями X/Y/Z
    /// составляет 1/3 от Measurement Time.
    ///
    /// @note Ток потребления пропорционален количеству измерений в секунду:
    /// BW=00: ~240–300 мкА (7 изм/с), BW=11: ~40–50 мкА.
    bw_t bw{bw_t::odr_100hz};

  private:
    void parse(
        mmc3630kj_reg_type reg)
    {
        sw_reset  = (reg & (1 << sw_reset_offset)) ? sw_rst_t::enable
                                                   : sw_rst_t::disable;
        z_inhibit = (reg & (1 << z_inhibit_offset)) ? z_inhibit_t::disable
                                                    : z_inhibit_t::enable;
        y_inhibit = (reg & (1 << y_inhibit_offset)) ? y_inhibit_t::disable
                                                    : y_inhibit_t::enable;
        x_inhibit = (reg & (1 << x_inhibit_offset)) ? x_inhibit_t::disable
                                                    : x_inhibit_t::enable;
        constexpr mmc3630kj_reg_type bw_mask{0x03};
        bw = static_cast<bw_t>(reg & bw_mask);
    }
};

/// @brief Управляющий регистр 2 MMC3630KJ (адрес 0x0A).
///
/// @details Регистр CTRL2 управляет прерываниями (завершение измерения,
/// обнаружение движения) и частотой непрерывного режима измерений
/// (Continuous Measurement Mode).
///
/// Регистр CTRL2 (0x0A):
/// | Бит | Имя | Описание |
/// |-----|-----|----------|
/// | 7 | ULP_SEL | Заводская настройка |
/// | 6 | INT_Meas_Done_EN | Прерывание завершения измерения |
/// | 5 | INT_MDT_EN | Прерывание детектора движения |
/// | 4 | Force_Vddl_Stby | Заводская настройка |
/// | 3–0 | CM_Freq[3:0] | Частота непрерывного режима |
///
/// Таблица CM_Freq (при BW=00):
/// | CM_Freq | Частота |
/// |---------|---------|
/// | 0000 | Непрерывный режим выключен |
/// | 0001 | 14 Гц |
/// | 0010 | 5 Гц |
/// | 0011 | 2.2 Гц |
/// | 0100 | 1 Гц |
/// | 0101 | 0.5 Гц |
/// | 0110 | 0.25 Гц |
/// | 0111 | 0.125 Гц |
/// | 1000 | 0.0625 Гц |
/// | 1001 | 0.03125 Гц |
/// | 1010 | 0.015625 Гц |
/// | 1011+ | Зарезервировано |
///
/// @note Детектор движения не работает, если CM_Freq = 0000.
class mmc3630kj_ctrl2_reg
{
    static constexpr int int_meas_done_en_offset{6U};
    static constexpr int int_mdt_en_offset{5U};
    static constexpr int cm_freq_offset{0U};

  public:
    /// @brief Адрес регистра CTRL2 в памяти устройства.
    static constexpr mmc3630kj_reg_type addr{0x0A};

    explicit mmc3630kj_ctrl2_reg(
        mmc3630kj_reg_type value = mmc3630kj_reg_type{0})
    { parse(value); }

    /// @brief Оператор преобразования в сырое значение регистра.
    explicit operator mmc3630kj_reg_type() const
    {
        return static_cast<mmc3630kj_reg_type>(
            (static_cast<std::uint8_t>(int_meas_done_en)
             << int_meas_done_en_offset)
            | (static_cast<std::uint8_t>(int_mdt_en) << int_mdt_en_offset)
            | (static_cast<std::uint8_t>(cm_freq) << cm_freq_offset));
    }

    /// @brief Оператор сравнения двух экземпляров регистра на равенство.
    bool operator==(
        const mmc3630kj_ctrl2_reg &other) const
    {
        return static_cast<mmc3630kj_reg_type>(*this)
               == static_cast<mmc3630kj_reg_type>(other);
    }

    /// @brief Перечисление состояний разрешения прерывания завершения
    /// измерения.
    enum struct int_meas_done_en_t : std::uint8_t {
        disable = 0, ///< Отключено.
        enable  = 1, ///< Включено.
    };

    /// @brief Разрешение прерывания при завершении измерения.
    ///
    /// @details Установка в 1 разрешает прерывание при завершении
    /// измерения магнитного поля или температуры. Прерывание
    /// генерируется при установке флагов `meas_m_done` или `meas_t_done`
    /// в STATUS (0x07). Вывод INT (active high) переходит в высокий
    /// уровень при срабатывании.
    int_meas_done_en_t int_meas_done_en{int_meas_done_en_t::disable};

    /// @brief Перечисление состояний разрешения прерывания детектора движения.
    enum struct int_mdt_en_t : std::uint8_t {
        disable = 0, ///< Отключено.
        enable  = 1, ///< Включено.
    };

    /// @brief Разрешение прерывания при обнаружении движения.
    ///
    /// @details Установка в 1 разрешает прерывание при обнаружении
    /// движения детектором. Прерывание генерируется при установке
    /// флага `motion_detect` в STATUS (0x07). Вывод INT (active high)
    /// переходит в высокий уровень при срабатывании.
    int_mdt_en_t int_mdt_en{int_mdt_en_t::disable};

    /// @brief Перечисление значений частоты непрерывного режима.
    enum struct cm_freq_t : std::uint8_t {
        continuous_mode_is_off = 0,  ///< Непрерывный режим отключен.
        freq_14hz              = 1,  ///< 14 Гц.
        freq_5hz               = 2,  ///< 5 Гц.
        freq_2_2hz             = 3,  ///< 2.2 Гц.
        freq_1hz               = 4,  ///< 1 Гц.
        freq_0_5hz             = 5,  ///< 0.5 Гц.
        freq_0_25hz            = 6,  ///< 0.25 Гц.
        freq_0_125hz           = 7,  ///< 0.125 Гц.
        freq_0_0625hz          = 8,  ///< 0.0625 Гц.
        freq_0_03125hz         = 9,  ///< 0.03125 Гц.
        freq_0_015625hz        = 10, ///< 0.015625 Гц.
    };

    /// @brief Частота непрерывного режима (Continuous Measurement Mode).
    ///
    /// @details Определяет, как часто устройство выполняет измерения
    /// в непрерывном режиме. Частоты указаны для BW=00 (100 Гц):
    /// - `continuous_mode_is_off` (0000): непрерывный режим выключен.
    ///   Детектор движения не работает в этом режиме.
    /// - `freq_14hz` (0001): 14 Гц
    /// - `freq_5hz` (0010): 5 Гц
    /// - `freq_2_2hz` (0011): 2.2 Гц
    /// - `freq_1hz` (0100): 1 Гц
    /// - `freq_0_5hz` (0101): 0.5 Гц
    /// - `freq_0_25hz` (0110): 0.25 Гц
    /// - `freq_0_125hz` (0111): 0.125 Гц
    /// - `freq_0_0625hz` (1000): 0.0625 Гц
    /// - `freq_0_03125hz` (1001): 0.03125 Гц
    /// - `freq_0_015625hz` (1010): 0.015625 Гц
    /// - 1011 и выше: зарезервировано, не используется.
    cm_freq_t cm_freq{cm_freq_t::continuous_mode_is_off};

  private:
    void parse(
        mmc3630kj_reg_type reg)
    {
        int_meas_done_en = (reg & (1 << int_meas_done_en_offset))
                               ? int_meas_done_en_t::enable
                               : int_meas_done_en_t::disable;
        int_mdt_en = (reg & (1 << int_mdt_en_offset)) ? int_mdt_en_t::enable
                                                      : int_mdt_en_t::disable;
        constexpr mmc3630kj_reg_type cm_freq_mask{0x1F};
        cm_freq = static_cast<cm_freq_t>(reg & cm_freq_mask);
    }
};

/// @brief Регистр идентификатора продукта MMC3630KJ (адрес 0x2F).
///
/// @details Регистр PRODUCT_ID содержит фиксированное значение,
/// позволяющее идентифицировать чип MMC3630KJ на шине I2C.
/// Ожидаемое значение — 0x0A.
///
/// @note Используется методом `mmc3630kj::is_detected()` для проверки
/// наличия датчика на шине I2C.
class mmc3630kj_product_id_reg
{
  public:
    /// @brief Адрес регистра PRODUCT_ID в памяти устройства.
    static constexpr mmc3630kj_reg_type addr{0x2F};

    /// @brief Ожидаемое значение идентификатора продукта.
    ///
    /// @details Для чипа MMC3630KJ значение PRODUCT_ID всегда равно
    /// 0x0A. Если при чтении по адресу 0x2F возвращается другое
    /// значение — устройство либо отсутствует, либо это другой чип.
    static constexpr mmc3630kj_reg_type expected_value{0x0A};

    explicit mmc3630kj_product_id_reg(
        mmc3630kj_reg_type value = mmc3630kj_reg_type{0}):
        product_id{value}
    {
    }

    /// @brief Оператор преобразования в сырое значение регистра.
    explicit operator mmc3630kj_reg_type() const { return product_id; }

    /// @brief Оператор сравнения двух экземпляров регистра на равенство.
    bool operator==(
        const mmc3630kj_product_id_reg &other) const
    { return product_id == other.product_id; }

    /// @brief Значение идентификатора продукта.
    mmc3630kj_reg_type product_id{0};
};

// NOLINTEND(*hicpp-signed-bitwise, *-member*)

} // namespace stv

#endif /* MMC3630KJ_REGS_HPP */
