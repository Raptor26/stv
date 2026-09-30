/// @file rm3100_i2c.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
///
/// @brief I2C-транспорт для магнитометра RM3100.
///
/// @details
///     Заголовок предоставляет шаблонные псевдонимы
///     @ref stv::rm3100_i2c_setup и @ref stv::rm3100_i2c, которые
///     специализируют универсальные структуру @ref stv::i2c_sensor_i2c_setup
///     и класс @ref stv::i2c_sensor_i2c для работы с магнитометром PNI
///     RM3100: тип регистра фиксируется как @ref stv::rm3100_reg_type, а
///     7-битный адрес устройства задаётся параметром шаблона. Базовый адрес
///     0x20 соответствует подключению адресных выводов A0–A2 к GND
///     (используется на платах V1_1 и palka).
///
///     Транспорт выполняет только обмен с регистрами датчика и не содержит
///     логики измерений: высокоуровневый драйвер @ref stv::rm3100
///     (rm3100.hpp) наследуется от @ref stv::rm3100_i2c. Для работы
///     транспорта пользователь должен передать в структуру настроек указатель
///     на собственную реализацию интерфейса @ref stv::i2c_interface
///     (stv/i2c.hpp), инкапсулирующую аппаратную шину конкретной платы.
///
/// @par Пример реализации интерфейса шины в пользовательском коде:
/// @code
/// // Упрощённый аналог класса board::i2c0 с платы palka:
/// class my_i2c: public stv::i2c_interface {
///   public:
///     auto write(byte_type slave_addr, byte_type reg_addr,
///                byte_type write_reg_value) const -> bool override
///     {
///         // Аппаратная передача: START → адрес+W → регистр → значение
///         // → STOP. Возвращает true при успешном обмене.
///         return board_i2c_write(slave_addr, reg_addr, write_reg_value);
///     }
///
///     auto read(byte_type slave_addr, byte_type reg_addr, void *dst,
///               std::size_t len) const -> bool override
///     {
///         // Аппаратный приём: START → адрес+W → регистр → Repeated
///         // START → адрес+R → чтение len байт → STOP.
///         return board_i2c_read(slave_addr, reg_addr, dst, len);
///     }
/// };
/// @endcode
///
/// @par Пример использования:
/// @code
/// // Для платы с уже инициализированной шиной I2C0:
/// auto &i2c_bus = board::i2c0_singleton::instance();
///
/// // Параметры подключения и транспорт для прямого доступа к регистрам:
/// stv::rm3100_i2c_setup<> i2c_setup{.i2c = &i2c_bus};
/// stv::rm3100_i2c<>       transport{i2c_setup};
///
/// // Те же параметры принимает драйвер верхнего уровня stv::rm3100:
/// stv::rm3100_setup<stv::mag<float, std::uint32_t>> setup{{.i2c = &i2c_bus}};
/// stv::rm3100<decltype(setup)> sensor{setup};
/// @endcode

#ifndef RM3100_I2C_HPP
#define RM3100_I2C_HPP

#include "i2c_sensor_i2c.hpp"
#include "rm3100_types.hpp"

namespace stv {

/// @brief Параметры подключения магнитометра RM3100 к шине I2C.
///
/// @details
///     Псевдоним специализации структуры @ref stv::i2c_sensor_i2c_setup с
///     типом регистра @ref stv::rm3100_reg_type (@c std::uint8_t) и 7-битным
///     адресом @p Addr. Структура содержит:
///     - статическое поле @c i2c_addr — 7-битный адрес устройства на шине;
///     - поле @c i2c — указатель на пользовательскую реализацию
///       @ref stv::i2c_interface, через которую выполняется обмен.
///
///     Базовый адрес 0x20 используется, когда адресные выводы датчика A0–A2
///     подключены к GND (согласно схемам плат V1_1 и palka). При другом
///     подключении адресных выводов фактический адрес передаётся параметром
///     шаблона, например: @c rm3100_i2c_setup<0x21>.
///
/// @tparam Addr 7-битный I2C-адрес устройства (по умолчанию 0x20).
///
/// @see stv::i2c_sensor_i2c_setup
/// @see stv::i2c_interface
template<rm3100_reg_type Addr = 0x20>
using rm3100_i2c_setup = i2c_sensor_i2c_setup<rm3100_reg_type, Addr>;

/// @brief Транспортный класс для обмена с магнитометром RM3100 по шине I2C.
///
/// @details
///     Псевдоним специализации @ref stv::i2c_sensor_i2c с типом регистра
///     @ref stv::rm3100_reg_type и 7-битным адресом @p Addr. Класс
///     инкапсулирует низкоуровневый обмен по шине и предоставляет
///     типобезопасные методы доступа к регистрам датчика:
///     - @c read(reg_addr, dst, len) — чтение блока из @c len регистров,
///       начиная с адреса @c reg_addr, в буфер @c dst;
///     - @c read(reg_addr) — чтение одного регистра;
///     - @c read<U>() — чтение регистра с преобразованием в структурный
///       тип @c U (регистровые структуры определены в rm3100_regs.hpp);
///     - @c write(reg_addr, value) — запись одного регистра;
///     - @c write(reg) — запись регистровой структуры.
///
///     Оператор @c bool показывает, что транспорт создан с валидным
///     указателем на @ref stv::i2c_interface. Обычно класс используется не
///     напрямую, а как базовый для драйвера @ref stv::rm3100.
///
/// @tparam Addr 7-битный I2C-адрес устройства (по умолчанию 0x20).
///
/// @see stv::i2c_sensor_i2c
/// @see stv::rm3100
template<rm3100_reg_type Addr = 0x20>
using rm3100_i2c = i2c_sensor_i2c<rm3100_reg_type, Addr>;

} // namespace stv

#endif /* RM3100_I2C_HPP */
