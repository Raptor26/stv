/// @file stvlink_dispatcher.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
///
/// NAME
///     stv::stvlink_dispatcher -- диспетчер сообщений протокола
///     stvlink.
///
/// DESCRIPTION
///     Payload-независимые классы приёмной стороны протокола stvlink:
///     тип представления сообщения stvlink_message_span, тип
///     функции-обработчика stvlink_message_handler_fnc_type,
///     диспетчер stvlink_dispatcher, выбирающий обработчик по полю
///     msg_id заголовка сообщения (stvlink_sender::message_header_t),
///     базовый класс модуля stvlink_base, структура настройки
///     stvlink_setup и модуль stvlink_module, обрабатывающий очередь
///     парсера.
///
///     Диспетчер читает идентификатор сообщения msg_id из заголовка
///     сообщения, находит обработчик в хэш-таблице, срезает заголовок
///     сообщения (trim_head) и вызывает обработчик с полезной
///     нагрузкой. Если обработчик для идентификатора не зарегистрирован,
///     сообщение отбрасывается. Формат кадра на проводе не меняется.
///
///     Семантика полезной нагрузки протоколу не принадлежит:
///     payload-структуры (например, протокола Cambridge) хранятся в
///     интегрирующих библиотеках и проектах.
///
/// SEE ALSO
///     stvlink_sender.hpp, stvlink_parser.hpp, serial_parser.hpp.

#ifndef STVLINK_DISPATCHER_HPP
#define STVLINK_DISPATCHER_HPP

#include "etl/queue.h"
#include "stv/communication/stvlink_sender.hpp"
#include "stv/utils.hpp"
#include <cstddef>
#include <span>

namespace stv {

/// @brief Тип представления для сообщений протокола stvlink.
/// @details std::span<std::byte> обеспечивает безопасный доступ к
/// неизменяемому буферу данных без копирования. Используется для передачи
/// полезных нагрузок между компонентами системы.
using stvlink_message_span = std::span<std::byte>;

/// @brief Тип указателя на функцию обработчика сообщения.
/// @details Функция принимает span с данными сообщения и возвращает статус
/// обработки. Используется в хэш-таблице, связывающей идентификаторы
/// сообщений с обработчиками.
/// @return true если сообщение обработано успешно, false при ошибке.
using stvlink_message_handler_fnc_type = bool (*)(stvlink_message_span);

/// @brief Класс-диспетчер сообщений протокола stvlink.
///
/// @details Диспетчер извлекает сообщение из очереди парсера, читает из
/// заголовка сообщения идентификатор сообщения (msg_id) и вызывает
/// соответствующий обработчик из хэш-таблицы. Если обработчик для
/// идентификатора не зарегистрирован, сообщение отбрасывается.
///
/// @tparam HashTable Тип хэш-таблицы, связывающей идентификаторы сообщений с
/// обработчиками. Должен поддерживать метод find() и итерацию.
/// @tparam SimBuffer Тип буфера, содержащего сообщения. Должен предоставлять
/// методы data(), size(), trim_head().
///
/// @code
/// // Определение обработчика
/// bool my_handler(stv::stvlink_message_span msg) {
///     // Обработка данных...
///     return true;
/// }
///
/// // Инициализация таблицы обработчиков
/// etl::unordered_map<int, stv::stvlink_message_handler_fnc_type, 10>
///     handlers;
/// handlers.insert({0x01, my_handler});
///
/// // Создание диспетчера
/// stvlink_dispatcher dispatcher(&handlers);
///
/// // Обработка сообщения
/// buffer_type msg = ...; // сообщение с заголовком сообщения stvlink
/// dispatcher.dispatch_message(msg);
/// @endcode
template<typename HashTable, typename SimBuffer>
class stvlink_dispatcher
{
    using hash_table_type = HashTable;
    using buffer_type     = SimBuffer;

    /// @brief Указатель на хэш-таблицу обработчиков.
    /// @details Не владеет памятью — время жизни таблицы должно превышать
    /// время жизни диспетчера.
    const hash_table_type *handlers_;

  public:
    /// @brief Конструктор класса.
    ///
    /// @param[in] handlers Указатель на хэш-таблицу обработчиков сообщений.
    /// @pre handlers не должен быть nullptr для корректной работы.
    explicit stvlink_dispatcher(
        const hash_table_type *handlers):
        handlers_{handlers}
    {
    }

    /// @brief Виртуальный деструктор.
    /// @details Обеспечивает корректное уничтожение производных классов.
    virtual ~stvlink_dispatcher() = default;

    /// @brief Проверяет корректность инициализации класса.
    ///
    /// @return true если диспетчер корректно инициализирован (handlers_ !=
    /// nullptr), false в противном случае.
    /// @note Оператор приведения к bool позволяет использовать объект в
    /// условных выражениях: if (dispatcher) { ... }
    explicit operator bool() const { return stv::all_true(handlers_); }

