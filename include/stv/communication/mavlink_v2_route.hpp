/// @file mavlink_v2_route.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
///
/// NAME
///     stv::mavlink_v2_route -- маршрутизатор сообщений протокола
///     MAVLink v2.
///
/// DESCRIPTION
///     mavlink_v2_route читает очередь распарсенных кадров MAVLink v2
///     (stv::parsed_queue<stv::mavlink_v2_frame>), извлекает из заголовка
///     идентификатор компонента-источника compid и перенаправляет пакет в
///     целевую очередь по хэш-таблице. Широковещательная рассылка не
///     поддерживается.
///
/// SEE ALSO
///     mavlink_v2_frame.hpp, parsed_queue.hpp, serial_parser.hpp.

#ifndef MAVLINK_V2_ROUTE_HPP
#define MAVLINK_V2_ROUTE_HPP

#include "stv/communication/parsed_queue.hpp"
#include "stv/mutex_guard.hpp"
#include "stv/utils.hpp"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <utility>

namespace stv {

/// @brief Параметры инициализации маршрутизатора MAVLink v2.
///
/// @details
/// Структура связывает типизированную входную очередь
/// @ref parsed_queue<stv::mavlink_v2_frame> и хэш-таблицу, в которой каждому
/// идентификатору компонента-источника compid соответствует указатель на
/// целевую очередь. Использование @ref parsed_queue гарантирует на этапе
/// компиляции, что в маршрутизатор подаётся очередь кадров MAVLink v2.
///
/// @tparam TFrame Тип декоратора кадра, например
///     @c stv::mavlink_v2_frame<TCrcExtraProvider>.
/// @tparam TQueue Тип очереди для чтения сообщений.
/// @tparam THash Тип хэш-таблицы: ключ — идентификатор компонента,
///     значение — указатель на очередь типа @c TQueue.
/// @tparam TMutexOrPtr Тип мьютекса или указатель на него.
///     По умолчанию используется @ref stv::empty_mutex.
template<typename TFrame, typename TQueue, typename THash,
         typename TMutexOrPtr = stv::empty_mutex>
class mavlink_v2_route_setup
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
    /// @brief Тип декоратора кадра MAVLink v2.
    using frame_type = TFrame;

    /// @brief Тип очереди для чтения сообщений.
    using queue_type = TQueue;

    /// @brief Тип хэш-таблицы маршрутов.
    using hash_type = THash;

    /// @brief Тип мьютекса или указателя на него.
    using mutex_type = TMutexOrPtr;

    /// @brief Тип типизированной входной очереди кадров MAVLink v2.
    using queue_to_read_type = parsed_queue<frame_type, TQueue>;

    /// @brief Типизированная входная очередь кадров MAVLink v2.
    queue_to_read_type queue_to_read;

    /// @brief Указатель на хэш-таблицу очередей получателей.
    ///
    /// @details
    /// Ключом служит идентификатор компонента-источника @c compid,
    /// значением — указатель на очередь, в которую нужно направить
    /// сообщение.
    hash_type *hash_to_write{nullptr};

    /// @brief Внешний мьютекс либо пустой объект.
    ///
    /// @details
    /// При использовании внешнего мьютекса следует передать его адрес.
    /// Метод @ref mavlink_v2_route::run() захватывает мьютекс на всё время
    /// доступа к хэш-таблице и очередям. Мьютекс сериализует только
    /// стороны, захватывающие тот же мьютекс: поток-производитель,
    /// помещающий сообщения во входную очередь (например, serial_parser
    /// в другой задаче), этим мьютексом не защищается.
    /// Потокобезопасность канала парсер→маршрутизатор определяется
    /// выбором потокобезопасного типа очереди (например,
    /// etl::queue_spsc_atomic или etl::queue_mpmc_mutex), а не этим
    /// мьютексом. При использовании @ref stv::empty_mutex синхронизация
    /// отсутствует.
    mutex_condition_type mutex{};
};

/// @brief Маршрутизатор распарсенных кадров MAVLink v2 по очередям
///     получателей.
///
/// @details
/// Класс читает сообщения из входной очереди, извлекает из заголовка
/// маршрутизации идентификатор компонента-источника @c compid и перемещает
/// сообщение в соответствующую очередь из хэш-таблицы. После отрезания
/// парсером заголовка кадра (0xFD, len, incompat_flags) и хвоста CRC
/// раскладка сообщения: [compat_flags, seq, sysid, compid, msgid (3 байта,
/// LE), payload], поэтому @c compid расположен по смещению 3. Если ключ
/// отсутствует или целевая очередь переполнена, сообщение удаляется из
/// входной очереди без передачи получателю, что предотвращает её
/// переполнение. Широковещательная рассылка не реализована.
///
/// Если в setup передан внешний мьютекс, метод @c run() захватывает его
/// на всё время обхода хэш-таблицы и доступа к очередям. Мьютекс
/// сериализует только стороны, захватывающие тот же мьютекс; канал
/// парсер→маршрутизатор он не защищает — потокобезопасность входной
/// очереди обеспечивается выбором её типа (SPSC/MPMC). При использовании
/// @ref stv::empty_mutex накладных расходов на синхронизацию нет.
///
/// @tparam TSetup Тип параметров инициализации, например
///     @ref mavlink_v2_route_setup.
template<typename TSetup>
class mavlink_v2_route: virtual public stv::non_movable_non_copyable
{
    using setup_type = TSetup;
    using frame_type = typename setup_type::frame_type;
    using queue_type = typename setup_type::queue_type;
    using hash_type  = typename setup_type::hash_type;
    using mutex_type = typename setup_type::mutex_type;

