/// @file icm42688p.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.

#ifndef ICM42688P_HPP
#define ICM42688P_HPP

#include "icm42688p_fifo.hpp"
#include "icm42688p_i2c.hpp"
#include "icm42688p_regs.hpp"
#include "icm42688p_types.hpp"
#include "stv/drivers/sample_validity.hpp"
#include "stv/gyraccmag_types.hpp"
#include "stv/utils.hpp"
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <numbers>
#include <optional>
#include <span>

namespace stv {

/// @brief Режим получения данных сенсоров ICM-42688-P.
///
/// @details Режим выбирается один раз при инициализации init() и не может
///          быть изменен без повторной инициализации:
///          - registers_16bit — 16-битные данные акселерометра, гироскопа и
///            температуры читаются блоком из сенсорных регистров;
///          - fifo_20bit — 20-битные данные акселерометра и гироскопа читаются
///            из FIFO пакетами по 20 байт (см. stv::icm42688p_fifo_packet).
enum struct icm42688p_data_mode : std::uint8_t {
    registers_16bit = 0, ///< 16-битные данные из сенсорных регистров.
    fifo_20bit      = 1, ///< 20-битные данные из FIFO.
};

/// @brief Структура для хранения параметров настройки датчика ICM-42688-P.
///
/// @details Наследует `stv::icm42688p_i2c_setup` и содержит конфигурационные
///          параметры, необходимые для инициализации датчика: частоты
///          дискретизации (ODR) и диапазоны измерений (FSR) сенсоров, режим
///          получения данных и электрику вывода прерывания INT1. При
///          режиме fifo_20bit драйвер дополнительно настраивает FIFO
///          (потоковый режим, 20-битные пакеты с температурой и меткой
///          времени, счётчик FIFO в записях) — отдельные флаги FIFO в setup
///          не выносятся.
template<icm42688p_reg_type Addr = 0x68>
struct icm42688p_setup: public stv::icm42688p_i2c_setup<Addr> {
    using int_config_reg = stv::icm42688p_int_config_reg;

    /// @brief Тип выходного каскада прерывания INT1 (по умолчанию
    ///        двухтактный выход).
    int_config_reg::drive_circuit_t int1_drive_circuit{
        int_config_reg::drive_circuit_t::push_pull,
    };

    /// @brief Полярность прерывания INT1 (по умолчанию активный высокий
    ///        уровень).
    int_config_reg::polarity_t int1_polarity{
        int_config_reg::polarity_t::active_high,
    };

    /// @brief Режим прерывания INT1 (по умолчанию импульс 100 мкс).
    int_config_reg::mode_t int1_mode{int_config_reg::mode_t::pulse};

    /// @brief Частота дискретизации акселерометра (по умолчанию 50 Гц —
    ///        период 20 мс, целевой такт системы управления).
    stv::icm42688p_odr_t accel_odr{stv::icm42688p_odr_t::odr_50};

    /// @brief Частота дискретизации гироскопа (по умолчанию 50 Гц).
    stv::icm42688p_odr_t gyro_odr{stv::icm42688p_odr_t::odr_50};

    /// @brief Диапазон измерений акселерометра (по умолчанию ±16 g).
    stv::icm42688p_accel_fsr_t accel_fsr{stv::icm42688p_accel_fsr_t::g_16};

    /// @brief Диапазон измерений гироскопа (по умолчанию ±2000 dps).
    stv::icm42688p_gyro_fsr_t gyro_fsr{stv::icm42688p_gyro_fsr_t::dps_2000};

