/// @file qmc5883p.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.

#ifndef QMC5883P_HPP
#define QMC5883P_HPP

#include "qmc5883p_i2c.hpp"
#include "qmc5883p_regs.hpp"
#include "stv/gyraccmag_types.hpp"
#include "stv/utils.hpp"
#include <cstdint>
#include <limits>
#include <utility>

namespace stv {

/// @brief Структура для хранения параметров настройки датчика QMC5883P.
///
/// @details Наследует `stv::qmc5883p_i2c_setup` и содержит конфигурационные
/// параметры, необходимые для инициализации датчика.
struct qmc5883p_setup: public stv::qmc5883p_i2c_setup {
    /// @brief Регистр управления 1.
    qmc5883p_ctrl1_reg ctrl1_reg;

    /// @brief Регистр управления 2.
    qmc5883p_ctrl2_reg ctrl2_reg;

    /// @brief Конструктор по умолчанию.
    ///
    /// @details Устанавливает конфигурацию по умолчанию:
    ///   - MODE = continuous;
    ///   - ODR = 10 Гц;
    ///   - RNG = ±30 Гс;
    ///   - OSR1 = 4;
    ///   - OSR2 = 1;
    ///   - SET/RESET MODE = set_and_reset_on.
    qmc5883p_setup()
    {
        ctrl1_reg.mode = qmc5883p_ctrl1_reg::mode_t::continuous;
        ctrl1_reg.odr  = qmc5883p_ctrl1_reg::odr_t::output_data_rate_10hz;
        ctrl1_reg.osr1 = qmc5883p_ctrl1_reg::osr1_t::oversampling_4;
        ctrl1_reg.osr2 = qmc5883p_ctrl1_reg::osr2_t::oversampling_1;

        ctrl2_reg.rng = qmc5883p_ctrl2_reg::rng_t::full_scale_30g;
        ctrl2_reg.set_reset_mode =
            qmc5883p_ctrl2_reg::set_reset_mode_t::set_and_reset_on;
    }
};

/// @brief Класс для работы с магнитометром QMC5883P.
///
/// @details Этот класс предоставляет высокоуровневый интерфейс для
/// взаимодействия с датчиком QMC5883P по шине I2C.
///
/// @tparam MagType Тип данных для нормализованных измерений магнитного поля.
template<typename MagType>
class qmc5883p: public stv::qmc5883p_i2c, public stv::imag<MagType>
{
    using mag_type       = MagType;
    using base_type      = stv::imag<MagType>;
    using timestamp_type = typename mag_type::timestamp_type;
    using value_type     = typename mag_type::value_type;

  public:
    STV_NO_PADDING_NO_OPTIMIZE_BEGIN

    /// @brief Структура для хранения необработанных (raw) данных с
    /// магнитометра.
    struct raw_t {
        /// @brief Значение по оси X.
        std::int16_t x{0x00};

        /// @brief Значение по оси Y.
        std::int16_t y{0x00};

        /// @brief Значение по оси Z.
        std::int16_t z{0x00};

        /// @brief Значение регистра статуса.
        std::uint8_t status_reg{0x00};

        /// @brief Проверяет валидность данных.
        explicit operator bool() const
        {
            auto is_data_valid{true};

            if(qmc5883p_status_reg{status_reg}.ovfl
               == qmc5883p_status_reg::ovfl_t::data_overflow)
            {
                is_data_valid = false;
            }

            if(is_default())
            {
                is_data_valid = false;
            }

            if(is_data_valid)
            {
                if(!is_axis_valid(x) || !is_axis_valid(y) || !is_axis_valid(z))
                {
                    is_data_valid = false;
                }
            }

            return is_data_valid;
        }

      private:
        [[nodiscard]] static auto is_axis_valid(
            std::int16_t axis) -> bool
        {
            auto is_axis_valid{true};

            if((axis == std::numeric_limits<std::int16_t>::max())
               || (axis == std::numeric_limits<std::int16_t>::min()))
            {
                is_axis_valid = false;
            }

            return is_axis_valid;
        }

        [[nodiscard]] auto is_default() const -> bool
        { return (x == 0) && (y == 0) && (z == 0); }
    };

    STV_NO_PADDING_NO_OPTIMIZE_END

  private:
    timestamp_type timestamp_{};

    /// @brief Внутренний буфер для хранения последних считанных данных.
    raw_t    raw_{};

    mag_type mag_;

    /// @brief Масштабный коэффициент для приведения измерений к Гаусс.
    value_type g_per_lsb_{0};

    /// @brief Флаг успешной инициализации.
    bool is_init_success_{false};