    /// @brief Метод вызывает обработчик, который соответствует полю msg_id в
    /// заголовке сообщения msg.
    ///
    /// @details Метод читает заголовок сообщения stvlink
    /// (@see stv::stvlink_sender::message_header_t), извлекает
    /// идентификатор сообщения и находит соответствующий обработчик в
    /// хэш-таблице. Если обработчик найден, он вызывается с полезной
    /// нагрузкой сообщения (без заголовка).
    ///
    /// @param[in] msg Контейнер, который содержит заголовок сообщения
    /// (@see stv::stvlink_sender::message_header_t) и следом полезную
    /// нагрузку сообщения.
    ///
    /// @return true если сообщение успешно обработано, false в противном
    /// случае (например, если обработчик не найден, вызвал ошибку, или
    /// сообщение невалидно).
    ///
    /// @attention Метод модифицирует входной буфер, удаляя заголовок
    /// сообщения через trim_head(). После вызова исходное сообщение
    /// больше не содержит заголовка.
    bool dispatch_message(
        buffer_type &msg)
    {
        const auto *msg_header =
            reinterpret_cast<const stv::stvlink_sender::message_header_t *>(
                msg.data());

        const auto msg_id = msg_header->msg_id;

        auto       success{false};

        // Используем итератор чтобы избежать выброса исключений.
        const auto handler_it =
            handlers_->find(static_cast<hash_table_type::key_type>(msg_id));
        if(handler_it != handlers_->end())
        {
            // Заголовок сообщения уже считан, поэтому срежем его в
            // сообщении и передадим в обработчик только полезную нагрузку.
            msg.trim_head(stv::stvlink_sender::message_header_size());

            success = handler_it->second(stv::stvlink_message_span{
                reinterpret_cast<std::byte *>(msg.data()),
                msg.size(),
            });
        }

        return success;
    }
};

/// @brief Базовый класс для реализации модуля протокола stvlink.
///
/// @details Этот класс предоставляет основную функциональность для
/// обработки очереди сообщений с использованием диспетчера сообщений.
/// Является абстрактным базовым классом, который нельзя копировать или
/// перемещать (наследуется от stv::non_movable_non_copyable).
///
/// @tparam QueueBase Тип очереди, используемой для хранения сообщений.
/// Должен поддерживать методы empty(), front(), pop().
/// @tparam ProcessingHash Тип хэш-таблицы обработчиков сообщений.
///
/// @par Архитектура:
/// Класс реализует шаблонный метод process_one(), который извлекает
/// сообщение из очереди и передаёт его диспетчеру. Производные классы
/// могут переопределять поведение через виртуальные методы.
template<typename QueueBase, typename ProcessingHash>
class stvlink_base: virtual public stv::non_movable_non_copyable
{
    using queue_base_type       = QueueBase;
    using queue_base_value_type = typename queue_base_type::value_type;
    using processing_hash_type  = ProcessingHash;
    using dispatcher_type =
        stv::stvlink_dispatcher<processing_hash_type, queue_base_value_type>;

    /// @brief Очередь, в которой хранятся сообщения для обработки.
    /// @details Не владеет очередью — ссылка на внешний объект. Время жизни
    /// очереди должно превышать время жизни этого класса.
    queue_base_type &queue_;

    /// @brief Диспетчер сообщений: читает идентификатор сообщения из
    /// заголовка и вызывает соответствующий обработчик из хэш-таблицы.
    /// @details Инициализируется в конструкторе, не может быть изменён после
    /// создания.
    dispatcher_type dispatcher_;

  protected:
    /// @brief Конструктор класса.
    ///
    /// @param[in] queue Ссылка на очередь сообщений.
    /// @param[in] processing_hash_table Указатель на хэш-таблицу обработчиков.
    /// @pre processing_hash_table не должен быть nullptr.
    stvlink_base(
        queue_base_type            &queue,
        const processing_hash_type *processing_hash_table):
        queue_{queue},
        dispatcher_{processing_hash_table}
    {
    }

  public:
    /// @brief Проверяет корректность инициализации класса.
    ///
    /// @return true если базовый класс корректно инициализирован (диспетчер
    /// валиден), false в противном случае.
    explicit operator bool() const { return dispatcher_.operator bool(); }

    /// @brief Обработчик сообщений в очереди.
    ///
    /// @details Метод извлекает одно сообщение из очереди, передает его
    /// диспетчеру и удаляет из очереди. Если очередь пуста, возвращает false.
    /// Обработка происходит по принципу FIFO.
    ///
    /// @return true если сообщение в очереди успешно обработано, false если
    /// очередь пуста или обработка завершилась ошибкой.
    ///
    /// @attention Метод удаляет сообщение из очереди независимо от результата
    /// обработки. Неудачно обработанные сообщения не повторяются.
    auto process_one()
    {
        auto is_success{false};

        if(!queue_.empty())
        {
            decltype(auto) msg = queue_.front();
            if(msg)
            {
                is_success = dispatcher_.dispatch_message(msg);
            }
            queue_.pop();
        }

        return is_success;
    }

