/// @file mmc3630kj_i2c.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.

#ifndef MMC3630KJ_I2C_HPP
#define MMC3630KJ_I2C_HPP

#include "i2c_sensor_i2c.hpp"
#include "mmc3630kj_types.hpp"

namespace stv {

/// @brief Структура для инициализации I2C-подключения к датчику MMC3630KJ.
///
/// @details Алиас на `i2c_sensor_i2c_setup<mmc3630kj_reg_type, 0x30>`.
/// Содержит параметры, необходимые для установления связи с датчиком
/// MMC3630KJ по шине I2C. I2C адрес устройства — 0x30 (7-битный).
/// Шина работает в FAST mode (≤400 кГц) согласно даташиту.
///
/// @note Для корректной работы I2C шины рекомендуется использовать
/// подтягивающие резисторы: 4.7 кОм для короткой шины (<10 см),
/// 10 кОм для шины <5 см.
using mmc3630kj_i2c_setup = i2c_sensor_i2c_setup<mmc3630kj_reg_type, 0x30>;

/// @brief Класс для выполнения операций чтения и записи регистров датчика
/// MMC3630KJ по шине I2C.
///
/// @details Алиас на `i2c_sensor_i2c<mmc3630kj_reg_type, 0x30>`.
/// Предоставляет низкоуровневый интерфейс для обмена данными с датчиком
/// MMC3630KJ через шину I2C. Реализует базовые операции: чтение регистра,
/// запись регистра, чтение блока данных.
///
/// @see mmc3630kj_i2c_setup
using mmc3630kj_i2c = i2c_sensor_i2c<mmc3630kj_reg_type, 0x30>;

} // namespace stv

#endif /* MMC3630KJ_I2C_HPP */