    /// @brief Копия структуры инициализации.
    stv::qmc5883p_setup init_copy_;

    /// @brief Флаг валидности self-test.
    bool is_self_test_valid_{false};

    /// @brief Максимальное количество попыток ожидания флага DRDY.
    static constexpr std::size_t max_data_ready_polls{1000U};

    /// @brief Записывает регистр в память датчика, а затем считывает его и
    /// сравнивает с тем что планировалось записать.
    ///
    /// @param[in] reg регистр, значение которого необходимо записать.
    /// @return true если записанное и считанное значение совпали.
    auto write_reg_then_check(
        const auto &reg)
    {
        volatile auto is_reg_written_success{false};
        if(write(reg) && (read<decltype(reg)>() == reg))
        {
            is_reg_written_success = true;
        }

        return is_reg_written_success;
    }

    /// @brief Вычисляет масштабный коэффициент Гаусс на младший значащий
    /// разряд.
    void compute_g_per_lsb()
    {
        const auto ctrl2 = read<qmc5883p_ctrl2_reg>();

        switch(ctrl2.rng)
        {
            case qmc5883p_ctrl2_reg::rng_t::full_scale_30g:
                g_per_lsb_ = static_cast<value_type>(1.0F / 1000.0F);
                break;

            case qmc5883p_ctrl2_reg::rng_t::full_scale_12g:
                g_per_lsb_ = static_cast<value_type>(1.0F / 2500.0F);
                break;

            case qmc5883p_ctrl2_reg::rng_t::full_scale_8g:
                g_per_lsb_ = static_cast<value_type>(1.0F / 3750.0F);
                break;

            case qmc5883p_ctrl2_reg::rng_t::full_scale_2g:
                g_per_lsb_ = static_cast<value_type>(1.0F / 15000.0F);
                break;
        }
    }

    /// @brief Ожидает установки флага готовности данных DRDY.
    ///
    /// @tparam TDelayFnMs Тип функции задержки (callable с параметром ms).
    /// @param[in] delay_ms Функция задержки в миллисекундах.
    /// Между опросами вызывается с аргументом 1 мс.
    ///
    /// @return true, если флаг DRDY установлен в пределах допустимого числа
    /// попыток.
    template<typename TDelayFnMs>
    [[nodiscard]] auto wait_data_ready(
        TDelayFnMs delay_ms) -> bool
    {
        for(std::size_t attempt{0}; attempt < max_data_ready_polls; ++attempt)
        {
            if(is_data_ready())
            {
                return true;
            }

            delay_ms(1);
        }

        return false;
    }

  public:
    /// @brief Конструктор объекта для работы с датчиком QMC5883P.
    ///
    /// @param[in] setup Конфигурационная структура с параметрами
    /// подключения по шине I2C.
    explicit qmc5883p(
        const stv::qmc5883p_setup &setup):
        qmc5883p_i2c{setup}
    {
    }

    /// @brief Виртуальный деструктор класса.
    ~qmc5883p() override = default;

    /// @brief Проверяет корректность инициализации датчика.
    ///
    /// @details Проверяется только валидность базового I2C-интерфейса.
    /// Для проверки успешности `init()` используется `is_initialized()`.
    ///
    /// @return `true` - если базовый I2C-объект валиден.
    explicit operator bool() const override
    {
        auto is_valid{true};
        if(!stv::qmc5883p_i2c::operator bool())
        {
            is_valid = false;
        }

        return is_valid;
    }

    /// @brief Проверяет, была ли успешно выполнена инициализация датчика.
    ///
    /// @return `true` - если `init()` завершился успешно.
    [[nodiscard]] auto is_initialized() const -> bool
    { return is_init_success_; }

    /// @brief Статический метод для проверки наличия датчика QMC5883P на
    /// шине I2C.
    ///
    /// @param[in] i2c Указатель на объект интерфейса I2C.
    ///
    /// @return `true` - если по адресу ответило устройство с ожидаемым ID.
    static auto is_detected(
        stv::i2c_interface *i2c) -> bool
    {
        auto                               is_detected{false};
        static constexpr qmc5883p_reg_type chip_id_valid{
            qmc5883p_chip_id_reg::expected_value};
        static constexpr qmc5883p_reg_type chip_id_addr{
            qmc5883p_chip_id_reg::addr};
        qmc5883p_reg_type chip_id{0x00};
        const auto        is_success =
            i2c->read(static_cast<stv::i2c_interface::byte_type>(
                          qmc5883p_i2c_setup::i2c_addr),
                      static_cast<stv::i2c_interface::byte_type>(chip_id_addr),
                      &chip_id, sizeof(chip_id));
        if(is_success && (chip_id == chip_id_valid))
        {
            is_detected = true;
        }

        return is_detected;
    }

