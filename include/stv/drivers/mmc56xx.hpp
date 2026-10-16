/// @file mmc56xx.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.

#ifndef MMC56XX_HPP
#define MMC56XX_HPP

#include "mmc56xx_i2c.hpp"
#include "mmc56xx_regs.hpp"
#include "stv/drivers/sample_validity.hpp"
#include "stv/gyraccmag_types.hpp"
#include "stv/utils.hpp"
#include <array>
#include <cstdint>

namespace stv {

/// @brief Структура для хранения параметров настройки датчика MMC56xx.
///
/// @details Наследует `stv::mmc56xx_i2c_setup` и содержит конфигурационные
/// параметры, необходимые для инициализации датчика.
struct mmc56xx_setup: public stv::mmc56xx_i2c_setup {
    /// @brief Регистр частоты дискретизации.
    mmc56xx_odr_reg odr_reg;

    /// @brief Регистр управления 0.
    mmc56xx_ctrl0_reg ctrl0_reg;

    /// @brief Регистр управления 1.
    mmc56xx_ctrl1_reg ctrl1_reg;

    /// @brief Регистр управления 2.
    mmc56xx_ctrl2_reg ctrl2_reg;
};

/// @brief Класс для работы с магнитометром MMC56xx.
///
/// @details Этот класс предоставляет высокоуровневый интерфейс для
/// взаимодействия с датчиком MMC56xx по шине I2C.
///
/// @tparam MagType Тип данных для нормализованных измерений магнитного поля.
template<typename MagType>
class mmc56xx: public stv::mmc56xx_i2c, public stv::imag<MagType>
{
    using mag_type       = MagType;
    using base_type      = stv::imag<MagType>;
    using timestamp_type = typename mag_type::timestamp_type;
    using value_type     = typename mag_type::value_type;

    STV_NO_PADDING_NO_OPTIMIZE_BEGIN

    /// @brief Структура для хранения необработанных (raw) данных с
    /// магнитометра.
    struct raw_t {
        /// @brief Значение по оси X.
        std::int32_t x{0x00};

        /// @brief Значение по оси Y.
        std::int32_t y{0x00};

        /// @brief Значение по оси Z.
        std::int32_t z{0x00};

        /// @brief Проверяет валидность данных.
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

    /// @brief Структура для хранения необработанных байтов из регистров.
    KRASLIBS_NO_PADDING_NO_OPTIMIZE_BEGIN

    struct raw_bytes_t {
        std::uint8_t xout0;
        std::uint8_t xout1;
        std::uint8_t yout0;
        std::uint8_t yout1;
        std::uint8_t zout0;
        std::uint8_t zout1;
        std::uint8_t xout2;
        std::uint8_t yout2;
        std::uint8_t zout2;
    };

    KRASLIBS_NO_PADDING_NO_OPTIMIZE_END

    /// @brief Масштабный коэффициент для приведения измерений к Гаусс.
    /// Для MMC56xx: 16384 LSB/G при 20-bit режиме.
    static constexpr value_type lsb_per_g_{16384};

    /// @brief Смещение для преобразования 20-битного беззнакового значения в
    /// знаковое.
    static constexpr std::int32_t null_field_output{524288}; // 2^19

    timestamp_type                timestamp_{};

    /// @brief Внутренний буфер для хранения последних считанных данных.
    raw_t    raw_{};

    mag_type mag_;

    /// @brief Флаг успешной инициализации.
    bool is_init_success_{false};

    /// @brief Копия структуры инициализации.
    stv::mmc56xx_setup init_copy_;

    /// @brief Флаг валидности self-test.
    bool is_self_test_valid_{false};

    /// @brief Преобразует необработанные байты оси в 20-битное знаковое
    /// значение.
    static auto convert_raw_axis(
        std::uint8_t out0, std::uint8_t out1, std::uint8_t out2) -> std::int32_t
    {
        std::int32_t converted = (static_cast<std::uint32_t>(out0) << 12)
                                 | (static_cast<std::uint32_t>(out1) << 4)
                                 | (static_cast<std::uint32_t>(out2) >> 4);

        return converted - null_field_output;
    }

    /// @brief Записывает регистр в память датчика, а затем считывает его и
    /// сравнивает с тем что планировалось записать.
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
    /// @brief Конструктор объекта для работы с датчиком MMC56xx.
    ///
    /// @param[in] setup Конфигурационная структура с параметрами
    /// подключения по шине I2C.
    explicit mmc56xx(
        const stv::mmc56xx_setup &setup):
        mmc56xx_i2c{setup}
    {
    }

    /// @brief Виртуальный деструктор класса.
    ~mmc56xx() override = default;

    /// @brief Проверяет корректность инициализации датчика.
    ///
    /// @return `true` - если датчик был успешно инициализирован.
    explicit operator bool() const override
    {
        auto is_valid{true};
        if(!stv::mmc56xx_i2c::operator bool())
        {
            is_valid = false;
        }

        return is_valid;
    }

