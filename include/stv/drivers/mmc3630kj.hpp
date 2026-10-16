/// @file mmc3630kj.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.

#ifndef MMC3630KJ_HPP
#define MMC3630KJ_HPP

#include "mmc3630kj_i2c.hpp"
#include "mmc3630kj_regs.hpp"
#include "stv/drivers/sample_validity.hpp"
#include "stv/gyraccmag_types.hpp"
#include "stv/utils.hpp"
#include <array>
#include <cstdint>

namespace stv {

/// @brief Структура параметров настройки датчика MMC3630KJ.
///
/// @details Наследует `stv::mmc3630kj_i2c_setup` (I2C адрес 0x30) и
/// содержит начальные значения для управляющих регистров CTRL0, CTRL1
/// и CTRL2. Используется для конфигурации датчика при инициализации
/// через метод `mmc3630kj::init()`.
struct mmc3630kj_setup: public stv::mmc3630kj_i2c_setup {
    /// @brief Регистр управления 0 (адрес 0x08).
    ///
    /// @details Управляет измерениями, SET/RESET катушкой, перезарядкой
    /// конденсатора и детектором движения.
    mmc3630kj_ctrl0_reg ctrl0_reg;

    /// @brief Регистр управления 1 (адрес 0x09).
    ///
    /// @details Управляет программным сбросом, отключением каналов
    /// и частотой дискретизации (BW).
    mmc3630kj_ctrl1_reg ctrl1_reg;

    /// @brief Регистр управления 2 (адрес 0x0A).
    ///
    /// @details Управляет прерываниями и частотой непрерывного режима.
    mmc3630kj_ctrl2_reg ctrl2_reg;
};

/// @brief Высокоуровневый драйвер магнитометра MMC3630KJ.
///
/// @details Предоставляет интерфейс для инициализации, чтения данных,
/// SET/RESET операций, self-test и нормализации.
///
/// Датчик MMC3630KJ — полноценный 3-осевой магнитометр с встроенной
/// обработкой сигнала и интерфейсом I2C. Измеряет магнитное поле в
/// диапазоне ±30 Г с разрешением 1 мГ/LSB (16 бит) и шумом 2 мГ RMS
/// (typ, BW=00/01). Встроенная функция SET/RESET устраняет температурный
/// дрейф Null Field Output и остаточную намагниченность от сильных
/// внешних полей (>36 Г).
///
/// **Теория работы SET/RESET:**
/// AMR-сенсоры MEMSIC — это тонкоплёночные резисторы из пермаллоя в
/// конфигурации Уитстона. Сильное внешнее поле может изменить полярность
/// намагниченности плёнки. SET/RESET восстанавливает характеристики:
/// 1. **SET**: намагниченность в направлении SET поля → измерение:
///    `Output1 = +H + Offset`
/// 2. **RESET**: намагниченность в противоположном направлении (180°) →
///    измерение: `Output2 = -H + Offset`
/// 3. **Точное H**: `(Output1 - Output2) / 2` — устраняет Offset и
///    температурный дрейф
/// 4. **Offset**: `(Output1 + Output2) / 2` — можно хранить и вычитать
///    из последующих измерений
///
/// **Временные параметры:**
/// | Параметр | Значение | Описание |
/// |----------|----------|----------|
/// | t_Op | 5 мс | Время готовности после подачи VDD |
/// | t_RF (dual) | 1 мс | От Refill Cap до SET/RESET |
/// | t_RF (single) | 50 мс | От Refill Cap до SET/RESET |
/// | t_SR | 1 мс | Мин. интервал между SET/RESET |
/// | t_TM (BW=00) | 10 мс | Время измерения |
/// | t_TM (BW=01) | 5 мс | Время измерения |
/// | t_TM (BW=10) | 2.5 мс | Время измерения |
/// | t_TM (BW=11) | 1.6 мс | Время измерения |
///
/// **Электрические характеристики:**
/// - Диапазон измерения: ±30 Г
/// - Разрешение: 1 мГ/LSB (16 бит)
/// - Шум: 2 мГ RMS (typ, BW=00/01)
/// - Чувствительность: 1024 counts/G
/// - Ток потребления: 240–300 мкА (BW=00, 7 изм/с), 40–50 мкА (BW=11)
/// - Ток в power-down: 1 мкА
/// - I2C: FAST mode ≤400 кГц
/// - Рабочая температура: -40°C … +85°C
///
/// @tparam MagType Тип данных для нормализованных измерений магнитного поля.
/// Должен предоставлять `timestamp_type`, `value_type` и конструктор
/// от четырёх параметров (x, y, z, timestamp).
template<typename MagType>
class mmc3630kj: public stv::mmc3630kj_i2c, public stv::imag<MagType>
{
    using mag_type       = MagType;
    using base_type      = stv::imag<MagType>;
    using timestamp_type = typename mag_type::timestamp_type;
    using value_type     = typename mag_type::value_type;

