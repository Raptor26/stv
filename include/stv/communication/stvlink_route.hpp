/// @file stvlink_route.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
///
/// NAME
///     stv::stvlink_route -- маршрутизатор пакетов протокола stvlink.
///
/// DESCRIPTION
///     stvlink_route читает очередь распарсенных сообщений stvlink,
///     извлекает из заголовка
///     stv::stvlink_route_tx::routing_header_t идентификатор
///     получателя dst_id и перенаправляет пакет в целевую
///     очередь по хэш-таблице. Если ключ отсутствует в таблице
///     или сообщение пустое, пакет удаляется из входной
///     очереди без передачи получателю.
///
///     stvlink_route_setup<TQueue, THash, TMutexOrPtr>
///         Параметры инициализации маршрутизатора.
///         Задают входную очередь queue_to_read и
///         хэш-таблицу hash_to_write, где ключом
///         служит dst_id, а значением -- указатель на
///         очередь получателя.
///
///     stvlink_route<TSetup>
///         Маршрутизатор сообщений. Метод run()
///         обрабатывает все сообщения во входной
///         очереди. Если dst_id отсутствует в таблице
///         или сообщение пустое, пакет удаляется из
///         входной очереди без передачи получателю.
///         Возвращает количество успешно
///         маршрутизированных сообщений.
///
/// SEE ALSO
///     stvlink_sender.hpp, serial_parser.hpp, test_serial_parser.cpp.

#ifndef STVLINK_ROUTE_HPP
#define STVLINK_ROUTE_HPP

#include "stv/communication/stvlink_sender.hpp"
#include "stv/mutex_guard.hpp"
#include "stv/utils.hpp"
#include <type_traits>
#include <utility>
#include <variant>

namespace stv {

/// @brief Параметры инициализации маршрутизатора сообщений.
///
/// @details
/// Структура связывает очередь с уже распарсенными сообщениями и
/// хэш-таблицу, в которой каждому идентификатору получателя
/// соответствует указатель на целевую очередь. Идентификатор
/// получателя извлекается из заголовка
/// @ref stv::stvlink_route_tx::routing_header_t.
///
/// @tparam TQueue Тип очереди для чтения сообщений.
/// @tparam THash Тип хэш-таблицы: ключ — идентификатор получателя,
///     значение — указатель на очередь типа @c TQueue.
/// @tparam TMutexOrPtr Тип мьютекса или указатель на него.
///     По умолчанию используется @ref stv::empty_mutex.
template<typename TQueue, typename THash,
         typename TMutexOrPtr = stv::empty_mutex>
class stvlink_route_setup
{
    /// @brief Базовый тип мьютекса (без указателя).
    using mutex_base_type = std::remove_pointer_t<TMutexOrPtr>;

    /// @brief true, если пользователь передал указатель на внешний мьютекс.
    static constexpr bool is_external_mutex = std::is_pointer_v<TMutexOrPtr>;

    /// @brief Тип хранения мьютекса в поле @c mutex.
    using mutex_condition_type =
        std::conditional_t<is_external_mutex, mutex_base_type *,
                           std::monostate>;

  public:
    /// @brief Тип очереди для чтения сообщений.
    using queue_type = TQueue;

    /// @brief Тип хэш-таблицы маршрутов.
    using hash_type = THash;

    /// @brief Тип мьютекса или указателя на него.
    using mutex_type = TMutexOrPtr;

    /// @brief Указатель на очередь, из которой читаются сообщения.
    queue_type *queue_to_read{nullptr};

    /// @brief Указатель на хэш-таблицу очередей получателей.
    ///
    /// @details
    /// Ключом служит идентификатор получателя @c dst_id, значением —
    /// указатель на очередь, в которую нужно направить сообщение.
    hash_type *hash_to_write{nullptr};

