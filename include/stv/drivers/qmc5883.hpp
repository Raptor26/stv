/// @file qmc5883.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.

#ifndef QMC5883_HPP
#define QMC5883_HPP

#include "qmc5883_i2c.hpp"
#include "qmc5883_regs.hpp"
#include "stv/gyraccmag_types.hpp"
#include "stv/utils.hpp"
#include <array>
#include <cstdint>
#include <limits>

namespace stv {

/// @brief Структура для хранения параметров настройки датчика QMC5883.
///
/// @details Наследует `stv::qmc5883_i2c_setup` и содержит конфигурационные
/// параметры, необходимые для инициализации датчика.
struct qmc5883_setup: public stv::qmc5883_i2c_setup {
    /// @brief Регистр управления 1.
    qmc5883_ctrl1_reg ctrl1_reg;

    /// @brief Регистр управления 2.
    qmc5883_ctrl2_reg ctrl2_reg;

    /// @brief Регистр периода сброса.
    qmc5883_set_reset_period_reg set_reset_period_reg;
};

/// @brief Класс для работы с магнитометром QMC5883.
///
/// @details Этот класс предоставляет высокоуровневый интерфейс для
/// взаимодействия с датчиком QMC5883 по шине I2C.
///
/// @tparam MagType Тип данных для нормализованных измерений магнитного поля.
template<typename MagType>
class qmc5883: public stv::qmc5883_i2c, public stv::imag<MagType>
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

            if(qmc5883_status_reg{status_reg}.ovl
               == qmc5883_status_reg::ovl_t::data_overflow)
            {
                is_data_valid = false;
            }

            if(is_default())
            {
                is_data_valid = false;
            }