    STV_NO_PADDING_NO_OPTIMIZE_BEGIN

    /// @brief Структура необработанных (raw) данных с магнитометра.
    ///
    /// @details Содержит три 16-битных unsigned значения для осей X, Y, Z.
    /// Данные приходят из датчика в формате: X[7:0], X[15:8], Y[7:0],
    /// Y[15:8], Z[7:0], Z[15:8] (little-endian). Нулевое поле соответствует
    /// 32768 (0x8000). Для конвертации в signed: `signed = raw - 32768`.
    ///
    /// Валидация данных: структура считается невалидной, если все значения
    /// равны нулю, или если любое значение равно min/max `uint16_t`.
    struct raw_t {
        /// @brief Необработанное значение по оси X (unsigned 16 бит).
        std::uint16_t x{0x00};

        /// @brief Необработанное значение по оси Y (unsigned 16 бит).
        std::uint16_t y{0x00};

        /// @brief Необработанное значение по оси Z (unsigned 16 бит).
        std::uint16_t z{0x00};

        /// @brief Сбрасывает все значения структуры в ноль.
        void reset()
        {
            x = 0;
            y = 0;
            z = 0;
        }

        /// @brief Проверяет валидность данных.
        ///
        /// @return true, если данные валидны (не все нули, не min/max).
        explicit operator bool() const
        {
            auto is_data_valid{true};

            if(is_default())
            {
                is_data_valid = false;
            }

            if(is_data_valid && !stv::are_axes_valid(x, y, z))
            {
                is_data_valid = false;
            }

            return is_data_valid;
        }

      private:
        [[nodiscard]] auto is_default() const -> bool
        { return (x == 0) && (y == 0) && (z == 0); }
    };

    STV_NO_PADDING_NO_OPTIMIZE_END

    /// @brief Константа нулевого поля (Null Field Output).
    ///
    /// @details Значение 32768 (0x8000) соответствует отсутствию внешнего
    /// магнитного поля в unsigned 16-bit формате. Используется для
    /// конвертации unsigned raw данных в signed: `signed = raw - 32768`.
    static constexpr std::int32_t null_field_output{32768};

    timestamp_type                timestamp_{};

    /// @brief Внутренний буфер для хранения последних считанных raw данных.
    raw_t    raw_{};

    mag_type mag_;

    /// @brief Масштабный коэффициент для приведения измерений к Гаусс.
    ///
    /// @details Значение 1024 counts/G — чувствительность датчика MMC3630KJ.
    /// Нормализация: `Gauss = (raw - 32768) / 1024`.
    value_type lsb_per_g_{1024};

    /// @brief Флаг успешности инициализации датчика.
    bool is_init_success_{false};

    /// @brief Флаг валидности self-test.
    bool is_self_test_valid_{false};

