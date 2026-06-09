/// @file mmc56xx_i2c.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.

#ifndef MMC56XX_I2C_HPP
#define MMC56XX_I2C_HPP

#include "i2c_sensor_i2c.hpp"
#include "mmc56xx_types.hpp"

namespace stv {

/// @brief Структура для инициализации класса mmc56xx_i2c (настройки подключения
/// по I2C).
using mmc56xx_i2c_setup = i2c_sensor_i2c_setup<mmc56xx_reg_type, 0x30>;

/// @brief Класс для выполнения операций чтения и записи регистров датчика
/// MMC56xx по шине I2C.
///
/// @details Является алиасом на i2c_sensor_i2c с параметрами MMC56xx.
using mmc56xx_i2c = i2c_sensor_i2c<mmc56xx_reg_type, 0x30>;

} // namespace stv

#endif /* MMC56XX_I2C_HPP */