    /// @brief Получает ссылку на внутреннюю очередь сообщений.
    ///
    /// @return Ссылку на очередь сообщений.
    /// @note Позволяет внешнему коду добавлять сообщения в очередь или
    /// проверять её состояние.
    queue_base_type &queue() { return queue_; }

    /// @brief Виртуальный деструктор.
    /// @details Обеспечивает корректное уничтожение производных классов.
    virtual ~stvlink_base() = default;
};

/// @brief Структура настройки для инициализации модуля протокола stvlink.
///
/// @details Содержит параметры, необходимые для создания экземпляра
/// модуля протокола stvlink, включая хэш-таблицу обработчиков сообщений.
///
/// @tparam ProcessingHash Тип хэш-таблицы обработчиков сообщений.
/// Может быть следующим значением: etl::iunordered_map<int,
/// stv::stvlink_message_handler_fnc_type>
///
/// @code
/// using hash_table_t =
///     etl::unordered_map<int, stv::stvlink_message_handler_fnc_type, 16>;
///
/// hash_table_t handlers;
/// handlers.insert({0x01, my_handler_function});
///
/// stvlink_setup<hash_table_t> setup;
/// setup.hash_table = &handlers;
///
/// stvlink_module<queue_type, stvlink_setup<hash_table_t>> module(queue,
///                                                                setup);
/// @endcode
template<typename ProcessingHash>
struct stvlink_setup {
    /// @brief Тип хэш-таблицы обработчиков сообщений.
    /// @details Алиас для удобства использования в шаблонных контекстах.
    using processing_msg_hash_table_type = ProcessingHash;

    /// @brief Указатель на хэш-таблицу обработчиков сообщений.
    /// @details Должен быть проинициализирован пользователем перед
    /// использованием.
    /// @warning Время жизни инициализированной пользователем таблицы должно
    /// быть равно или больше времени жизни модуля stvlink. Нарушение этого
    /// требования приведёт к неопределённому поведению (use-after-free).
    const processing_msg_hash_table_type *hash_table{nullptr};
};

/// @brief Основной класс модуля протокола stvlink.
///
/// @details Этот класс объединяет все компоненты приёмной стороны протокола
/// stvlink: очередь сообщений парсера, диспетчер и хэш-таблицу обработчиков.
/// Наследуется от базового класса и предоставляет конкретную реализацию
/// очереди с фиксированным размером на основе ETL.
///
/// @tparam TQueue Тип очереди сообщений.
/// @tparam TSetup Тип структуры настроек (@see stvlink_setup).
///
/// @par Потокобезопасность:
/// Класс не является потокобезопасным. Синхронизация доступа к экземплярам
/// должна обеспечиваться на уровне вызывающего кода при использовании в
/// многопоточной среде.
///
/// @code
/// using setup_t = stvlink_setup<my_hash_table_type>;
/// using queue_t = etl::queue<stv::sim_buff<stv::empty_mutex>, 20>;
///
/// setup_t setup;
/// setup.hash_table = &my_handlers;
///
/// queue_t queue;
/// stvlink_module<queue_t, setup_t> module(queue, setup);
///
/// if (module) {
///     while (module.process_one()) {
///         // Обработка сообщений...
///     }
/// }
/// @endcode
template<typename TQueue, typename TSetup>
class stvlink_module:
    public stvlink_base<etl::iqueue<typename TQueue::value_type>,
                        typename TSetup::processing_msg_hash_table_type>
{
    using setup_type = TSetup;
    using queue_type = TQueue;
    using queue_base_type =
        typename etl::iqueue<typename queue_type::value_type>;
    using processing_msg_hash_table_type =
        typename setup_type::processing_msg_hash_table_type;
    using base_type =
        stvlink_base<queue_base_type, processing_msg_hash_table_type>;

  public:
    /// @brief Конструктор класса.
    ///
    /// @param[in] queue Ссылка на очередь сообщений.
    /// @param[in] setup Структура настроек, содержащая хэш-таблицу
    /// обработчиков.
    /// @pre setup.hash_table не должен быть nullptr.
    explicit stvlink_module(
        queue_type &queue, const setup_type &setup):
        base_type{queue, setup.hash_table}
    {
    }

    /// @brief Виртуальный деструктор класса.
    /// @details Обеспечивает корректное уничтожение объекта.
    ~stvlink_module() override = default;

    /// @brief Проверяет корректность инициализации класса.
    ///
    /// @return true если класс корректно инициализирован (базовый класс
    /// валиден), false в противном случае.
    explicit operator bool() const { return base_type::operator bool(); }
};

} // namespace stv

#endif /* STVLINK_DISPATCHER_HPP */