    /// @brief Преобразует необработанное значение оси в знаковое.
    ///
    /// @details Выполняет конвертацию: `signed = raw - null_field_output`.
    /// Результат приводится к типу `std::int16_t`.
    ///
    /// @param[in] raw_axis Необработанное значение оси (unsigned 16 бит).
    /// @return Знаковое значение оси.
    static auto convert_axis_to_signed(
        std::uint16_t raw_axis)
    {
        return static_cast<std::int16_t>(static_cast<std::int32_t>(raw_axis)
                                         - null_field_output);
    }

    /// @brief Проверяет, готовы ли новые данные измерения.
    ///
    /// @details Читает регистр STATUS (0x07) и проверяет флаг
    /// `meas_m_done`. Необходимо вызывать перед чтением выходных данных
    /// Xout/Yout/Zout.
    ///
    /// @return true, если измерение магнитного поля завершено.
  public:
    auto is_new_data_ready()
    {
        const auto status_reg = read<mmc3630kj_status_reg>();
        return status_reg.meas_m_done
               == mmc3630kj_status_reg::meas_m_done_t::finished;
    }

    /// @brief Выполняет проверку self-test.
    ///
    /// @details Проверяет, что чип успешно прочитал OTP (`otp_rd_done` в
    /// STATUS), ожидает завершения измерения (`is_new_data_ready`), затем
    /// читает raw данные. Self-test считается пройденным, если OTP
    /// прочитан и данные корректны.
    ///
    /// @return true, если self-test пройден успешно.
    auto check_self_test()
    {
        auto       is_self_test_valid{true};

        const auto status_reg = read<mmc3630kj_status_reg>();

        if(status_reg.otp_rd_done
           == mmc3630kj_status_reg::otp_rd_done_t::not_able_to_read)
        {
            is_self_test_valid = false;
        }

        if(is_self_test_valid)
        {
            while(!is_new_data_ready()) {}
        }

        if(!read_meas_raw())
        {
            is_self_test_valid = false;
        }

        is_self_test_valid_ = is_self_test_valid;

        return is_self_test_valid_;
    }

    /// @brief Записывает регистр с верификацией.
    ///
    /// @details Записывает регистр в память датчика, затем считывает его
    /// обратно и сравнивает с записанным значением. Полезно для критических
    /// конфигурационных регистров, где требуется гарантия корректной записи.
    ///
    /// @param[in] reg Регистр, значение которого необходимо записать.
    /// @return true, если записанное и считанное значения совпали.
    auto write_reg_then_check(
        const auto &reg)
    {
        volatile auto is_reg_written_success{false};
        write(reg);
        if(read<decltype(reg)>() == reg)
        {
            is_reg_written_success = true;
        }

        return is_reg_written_success;
    }

  public:
    /// @brief Конструктор драйвера MMC3630KJ.
    ///
    /// @param[in] setup Конфигурационная структура с параметрами I2C
    /// и начальными значениями регистров CTRL0/1/2.
    explicit mmc3630kj(
        const stv::mmc3630kj_setup &setup):
        mmc3630kj_i2c{setup}
    {
    }

    /// @brief Виртуальный деструктор.
    ~mmc3630kj() override = default;

    /// @brief Проверяет корректность инициализации датчика.
    ///
    /// @details Возвращает true, если I2C-интерфейс инициализирован
    /// (`mmc3630kj_i2c::operator bool`) и все конфигурационные регистры
    /// успешно записаны (`is_init_success_`).
    ///
    /// @return `true` — если датчик был успешно инициализирован.
    explicit operator bool() const override
    {
        auto is_valid{true};
        if(!stv::mmc3630kj_i2c::operator bool())
        {
            is_valid = false;
        }
        if(!is_init_success_)
        {
            is_valid = false;
        }

        return is_valid;
    }

