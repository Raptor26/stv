/// @file mc3479_i2c.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.

#ifndef MC3479_I2C_HPP
#define MC3479_I2C_HPP

#include "etl/binary.h"
#include "stv/drivers/mc3479_types.hpp"
#include "stv/i2c.hpp"
#include <cstring>

namespace stv {

struct mc3479_i2c_setup {
    /// @brief I2C адрес устройства, когда вывод A6 подключен к земле (GND).
    static constexpr mc3479_reg_type i2c_addr_connect_to_gnd{0x4c};

    /// @brief I2C адрес устройства, когда вывод A6 подключен к питанию (VDD).
    static constexpr mc3479_reg_type i2c_addr_connect_to_vdd{0x6c};

    /// @brief Указатель на объект интерфейса шины I2C (i2c_interface).
    stv::i2c_interface *i2c{nullptr};

    /// @brief Адрес устройства на шине I2C. По умолчанию используется адрес для
    /// AD6=GND.
    mc3479_reg_type i2c_addr{i2c_addr_connect_to_gnd};
};

class mc3479_i2c:
    public virtual stv::non_copyable,
    public virtual stv::non_movable
{
    /// @brief Указатель на интерфейс шины I2C.
    const stv::i2c_interface *i2c_;

    /// @brief Адрес устройства на шине I2C.
    const mc3479_reg_type i2c_addr_;

    template<typename T>
    [[nodiscard]] auto reverse_bits(
        T value) const
    {
#if defined(MC3479_BITS_REVERS) && (MC3479_BITS_REVERS == 1)
        return mc3479_reg_type{
            etl::reverse_bits(static_cast<std::uint8_t>(value))};
#else
        return mc3479_reg_type{value};
#endif
    }

  public:
    explicit mc3479_i2c(
        const mc3479_i2c_setup &setup):
        i2c_{setup.i2c},
        i2c_addr_{setup.i2c_addr}
    {
    }

    virtual ~mc3479_i2c() = default;

    explicit operator bool() const { return i2c_ != nullptr; }

    /// @brief Чтение последовательности байтов из регистров устройства.
    ///
    /// @param reg_addr Адрес начального регистра для чтения.
    /// @param dst Указатель на буфер в памяти пользователя, куда будут записаны
    /// данные.
    /// @param len Количество байтов для чтения.
    /// @return Результат операции чтения (зависит от реализации i2c_interface).
    auto read(
        mc3479_reg_type reg_addr, void *dst, std::size_t len) const
    {
        return i2c_->read(i2c_addr_, reg_addr, dst, len);
    }

    /// @brief Чтение регистра по указанному адресу.
    ///
    /// @param reg_addr Адрес регистра.
    ///
    /// @return Считанное по указанному адресу значение.
    [[nodiscard]] auto read(
        mc3479_reg_type reg_addr) const
    {
        mc3479_reg_type reg_val{0xFF};
        read(reg_addr, &reg_val, sizeof(reg_val));
        return reverse_bits(reg_val);
    }

    template<typename U>
    [[nodiscard]] auto read(
        U reg) const
    {
        static_assert(sizeof(U) == 1, "Register size must be equal to 1");
        auto read_val = read(reg.addr);
        U    typed_val;
        std::memcpy(reinterpret_cast<void *>(&typed_val),
                    reinterpret_cast<const void *>(&read_val),
                    sizeof(typed_val));
        return typed_val;
    }

    /// @brief Запись одного байта в указанный регистр устройства.
    ///
    /// @param reg_addr Адрес регистра для записи.
    /// @param value Значение для записи в регистр.
    /// @return Результат операции записи (зависит от реализации i2c_interface).
    auto write(
        mc3479_reg_type reg_addr, mc3479_reg_type value)
    {
        return i2c_->write(i2c_addr_, reg_addr, reverse_bits(value));
    }

    template<typename U>
    auto write(
        U reg)
    {
        static_assert(sizeof(U) == 1, "Register size must be equal to 1");
        mc3479_reg_type reg_val{};
        std::memcpy(reinterpret_cast<void *>(&reg_val),
                    reinterpret_cast<const void *>(&reg), sizeof(reg_val));
        write(reg.addr, reg_val);
    }
};

} // namespace stv

#endif /* MC3479_I2C_HPP */
