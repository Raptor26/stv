/// @file stvlinkv2_kit.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
///
/// NAME
///     stv::stvlinkv2_kit -- набор интеграции протокола stvlinkv2.
///
/// DESCRIPTION
///     Шаблоны-утилиты, общие для всех интегрирующих проектов протокола
///     stvlinkv2 (bare-metal прошивки, PC-приложения): тип хэш-таблицы
///     обработчиков stvlink_handler_table, конструктор параметров
///     заголовка сообщения make_msg_setup(), префикс-десериализатор
///     try_deserialize() и сборщик make_handler_table().
///
///     Заголовок ничего не знает о payload-структурах конкретных проектов
///     (tablet_params, перечислениях идентификаторов сообщений) -- они
///     передаются шаблонными параметрами со стороны интегрирующего проекта.
///
/// SEE ALSO
///     stvlink_dispatcher.hpp, stvlinkv2_parser.hpp, stvlinkv2_sender.hpp,
///     message_catalog.hpp (типовой реестр сообщений и сборка таблицы
///     обработчиков поверх каталога),
///     tests/test_message_catalog_communication.cpp (сквозной учебный
///     пример передающей и принимающей сторон обмена сообщениями).

#ifndef STVLINKV2_KIT_HPP
#define STVLINKV2_KIT_HPP

#include "etl/unordered_map.h"
#include "stv/communication/message_traits.hpp"
#include "stv/communication/stvlink_dispatcher.hpp"
#include "stv/communication/stvlinkv2_parser.hpp"
#include "stv/communication/stvlinkv2_sender.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <optional>
#include <span>
#include <tuple>
#include <type_traits>
#include <utility>