            if(is_data_valid
               && (!is_axis_valid(x) || !is_axis_valid(y) || !is_axis_valid(z)))
            {
                is_data_valid = false;
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
    value_type lsb_per_g_{0};

    /// @brief Флаг успешной инициализации.
    bool is_init_success_{false};

    /// @brief Копия структуры инициализации.
    stv::qmc5883_setup init_copy_;

    /// @brief Флаг валидности self-test.
    bool is_self_test_valid_{false};

    /// @brief Записывает регистр в память датчика, а затем считывает его и
    /// сравнивает с тем что планировалось записать.
    ///
    /// @param[in] reg регистр, значение которого необходимо записать.
    /// @return true если записанное и считанное значение совпали.
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

    /// @brief Вычисляет масштабный коэффициент LSB.
    void compute_lsb()
    {
        const auto ctrl1 = read<qmc5883_ctrl1_reg>();
        if(ctrl1.rng == qmc5883_ctrl1_reg::rng_t::full_scale_2g)
        {
            lsb_per_g_ = 12000;
        }
        else if(ctrl1.rng == qmc5883_ctrl1_reg::rng_t::full_scale_8g)
        {
            lsb_per_g_ = 3000;
        }
    }

    /// @brief Имитация self-test.
    auto self_test_imitation()
    {
        while(!is_data_ready()) {}
        return static_cast<bool>(read_meas_raw());
    }

  public:
    /// @brief Конструктор объекта для работы с датчиком QMC5883.
    ///
    /// @param[in] setup Конфигурационная структура с параметрами
    /// подключения по шине I2C.
    explicit qmc5883(
        const stv::qmc5883_setup &setup):
        qmc5883_i2c{setup}
    {
    }

    /// @brief Виртуальный деструктор класса.
    ~qmc5883() override = default;

    /// @brief Проверяет корректность инициализации датчика.
    ///
    /// @return `true` - если датчик был успешно инициализирован.
    explicit operator bool() const override
    {
        auto is_valid{true};
        if(!stv::qmc5883_i2c::operator bool())
        {
            is_valid = false;
        }

        return is_valid;
    }

    /// @brief Статический метод для проверки наличия датчика QMC5883 на
    /// шине I2C.
    ///
    /// @param[in] i2c Указатель на объект интерфейса I2C.
    ///
    /// @return `true` - если по адресу ответило устройство с ожидаемым ID.
    static auto is_detected(
        stv::i2c_interface *i2c)
    {
        auto                              is_detected{false};
        static constexpr qmc5883_reg_type chip_id_valid{
            qmc5883_chip_id_reg::expected_value,
        };
        static constexpr qmc5883_reg_type chip_id_addr{
            qmc5883_chip_id_reg::addr,
        };
        qmc5883_reg_type chip_id{0x00};
        const auto       is_success =
            i2c->read(static_cast<stv::i2c_interface::byte_type>(
                          qmc5883_i2c_setup::i2c_addr),
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
        qmc5883_ctrl2_reg ctrl2;
        ctrl2.soft_reset = qmc5883_ctrl2_reg::soft_reset_t::enable;
        ctrl2.rol_pnt    = qmc5883_ctrl2_reg::rol_pnt_t::normal;
        return write(ctrl2);
    }

    /// @brief Выполняет инициализацию датчика QMC5883 с заданными
    /// параметрами.
    ///
    /// @param[in] setup Структура `qmc5883_setup`, содержащая значения для
    /// ключевых регистров датчика.
    /// @param[in] is_need_self_test Если true, выполняет self-test.
    ///
    /// @return `true` - если все регистры были успешно записаны и прочитаны
    /// обратно с проверкой.
    auto init(
        const stv::qmc5883_setup &setup, bool is_need_self_test = false)
    {
        is_init_success_ = false;
        auto is_init_success{true};

        if(is_init_success)
        {
            is_init_success = write_reg_then_check(setup.set_reset_period_reg);
        }

        if(is_init_success)
        {
            is_init_success = write_reg_then_check(setup.ctrl1_reg);
        }

        if(is_init_success)
        {
            is_init_success = write_reg_then_check(setup.ctrl2_reg);
        }

        if(is_init_success)
        {
            compute_lsb();
        }

        if(is_init_success && is_need_self_test)
        {
            check_self_test();
        }

        init_copy_       = setup;
        is_init_success_ = is_init_success;
        return is_init_success;
    }

    /// @brief Повторная инициализация датчика.
    auto reinit() { return init(init_copy_, false); }

    /// @brief Проверяет self-test.
    auto check_self_test()
    {
        is_self_test_valid_ = self_test_imitation();
        return is_self_test_valid_;
    }

    [[nodiscard]] auto is_self_test_valid() const
    { return is_self_test_valid_; }

    /// @brief Возвращает true, если новые данные готовы для чтения.
    auto is_data_ready()
    {
        const auto status_reg = read_status_reg();
        return status_reg.drdy == qmc5883_status_reg::drdy_t::new_data_is_ready;
    }

    /// @brief Возвращает true, если доступны данные для чтения.
    auto is_can_read()
    {
        auto is_can_read{false};
        auto status_reg = read_status_reg();

        if((status_reg.drdy == qmc5883_status_reg::drdy_t::new_data_is_ready)
           || (status_reg.dor
               == qmc5883_status_reg::dor_t::data_skipped_for_reading))
        {
            is_can_read = true;
        }

        return is_can_read;
    }

    /// @brief Считывает значение регистра статуса.
    auto read_status_reg() { return read<qmc5883_status_reg>(); }

    /// @brief Считывает идентификатор чипа.
    auto read_chip_id() { return read<qmc5883_chip_id_reg>(); }

    /// @brief Проверяет валидность идентификатора чипа.
    auto is_chip_id_valid()
    { return (read_chip_id().chip_id == qmc5883_chip_id_reg::expected_value); }

    /// @brief Считывает необработанные данные измерений.
    auto read_meas_raw()
    {
        raw_t meas_raw{};

        read(0x00, reinterpret_cast<void *>(&meas_raw), sizeof(meas_raw));

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
        return mag_type{
            static_cast<value_type>(raw_meas.x) / lsb_per_g_,
            static_cast<value_type>(raw_meas.y) / lsb_per_g_,
            static_cast<value_type>(raw_meas.z) / lsb_per_g_,
            timestamp_,
        };
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

    /// @brief Проверяет, содержит ли статусный регистр флаг переполнения.
    [[nodiscard("Read overflow status")]] static auto is_status_reg_overflow(
        qmc5883_reg_type status_reg) -> bool
    {
        auto       is_overflow_detect{false};
        const auto reg = qmc5883_status_reg{status_reg};

        if(reg.ovl == qmc5883_status_reg::ovl_t::data_overflow)
        {
            is_overflow_detect = true;
        }

        return is_overflow_detect;
    }
};

} // namespace stv

#endif /* QMC5883_HPP */
