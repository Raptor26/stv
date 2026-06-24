/// @file i2c.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.

/// NAME
///     stv::i2c_interface - абстрактный интерфейс шины I2C.
///
/// SYNOPSIS
///     #include <stv/i2c.hpp>
///
///     namespace stv {
///     class i2c_interface;
///     }
///
/// DESCRIPTION
///     stv::i2c_interface задает контракт для низкоуровневого
///     драйвера шины I2C. Класс наследуется от
///     stv::non_movable_non_copyable, поэтому объекты интерфейса нельзя
///     копировать или перемещать. Реализации должны предоставить два
///     чисто виртуальных метода:
///     - write(slave_addr, reg_addr, write_reg_value): запись байта в
///       регистр ведомого устройства;
///     - read(slave_addr, reg_addr, dst, len): чтение блока регистров
///       ведомого устройства.
///
///     Адрес ведомого (slave_addr) задается в 7-битном формате. Все
///     методы возвращают true при успехе и false при ошибке. Тип
///     byte_type определен как std::byte.
///
///     Интерфейс используется классом i2c_sensor_i2c, который
///     предоставляет высокоуровневые шаблонные обертки для работы с
///     регистрами конкретных датчиков.
///
/// SEE ALSO
///     stv::i2c_sensor_i2c(3), stv::non_movable_non_copyable(3).
///
/// AUTHORS
///     Mickle Isaev (mrraptor26@gmail.com).

#ifndef I2C_HPP
#define I2C_HPP

#include "stv/utils.hpp"
#include <cstddef>

namespace stv {

class i2c_interface: public stv::non_movable_non_copyable
{
  public:
    using byte_type = std::byte;

    virtual ~i2c_interface() = default;

    /// @brief Записывает значение в указанный регистр устройства.
    ///
    /// @param[in] slave_addr Адрес ведомого устройства в 7-ми битном формате.
    /// @param[in] reg_addr Адрес регистра для записи.
    /// @param[in] write_reg_value Значение для записи.
    ///
    /// @return true, если операция успешно завершена, false в противном
    ///         случае.
    [[nodiscard("Check i2c write operation status")]] virtual auto
    write(byte_type slave_addr, byte_type reg_addr,
          byte_type write_reg_value) const -> bool = 0;

    /// @brief Считывает указанное количество регистров начиная с адреса.
    ///
    /// @param[in] slave_addr Адрес ведомого устройства в 7-ми битном формате.
    /// @param[in] reg_addr Адрес первого регистра для чтения.
    /// @param[out] dst Указатель на буфер для записи данных.
    /// @param[in] len Количество регистров для чтения.
    ///
    /// @return true, если операция успешно завершена, false в противном
    ///         случае.
    [[nodiscard("Check i2c read operation status")]] virtual auto
    read(byte_type slave_addr, byte_type reg_addr, void *dst,
         std::size_t len) const -> bool = 0;

  protected:
    i2c_interface() = default;
};

} // namespace stv

#endif /* I2C_HPP */