    /// @brief Смещение байта compid в сообщении после отрезания парсером
    ///     заголовка и CRC.
    static constexpr std::size_t compid_offset{3U};

    /// @brief Размер заголовка маршрутизации в отрезанном сообщении
    ///     (compat_flags, seq, sysid, compid, msgid).
    static constexpr std::size_t routing_header_size{7U};

    /// @brief Очередь, из которой считываются входящие сообщения.
    queue_type *const queue_to_read_{nullptr};

    /// @brief Хэш-таблица очередей получателей.
    hash_type *const hash_to_write_{nullptr};

    /// @brief Мьютекс или указатель на него.
    mutable mutex_type mutex_{};

    /// @brief Возвращает ссылку на реальный мьютекс.
    ///
    /// @details
    /// Если @c mutex_ является указателем, разыменовывает его; иначе
    /// возвращает сам объект. Используется для единообразной работы
    /// с @ref stv::lock_guard.
    ///
    /// @return Ссылка на мьютекс.
    auto get_mutex_ref() const -> std::remove_pointer_t<mutex_type> &
    {
        if constexpr(std::is_pointer_v<mutex_type>)
        {
            assert(mutex_ != nullptr);
            return *mutex_;
        }
        else
        {
            return mutex_;
        }
    }

  public:
    /// @brief Конструирует маршрутизатор на основе параметров.
    ///
    /// @param[in] setup Структура с указателями на входную очередь и
    ///     таблицу маршрутов.
    explicit mavlink_v2_route(
        const setup_type &setup):
        queue_to_read_{
            (setup.queue_to_read.queue() != nullptr)
                ? static_cast<queue_type *>(setup.queue_to_read.queue())
                : nullptr,
        },
        hash_to_write_{setup.hash_to_write}
    {
        static_assert(std::is_same_v<typename setup_type::queue_to_read_type,
                                     parsed_queue<frame_type, queue_type>>,
                      "queue_to_read must be a parsed_queue<TFrame, TQueue>");
        if constexpr(std::is_pointer_v<mutex_type>)
        {
            mutex_ = setup.mutex;
        }
    }

    /// @brief Деструктор по умолчанию.
    virtual ~mavlink_v2_route() = default;

    /// @brief Проверяет корректность инициализации маршрутизатора.
    ///
    /// @return @c true, если оба указателя (очередь и хэш-таблица)
    ///     установлены; иначе @c false.
    explicit operator bool() const
    {
        if constexpr(std::is_pointer_v<mutex_type>)
        {
            return stv::all_true(queue_to_read_, hash_to_write_,
                                 mutex_ != nullptr);
        }
        else
        {
            return stv::all_true(queue_to_read_, hash_to_write_);
        }
    }

    /// @brief Выполняет маршрутизацию сообщений из входной очереди.
    ///
    /// @details
    /// Метод обрабатывает все сообщения, находящиеся во входной очереди
    /// на момент вызова. Для каждого сообщения извлекается @c compid
    /// (байт со смещением 3 в отрезанном сообщении), выполняется поиск
    /// ключа в хэш-таблице и перемещение сообщения в целевую очередь,
    /// если она не переполнена. В любом случае сообщение удаляется из
    /// входной очереди.
    ///
    /// Если в setup передан внешний мьютекс, весь метод выполняется под
    /// его защитой. Мьютекс сериализует только стороны, захватывающие
    /// тот же мьютекс: producer, помещающий сообщения во входную очередь,
    /// им не сериализуется — тип входной очереди должен быть
    /// потокобезопасным. При использовании @ref stv::empty_mutex
    /// блокировка отсутствует и не добавляет накладных расходов.
    ///
    /// @return Общее количество успешных доставок.
    auto run() -> std::size_t
    {
        const stv::lock_guard critical{get_mutex_ref()};

        std::size_t           message_routed_cnt{0U};

        while(!queue_to_read_->empty())
        {
            auto        msg  = std::move(queue_to_read_->front());
            const auto *data = msg.template data<std::byte>();

            // Входная очередь — публичный интерфейс: повреждённое
            // сообщение отбрасывается (политика drop, как при переполненной
            // целевой очереди), а не приводит к чтению вне границ.
            if((data == nullptr) || (msg.size_bytes() < routing_header_size))
            {
                queue_to_read_->pop();
                continue;
            }

            const auto compid = static_cast<std::uint8_t>(data[compid_offset]);

            message_routed_cnt += route_unicast(std::move(msg), compid);

            queue_to_read_->pop();
        }

        return message_routed_cnt;
    }

  private:
    /// @brief Маршрутизирует сообщение одному получателю по ключу
    ///     @c compid.
    ///
    /// @param[in,out] msg Сообщение для доставки.
    /// @param[in] compid Идентификатор компонента-источника.
    /// @return 1, если сообщение успешно помещено в целевую очередь; 0
    ///     в противном случае.
    auto route_unicast(
        typename queue_type::value_type msg, std::uint8_t compid) -> std::size_t
    {
        const auto entry = hash_to_write_->find(
            static_cast<typename hash_type::key_type>(compid));

        if(entry == hash_to_write_->end())
        {
            return 0U;
        }

        auto *const target_queue = entry->second;
        if((target_queue == nullptr) || target_queue->full())
        {
            return 0U;
        }

        target_queue->push(std::move(msg));
        return 1U;
    }
};

} // namespace stv

#endif /* MAVLINK_V2_ROUTE_HPP */
