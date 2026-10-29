/// @file message_traits.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
///
/// NAME
///     stv::message_traits -- первичный шаблон traits сообщения и концепты.
///
/// DESCRIPTION
///     Протоколонезависимая механика типового реестра сообщений: первичный
///     шаблон message_traits без определения и концепты has_message_traits
///     и has_period.
///
///     Правило «сообщение = тип»: каждое сообщение описывается типом и явной
///     специализацией message_traits с идентификатором и минимальным размером
///     полезной нагрузки.
///
///     Явная специализация шаблона stv::message_traits обязана объявляться
///     в namespace stv (правило C++): интегрирующий проект описывает типы
///     в своём namespace, а специализации traits -- в блоке namespace stv.
///
/// SEE ALSO
///     type_list.hpp (type_list и constexpr-хелперы списков типов).

#ifndef MESSAGE_TRAITS_HPP
#define MESSAGE_TRAITS_HPP

#include <cstddef>

namespace stv {

/// @brief Первичный шаблон traits сообщения.
///
/// @details Определения не имеет: каждый тип сообщения описывается явной
///     специализацией рядом со структурой сообщения (в блоке namespace
///     stv). Обязан задавать поля: msg_id -- идентификатор сообщения
///     (перечисление интегрирующего проекта) и min_pload_size_bytes --
///     минимальный размер полезной нагрузки в байтах (смещение последнего
///     обязательного поля, 0 для сообщения без полей).
template<typename T>
struct message_traits;

/// @brief Концепт: тип зарегистрирован в типовом реестре сообщений.
///
/// @details Удовлетворяется, если для типа определена специализация
///     message_traits с обоими обязательными полями.
template<typename T>
concept has_message_traits = requires {
    message_traits<T>::msg_id;
    message_traits<T>::min_pload_size_bytes;
};

/// @brief Концепт: тип сообщения имеет заданный период периодической
///     передачи.
///
/// @details Удовлетворяется, если специализация message_traits типа задаёт
///     поле period (std::chrono::milliseconds, явно, без default-значений).
///     Служит предикатом выборки периодических передатчиков прошивки из
///     списка сообщений.
template<typename T>
concept has_period = requires { message_traits<T>::period; };

} // namespace stv

#endif // MESSAGE_TRAITS_HPP