    /// @brief Статический метод для проверки наличия датчика MMC56xx на
    /// шине I2C.
    ///
    /// @param[in] i2c Указатель на объект интерфейса I2C.
    /// @param[in] expected_product_id Ожидаемый идентификатор продукта.
    ///
    /// @return `true` - если по адресу ответило устройство с ожидаемым ID.
    static auto is_detected(
        stv::i2c_interface *i2c,
        mmc56xx_reg_type    expected_product_id =
            mmc56xx_product_id_reg::expected_value_mmc5616wa)
    {
        auto                              is_detected{false};
        static constexpr mmc56xx_reg_type chip_id_addr{
            mmc56xx_product_id_reg::addr};
        mmc56xx_reg_type chip_id{0x00};
        const auto       is_success =
            i2c->read(static_cast<stv::i2c_interface::byte_type>(
                          mmc56xx_i2c_setup::i2c_addr),
                      static_cast<stv::i2c_interface::byte_type>(chip_id_addr),
                      &chip_id, sizeof(chip_id));
        if(is_success && (chip_id == expected_product_id))
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
        mmc56xx_ctrl1_reg ctrl1;
        ctrl1.sw_reset = mmc56xx_ctrl1_reg::sw_reset_t::enable;
        return write(ctrl1);
    }

    /// @brief Выполняет инициализацию датчика MMC56xx с заданными
    /// параметрами.
    ///
    /// @param[in] setup Структура `mmc56xx_setup`, содержащая значения для
    /// ключевых регистров датчика.
    /// @param[in] is_need_self_test Если true, выполняет self-test.
    ///
    /// @return `true` - если все регистры были успешно записаны и прочитаны
    /// обратно с проверкой.
    auto init(
        const stv::mmc56xx_setup &setup, bool is_need_self_test = false)
    {
        is_init_success_ = false;
        auto is_init_success{true};

        if(is_init_success)
        {
            is_init_success = write_reg_then_check(setup.odr_reg);
        }

        if(is_init_success)
        {
            is_init_success = write_reg_then_check(setup.ctrl0_reg);
        }

        if(is_init_success)
        {
            is_init_success = write_reg_then_check(setup.ctrl1_reg);
        }

        if(is_init_success)
        {
            is_init_success = write_reg_then_check(setup.ctrl2_reg);
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
        auto status_reg = read_status_reg();
        return status_reg.meas_m_done
               == mmc56xx_status_reg::meas_m_done_t::is_done;
    }

    /// @brief Считывает значение регистра статуса.
    auto read_status_reg() { return read<mmc56xx_status_reg>(); }

    /// @brief Считывает идентификатор чипа.
    auto read_chip_id() { return read<mmc56xx_product_id_reg>(); }

    /// @brief Проверяет валидность идентификатора чипа.
    auto is_chip_id_valid(
        mmc56xx_reg_type expected_id =
            mmc56xx_product_id_reg::expected_value_mmc5616wa)
    { return (read_chip_id().product_id == expected_id); }

    /// @brief Считывает необработанные данные измерений.
    auto read_meas_raw()
    {
        raw_bytes_t raw_bytes{};

        read(0x00, reinterpret_cast<void *>(&raw_bytes), sizeof(raw_bytes));

        raw_t meas_raw{};
        meas_raw.x =
            convert_raw_axis(raw_bytes.xout0, raw_bytes.xout1, raw_bytes.xout2);
        meas_raw.y =
            convert_raw_axis(raw_bytes.yout0, raw_bytes.yout1, raw_bytes.yout2);
        meas_raw.z =
            convert_raw_axis(raw_bytes.zout0, raw_bytes.zout1, raw_bytes.zout2);

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
        return mag_type{static_cast<value_type>(raw_meas.x) / lsb_per_g_,
                        static_cast<value_type>(raw_meas.y) / lsb_per_g_,
                        static_cast<value_type>(raw_meas.z) / lsb_per_g_,
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

    /// @brief Запрашивает однократное измерение магнитного поля.
    auto request_magnetic_field_meas()
    {
        mmc56xx_ctrl0_reg ctrl0;
        ctrl0.take_meas_m = mmc56xx_ctrl0_reg::take_meas_m_t::enable;
        return write(ctrl0);
    }

    /// @brief Отправляет команду "Set" для калибровки магнитометра.
    auto send_do_set()
    {
        mmc56xx_ctrl0_reg ctrl0;
        ctrl0.do_set = mmc56xx_ctrl0_reg::do_set_t::set;
        return write(ctrl0);
    }

    /// @brief Отправляет команду "Reset" для калибровки магнитометра.
    auto send_do_reset()
    {
        mmc56xx_ctrl0_reg ctrl0;
        ctrl0.do_reset = mmc56xx_ctrl0_reg::do_reset_t::set;
        return write(ctrl0);
    }

  private:
    /// @brief Имитация self-test.
    auto self_test_imitation()
    {
        while(!is_data_ready()) {}
        return static_cast<bool>(read_meas_raw());
    }
};

} // namespace stv

#endif /* MMC56XX_HPP */
