/// @file qmc5883_i2c.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.

#ifndef QMC5883_I2C_HPP
#define QMC5883_I2C_HPP

#include "i2c_sensor_i2c.hpp"
#include "qmc5883_types.hpp"

namespace stv {

/// @brief Структура для инициализации класса qmc5883_i2c (настройки подключения
/// по I2C).
///
/// @details Содержит параметры, необходимые для установления связи с датчиком
/// QMC5883 по шине I2C. Используется конструктором класса qmc5883_i2c.
using qmc5883_i2c_setup = i2c_sensor_i2c_setup<qmc5883_reg_type, 0x0D>;

/// @brief Класс для выполнения операций чтения и записи регистров датчика
/// QMC5883 по шине I2C.
///
/// @details Этот класс предоставляет высокоуровневый интерфейс для обмена
/// данными с датчиком QMC5883 через шину I2C. Является алиасом на
/// i2c_sensor_i2c с параметрами QMC5883.
using qmc5883_i2c = i2c_sensor_i2c<qmc5883_reg_type, 0x0D>;

} // namespace stv

#endif /* QMC5883_I2C_HPP */
