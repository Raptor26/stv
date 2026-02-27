/// @file mc3479.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.

#ifndef MC3479_HPP
#define MC3479_HPP

#include "stv/drivers/mc3479_i2c.hpp"
#include "stv/drivers/mc3479_regs.hpp"
#include "stv/drivers/mc3479_types.hpp"
#include "stv/gyraccmag_types.hpp"
#include "stv/utils.hpp"
#include <cstddef>
#include <cstring>
#include <utility>

namespace stv {

struct mc3479_setup: public stv::mc3479_i2c_setup {
};

/// @brief Класс работы с акселерометром mc3479.
template<typename AccType>
class mc3479:
    public stv::mc3479_i2c,
    public stv::iacc<AccType>,
    virtual public stv::non_movable_non_copyable
{
    using acc_type       = AccType;
    using timestamp_type = typename acc_type::timestamp_type;

    STV_NO_PADDING_NO_OPTIMIZE_BEGIN

    struct acc_t {
        static constexpr mc3479_reg_type addr{0x0D};

        std::int16_t                     x;
        std::int16_t                     y;
        std::int16_t                     z;
    };

    STV_NO_PADDING_NO_OPTIMIZE_END

    float lsb_{1.0F / 16384.0F};

    /// @brief Нормированные измерения акселерометра.
    acc_type       acc_;

    timestamp_type timestamp_{};

    auto           read_dev_status_red()
    {
        return mc3479_i2c::read(mc3479_device_status_reg{});
    }

    auto write_reg_then_check(
        const auto &reg)
    {
        write(reg);

        const auto read_val = read(reg);

        const auto is_equal =
            std::memcmp(reinterpret_cast<const void *>(&reg),
                        reinterpret_cast<const void *>(&read_val),
                        sizeof(read_val))
            == 0;

        return is_equal;
    }

    auto write_reg_without_check(
        auto reg)
    {
        write(reg);
        return true;
    }

    void set_mode(
        stv::mc3479_state mode)
    {
        auto val = static_cast<std::uint8_t>(read(mc3479_mode_reg::addr));

        val &= 0b11110000U;
        val |= std::to_underlying(mode);

        write(mc3479_mode_reg::addr, std::byte{val});
    }

    auto get_lsb()
    {
        float      lsb{};
        const auto reg = read(stv::mc3479_range_and_scale_control_reg{});
        switch(reg.range)
        {
            case mc3479_range::g2:
                lsb = 16384.0F;
                break;
            case mc3479_range::g4:
                lsb = 8192.0F;
                break;
            case mc3479_range::g8:
                lsb = 4096.0F;
                break;
            case mc3479_range::g16:
                lsb = 2048.0F;
                break;
            case mc3479_range::g12:
                lsb = 2730.0F;
                break;

            default:
                break;
        }
        return 1.0F / lsb;
    }

    auto normalize(
        const auto &raw)
    {
        return acc_type{raw.x * lsb_, raw.y * lsb_, raw.z * lsb_, timestamp_};
    }

  public:
    explicit mc3479(
        const mc3479_setup &setup):
        stv::mc3479_i2c{setup}
    {
        (void)setup;
    }

    explicit operator bool() const
    {
        return stv::all_true(stv::mc3479_i2c::operator bool());
    }

    static auto is_detected(
        stv::i2c_interface *i2c, mc3479_reg_type slave_addr)
    {
        static constexpr mc3479_reg_type chip_id_valid{0xA4};
        static constexpr mc3479_reg_type chip_id_addr{0x18};
        mc3479_reg_type                  chip_id{0xFF};
        const auto                       is_success =
            i2c->read(slave_addr, chip_id_addr, &chip_id, sizeof(chip_id));

        return stv::all_true(is_success, (chip_id == chip_id_valid));
    }

    template<typename DelayFn>
    void reset(
        DelayFn &delay)
    {
        // Stand by mode
        {
            stv::mc3479_mode_reg reg;
            reg.state = mc3479_state::sleep;
            write(reg);
        }

        delay(10);

        // power-on-reset
        write(std::byte(0x1c), std::byte(0x40));

        delay(50);

        // Disable interrupt
        write(std::byte(0x06), std::byte(0x00));
        delay(10);
        // 1.00x Aanalog Gain
        write(std::byte(0x2B), std::byte(0x00));
        delay(10);

        // DCM disable
        write(std::byte(0x15), std::byte(0x00));

        delay(50);
    }

    auto read_status_reg()
    {
        mc3479_status_reg val{};
        mc3479_i2c::read(mc3479_status_reg::addr,
                         reinterpret_cast<void *>(&val), sizeof(val));
        return val;
    }

    auto read_raw()
    {
        acc_t raw;
        read(acc_t::addr, reinterpret_cast<void *>(&raw), sizeof(raw));
        ++timestamp_;

        if constexpr(std::endian::native == std::endian::big)
        {
            raw.x = std::byteswap(raw.x);
            raw.y = std::byteswap(raw.y);
            raw.z = std::byteswap(raw.z);
        }

        return raw;
    }

    auto read_norm()
    {
        acc_ = normalize(read_raw());

        return acc_;
    }

    /// @brief Перевести датчик в standby (остановить измерения).
    /// @details
    /// Устанавливает режим mc3479_state::standby. Используйте перед записью
    /// конфигурационных регистров или для экономии энергии.
    void stop() { set_mode(stv::mc3479_state::standby); }

    /// @brief Перевести датчик в режим измерений (wake).
    /// @details
    /// Устанавливает режим mc3479_state::wake. Обычно вызывается после init().
    void wake() { set_mode(stv::mc3479_state::wake); }

    /// @brief Получить последнее нормированное измерение.
    /// @details
    /// Возвращает значение, сохранённое в acc_. Чтобы обновить acc_, вызовите
    /// read_norm(). Метод не выполняет I2C-операций.
    ///
    /// @return Последнее измерение в формате AccType.
    [[nodiscard]] auto get_acc() const -> acc_type override { return acc_; }

    /// @brief Инициализация драйвера.
    ///
    /// @param[in] setup Настройки, которыми нужно инициализировать датчик.
    ///
    /// @return true если инициализация успешно завершена, false в противном
    /// случае.
    auto init(
        const stv::mc3479_regs_setup &setup)
    {
        stop();
        const auto is_init_success = stv::all_true(
            write_reg_then_check(setup.interrupt_enable),
            write_reg_without_check(setup.sample_rate),
            write_reg_then_check(setup.motion_control),
            write_reg_without_check(setup.range_and_scale_control),
            write_reg_then_check(setup.mode));

        lsb_ = get_lsb();

        wake();
        return is_init_success;
    }
};

} // namespace stv

#endif /* MC3479_HPP */