    /// @brief Выполняет программный сброс датчика.
    ///
    /// @return true, если сброс выполнен успешно.
    auto soft_reset()
    {
        // Согласно даташиту (Table 18 / Soft Reset Example), программный сброс
        // выполняется записью только бита SOFT_RST (0x80). Остальные биты
        // должны быть нулевыми, иначе датчик может некорректно обработать
        // команду сброса.
        qmc5883p_ctrl2_reg ctrl2{static_cast<qmc5883p_reg_type>(0x00)};
        ctrl2.soft_reset = qmc5883p_ctrl2_reg::soft_reset_t::enable;
        return write(ctrl2);
    }

    /// @brief Выполняет инициализацию датчика QMC5883P без self-test.
    ///
    /// @param[in] setup Структура `qmc5883p_setup`, содержащая значения для
    /// ключевых регистров датчика.
    ///
    /// @return `true` - если все регистры были успешно записаны и прочитаны
    /// обратно с проверкой.
    auto init(
        const stv::qmc5883p_setup &setup)
    {
        is_init_success_ = false;
        auto is_init_success{true};

        if(is_init_success)
        {
            // Согласно примерам инициализации из даташита (7.1 Normal Mode,
            // 7.2 Continuous Mode, 7.3 Self-test) первым шагом выполняется
            // запись 0x06 в регистр 0x29 (знаки осей X, Y, Z). Регистр не
            // входит в Register Map даташита, поэтому read-back верификация
            // не выполняется.
            is_init_success = write(qmc5883p_axis_sign_reg{});
        }

        if(is_init_success)
        {
            is_init_success = write_reg_then_check(setup.ctrl2_reg);
        }

        if(is_init_success)
        {
            is_init_success = write_reg_then_check(setup.ctrl1_reg);
        }

        if(is_init_success)
        {
            compute_g_per_lsb();
        }

        init_copy_       = setup;
        is_init_success_ = is_init_success;
        return is_init_success;
    }

    /// @brief Выполняет инициализацию датчика QMC5883P с возможным
    /// выполнением self-test.
    ///
    /// @tparam TDelayFnMs Тип функции задержки (callable с параметром ms).
    /// @param[in] setup Структура `qmc5883p_setup`, содержащая значения для
    /// ключевых регистров датчика.
    /// @param[in] is_need_self_test Если true, выполняет self-test.
    /// @param[in] delay_ms Функция задержки в миллисекундах, используемая
    /// при self-test.
    ///
    /// @return `true` - если все регистры были успешно записаны и прочитаны
    /// обратно с проверкой.
    template<typename TDelayFnMs>
    auto init(
        const stv::qmc5883p_setup &setup, bool is_need_self_test,
        TDelayFnMs delay_ms)
    {
        auto is_init_success = init(setup);

        if(is_init_success && is_need_self_test)
        {
            is_init_success = check_self_test(delay_ms);
        }

        is_init_success_ = is_init_success;
        return is_init_success;
    }

    /// @brief Повторная инициализация датчика.
    auto reinit() { return init(init_copy_); }