    /// @brief Статическая проверка наличия датчика на шине I2C.
    ///
    /// @details Читает регистр PRODUCT_ID (0x2F) по I2C адресу 0x30.
    /// Возвращает true, если прочитанное значение совпадает с
    /// ожидаемым (`expected_value` = 0x0A).
    ///
    /// @param[in] i2c Указатель на объект интерфейса I2C.
    /// @return `true` — если по адресу ответило устройство с ожидаемым ID.
    static auto is_detected(
        stv::i2c_interface *i2c)
    {
        auto                                is_detected{false};
        static constexpr mmc3630kj_reg_type chip_id_valid{
            mmc3630kj_product_id_reg::expected_value,
        };
        static constexpr mmc3630kj_reg_type chip_id_addr{
            mmc3630kj_product_id_reg::addr,
        };
        mmc3630kj_reg_type chip_id{0xFF};
        const auto         is_success =
            i2c->read(static_cast<stv::i2c_interface::byte_type>(
                          mmc3630kj_i2c_setup::i2c_addr),
                      static_cast<stv::i2c_interface::byte_type>(chip_id_addr),
                      &chip_id, sizeof(chip_id));
        if(is_success && (chip_id == chip_id_valid))
        {
            is_detected = true;
        }

        return is_detected;
    }

    /// @brief Читает необработанные данные с магнитометра.
    ///
    /// @details Читает 6 байт с адреса 0x00 (Xout low/high, Yout low/high,
    /// Zout low/high). Данные собираются в 16-битные unsigned значения.
    /// Если данные невалидны (все нули или min/max) — сбрасывает буфер
    /// в 0. При успешном чтении инкрементирует timestamp.
    ///
    /// @return Структура `raw_t` с тремя 16-битными значениями X, Y, Z.
    /// Если чтение неуспешно или данные невалидны — все поля равны нулю.
    auto read_meas_raw()
    {
        static constexpr mmc3630kj_reg_type start_addr{0x00};

        std::array<std::uint8_t, 6>         storage{};

        if(read(start_addr, reinterpret_cast<void *>(storage.data()),
                sizeof(storage)))
        {
            // Данные приходят в порядке X low, X high, Y low, Y high, Z low, Z
            // high
            raw_.x = static_cast<std::uint16_t>(storage[0]) |
                     // NOLINTNEXTLINE(hicpp-signed-bitwise)
                     static_cast<std::uint16_t>(
                         static_cast<std::uint16_t>(storage[1]) << 8U);
            raw_.y = static_cast<std::uint16_t>(storage[2]) |
                     // NOLINTNEXTLINE(hicpp-signed-bitwise)
                     static_cast<std::uint16_t>(
                         static_cast<std::uint16_t>(storage[3]) << 8U);
            raw_.z = static_cast<std::uint16_t>(storage[4]) |
                     // NOLINTNEXTLINE(hicpp-signed-bitwise)
                     static_cast<std::uint16_t>(
                         static_cast<std::uint16_t>(storage[5]) << 8U);

            if(!raw_)
            {
                raw_.reset();
            }
            else
            {
                ++timestamp_;
            }
        }
        else
        {
            raw_.reset();
        }

        return raw_;
    }

    /// @brief Нормализует raw данные в Гаусс.
    ///
    /// @details Выполняет конвертацию: для каждой оси вычисляет
    /// `signed = raw - 32768`, затем делит на `lsb_per_g_` (1024).
    /// Результат — магнитное поле в Гаусс.
    ///
    /// @param[in] raw Структура с необработанными данными.
    /// @return Структура `mag_type` с нормализованными значениями осей.
    auto normalize(
        const raw_t &raw)
    {
        return mag_type{
            static_cast<value_type>(convert_axis_to_signed(raw.x)) / lsb_per_g_,
            static_cast<value_type>(convert_axis_to_signed(raw.y)) / lsb_per_g_,
            static_cast<value_type>(convert_axis_to_signed(raw.z)) / lsb_per_g_,
            timestamp_,
        };
    }

    /// @brief Читает нормализованные данные с магнитометра.
    ///
    /// @details Выполняет `normalize(read_meas_raw())` — читает raw данные
    /// и конвертирует их в Гаусс.
    ///
    /// @return Структура `mag_type` с нормализованными значениями.
    auto read_normalized()
    {
        mag_ = normalize(read_meas_raw());
        return mag_;
    }