    /// @brief Режим получения данных (по умолчанию 16-битные регистры).
    stv::icm42688p_data_mode mode{stv::icm42688p_data_mode::registers_16bit};
};

/// @brief Класс для работы с IMU ICM-42688-P.
///
/// @details Этот класс предоставляет высокоуровневый интерфейс для
///          взаимодействия с 6-осевым датчиком ICM-42688-P (3-осевой
///          акселерометр + 3-осевой гироскоп + термометр) по шине I2C.
///          Класс наследует:
///          - `stv::icm42688p_i2c` для низкоуровневого доступа к регистрам
///            датчика,
///          - `stv::iacc` для получения нормализованных данных
///            акселерометра (в g),
///          - `stv::igyr` для получения нормализованных данных гироскопа
///            (в рад/с с периодом интегрирования dT_sec).
///
///          Для использования необходимо создать объект класса, передав в
///          конструктор настроенную структуру `icm42688p_setup`, затем
///          вызвать метод `init()` с инъецированным функтором задержки
///          (миллисекунды). После успешной инициализации можно
///          периодически вызывать `read_normalized()` для получения пары
///          измерений акселерометра и гироскопа.
///
///          Прерывание готовности данных (контракт для кода автопилота):
///          датчик маршрутизирует прерывание UI_DRDY на вывод INT1.
///          Дефолтная электрика — push-pull, active-high, импульсный режим
///          (100 мкс). Код автопилота обязан сконфигурировать вывод MCU, к
///          которому подключён INT1, как вход EXTI по нарастающему фронту
///          (при active-high) с выбором приоритета. Получение прерывания
///          означает начало нового такта системы управления (целевой период
///          20 мс при ODR 50 Гц). Чтение сенсорных регистров очищает
///          прерывание (UI_DRDY_INT_CLEAR = 0b10).
///
/// @tparam AccType Тип нормализованных данных акселерометра.
/// @tparam GyrType Тип нормализованных данных гироскопа.
/// @tparam Addr 7-битный I2C адрес устройства (по умолчанию 0x68).
template<typename AccType        = stv::acc<float, std::uint32_t>,
         typename GyrType        = stv::gyr<float, std::uint32_t>,
         icm42688p_reg_type Addr = 0x68>
class icm42688p:
    public stv::icm42688p_i2c<Addr>,
    public stv::iacc<AccType>,
    public stv::igyr<typename GyrType::value_type,
                     typename GyrType::timestamp_type>
{
    using base_i2c       = stv::icm42688p_i2c<Addr>;
    using acc_type       = AccType;
    using gyr_type       = GyrType;
    using acc_value_type = typename acc_type::value_type;
    using gyr_value_type = typename gyr_type::value_type;
    using timestamp_type = typename acc_type::timestamp_type;
    using period_type    = typename gyr_type::period_type;

    using base_i2c::read;
    using base_i2c::write;

  public:
    /// @brief Структура для хранения необработанных (raw) данных с датчика.
    ///
    /// @details Содержит 16-битные знаковые значения температуры, осей
    ///          акселерометра и осей гироскопа в формате two's complement
    ///          big-endian, прочитанные блоком из регистров 0x1D–0x2A.
    STV_NO_PADDING_NO_OPTIMIZE_BEGIN

    struct raw_t {
        /// @brief Необработанное значение температуры, LSB.
        std::int16_t temp{0x00};

        /// @brief Необработанное значение оси X акселерометра, LSB.
        std::int16_t accel_x{0x00};

        /// @brief Необработанное значение оси Y акселерометра, LSB.
        std::int16_t accel_y{0x00};

        /// @brief Необработанное значение оси Z акселерометра, LSB.
        std::int16_t accel_z{0x00};

        /// @brief Необработанное значение оси X гироскопа, LSB.
        std::int16_t gyro_x{0x00};

        /// @brief Необработанное значение оси Y гироскопа, LSB.
        std::int16_t gyro_y{0x00};

        /// @brief Необработанное значение оси Z гироскопа, LSB.
        std::int16_t gyro_z{0x00};

        /// @brief Проверяет валидность данных.
        explicit operator bool() const
        {
            auto is_data_valid{true};

            if(is_default())
            {
                is_data_valid = false;
            }

            if(is_data_valid && !stv::are_axes_valid(accel_x, accel_y, accel_z))
            {
                is_data_valid = false;
            }

            if(is_data_valid && !stv::are_axes_valid(gyro_x, gyro_y, gyro_z))
            {
                is_data_valid = false;
            }

            return is_data_valid;
        }

      private:
        /// @brief Проверяет, все ли поля структуры равны нулю.
        [[nodiscard]] auto is_default() const -> bool
        {
            return (temp == 0) && (accel_x == 0) && (accel_y == 0)
                   && (accel_z == 0) && (gyro_x == 0) && (gyro_y == 0)
                   && (gyro_z == 0);
        }
    };

    STV_NO_PADDING_NO_OPTIMIZE_END

    /// @brief Структура для хранения пары нормализованных измерений
    ///        акселерометра и гироскопа.
    struct measurement_t {
        /// @brief Нормализованные данные акселерометра (в g).
        acc_type acc{};

        /// @brief Нормализованные данные гироскопа (в рад/с).
        gyr_type gyr{};

        /// @brief Оператор проверки валидности обоих измерений.
        ///
        /// @details Повторяет семантику stv::gyr::operator bool (валидные
        ///          конечные оси и ненулевой период интегрирования), но не
        ///          использует сравнение на равенство с плавающей точкой.
        explicit operator bool() const
        {
            const auto is_gyr_valid =
                gyr.is_axises_valid()
                && (std::fpclassify(gyr.dT_sec) != FP_ZERO);
            return static_cast<bool>(acc) && is_gyr_valid;
        }
    };

    /// @brief Структура результата самотестирования датчика.
    ///
    /// @details Содержит отклики самотестирования по шести осям в LSB
    ///          (среднее значение с включенным самотестированием минус
    ///          среднее значение с выключенным самотестированием) и
    ///          заводские трим-значения self-test из регистров датчика.
    ///          Пороговые проверки не выполняются — оценку результата
    ///          выполняет вызывающий код.
    struct self_test_result {
        /// @brief Отклик самотестирования оси X акселерометра, LSB.
        float accel_x{0.0F};

        /// @brief Отклик самотестирования оси Y акселерометра, LSB.
        float accel_y{0.0F};

        /// @brief Отклик самотестирования оси Z акселерометра, LSB.
        float accel_z{0.0F};

        /// @brief Отклик самотестирования оси X гироскопа, LSB.
        float gyro_x{0.0F};

        /// @brief Отклик самотестирования оси Y гироскопа, LSB.
        float gyro_y{0.0F};

        /// @brief Отклик самотестирования оси Z гироскопа, LSB.
        float gyro_z{0.0F};

        /// @brief Заводской трим self-test оси X гироскопа (bank 1, 0x5F).
        std::uint8_t gyro_trim_x{0x00};

        /// @brief Заводской трим self-test оси Y гироскопа (bank 1, 0x60).
        std::uint8_t gyro_trim_y{0x00};

        /// @brief Заводской трим self-test оси Z гироскопа (bank 1, 0x61).
        std::uint8_t gyro_trim_z{0x00};

        /// @brief Заводской трим self-test оси X акселерометра (bank 2,
        ///        0x3B).
        std::uint8_t accel_trim_x{0x00};

        /// @brief Заводской трим self-test оси Y акселерометра (bank 2,
        ///        0x3C).
        std::uint8_t accel_trim_y{0x00};

        /// @brief Заводской трим self-test оси Z акселерометра (bank 2,
        ///        0x3D).
        std::uint8_t accel_trim_z{0x00};
    };

  private:
    /// @brief Адрес начального регистра блока сенсорных данных (TEMP_DATA1).
    static constexpr icm42688p_reg_type sensor_data_start_addr{0x1D};

    /// @brief Размер блока сенсорных данных: температура (2 байта),
    ///        акселерометр XYZ (6 байт), гироскоп XYZ (6 байт).
    static constexpr std::size_t sensor_data_burst_size{14U};

    /// @brief Адрес регистра FIFO_COUNTH (читается первым, фиксирует
    ///        счётчик FIFO вместе с FIFO_COUNTL).
    static constexpr icm42688p_reg_type fifo_counth_addr{0x2E};

    /// @brief Адрес регистра FIFO_COUNTL.
    static constexpr icm42688p_reg_type fifo_countl_addr{0x2F};

    /// @brief Адрес регистра FIFO_DATA.
    static constexpr icm42688p_reg_type fifo_data_addr{0x30};

    /// @brief Адрес начального регистра заводских тримов гироскопа
    ///        (XG_ST_DATA, bank 1).
    static constexpr icm42688p_reg_type gyro_trim_start_addr{0x5F};

    /// @brief Адрес начального регистра заводских тримов акселерометра
    ///        (XA_ST_DATA, bank 2).
    static constexpr icm42688p_reg_type accel_trim_start_addr{0x3B};

    /// @brief Количество осей (и заводских тримов) одного сенсора.
    static constexpr std::size_t axis_count{3U};

    /// @brief Задержка после программного сброса, мс (не менее 1 мс).
    static constexpr std::uint32_t soft_reset_delay_ms{1U};

    /// @brief Задержка после выхода сенсоров из состояния OFF, мс (не
    ///        менее 200 мкс).
    static constexpr std::uint32_t power_up_delay_ms{1U};

    /// @brief Время установления после включения сенсоров перед
    ///        самотестированием, мс (гироскопу требуется не менее 45 мс).
    static constexpr std::uint32_t self_test_settle_ms{50U};

    /// @brief Количество выборок для усреднения в самотестировании.
    static constexpr int self_test_samples{8};

    /// @brief Задержка между опросами флага готовности данных в
    ///        самотестировании, мс.
    static constexpr std::uint32_t self_test_data_rdy_poll_delay_ms{1U};

    /// @brief Максимальное число опросов флага готовности данных на одну
    ///        выборку в самотестировании (перекрывает период самой низкой
    ///        поддерживаемой частоты дискретизации 12.5 Гц — 80 мс — с
    ///        запасом).
    static constexpr std::uint32_t self_test_data_rdy_max_polls{200U};

    /// @brief Маска значимых битов INTF_CONFIG6 (биты 7, 4:0). Резервные
    ///        биты 6:5 не моделируются классом регистра и на кристалле после
    ///        сброса читаются как 0b10 независимо от записи (reset value
    ///        регистра 0x5F), поэтому исключаются из проверки обратного
    ///        чтения. Остальные регистры init-последовательности имеют
    ///        резервные биты нулевого reset value (сверено по разделам 14–15
    ///        даташита DS-000347 rev 1.6) и верифицируются побайтно.
    static constexpr icm42688p_reg_type intf_config6_significant_mask{0x9F};

    /// @brief Чувствительность температуры, LSB/°C.
    static constexpr float temp_sensitivity_lsb_per_c{132.48F};

    /// @brief Смещение температурной шкалы, °C.
    static constexpr float temp_offset_c{25.0F};

    /// @brief Коэффициент перевода градусов в радианы.
    static constexpr float gyro_deg_to_rad{std::numbers::pi_v<float> / 180.0F};

    timestamp_type         timestamp_{};

    /// @brief Внутренний буфер для хранения последних считанных
    ///        необработанных данных.
    raw_t    raw_{};

    acc_type acc_{};
    gyr_type gyr_{};

    /// @brief Чувствительность акселерометра для текущего диапазона,
    ///        LSB/g.
    float accel_lsb_per_g_{2048.0F};

    /// @brief Чувствительность гироскопа для текущего диапазона,
    ///        LSB/dps.
    float gyro_lsb_per_dps_{16.4F};

    /// @brief Период дискретизации гироскопа, с (для поля dT_sec).
    period_type gyro_dt_sec_{0};

    /// @brief Флаг успешной инициализации.
    bool is_init_success_{false};

    /// @brief Копия структуры инициализации.
    stv::icm42688p_setup<Addr> init_copy_{};

    /// @brief Структура усреднённых значений шести осей.
    struct axes_average_t {
        float accel_x{0.0F};
        float accel_y{0.0F};
        float accel_z{0.0F};
        float gyro_x{0.0F};
        float gyro_y{0.0F};
        float gyro_z{0.0F};
    };

    /// @brief Возвращает чувствительность акселерометра для диапазона,
    ///        LSB/g.
    [[nodiscard]] static constexpr auto accel_sensitivity(
        stv::icm42688p_accel_fsr_t fsr) -> float
    {
        switch(fsr)
        {
            case stv::icm42688p_accel_fsr_t::g_16:
                return 2048.0F;
            case stv::icm42688p_accel_fsr_t::g_8:
                return 4096.0F;
            case stv::icm42688p_accel_fsr_t::g_4:
                return 8192.0F;
            case stv::icm42688p_accel_fsr_t::g_2:
                return 16384.0F;
        }

        return 2048.0F;
    }

    /// @brief Возвращает чувствительность гироскопа для диапазона,
    ///        LSB/dps.
    [[nodiscard]] static constexpr auto gyro_sensitivity(
        stv::icm42688p_gyro_fsr_t fsr) -> float
    {
        switch(fsr)
        {
            case stv::icm42688p_gyro_fsr_t::dps_2000:
                return 16.4F;
            case stv::icm42688p_gyro_fsr_t::dps_1000:
                return 32.8F;
            case stv::icm42688p_gyro_fsr_t::dps_500:
                return 65.5F;
            case stv::icm42688p_gyro_fsr_t::dps_250:
                return 131.0F;
            case stv::icm42688p_gyro_fsr_t::dps_125:
                return 262.0F;
            case stv::icm42688p_gyro_fsr_t::dps_62p5:
                return 524.3F;
            case stv::icm42688p_gyro_fsr_t::dps_31p25:
                return 1048.6F;
            case stv::icm42688p_gyro_fsr_t::dps_15p625:
                return 2097.2F;
        }

        return 16.4F;
    }

    /// @brief Возвращает период дискретизации для частоты ODR, с.
    [[nodiscard]] static constexpr auto period_sec(
        stv::icm42688p_odr_t odr) -> period_type
    {
        return static_cast<period_type>(1)
               / static_cast<period_type>(stv::icm42688p_odr_to_hz(odr));
    }

    /// @brief Преобразует пару байт big-endian в 16-битное знаковое
    ///        значение оси.
    [[nodiscard]] static auto convert_axis(
        std::uint8_t msb, std::uint8_t lsb) -> std::int16_t
    {
        const auto raw =
            static_cast<std::uint16_t>((static_cast<std::uint32_t>(msb) << 8U)
                                       | static_cast<std::uint32_t>(lsb));

        return static_cast<std::int16_t>(raw);
    }

    /// @brief Записывает регистр в память датчика, а затем считывает его и
    ///        сравнивает с тем, что планировалось записать.
    auto write_reg_then_check(
        const auto &reg)
    {
        volatile auto is_reg_written_success{false};
        this->write(reg);
        if(this->template read<decltype(reg)>() == reg)
        {
            is_reg_written_success = true;
        }

        return is_reg_written_success;
    }

    /// @brief Записывает регистр в память датчика, а затем считывает его и
    ///        сравнивает значимые биты с тем, что планировалось записать.
    ///
    /// @details Используется для регистров с резервными битами, которые на
    ///          кристалле читаются отлично от записанного значения
    ///          (например, биты 6:5 INTF_CONFIG6 после сброса читаются как
    ///          0b10 независимо от записи). Маска значимых битов задаётся
    ///          вызывающим кодом (см. intf_config6_significant_mask).
    ///
    /// @param[in] reg Записываемый регистр.
    /// @param[in] significant_mask Маска значимых (смоделированных) битов.
    auto write_reg_then_check(
        const auto &reg, icm42688p_reg_type significant_mask)
    {
        volatile auto is_reg_written_success{false};
        this->write(reg);
        const auto written  = static_cast<icm42688p_reg_type>(reg);
        const auto readback = this->read(reg.addr);
        if((readback & significant_mask) == (written & significant_mask))
        {
            is_reg_written_success = true;
        }

        return is_reg_written_success;
    }

    /// @brief Выбирает банк регистров датчика.
    auto select_bank(
        stv::icm42688p_reg_bank_sel_reg::bank_t bank)
    {
        stv::icm42688p_reg_bank_sel_reg bank_sel;
        bank_sel.bank_sel = bank;
        return write(bank_sel);
    }

    /// @brief Выполняет программный сброс датчика и переводит его из
    ///        SPI-режима в I2C-режим (заметка 12.3 даташита).
    template<typename TDelayFnMs>
    auto reset_and_enable_i2c(
        TDelayFnMs &delay_ms)
    {
        stv::icm42688p_device_config_reg device_config;
        device_config.soft_reset_config =
            stv::icm42688p_device_config_reg::soft_reset_t::enable;
        auto success = write(device_config);
        delay_ms(soft_reset_delay_ms);

        stv::icm42688p_intf_config6_reg intf_config6;
        intf_config6.i3c_sdr_en =
            stv::icm42688p_intf_config6_reg::switcher_t::disable;
        intf_config6.i3c_ddr_en =
            stv::icm42688p_intf_config6_reg::switcher_t::disable;

        success = stv::all_true(
            success,
            select_bank(stv::icm42688p_reg_bank_sel_reg::bank_t::bank_1));
        success = stv::all_true(
            success,
            write_reg_then_check(intf_config6, intf_config6_significant_mask));
        success = stv::all_true(
            success,
            select_bank(stv::icm42688p_reg_bank_sel_reg::bank_t::bank_0));

        return success;
    }

    /// @brief Настраивает выводы и маршрутизацию прерывания UI_DRDY на
    ///        INT1.
    auto configure_interrupts(
        const stv::icm42688p_setup<Addr> &setup)
    {
        stv::icm42688p_drive_config_reg drive_config;
        drive_config.i2c_slew_rate =
            stv::icm42688p_drive_config_reg::slew_rate_t::ns_12_36;
        drive_config.spi_slew_rate =
            stv::icm42688p_drive_config_reg::slew_rate_t::ns_12_36;

        stv::icm42688p_int_config_reg int_config;
        int_config.int1_drive_circuit = setup.int1_drive_circuit;
        int_config.int1_polarity      = setup.int1_polarity;
        int_config.int1_mode          = setup.int1_mode;

        stv::icm42688p_int_config1_reg int_config1;
        int_config1.int_async_reset =
            stv::icm42688p_int_config1_reg::async_reset_t::no_reset;

        stv::icm42688p_int_config0_reg int_config0;
        int_config0.ui_drdy_int_clear =
            stv::icm42688p_int_config0_reg::int_clear_t::sensor_register_read;

        stv::icm42688p_int_source0_reg int_source0;
        int_source0.ui_drdy_int1_en =
            stv::icm42688p_int_source0_reg::switcher_t::enable;

        return stv::all_true(write_reg_then_check(drive_config),
                             write_reg_then_check(int_config),
                             write_reg_then_check(int_config1),
                             write_reg_then_check(int_config0),
                             write_reg_then_check(int_source0));
    }

    /// @brief Настраивает частоты дискретизации и диапазоны сенсоров, а
    ///        также FIFO в режиме fifo_20bit (потоковый режим, 20-битные
    ///        пакеты с температурой и меткой времени, счётчик FIFO в
    ///        записях).
    auto configure_sensors(
        const stv::icm42688p_setup<Addr> &setup)
    {
        stv::icm42688p_gyro_config0_reg gyro_config0;
        gyro_config0.fs_sel = setup.gyro_fsr;
        gyro_config0.odr    = setup.gyro_odr;

        stv::icm42688p_accel_config0_reg accel_config0;
        accel_config0.fs_sel = setup.accel_fsr;
        accel_config0.odr    = setup.accel_odr;

        auto success = stv::all_true(write_reg_then_check(gyro_config0),
                                     write_reg_then_check(accel_config0));

        if(success && (setup.mode == stv::icm42688p_data_mode::fifo_20bit))
        {
            stv::icm42688p_fifo_config_reg fifo_config;
            fifo_config.fifo_mode =
                stv::icm42688p_fifo_config_reg::fifo_mode_t::stream_to_fifo;

            stv::icm42688p_fifo_config1_reg fifo_config1;
            fifo_config1.hires_en =
                stv::icm42688p_fifo_config1_reg::switcher_t::enable;
            fifo_config1.tmst_fsync_en =
                stv::icm42688p_fifo_config1_reg::switcher_t::enable;
            fifo_config1.temp_en =
                stv::icm42688p_fifo_config1_reg::switcher_t::enable;
            fifo_config1.gyro_en =
                stv::icm42688p_fifo_config1_reg::switcher_t::enable;
            fifo_config1.accel_en =
                stv::icm42688p_fifo_config1_reg::switcher_t::enable;

            stv::icm42688p_intf_config0_reg intf_config0;
            intf_config0.fifo_count_rec =
                stv::icm42688p_intf_config0_reg::switcher_t::enable;

            success = stv::all_true(write_reg_then_check(fifo_config),
                                    write_reg_then_check(fifo_config1),
                                    write_reg_then_check(intf_config0));
        }

        return success;
    }

    /// @brief Включает сенсоры в режим малого шума (Low Noise).
    template<typename TDelayFnMs>
    auto enable_sensors(
        TDelayFnMs &delay_ms)
    {
        stv::icm42688p_pwr_mgmt0_reg pwr_mgmt0;
        pwr_mgmt0.gyro_mode =
            stv::icm42688p_pwr_mgmt0_reg::gyro_mode_t::low_noise;
        pwr_mgmt0.accel_mode =
            stv::icm42688p_pwr_mgmt0_reg::accel_mode_t::low_noise;

        const auto success = write_reg_then_check(pwr_mgmt0);
        delay_ms(power_up_delay_ms);

        return success;
    }

    /// @brief Ожидает установки флага готовности данных DATA_RDY_INT.
    ///
    /// @details Опрос выполняется с задержкой
    ///          self_test_data_rdy_poll_delay_ms между попытками; число
    ///          попыток ограничено self_test_data_rdy_max_polls (без вечного
    ///          цикла).
    ///
    /// @tparam TDelayFnMs Тип функтора задержки (аргумент — миллисекунды).
    ///
    /// @param[in] delay_ms Функтор задержки в миллисекундах.
    ///
    /// @return `true` - если флаг готовности установлен; `false` - при
    ///         истечении числа попыток.
    template<typename TDelayFnMs>
    auto wait_data_ready(
        TDelayFnMs &delay_ms) -> bool
    {
        auto is_ready{false};
        for(std::uint32_t attempt{0U}; attempt < self_test_data_rdy_max_polls;
            ++attempt)
        {
            if(is_data_ready())
            {
                is_ready = true;
                break;
            }
            delay_ms(self_test_data_rdy_poll_delay_ms);
        }

        return is_ready;
    }

    /// @brief Ожидает готовности данных, считывает и усредняет несколько
    ///        выборок шести осей.
    ///
    /// @details Перед каждой выборкой ожидается флаг DATA_RDY_INT (см.
    ///          wait_data_ready): без ожидания при типовой частоте
    ///          дискретизации 50 Гц все выборки оказались бы идентичными, и
    ///          усреднение не усилило бы отклик самотестирования.
    ///
    /// @tparam TDelayFnMs Тип функтора задержки (аргумент — миллисекунды).
    ///
    /// @param[in] delay_ms Функтор задержки в миллисекундах.
    ///
    /// @return Усреднённые значения шести осей; std::nullopt - при истечении
    ///         ожидания готовности данных.
    template<typename TDelayFnMs>
    auto average_axes(
        TDelayFnMs &delay_ms) -> std::optional<axes_average_t>
    {
        axes_average_t average{};

        for(int i{0}; i < self_test_samples; ++i)
        {
            if(!wait_data_ready(delay_ms))
            {
                return std::nullopt;
            }

            const auto raw   = read_raw();
            average.accel_x += static_cast<float>(raw.accel_x);
            average.accel_y += static_cast<float>(raw.accel_y);
            average.accel_z += static_cast<float>(raw.accel_z);
            average.gyro_x  += static_cast<float>(raw.gyro_x);
            average.gyro_y  += static_cast<float>(raw.gyro_y);
            average.gyro_z  += static_cast<float>(raw.gyro_z);
        }

        const auto samples  = static_cast<float>(self_test_samples);
        average.accel_x    /= samples;
        average.accel_y    /= samples;
        average.accel_z    /= samples;
        average.gyro_x     /= samples;
        average.gyro_y     /= samples;
        average.gyro_z     /= samples;

        return average;
    }

  public:
    /// @brief Конструктор объекта для работы с датчиком ICM-42688-P.
    ///
    /// @param[in] setup Конфигурационная структура с параметрами
    /// подключения по шине I2C.
    explicit icm42688p(
        const stv::icm42688p_setup<Addr> &setup):
        base_i2c{setup}
    {
    }

    /// @brief Виртуальный деструктор класса.
    ~icm42688p() override = default;

    /// @brief Проверяет корректность инициализации датчика.
    ///
    /// @details Учитывает состояние базового транспорта
    ///          (stv::icm42688p_i2c) и флаг успешной инициализации: до
    ///          первого успешного вызова init() оператор возвращает `false`.
    ///
    /// @return `true` - если датчик был успешно инициализирован.
    explicit operator bool() const override
    {
        auto is_valid{true};
        if(!base_i2c::operator bool())
        {
            is_valid = false;
        }

        if(is_valid && !is_init_success_)
        {
            is_valid = false;
        }

        return is_valid;
    }

    /// @brief Статический метод для проверки наличия датчика ICM-42688-P
    ///        на шине I2C.
    ///
    /// @details Метод читает регистр WHO_AM_I (bank 0, адрес 0x75) и
    ///          сравнивает его значение с ожидаемым (0x47).
    ///
    /// @param[in] i2c Указатель на объект интерфейса I2C.
    /// @param[in] addr 7-битный I2C адрес устройства (по умолчанию 0x68,
    ///            вывод AD0 прижат к земле; при подтяжке AD0 к питанию
    ///            адрес равен 0x69).
    ///
    /// @return `true` - если по адресу ответило устройство с ожидаемым
    ///         идентификатором.
    static auto is_detected(
        stv::i2c_interface *i2c, icm42688p_reg_type addr = 0x68)
    {
        auto               is_found{false};

        icm42688p_reg_type who_am_i{0x00};
        const auto         is_success =
            i2c->read(static_cast<stv::i2c_interface::byte_type>(addr),
                      static_cast<stv::i2c_interface::byte_type>(
                          stv::icm42688p_who_am_i_reg::addr),
                      &who_am_i, sizeof(who_am_i));
        if(is_success
           && (who_am_i == stv::icm42688p_who_am_i_reg::expected_value))
        {
            is_found = true;
        }

        return is_found;
    }

    /// @brief Выполняет инициализацию датчика ICM-42688-P с заданными
    ///        параметрами.
    ///
    /// @details Метод выполняет программный сброс датчика, переводит его
    ///          из SPI-режима в I2C-режим, настраивает выводы прерывания
    ///          INT1 (электрика из setup, UI_DRDY маршрутизируется на
    ///          INT1), частоты дискретизации и диапазоны сенсоров, а
    ///          затем включает сенсоры в режим малого шума. Каждый
    ///          конфигурационный регистр записывается и верифицируется
    ///          обратным чтением (write_reg_then_check). PWR_MGMT0
    ///          записывается последним, так как после выхода сенсоров из
    ///          состояния OFF запись в регистры запрещена в течение
    ///          200 мкс — после записи выполняется задержка 1 мс.
    ///          Гироскопу требуется не менее 45 мс после включения до
    ///          выхода на валидные данные.
    ///
    /// @tparam TDelayFnMs Тип функтора задержки (аргумент — миллисекунды).
    ///
    /// @param[in] setup Структура `icm42688p_setup` с параметрами
    ///            инициализации.
    /// @param[in] delay_ms Функтор задержки в миллисекундах.
    ///
    /// @return `true` - если все регистры были успешно записаны и
    ///         прочитаны обратно с проверкой.
    template<typename TDelayFnMs>
    auto init(
        const stv::icm42688p_setup<Addr> &setup, TDelayFnMs &delay_ms)
    {
        is_init_success_ = false;

        auto success = reset_and_enable_i2c(delay_ms);
        if(success)
        {
            success = configure_interrupts(setup);
        }

        if(success)
        {
            success = configure_sensors(setup);
        }

        if(success)
        {
            success = enable_sensors(delay_ms);
        }

        accel_lsb_per_g_  = accel_sensitivity(setup.accel_fsr);
        gyro_lsb_per_dps_ = gyro_sensitivity(setup.gyro_fsr);
        gyro_dt_sec_      = period_sec(setup.gyro_odr);

        init_copy_       = setup;
        is_init_success_ = success;

        return success;
    }

    /// @brief Повторная инициализация датчика с параметрами последнего
    ///        вызова init().
    ///
    /// @tparam TDelayFnMs Тип функтора задержки (аргумент — миллисекунды).
    ///
    /// @param[in] delay_ms Функтор задержки в миллисекундах.
    template<typename TDelayFnMs>
    auto reinit(
        TDelayFnMs &delay_ms)
    {
        return init(init_copy_, delay_ms);
    }

    /// @brief Проверяет готовность новых данных (флаг DATA_RDY_INT регистра
    ///        INT_STATUS, бит 3).
    auto is_data_ready()
    {
        const auto status_reg =
            this->template read<stv::icm42688p_int_status_reg>();
        return status_reg.data_rdy_int
               == stv::icm42688p_int_status_reg::switcher_t::enable;
    }

    /// @brief Считывает необработанные данные из регистров датчика.
    ///
    /// @details Метод выполняет блоковое чтение 14 байт из регистров
    ///          0x1D–0x2A: температура, акселерометр XYZ, гироскоп XYZ.
    ///          Данные 16-битные, знаковые, big-endian (two's complement).
    ///          При успешной валидации счётчик timestamp_ инкрементируется;
    ///          при ошибке чтения или невалидных данных возвращается
    ///          структура raw_t, сброшенная в ноль.
    ///
    /// @return Структура `raw_t` с необработанными данными.
    auto read_raw()
    {
        std::array<std::uint8_t, sensor_data_burst_size> storage{};

        if(read(sensor_data_start_addr, storage.data(), storage.size()))
        {
            raw_t meas_raw{};
            meas_raw.temp    = convert_axis(storage.at(0), storage.at(1));
            meas_raw.accel_x = convert_axis(storage.at(2), storage.at(3));
            meas_raw.accel_y = convert_axis(storage.at(4), storage.at(5));
            meas_raw.accel_z = convert_axis(storage.at(6), storage.at(7));
            meas_raw.gyro_x  = convert_axis(storage.at(8), storage.at(9));
            meas_raw.gyro_y  = convert_axis(storage.at(10), storage.at(11));
            meas_raw.gyro_z  = convert_axis(storage.at(12), storage.at(13));

            if(meas_raw)
            {
                ++timestamp_;
            }
            else
            {
                meas_raw = raw_t{};
            }

            raw_ = meas_raw;
        }
        else
        {
            raw_ = raw_t{};
        }

        return raw_;
    }

    /// @brief Преобразует необработанные данные в нормализованные.
    ///
    /// @details Акселерометр приводится к g (сырое значение, делённое на
    ///          LSB/g текущего диапазона), гироскоп — к рад/с (сырое
    ///          значение, делённое на LSB/dps текущего диапазона и
    ///          умноженное на π/180). Период интегрирования гироскопа
    ///          dT_sec равен периоду текущей частоты дискретизации
    ///          гироскопа.
    ///
    /// @param[in] raw Структура с необработанными данными.
    ///
    /// @return Структура `measurement_t` с нормализованными данными
    ///         акселерометра и гироскопа.
    [[nodiscard]] auto normalize(
        const raw_t &raw) const -> measurement_t
    {
        const auto accel_x = static_cast<acc_value_type>(raw.accel_x)
                             / static_cast<acc_value_type>(accel_lsb_per_g_);
        const auto accel_y = static_cast<acc_value_type>(raw.accel_y)
                             / static_cast<acc_value_type>(accel_lsb_per_g_);
        const auto accel_z = static_cast<acc_value_type>(raw.accel_z)
                             / static_cast<acc_value_type>(accel_lsb_per_g_);

        const auto gyro_x = static_cast<gyr_value_type>(
            (static_cast<float>(raw.gyro_x) / gyro_lsb_per_dps_)
            * gyro_deg_to_rad);
        const auto gyro_y = static_cast<gyr_value_type>(
            (static_cast<float>(raw.gyro_y) / gyro_lsb_per_dps_)
            * gyro_deg_to_rad);
        const auto gyro_z = static_cast<gyr_value_type>(
            (static_cast<float>(raw.gyro_z) / gyro_lsb_per_dps_)
            * gyro_deg_to_rad);

        const auto acc = acc_type{accel_x, accel_y, accel_z, timestamp_};
        const auto gyr =
            gyr_type{gyro_x, gyro_y, gyro_z, timestamp_, gyro_dt_sec_};

        return {acc, gyr};
    }

    /// @brief Читает и нормализует данные акселерометра и гироскопа.
    ///
    /// @details При валидных необработанных данных обновляет внутренние
    ///          буферы acc_ и gyr_, которые затем доступны через
    ///          get_acc() и get_gyr().
    ///
    /// @return Структура `measurement_t` с нормализованными данными.
    auto read_normalized()
    {
        const auto raw  = read_raw();
        auto       meas = normalize(raw);
        if(raw)
        {
            acc_ = meas.acc;
            gyr_ = meas.gyr;
        }

        return meas;
    }

    /// @brief Возвращает последние валидные нормализованные данные
    ///        акселерометра.
    [[nodiscard]] auto get_acc() const -> acc_type override { return acc_; }

    /// @brief Возвращает последние валидные нормализованные данные
    ///        гироскопа.
    [[nodiscard]] auto get_gyr() const -> gyr_type override { return gyr_; }

    /// @brief Возвращает температуру, измеренную в последнем блоке чтения.
    ///
    /// @details Температура вычисляется по формуле даташита:
    ///          °C = TEMP_DATA / 132.48 + 25. Значение актуально после
    ///          вызова read_raw() или read_normalized().
    ///
    /// @return Температура в градусах Цельсия.
    [[nodiscard]] auto get_temperature() const -> float
    {
        return (static_cast<float>(raw_.temp) / temp_sensitivity_lsb_per_c)
               + temp_offset_c;
    }

    /// @brief Устанавливает частоты дискретизации сенсоров во время
    ///        работы.
    ///
    /// @details Пересчитывает период интегрирования гироскопа dT_sec.
    ///
    /// @param[in] accel_odr Частота дискретизации акселерометра.
    /// @param[in] gyro_odr Частота дискретизации гироскопа.
    ///
    /// @return `true` - если оба регистра были успешно записаны и
    ///         прочитаны обратно с проверкой.
    // NOLINTBEGIN(bugprone-easily-swappable-parameters): параметры
    // семантически различны (акселерометр/гироскоп), порядок зафиксирован
    // контрактом метода.
    auto set_odr(
        stv::icm42688p_odr_t accel_odr, stv::icm42688p_odr_t gyro_odr)
    {
        auto gyro_config0 =
            this->template read<stv::icm42688p_gyro_config0_reg>();
        auto accel_config0 =
            this->template read<stv::icm42688p_accel_config0_reg>();

        gyro_config0.odr  = gyro_odr;
        accel_config0.odr = accel_odr;
        gyro_dt_sec_      = period_sec(gyro_odr);

        return stv::all_true(write_reg_then_check(gyro_config0),
                             write_reg_then_check(accel_config0));
    }

    // NOLINTEND(bugprone-easily-swappable-parameters)

    /// @brief Устанавливает диапазоны измерений (FSR) сенсоров во время
    ///        работы.
    ///
    /// @param[in] accel_fsr Диапазон измерений акселерометра.
    /// @param[in] gyro_fsr Диапазон измерений гироскопа.
    ///
    /// @return `true` - если оба регистра были успешно записаны и
    ///         прочитаны обратно с проверкой.
    auto set_full_scale(
        stv::icm42688p_accel_fsr_t accel_fsr,
        stv::icm42688p_gyro_fsr_t  gyro_fsr)
    {
        auto gyro_config0 =
            this->template read<stv::icm42688p_gyro_config0_reg>();
        auto accel_config0 =
            this->template read<stv::icm42688p_accel_config0_reg>();

        gyro_config0.fs_sel  = gyro_fsr;
        accel_config0.fs_sel = accel_fsr;

        accel_lsb_per_g_  = accel_sensitivity(accel_fsr);
        gyro_lsb_per_dps_ = gyro_sensitivity(gyro_fsr);

        return stv::all_true(write_reg_then_check(gyro_config0),
                             write_reg_then_check(accel_config0));
    }

    /// @brief Выполняет самотестирование механической и электрической
    ///        частей сенсоров.
    ///
    /// @details Метод включает сенсоры в режим малого шума и выдерживает
    ///          время установления, усредняет выборки с выключенным
    ///          самотестированием, включает самотестирование всех шести
    ///          осей (SELF_TEST_CONFIG = 0x7F) и снова усредняет выборки,
    ///          после чего выключает самотестирование (включая питание
    ///          актуаторов акселерометра) и восстанавливает банк 0. Перед
    ///          каждой выборкой ожидается флаг готовности данных
    ///          DATA_RDY_INT (см. average_axes). Отклик оси — разность
    ///          средних значений с включенным и выключенным
    ///          самотестированием, в LSB. Дополнительно считываются
    ///          заводские трим-значения self-test (гироскоп — bank 1,
    ///          регистры 0x5F–0x61; акселерометр — bank 2, регистры
    ///          0x3B–0x3D). Пороговые проверки не выполняются.
    ///          Метод диагностический: при сбое шинных операций
    ///          соответствующие поля результата остаются нулевыми, при
    ///          истечении ожидания готовности данных возвращается
    ///          std::nullopt.
    ///
    /// @tparam TDelayFnMs Тип функтора задержки (аргумент — миллисекунды).
    ///
    /// @param[in] delay_ms Функтор задержки в миллисекундах.
    ///
    /// @return Структура `self_test_result` с откликами шести осей и
    ///         заводскими трим-значениями; std::nullopt - если ожидание
    ///         готовности данных вышло за лимит попыток.
    template<typename TDelayFnMs>
    auto run_self_test(
        TDelayFnMs &delay_ms) -> std::optional<self_test_result>
    {
        stv::icm42688p_pwr_mgmt0_reg pwr_mgmt0;
        pwr_mgmt0.gyro_mode =
            stv::icm42688p_pwr_mgmt0_reg::gyro_mode_t::low_noise;
        pwr_mgmt0.accel_mode =
            stv::icm42688p_pwr_mgmt0_reg::accel_mode_t::low_noise;
        (void)write_reg_then_check(pwr_mgmt0);
        delay_ms(self_test_settle_ms);

        const auto off_average = average_axes(delay_ms);
        if(!off_average.has_value())
        {
            return std::nullopt;
        }

        stv::icm42688p_self_test_config_reg self_test_config;
        self_test_config.accel_st_power =
            stv::icm42688p_self_test_config_reg::switcher_t::enable;
        self_test_config.en_az_st =
            stv::icm42688p_self_test_config_reg::switcher_t::enable;
        self_test_config.en_ay_st =
            stv::icm42688p_self_test_config_reg::switcher_t::enable;
        self_test_config.en_ax_st =
            stv::icm42688p_self_test_config_reg::switcher_t::enable;
        self_test_config.en_gz_st =
            stv::icm42688p_self_test_config_reg::switcher_t::enable;
        self_test_config.en_gy_st =
            stv::icm42688p_self_test_config_reg::switcher_t::enable;
        self_test_config.en_gx_st =
            stv::icm42688p_self_test_config_reg::switcher_t::enable;
        (void)write(self_test_config);
        delay_ms(self_test_settle_ms);

        const auto on_average = average_axes(delay_ms);

        self_test_config = stv::icm42688p_self_test_config_reg{};
        (void)write(self_test_config);

        if(!on_average.has_value())
        {
            return std::nullopt;
        }

        std::array<std::uint8_t, axis_count> gyro_trim{};
        std::array<std::uint8_t, axis_count> accel_trim{};
        (void)select_bank(stv::icm42688p_reg_bank_sel_reg::bank_t::bank_1);
        (void)read(gyro_trim_start_addr, gyro_trim.data(), gyro_trim.size());
        (void)select_bank(stv::icm42688p_reg_bank_sel_reg::bank_t::bank_2);
        (void)read(accel_trim_start_addr, accel_trim.data(), accel_trim.size());
        (void)select_bank(stv::icm42688p_reg_bank_sel_reg::bank_t::bank_0);

        return self_test_result{
            .accel_x = on_average->accel_x - off_average->accel_x,
            .accel_y = on_average->accel_y - off_average->accel_y,
            .accel_z = on_average->accel_z - off_average->accel_z,
            .gyro_x  = on_average->gyro_x - off_average->gyro_x,
            .gyro_y  = on_average->gyro_y - off_average->gyro_y,
            .gyro_z  = on_average->gyro_z - off_average->gyro_z,

            .gyro_trim_x  = gyro_trim.at(0),
            .gyro_trim_y  = gyro_trim.at(1),
            .gyro_trim_z  = gyro_trim.at(2),
            .accel_trim_x = accel_trim.at(0),
            .accel_trim_y = accel_trim.at(1),
            .accel_trim_z = accel_trim.at(2),
        };
    }

    /// @brief Считывает один 20-байтный пакет из FIFO.
    ///
    /// @details Сначала читается регистр FIFO_COUNTH (он фиксирует
    ///          значение счётчика FIFO вместе с FIFO_COUNTL), затем
    ///          FIFO_COUNTL. Драйвер настраивает FIFO_COUNT_REC = 1, поэтому
    ///          счётчик отсчитывается в записях: при наличии хотя бы одной
    ///          записи из FIFO_DATA читается 20 байт и передаются парсеру
    ///          stv::icm42688p_parse_fifo_packet. Если FIFO пуст или чтение
    ///          не удалось, возвращается невалидный пакет.
    ///
    /// @return Структура `icm42688p_fifo_packet` с распакованными
    ///         20-битными данными.
    [[nodiscard]] auto read_fifo_packet() -> stv::icm42688p_fifo_packet
    {
        const auto count_hi = read(fifo_counth_addr);
        const auto count_lo = read(fifo_countl_addr);
        const auto count    = static_cast<std::uint16_t>(
            (static_cast<std::uint32_t>(count_hi) << 8U)
            | static_cast<std::uint32_t>(count_lo));

        if(count == 0U)
        {
            return {};
        }

        std::array<std::uint8_t, stv::icm42688p_fifo_packet_size> data{};
        if(!read(fifo_data_addr, data.data(), data.size()))
        {
            return {};
        }

        return stv::icm42688p_parse_fifo_packet(
            std::span<const std::uint8_t, stv::icm42688p_fifo_packet_size>{
                data.data(), stv::icm42688p_fifo_packet_size});
    }
};

} // namespace stv

#endif /* ICM42688P_HPP */