namespace stv {

/// @brief Хэш-таблица обработчиков сообщений протокола stvlinkv2.
///
/// @details Ключ -- 16-битный идентификатор сообщения msg_id из заголовка
/// сообщения stvlinkv2, значение -- функция-обработчик полезной нагрузки
/// сообщения (@see stvlink_message_handler_fnc_type).
///
/// @tparam Capacity Максимальное количество обработчиков в таблице.
///
/// Пример:
/// @code{.cpp}
/// // Ёмкость таблицы -- число значений перечисления идентификаторов
/// // протокола (значение max в конце перечисления).
/// constexpr auto capacity = std::to_underlying(my_msg_id::max);
/// using my_handler_table  = stv::stvlink_handler_table<capacity>;
/// @endcode
template<std::size_t Capacity>
using stvlink_handler_table =
    etl::unordered_map<decltype(stvlinkv2_parser::message_header_t::msg_id),
                       stvlink_message_handler_fnc_type, Capacity>;

/// @brief Формирует параметры заголовка сообщения stvlinkv2 из
///     идентификатора сообщения.
///
/// @details Единая точка преобразования перечисления идентификаторов
///     сообщений интегрирующего проекта в параметры заголовка сообщения.
///
/// @tparam MsgIdEnum Перечислимый тип идентификаторов сообщений
///     интегрирующего проекта.
///
/// @param[in] msg_id Идентификатор сообщения.
///
/// @return Параметры заголовка сообщения с заполненным полем msg_id.
///
/// @par Пример
/// Запрос кадра с полезной нагрузкой у буфера сообщений передающей
/// стороны: заголовок (идентификатор сообщения) формируется
/// make_msg_setup, кадр собирается декоратором stvlinkv2_sender в
/// деструкторе объекта сообщения.
/// @code{.cpp}
/// // Очередь кадров и буфер сообщений передающей стороны
/// // (см. serial_sender.hpp).
/// queue_type tx_queue{};
/// auto       tx =
///     stv::make_serial_message_buffer(tx_queue, stv::stvlinkv2_sender{});
///
/// if(auto msg = tx.request<my_telemetry>(stv::make_msg_setup(
///        stv::message_traits<my_telemetry>::msg_id)))
/// {
///     msg->counter     = 42U;
///     msg->temperature = 25;
/// }
/// // Готовый кадр ждёт в tx_queue: его байты передаёт драйвер UART
/// // (в учебном примере -- перекладываются в общий буфер «провода»).
/// @endcode
///
/// Сквозной учебный пример передающей стороны:
/// stv/communication/tests/test_message_catalog_communication.cpp.
template<typename MsgIdEnum>
    requires std::is_enum_v<MsgIdEnum>
constexpr auto make_msg_setup(
    const MsgIdEnum msg_id) -> stvlinkv2_sender::setup_t
{
    static_assert(sizeof(std::underlying_type_t<MsgIdEnum>)
                      <= sizeof(std::uint16_t),
                  "message id type must not be wider than 16 bits "
                  "according to the stvlinkv2 message header format");

    return stvlinkv2_sender::setup_t{
        .msg_id = static_cast<std::uint16_t>(std::to_underlying(msg_id)),
    };
}

/// @brief Десериализует известный префикс полезной нагрузки.
/// @details Короткий вход обрабатывается полностью, неизвестный хвост длинного
///     входа игнорируется. Поля, отсутствующие или обрезанные внутри значения,
///     сохраняют default-значения, назначаемые десериализатором типа.
template<typename T>
    requires requires { T::serialized_size_bytes(); }
auto try_deserialize(
    std::span<std::byte> pload) -> std::optional<T>
{
    if(pload.empty())
    {
        return T{};
    }

    T          value{};
    const auto prefix = pload.first(
        std::min(pload.size_bytes(),
                 static_cast<std::size_t>(T::serialized_size_bytes())));
    std::ignore = value.deserialize_from(prefix.begin(), prefix.end());
    return value;
}

/// @brief Собирает хэш-таблицу обработчиков сообщений stvlinkv2.
///
/// @details Единая точка регистрации: пары (идентификатор сообщения,
///     обработчик) преобразуются в ключ таблицы через std::to_underlying().
///
/// @tparam Capacity Максимальное количество обработчиков в таблице.
/// @tparam MsgIdEnum Перечислимый тип идентификаторов сообщений
///     интегрирующего проекта.
/// @tparam Rest Типы остальных пар (идентификатор, обработчик).
///
/// @param[in] first Первая пара (идентификатор сообщения, обработчик).
/// @param[in] rest Остальные пары (идентификатор сообщения, обработчик).
///
/// @return Заполненная хэш-таблица обработчиков.
///
/// @par Пример
/// Ручная сборка приёмной стороны без каталога сообщений: таблица
/// собирается один раз (идиома функции-аксессора со static) и
/// передаётся модулю диспетчеризации stvlink_module
/// (см. stvlink_dispatcher.hpp).
/// @code{.cpp}
/// // Обработчики полезной нагрузки -- функции вида bool(span).
/// bool handle_telemetry(stv::stvlink_message_span pload);
/// bool handle_command(stv::stvlink_message_span pload);
///
/// // Ёмкость таблицы -- число значений перечисления идентификаторов.
/// constexpr auto capacity = std::to_underlying(my_msg_id::max);
///
/// const auto &handler_table()
/// {
///     const static auto table = stv::make_handler_table<capacity>(
///         std::make_pair(my_msg_id::telemetry, &handle_telemetry),
///         std::make_pair(my_msg_id::command, &handle_command));
///     return table;
/// }
/// @endcode
///
/// Проекты с типовым реестром сообщений собирают таблицу сборщиком
/// make_message_dispatcher_table() поверх каталога
/// (см. message_catalog.hpp): там проверка минимального размера
/// полезной нагрузки и полноты приёмной логики стороны выполняются
/// автоматически, а ручные пары (идентификатор, обработчик) не нужны.
template<std::size_t Capacity, typename MsgIdEnum, typename... Rest>
auto make_handler_table(
    std::pair<MsgIdEnum, stvlink_message_handler_fnc_type> first,
    const Rest &...rest) -> stvlink_handler_table<Capacity>
{
    stvlink_handler_table<Capacity> table{};

    const auto                      insert_pair = [&table](const auto &pair) {
        table.insert({
            static_cast<stvlink_handler_table<Capacity>::key_type>(
                std::to_underlying(pair.first)),
            pair.second,
        });
    };

    insert_pair(first);
    (insert_pair(rest), ...);

    return table;
}

} // namespace stv

#endif /* STVLINKV2_KIT_HPP */