    /// @brief Внешний мьютекс либо пустой объект.
    ///
    /// @details
    /// В текущей реализации маршрутизатор не использует мьютекс;
    /// поле сохранено для единообразия с @ref serial_parser_setup.
    mutex_type mutex{};
};

/// @brief Маршрутизатор распарсенных сообщений по очередям получателей.
///
/// @details
/// Класс читает сообщения из входной очереди, извлекает из заголовка
/// @ref stv::stvlink_route_tx::routing_header_t идентификатор получателя
/// @c dst_id и перемещает сообщение в соответствующую очередь из
/// хэш-таблицы. Если сообщение не содержит корректного адреса или ключ
/// отсутствует в таблице, оно удаляется из входной очереди без передачи
/// получателю, что предотвращает её переполнение.
///
/// @tparam TSetup Тип параметров инициализации, например
///     @ref stvlink_route_setup.
template<typename TSetup>
class stvlink_route: virtual public stv::non_movable_non_copyable
{
    using setup_type = TSetup;
    using queue_type = typename setup_type::queue_type;
    using hash_type  = typename setup_type::hash_type;

    /// @brief Очередь, из которой считываются входящие сообщения.
    queue_type *const queue_to_read_{nullptr};

    /// @brief Хэш-таблица очередей получателей.
    hash_type *const hash_to_write_{nullptr};

  public:
    /// @brief Конструирует маршрутизатор на основе параметров.
    ///
    /// @param[in] setup Структура с указателями на входную очередь и
    ///     таблицу маршрутов.
    explicit stvlink_route(
        const setup_type &setup):
        queue_to_read_{setup.queue_to_read},
        hash_to_write_{setup.hash_to_write}
    { (void)setup; }

    /// @brief Деструктор по умолчанию.
    virtual ~stvlink_route() = default;

    /// @brief Проверяет корректность инициализации маршрутизатора.
    ///
    /// @return @c true, если оба указателя (очередь и хэш-таблица)
    ///     установлены; иначе @c false.
    explicit operator bool() const
    { return stv::all_true(queue_to_read_, hash_to_write_); }

    /// @brief Выполняет маршрутизацию сообщений из входной очереди.
    ///
    /// @details
    /// Метод обрабатывает все сообщения, находящиеся во входной очереди
    /// на момент вызова. Для каждого сообщения извлекается @c dst_id
    /// из заголовка @ref stv::stvlink_route_tx::routing_header_t.
    /// Если соответствующий ключ найден в хэш-таблице и целевая очередь
    /// не переполнена, сообщение перемещается в неё. В любом случае
    /// сообщение удаляется из входной очереди.
    ///
    /// @return Количество сообщений, успешно направленных получателям.
    auto run()
    {
        auto message_routed_cnt{0U};
        while(!queue_to_read_->empty())
        {
            decltype(auto) msg = queue_to_read_->front();

            const auto    *router_ptr = reinterpret_cast<
                const stv::stvlink_route_tx::routing_header_t *>(msg.data());

            if(router_ptr)
            {
                // Используем итератор, чтобы избежать исключений.
                auto dst_buff_key_val_it = hash_to_write_->find(
                    static_cast<hash_type::key_type>(router_ptr->dst_id));
                if(dst_buff_key_val_it != hash_to_write_->end())
                {
                    if(!dst_buff_key_val_it->second->full())
                    {
                        dst_buff_key_val_it->second->push(std::move(msg));
                        ++message_routed_cnt;
                    }
                    // Целевая очередь переполнена: сообщение отбрасывается
                    // (его буфер освобождается при pop из входной очереди).
                    // Push в полную очередь ETL перезаписывает живой элемент
                    // без вызова деструктора, что приводит к утечке памяти
                    // и порче счетчиков очереди.
                }
            }

            // Независимо от результата маршрутизации удаляем сообщение
            // из входной очереди, чтобы избежать её переполнения.
            queue_to_read_->pop();
        }

        return message_routed_cnt;
    }
};

} // namespace stv

#endif /* STVLINK_ROUTE_HPP */