    [[nodiscard]] auto get_mag() const -> mag_type override { return mag_; }

    /// @brief Выполняет программный сброс датчика.
    ///
    /// @details Устанавливает `SW_RST=1` в CTRL1 (0x09), ждёт 5 мс
    /// (время power-on t_Op), затем опрашивает регистр до 10 раз с
    /// интервалом 1 мс до сброса флага `sw_reset`. Аналогичен power-up:
    /// очищает все регистры, перечитывает OTP.
    ///
    /// @tparam TDelayFnMs Тип функции задержки (callable с параметром ms).
    /// @param[in] delay_ms Функция задержки в миллисекундах.
    /// @return true, если сброс выполнен успешно.
    template<typename TDelayFnMs>
    auto software_reset(
        TDelayFnMs &&delay_ms)
    {
        auto                success{true};

        mmc3630kj_ctrl1_reg ctrl1;
        ctrl1.sw_reset = mmc3630kj_ctrl1_reg::sw_rst_t::enable;
        success        = stv::all_true(
            write(ctrl1.addr, static_cast<mmc3630kj_reg_type>(ctrl1)), success);
        std::forward<TDelayFnMs>(delay_ms)(5);

        for(std::size_t i{0}; i < 10; ++i)
        {
            const auto reg = read<mmc3630kj_ctrl1_reg>();

            if(reg.sw_reset == mmc3630kj_ctrl1_reg::sw_rst_t::disable)
            {
                break;
            }

            std::forward<TDelayFnMs>(delay_ms)(1);
        }

        return success;
    }

    /// @brief Устанавливает режим работы датчика.
    ///
    /// @details mode = 0: standby (BW = 100 Гц, низкое энергопотребление).
    /// mode ≠ 0: active (BW = 600 Гц, максимальная частота дискретизации).
    ///
    /// @param[in] mode Режим работы: 0 — standby, иное — active.
    /// @return Статус операции записи и верификации регистра.
    auto set_mode(
        std::size_t mode)
    {
        auto ctrl1 = read<mmc3630kj_ctrl1_reg>();

        if(mode == 0)
        {
            ctrl1.bw = mmc3630kj_ctrl1_reg::bw_t::odr_100hz;
        }
        else
        {
            ctrl1.bw = mmc3630kj_ctrl1_reg::bw_t::odr_600hz;
        }

        return write_reg_then_check(ctrl1);
    }

    /// @brief Инициализирует датчик MMC3630KJ.
    ///
    /// @details Записывает управляющие регистры CTRL0, CTRL1, CTRL2 из
    /// структуры `setup`. Опционально выполняет `check_self_test()`.
    /// Возвращает true, если все регистры успешно записаны.
    ///
    /// @param[in] setup Структура с начальными значениями регистров.
    /// @param[in] is_need_self_test Если true — выполняет self-test.
    /// @return `true` — если все регистры успешно записаны.
    auto init(
        const stv::mmc3630kj_setup &setup, bool is_need_self_test = false)
    {
        const auto is_init_success =
            stv::all_true(write(setup.ctrl0_reg), write(setup.ctrl1_reg),
                          write(setup.ctrl2_reg));

        if(is_init_success && is_need_self_test)
        {
            is_self_test_valid_ = check_self_test();
        }

        is_init_success_ = is_init_success;
        return is_init_success;
    }

    /// @brief Возвращает результат self-test.
    ///
    /// @return true, если self-test был пройден успешно.
    [[nodiscard]] auto is_self_test_valid() const
    { return is_self_test_valid_; }

