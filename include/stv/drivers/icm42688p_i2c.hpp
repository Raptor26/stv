/// @file icm42688p_i2c.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.

#ifndef ICM42688P_I2C_HPP
#define ICM42688P_I2C_HPP

#include "i2c_sensor_i2c.hpp"
#include "icm42688p_types.hpp"

namespace stv {

/// @brief Структура для инициализации класса icm42688p_i2c (настройки
/// подключения по I2C).
///
/// @tparam Addr 7-битный I2C адрес устройства (по умолчанию 0x68, вывод
///         AD0 прижат к земле; при подтяжке AD0 к питанию адрес равен 0x69).
template<icm42688p_reg_type Addr = 0x68>
using icm42688p_i2c_setup = i2c_sensor_i2c_setup<icm42688p_reg_type, Addr>;

/// @brief Класс для выполнения операций чтения и записи регистров датчика
/// ICM-42688-P по шине I2C.
///
/// @details Является алиасом на i2c_sensor_i2c с параметрами ICM-42688-P.
///
/// @tparam Addr 7-битный I2C адрес устройства (по умолчанию 0x68).
template<icm42688p_reg_type Addr = 0x68>
using icm42688p_i2c = i2c_sensor_i2c<icm42688p_reg_type, Addr>;

} // namespace stv

#endif /* ICM42688P_I2C_HPP */