    /// @brief Проверяет self-test по процедуре, описанной в даташите.
    ///
    /// @details После выполнения self-test восстанавливаются исходные
    /// значения регистров CTRL1 и CTRL2, поскольку датчик переходит в режим
    /// suspend.
    ///
    /// @tparam TDelayFnMs Тип функции задержки (callable с параметром ms).
    /// @param[in] delay_ms Функция задержки в миллисекундах.
    ///
    /// @return true, если self-test пройден успешно.
    template<typename TDelayFnMs>
    auto check_self_test(
        TDelayFnMs delay_ms)
    {
        is_self_test_valid_ = false;
        auto       is_self_test_success{false};

        const auto original_ctrl1 = read<qmc5883p_ctrl1_reg>();
        const auto original_ctrl2 = read<qmc5883p_ctrl2_reg>();

        // Перевод датчика в непрерывный режим измерений.
        qmc5883p_ctrl1_reg ctrl1_for_self_test = original_ctrl1;
        ctrl1_for_self_test.mode = qmc5883p_ctrl1_reg::mode_t::continuous;
        if(!write(ctrl1_for_self_test))
        {
            write(original_ctrl1);
            write(original_ctrl2);
            return is_self_test_valid_;
        }

        // Ожидание готовности данных и чтение первого набора значений.
        if(!wait_data_ready(delay_ms))
        {
            write(original_ctrl1);
            write(original_ctrl2);
            return is_self_test_valid_;
        }
        const auto data1 = read_meas_raw();

        // Включение встроенного self-test.
        auto ctrl2_self_test      = original_ctrl2;
        ctrl2_self_test.self_test = qmc5883p_ctrl2_reg::self_test_t::enable;
        if(!write(ctrl2_self_test))
        {
            write(original_ctrl1);
            write(original_ctrl2);
            return is_self_test_valid_;
        }

        // Задержка 5 мс после включения self-test, требуемая даташитом.
        delay_ms(5);

        // Ожидание завершения измерения с включенным self-test.
        if(!wait_data_ready(delay_ms))
        {
            write(original_ctrl1);
            write(original_ctrl2);
            return is_self_test_valid_;
        }
        const auto data2 = read_meas_raw();

        // Восстановление исходной конфигурации.
        write(original_ctrl1);
        write(original_ctrl2);

        // Проверка результатов self-test.
        if(data1 && data2)
        {
            const auto delta_x = static_cast<std::int32_t>(data1.x)
                                 - static_cast<std::int32_t>(data2.x);
            const auto delta_y = static_cast<std::int32_t>(data1.y)
                                 - static_cast<std::int32_t>(data2.y);
            const auto delta_z = static_cast<std::int32_t>(data1.z)
                                 - static_cast<std::int32_t>(data2.z);

            if(is_self_test_delta_valid(delta_x)
               && is_self_test_delta_valid(delta_y)
               && is_self_test_delta_valid(delta_z))
            {
                is_self_test_success = true;
            }
        }

        is_self_test_valid_ = is_self_test_success;
        return is_self_test_valid_;
    }

    [[nodiscard]] auto is_self_test_valid() const
    { return is_self_test_valid_; }

    /// @brief Возвращает true, если новые данные готовы для чтения.
    auto is_data_ready()
    {
        auto status_reg = read_status_reg();
        return status_reg.drdy
               == qmc5883p_status_reg::drdy_t::new_data_is_ready;
    }

    /// @brief Считывает значение регистра статуса.
    auto read_status_reg() { return read<qmc5883p_status_reg>(); }

    /// @brief Считывает необработанные данные измерений.
    auto read_meas_raw()
    {
        raw_t      meas_raw{};

        const auto is_axis_read_success = read(
            static_cast<qmc5883p_reg_type>(0x01),
            reinterpret_cast<void *>(&meas_raw.x), sizeof(std::int16_t) * 3U);
        const auto is_status_read_success =
            read(qmc5883p_status_reg::addr,
                 reinterpret_cast<void *>(&meas_raw.status_reg),
                 sizeof(meas_raw.status_reg));

        if(!is_axis_read_success || !is_status_read_success)
        {
            meas_raw = raw_t{};
            raw_     = meas_raw;
            return meas_raw;
        }

        if(meas_raw)
        {
            ++timestamp_;
        }
        else
        {
            meas_raw = raw_t{};
        }

        raw_ = meas_raw;
        return meas_raw;
    }

    /// @brief Преобразует необработанные данные в нормализованные (в Гаусс).
    auto normalize(
        const raw_t &raw_meas)
    {
        return mag_type{static_cast<value_type>(raw_meas.x) * g_per_lsb_,
                        static_cast<value_type>(raw_meas.y) * g_per_lsb_,
                        static_cast<value_type>(raw_meas.z) * g_per_lsb_,
                        timestamp_};
    }

    /// @brief Читает нормализованные данные с магнитометра.
    auto read_normalized()
    {
        auto meas = normalize(read_meas_raw());
        if(meas)
        {
            mag_ = meas;
        }

        return get_latest_normalize();
    }

    [[nodiscard]] auto get_latest_normalize() const { return mag_; }

    [[nodiscard]] auto get_mag() const -> mag_type override { return mag_; }

  private:
    /// @brief Проверяет валидность дельты по одной оси для self-test.
    ///
    /// @param[in] delta Разность между двумя измерениями по оси.
    /// @return true, если дельта отлична от нуля и не достигает
    /// экстремальных значений `int16_t`.
    [[nodiscard]] static auto is_self_test_delta_valid(
        std::int32_t delta) -> bool
    {
        constexpr std::int32_t max_delta{
            std::numeric_limits<std::int16_t>::max()};

        const auto abs_delta = (delta < 0) ? -delta : delta;
        return (abs_delta > 0) && (abs_delta < max_delta);
    }
};

} // namespace stv

#endif /* QMC5883P_HPP */