    /// @brief Запрашивает перезарядку конденсатора на VCAP.
    ///
    /// @details Устанавливает бит `refill_cap` в CTRL0 (0x08).
    /// Требуется перед операциями SET/RESET при использовании single
    /// supply (внутренний charge pump). После вызова необходимо ожидать
    /// завершения charge pump через `is_charge_pump_complete()`.
    ///
    /// @note При dual supply (внешнее питание VCAP) этот метод
    /// не требуется — бит `refill_cap` зарезервирован.
    ///
    /// @note Время подготовки: single supply — 50 мс, dual supply — 1 мс.
    ///
    /// @return Статус операции шины I2C.
    auto refill_cap()
    {
        mmc3630kj_ctrl0_reg ctrl0;
        ctrl0.refill_cap =
            mmc3630kj_ctrl0_reg::refill_cap_t::request_recharge_capacity;

        return write(ctrl0);
    }

    /// @brief Выполняет SET катушки.
    ///
    /// @details Устанавливает бит `set` в CTRL0 (0x08). Пропускает большой
    /// ток через SET/RESET coil в одном направлении, устанавливая
    /// намагниченность AMR-сенсоров. Устраняет температурный дрейф
    /// Null Field Output и очищает остаточную намагниченность от сильных
    /// внешних полей (>36 Г).
    ///
    /// @note Перед SET/RESET при single supply необходимо сначала
    /// выполнить `refill_cap()` и подождать не менее 50 мс.
    /// @note Минимальный интервал между SET и RESET (t_SR) — 1 мс.
    ///
    /// @return Статус операции шины I2C.
    auto set_coil()
    {
        mmc3630kj_ctrl0_reg ctrl0;
        ctrl0.set = mmc3630kj_ctrl0_reg::set_t::enable;

        return write(ctrl0);
    }

    /// @brief Выполняет RESET катушки.
    ///
    /// @details Устанавливает бит `reset` в CTRL0 (0x08). Пропускает
    /// большой ток через SET/RESET coil в направлении, противоположном
    /// SET (180°). Сбрасывает намагниченность AMR-сенсоров.
    ///
    /// @note Перед SET/RESET при single supply необходимо сначала
    /// выполнить `refill_cap()` и подождать не менее 50 мс.
    /// @note Минимальный интервал между SET и RESET (t_SR) — 1 мс.
    ///
    /// @return Статус операции шины I2C.
    auto reset_coil()
    {
        mmc3630kj_ctrl0_reg ctrl0;
        ctrl0.reset = mmc3630kj_ctrl0_reg::reset_t::enable;

        return write(ctrl0);
    }

    /// @brief Читает статусный регистр датчика.
    ///
    /// @return Структура `mmc3630kj_status_reg` с текущим состоянием
    /// флагов устройства.
    auto read_status() { return read<mmc3630kj_status_reg>(); }

    /// @brief Проверяет завершение работы зарядного насоса.
    ///
    /// @details Читает STATUS (0x07) и проверяет флаг `pump_on`.
    /// Charge pump завершён, когда `pump_on == charge_pump_complete`.
    ///
    /// @return true, если зарядный насос завершил работу.
    auto is_charge_pump_complete()
    {
        return read_status().pump_on
               == mmc3630kj_status_reg::pump_on_t::charge_pump_complete;
    }

    /// @brief Сбрасывает флаги прерываний.
    ///
    /// @details Записывает 1 в биты `meas_m_done`, `meas_t_done` и
    /// `motion_detect` регистра STATUS (0x07) для очистки флагов
    /// прерываний. Все флаги STATUS сбрасываются записью логической 1.
    ///
    /// @return Статус операции записи.
    auto reset_interrupts()
    {
        mmc3630kj_status_reg status_reg;
        status_reg.meas_m_done = mmc3630kj_status_reg::meas_m_done_t::finished;
        status_reg.meas_t_done = mmc3630kj_status_reg::meas_t_done_t::finished;
        status_reg.motion_detect =
            mmc3630kj_status_reg::motion_detected_t::motion_detect;

        return write(status_reg);
    }
};

} // namespace stv

#endif /* MMC3630KJ_HPP */
