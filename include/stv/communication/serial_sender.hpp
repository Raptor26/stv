/// @file serial_sender.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
///
/// NAME
///     stv::serial_sender -- буфер серийных сообщений.
///
/// DESCRIPTION
///     serial_sender предоставляет классы для создания,
///     заполнения и отправки серийных сообщений через
///     очередь. Сообщение автоматически собирается в
///     деструкторе: декораторы заполняют заголовки и
///     хвосты, после чего готовый буфер помещается в
///     очередь, если в ней есть место.
///
///     composite_serial_message<TQueueBase, Decorators...>
///         Выделяет память под полезную нагрузку и
///         служебные поля, копирует полезную нагрузку при
///         конструировании и помещает готовый буфер в
///         очередь в деструкторе.
///
///     serial_message<UserData, TQueueBaseType, Decorators...>
///         Оболочка над composite_serial_message,
///         предоставляющая доступ к полезной нагрузке как
///         к объекту типа UserData через operator->().
///
///     serial_message_buffer_base<TQueueBaseType, Decorators...>
///         Базовый класс буфера с фабричными методами
///         request() и request_null(). Поддерживает строки,
///         контейнеры, span, POD-структуры, итераторы и
///         явный размер. Запрещает копирование и
///         перемещение.
///
///     serial_message_buffer<TQueue, Decorators...>
///         Адаптация базового буфера для очередей на базе
///         etl::iqueue, хранящих элементы типа stv::sim_buff.
///
///     make_serial_message_buffer(queue, decorators...)
///         Фабричная функция, упрощающая создание
///         serial_message_buffer. Тип очереди выводится
///         автоматически.
///
///     Поддерживаемые источники полезной нагрузки для
///     request():
///     - std::string_view и C-строки;
///     - контейнеры с непрерывной памятью
///       (std::array, std::vector и др.);
///     - std::span<const T>;
///     - POD-структуры;
///     - пара итераторов;
///     - явный размер в байтах (без копирования данных).
///
/// EXAMPLE
///     Пример отправки типизированного пакета со
///     стартовым кадром, CRC и маршрутизацией:
///     ```cpp
///     #include <stv/communication/serial_sender.hpp>
///     #include <stv/containers/simbuff.hpp>
///     #include <stv/mutex_guard.hpp>
///     #include <etl/queue.h>
///
///     struct UserData {
///         std::uint8_t i{0};
///         std::uint8_t j{0};
///         std::uint8_t k{0};
///         std::uint8_t z{0};
///     };
///
///     int main() {
///         using namespace stv;
///
///         using sim_buff_type = stv::sim_buff<stv::empty_mutex>;
///         using queue_type    = etl::queue<sim_buff_type, 10>;
///         queue_type queue{};
///
///         auto serial_message_buffer = make_serial_message_buffer(
///             queue, start_frame_and_crc_16{}, head_route{});
///
///         auto msg = serial_message_buffer.request<UserData>(
///             head_route::head_route_setup_t{
///                 .dst_id = 1, .pack_id = 7});
///
///         if (msg) {
///             msg->i = 11;
///             msg->j = 22;
///             msg->k = 33;
///             msg->z = 44;
///         }
///
///         // Заголовки и CRC заполняются в деструкторе msg,
///         // после чего готовый буфер помещается в queue.
///
///         return 0;
///     }
///     ```
///
///     Пример передачи строки:
///     ```cpp
///     auto msg = serial_message_buffer.request("Hello world");
///     ```
///
///     Пример передачи массива с параметрами декоратора:
///     ```cpp
///     std::array<std::uint16_t, 3> data{11, 22, 33};
///     head_route::head_route_setup_t route{
///         .dst_id = 1, .pack_id = 7};
///     auto msg = serial_message_buffer.request(data, route);
///     ```
///
/// SEE ALSO
///     test_serial_sender.cpp, bench_serial.cpp.

#ifndef SERIAL_HPP
#define SERIAL_HPP

#include "etl/queue.h"
#include "serial_decorators.hpp"
#include "stv/containers/simbuff.hpp"
#include <array>
#include <cassert>
#include <concepts>
#include <cstddef>
#include <etl/utility.h>
#include <iterator>
#include <limits>
#include <memory>
#include <span>
#include <string_view>

namespace stv {

/// @brief Составное серийное сообщение.
///
/// @details
/// Класс выделяет непрерывный блок памяти под полезную нагрузку и
/// дополнительные служебные поля (заголовки и хвосты), которые формируют
/// декораторы. Полезная нагрузка копируется в выделенную область при
/// конструировании, либо заполняется позже пользовательским кодом. В
/// деструкторе класса происходит окончательная сборка сообщения: каждый
/// декоратор заполняет свой заголовок и/или хвост, после чего готовый
/// буфер помещается во входную очередь.
///
/// @note Этот класс является внутренней реализацией. Обычно пользователю
/// удобнее работать через @ref serial_message_buffer и @ref serial_message.
///
/// @tparam TQueueBase Базовый тип очереди, в которую будет помещено
/// готовое сообщение. Должен предоставлять методы push(), full() и
/// хранить элементы типа, совместимого с @ref sim_buff.
/// @tparam Decorators Пакет декораторов, которые добавляют заголовки и/или
/// хвосты к полезной нагрузке. Каждый декоратор должен предоставлять
/// статические методы header_size(), trailer_size(), а также нестатические
/// setup_header() и/или setup_trailer().
template<typename TQueueBase, typename... Decorators>
class composite_serial_message
{
    using queue_base_type = TQueueBase;
    /// @brief Тип элемента очереди (буфера, в который будет записано
    /// сообщение).
    using container_type = queue_base_type::value_type;

    /// @brief Кортеж декораторов, применяемых к сообщению.
    const std::tuple<Decorators...> decorators_;

    /// @brief Размер полезной нагрузки в байтах.
    const std::uint16_t payload_size_;

    /// @brief Суммарный размер всех заголовков в байтах.
    const std::uint16_t header_size_;

    /// @brief Суммарный размер всех хвостов в байтах.
    const std::uint16_t trailer_size_;

    /// @brief Общий размер сообщения в байтах.
    const std::uint16_t total_size_;

    /// @brief Ссылка на очередь, в которую будет помещено сообщение.
    queue_base_type &queue_;

    /// @brief Буфер, выделенный под всё сообщение (заголовки, полезная
    /// нагрузка, хвосты).
    container_type memory_;

  public:
    /// @brief Тип неизменяемого span для представления байтового диапазона.
    using const_span_type = std::span<const std::byte>;

    /// @brief Тип изменяемого span для представления байтового диапазона.
    using span_type = std::span<std::byte>;

    /// @brief Конструирует сообщение и копирует полезную нагрузку.
    ///
    /// @param[in,out] queue Очередь, в которую будет помещено готовое
    /// сообщение в деструкторе.
    /// @param[in] pload Span, содержащий указатель на полезную нагрузку и
    /// её размер. Если pload.data() не равен nullptr, данные копируются в
    /// выделенную область памяти.
    /// @param[in] decorators Декораторы, которые будут применены к сообщению.
    /// Передаются по значению с perfect forwarding.
    composite_serial_message(
        queue_base_type &queue, const pload_span &pload,
        Decorators &&...decorators):
        decorators_{std::forward<Decorators>(decorators)...},
        payload_size_{static_cast<decltype(payload_size_)>(pload.size_bytes())},
        header_size_{
            static_cast<decltype(header_size_)>(compute_header_size()),
        },
        trailer_size_{
            static_cast<decltype(trailer_size_)>(compute_trailer_size()),
        },
        total_size_{
            static_cast<decltype(total_size_)>(header_size_ + payload_size_
                                               + trailer_size_),
        },
        queue_{queue},
        memory_{total_size_}
    {
        if(pload.data() && memory_.begin())
        {
            memcpy(memory_.begin() + header_size_, pload.data(),
                   pload.size_bytes());
        }
    }

    /// @brief Конструирует пустое (невалидное) сообщение.
    ///
    /// @details
    /// Используется для создания заглушки, которая не выделяет память и не
    /// помещает ничего в очередь при уничтожении. Декораторы должны быть
    /// default-constructible, но они не применяются, так как память не
    /// выделена.
    ///
    /// @param[in] null_tag Тег nullptr для разрешения перегрузки.
    /// @param[in,out] queue Очередь, связанная с сообщением.
    explicit composite_serial_message(
        [[maybe_unused]] std::nullptr_t null_tag, queue_base_type &queue):
        decorators_{},
        // value-initialization (требует default-constructible декораторов)

        payload_size_{0},
        header_size_{0},
        trailer_size_{0},
        total_size_{0},
        queue_{queue},
        memory_{0} // Пустой контейнер → memory_.data() == nullptr
    {
        // Декораторы не используются, так как в деструкторе есть проверка
        // if(memory_.begin())
    }

    /// @brief Проверяет, было ли успешно выделена память под сообщение.
    ///
    /// @return true, если память выделена; false в противном случае.
    explicit operator bool() const { return memory_.data() != nullptr; }

    /// @brief Деструктор. Собирает сообщение и помещает его в очередь.
    ///
    /// @details
    /// Если память была выделена, деструктор последовательно вызывает
    /// декораторы для заполнения заголовков и хвостов, а затем помещает
    /// готовый буфер в очередь, если в ней есть свободное место. Если
    /// очередь переполнена, сообщение теряется.
    virtual ~composite_serial_message()
    {
        if(memory_.begin())
        {
            setup_all_headers();

            setup_all_trailers(memory_.begin() + header_size_ + payload_size_,
                               memory_.begin(), total_size_);

            if(!queue_.full())
            {
                queue_.push(std::move(memory_));
            }
        }
    }

    /// @brief Возвращает указатель на начало полезной нагрузки.
    ///
    /// @return Указатель на первый байт полезной нагрузки.
    std::byte *pload() { return memory_.begin() + header_size_; }

    /// @brief Возвращает константный указатель на начало полезной нагрузки.
    ///
    /// @return Константный указатель на первый байт полезной нагрузки.
    [[nodiscard]] const std::byte *pload() const
    { return memory_.begin() + header_size_; }

    /// @brief Возвращает span, покрывающий полезную нагрузку.
    ///
    /// @return Изменяемый span размером с полезную нагрузку.
    auto pload_data()
    {
        return span_type{
            std::to_address(pload()),
            std::to_address(pload() + payload_size_),
        };
    }

    /// @brief Возвращает указатель на начало полезной нагрузки.
    ///
    /// @return Указатель на первый байт полезной нагрузки.
    [[nodiscard]] auto data() { return pload(); }

    /// @brief Возвращает итератор на начало всего сообщения.
    ///
    /// @return Итератор, указывающий на первый байт сообщения.
    [[nodiscard]] auto begin() { return memory_.begin(); }

    /// @brief Возвращает константный итератор на начало всего сообщения.
    ///
    /// @return Константный итератор, указывающий на первый байт сообщения.
    [[nodiscard]] auto begin() const { return memory_.begin(); }

    /// @brief Возвращает итератор на конец всего сообщения.
    ///
    /// @return Итератор, указывающий за последний байт сообщения.
    [[nodiscard]] auto end() { return memory_.end(); }

    /// @brief Возвращает константный итератор на конец всего сообщения.
    ///
    /// @return Константный итератор, указывающий за последний байт сообщения.
    [[nodiscard]] auto end() const { return memory_.end(); }

    /// @brief Возвращает общий размер сообщения в байтах.
    ///
    /// @return Размер сообщения в байтах.
    [[nodiscard]] auto size() const { return memory_.size(); }

  private:
    constexpr size_t compute_header_size()
    {
        return std::apply(
            [](auto &...decorators) {
                return (decorators.header_size() + ...);
            },
            decorators_);
    }

    constexpr size_t compute_trailer_size()
    {
        return std::apply(
            [](auto &...decorators) {
                return (decorators.trailer_size() + ...);
            },
            decorators_);
    }

    void setup_all_headers()
    {
        assert(memory_.begin());
        size_t offset{0};
        std::apply(
            [&](auto &...decorators) {
                (setup_single_header(decorators, offset), ...);
            },
            decorators_);
    }

    template<typename Decorator>
    void setup_single_header(
        Decorator &decorator, size_t &offset)
    {
        if constexpr(Decorator::header_size() > 0)
        {
            decorator.setup_header(
                memory_.begin() + offset,
                total_message_span(memory_.begin(), memory_.size()),
                pload_span(pload(), payload_size_));
            offset += Decorator::header_size();
        }
    }

    /// @brief Вызывает все декораторы, заполняющие хвосты сообщений.
    void setup_all_trailers(
        std::byte *dst, const std::byte *data, size_t data_size)
    {
        size_t offset{0};
        std::apply(
            [&](auto &...decorators) {
                (setup_single_trailer(decorators, dst, offset, data, data_size),
                 ...);
            },
            decorators_);
    }

    template<typename Decorator>
    void setup_single_trailer(
        Decorator &decorator, std::byte *dst, size_t &offset,
        const std::byte *full_message, size_t full_message_size)
    {
        if constexpr(Decorator::trailer_size() > 0)
        {
            decorator.setup_trailer(
                dst + offset,
                total_message_span(full_message, full_message_size));
            offset += Decorator::trailer_size();
        }
    }
};

/// @brief Типизированное серийное сообщение.
///
/// @details
/// Класс является обёрткой над @ref composite_serial_message, которая
/// предоставляет доступ к полезной нагрузке как к объекту типа UserData.
/// Благодаря оператору operator->() пользователь может заполнять поля
/// структуры естественным синтаксисом (например, msg->field = value).
/// Сообщение автоматически собирается и помещается в очередь при
/// уничтожении объекта.
///
/// @tparam UserData Тип, через который интерпретируется полезная нагрузка.
/// Должен быть тривиально копируемым и помещаться в выделенную память.
/// @tparam TQueueBaseType Базовый тип очереди для готовых сообщений.
/// @tparam Decorators Декораторы, применяемые к сообщению.
template<typename UserData, typename TQueueBaseType, typename... Decorators>
class serial_message
{
    using queue_base_type = TQueueBaseType;

    /// @brief Тип пользовательских данных.
    using user_type = UserData;

    /// @brief Тип составного сообщения, выполняющего выделение памяти и
    /// применение декораторов.
    using composite_serial_message_type =
        stv::composite_serial_message<queue_base_type, Decorators...>;

    /// @brief Тип неизменяемого span для байтового представления.
    using const_span_type =
        typename composite_serial_message_type::const_span_type;

    /// @brief Составное сообщение, которым владеет данный объект.
    composite_serial_message_type composite_message_;

  public:
    /// @brief Тип значения байта сообщения.
    using value_type = std::byte;

    /// @brief Тип указателя на байт сообщения.
    using pointer = value_type *;

    /// @brief Тип ссылки на байт сообщения.
    using reference = value_type &;

    /// @brief Тип итератора по байтам сообщения.
    using iterator_type = value_type *;

    /// @brief Тип константного итератора по байтам сообщения.
    using const_iterator = const value_type *;

    /// @brief Конструирует сообщение с копированием полезной нагрузки.
    ///
    /// @param[in,out] queue Очередь для готового сообщения.
    /// @param[in] pload Span с полезной нагрузкой.
    /// @param[in] decorators Декораторы, применяемые к сообщению.
    serial_message(
        queue_base_type &queue, const const_span_type &pload,
        Decorators... decorators):
        composite_message_{queue, pload,
                           std::forward<Decorators>(decorators)...}
    {
    }

    /// @brief Конструирует невалидное сообщение.
    ///
    /// @details
    /// Создаёт пустое сообщение, которое не выделяет память и не влияет на
    /// очередь при уничтожении. Полезно для возврата из request_null().
    ///
    /// @param[in] null_tag Тег nullptr.
    /// @param[in,out] queue Очередь, связанная с сообщением.
    explicit serial_message(
        [[maybe_unused]] std::nullptr_t null_tag, queue_base_type &queue):
        composite_message_{nullptr, queue}
    {
    }

    /// @brief Деструктор. По умолчанию делегирует уничтожение внутреннему
    /// составному сообщению.
    virtual ~serial_message() = default;

    /// @brief Проверяет, успешно ли выделена память под сообщение.
    ///
    /// @return true, если память выделена; false в противном случае.
    explicit operator bool() const
    { return static_cast<bool>(composite_message_); }

    /// @brief Возвращает указатель на полезную нагрузку как на UserData.
    ///
    /// @return Указатель на объект типа UserData, расположенный в памяти
    /// полезной нагрузки.
    auto operator->()
    { return reinterpret_cast<user_type *>(composite_message_.pload()); }

    /// @brief Возвращает указатель на начало полезной нагрузки.
    ///
    /// @return Указатель на первый байт полезной нагрузки.
    [[nodiscard]] auto data() { return composite_message_.data(); }

    /// @brief Возвращает span, покрывающий полезную нагрузку.
    ///
    /// @return Изменяемый span размером с полезную нагрузку.
    [[nodiscard]] auto pload_data() { return composite_message_.pload_data(); }

    /// @brief Возвращает итератор на начало всего сообщения.
    ///
    /// @return Итератор, указывающий на первый байт сообщения.
    [[nodiscard]] auto begin() { return composite_message_.begin(); }

    /// @brief Возвращает константный итератор на начало всего сообщения.
    ///
    /// @return Константный итератор, указывающий на первый байт сообщения.
    [[nodiscard]] auto begin() const { return composite_message_.begin(); }

    /// @brief Возвращает итератор на конец всего сообщения.
    ///
    /// @return Итератор, указывающий за последний байт сообщения.
    [[nodiscard]] auto end() { return composite_message_.end(); }

    /// @brief Возвращает константный итератор на конец всего сообщения.
    ///
    /// @return Константный итератор, указывающий за последний байт сообщения.
    [[nodiscard]] auto end() const { return composite_message_.end(); }

    /// @brief Возвращает общий размер сообщения в байтах.
    ///
    /// @return Размер сообщения в байтах.
    [[nodiscard]] auto size() const { return composite_message_.size(); }
};

/// @brief Базовый класс буфера серийных сообщений.
///
/// @details
/// Класс предоставляет фабричные методы request() и request_null() для
/// создания сообщений с заданными декораторами. Он хранит ссылку на очередь
/// и кортеж декораторов, которые копируются при каждом запросе сообщения.
/// Методы request() поддерживают различные источники полезной нагрузки:
/// строки, контейнеры, span, итераторы, POD-структуры и явный размер.
///
/// @note Этот класс запрещает копирование и перемещение.
///
/// @tparam TQueueBaseType Базовый тип очереди, в которую помещаются
/// готовые сообщения.
/// @tparam Decorators Декораторы, применяемые к каждому создаваемому
/// сообщению.
template<typename TQueueBaseType, typename... Decorators>
class serial_message_buffer_base:
    private stv::non_copyable,
    private stv::non_movable
{
  public:
    /// @brief Базовый тип очереди для готовых сообщений.
    using queue_base_type = TQueueBaseType;

  private:
    /// @brief Кортеж декораторов по умолчанию.
    const std::tuple<Decorators...> decorators_;

    /// @brief Ссылка на очередь, в которую помещаются сообщения.
    queue_base_type &queue_;

  public:
    /// @brief Тип неизменяемого байтового span.
    using span_type = std::span<const std::byte>;

    /// @brief Конструирует буфер сообщений.
    ///
    /// @param[in,out] queue Очередь, в которую будут помещаться готовые
    /// сообщения.
    /// @param[in] decorators Декораторы, применяемые к сообщениям.
    explicit serial_message_buffer_base(
        queue_base_type &queue, Decorators &&...decorators):
        decorators_{std::forward<Decorators>(decorators)...},
        queue_{queue}
    {
    }

    /// @brief Деструктор по умолчанию.
    virtual ~serial_message_buffer_base() = default;

    /// @brief Создаёт невалидное сообщение с явным типом UserData.
    ///
    /// @details
    /// Возвращает объект @ref serial_message, который не выделяет память и
    /// не помещает данные в очередь. Используется для единообразия API в
    /// случаях, когда выделение сообщения невозможно или не требуется.
    ///
    /// @tparam UserData Тип, под который интерпретируется полезная нагрузка.
    /// @tparam SetupParams Дополнительные типы параметров (игнорируются).
    /// @param[in] user_data Образец типа UserData (игнорируется).
    /// @param[in] setup_params Дополнительные параметры (игнорируются).
    /// @return Невалидный объект @ref serial_message.
    template<typename UserData, typename... SetupParams>
    auto request_null(
        [[maybe_unused]] const UserData &user_data,
        [[maybe_unused]] const SetupParams &...setup_params)
    {
        return serial_message<UserData, queue_base_type, Decorators...>(nullptr,
                                                                        queue_);
    }

    /// @brief Создаёт невалидное сообщение с явным типом UserData.
    ///
    /// @tparam UserData Тип, под который интерпретируется полезная нагрузка.
    /// @tparam SetupParams Дополнительные типы параметров (игнорируются).
    /// @param[in] setup_params Дополнительные параметры (игнорируются).
    /// @return Невалидный объект @ref serial_message.
    template<typename UserData, typename... SetupParams>
    auto request_null(
        [[maybe_unused]] const SetupParams &...setup_params)
    {
        return serial_message<UserData, queue_base_type, Decorators...>(nullptr,
                                                                        queue_);
    }

    /// @brief Создаёт невалидное сообщение с типом полезной нагрузки
    /// std::byte.
    ///
    /// @return Невалидный объект @ref serial_message.
    auto request_null()
    {
        return serial_message<std::byte, queue_base_type, Decorators...>(
            nullptr, queue_);
    }

    /// @brief Создаёт невалидное сообщение заданного размера.
    ///
    /// @tparam SetupParams Дополнительные типы параметров (игнорируются).
    /// @param[in] size Запрашиваемый размер (игнорируется).
    /// @param[in] setup_params Дополнительные параметры (игнорируются).
    /// @return Невалидный объект @ref serial_message.
    template<typename... SetupParams>
    auto request_null(
        [[maybe_unused]] std::size_t size,
        [[maybe_unused]] const SetupParams &...setup_params)
    {
        return serial_message<std::byte, queue_base_type, Decorators...>(
            nullptr, queue_);
    }

    /// @brief Создаёт невалидное сообщение на основе контейнера.
    ///
    /// @tparam Container Тип контейнера с непрерывной памятью (игнорируется).
    /// @param[in] container Контейнер-образец (игнорируется).
    /// @return Невалидный объект @ref serial_message.
    template<stv::contiguous_trivial_container_concept Container>
    auto request_null(
        [[maybe_unused]] const Container &container)
    {
        using value_type = typename Container::value_type;
        // return request_impl<value_type>(std::as_bytes(std::span{container}));
        return serial_message<value_type, queue_base_type, Decorators...>(
            nullptr, queue_);
    }

    /// @brief Запрашивает сообщение из строки std::string_view.
    ///
    /// @param[in] str Строковое представление полезной нагрузки.
    /// @return Объект @ref serial_message<char>.
    auto request(
        std::string_view str)
    {
        return request_impl<decltype(str)::value_type>(
            std::as_bytes(std::span(str.data(), str.size())));
    }

    /// @brief Запрашивает сообщение из строки std::string_view с параметрами
    /// декораторов.
    ///
    /// @tparam SetupParams Типы параметров для декораторов.
    /// @param[in] str Строковое представление полезной нагрузки.
    /// @param[in] setup_params Параметры, которые будут переданы
    /// соответствующим декораторам.
    /// @return Объект @ref serial_message<char>.
    template<typename... SetupParams>
    auto request(
        std::string_view str, SetupParams &&...setup_params)
    {
        return request_impl<decltype(str)::value_type>(
            std::as_bytes(std::span(str.data(), str.size())),
            std::forward<SetupParams>(setup_params)...);
    }

    /// @brief Запрашивает сообщение из C-строки.
    ///
    /// @param[in] str Указатель на нуль-терминированную строку.
    /// @return Объект @ref serial_message<char>.
    auto request(
        const char *str)
    { return request(std::string_view{str}); }

    /// @brief Запрашивает сообщение из C-строки с параметрами декораторов.
    ///
    /// @tparam SetupParams Типы параметров для декораторов.
    /// @param[in] str Указатель на нуль-терминированную строку.
    /// @param[in] setup_params Параметры, которые будут переданы
    /// соответствующим декораторам.
    /// @return Объект @ref serial_message<char>.
    template<typename... SetupParams>
    auto request(
        const char *str, SetupParams &&...setup_params)
    {
        return request(std::string_view{str},
                       std::forward<SetupParams>(setup_params)...);
    }

    /// @brief Запрашивает сообщение заданного размера в байтах.
    ///
    /// @details
    /// Создаёт сообщение с полезной нагрузкой размером size байт. Данные
    /// не копируются; пользователь должен заполнить область памяти через
    /// operator->() или data().
    ///
    /// @tparam SetupParams Типы параметров для декораторов.
    /// @param[in] size Размер полезной нагрузки в байтах.
    /// @param[in] setup_params Параметры, которые будут переданы
    /// соответствующим декораторам.
    /// @return Объект @ref serial_message<std::byte>.
    template<typename... SetupParams>
    auto request(
        std::unsigned_integral auto size, SetupParams &&...setup_params)
    {
        return request_impl<std::byte>(
            span_type(static_cast<const std::byte *>(nullptr), size),
            std::forward<SetupParams>(setup_params)...);
    }

    /// @brief Запрашивает сообщение из контейнера с непрерывной памятью.
    ///
    /// @details
    /// Поддерживает std::array, std::vector и другие контейнеры, которые
    /// удовлетворяют @ref contiguous_trivial_container_concept. Данные
    /// копируются в выделенную область памяти сообщения.
    ///
    /// @tparam Container Тип контейнера с непрерывной памятью.
    /// @param[in] container Контейнер с полезной нагрузкой.
    /// @return Объект @ref serial_message<value_type>.
    template<stv::contiguous_trivial_container_concept Container>
    auto request(
        const Container &container)
    {
        using value_type = typename Container::value_type;
        return request_impl<value_type>(std::as_bytes(std::span{container}));
    }

    /// @brief Запрашивает сообщение из контейнера с параметрами декораторов.
    ///
    /// @tparam Container Тип контейнера с непрерывной памятью.
    /// @tparam SetupParams Типы параметров для декораторов.
    /// @param[in] container Контейнер с полезной нагрузкой.
    /// @param[in] setup_params Параметры, которые будут переданы
    /// соответствующим декораторам.
    /// @return Объект @ref serial_message<value_type>.
    template<stv::contiguous_trivial_container_concept Container,
             typename... SetupParams>
    auto request(
        const Container &container, SetupParams &&...setup_params)
    {
        using value_type = typename Container::value_type;
        return request_impl<value_type>(
            std::as_bytes(std::span{container}),
            std::forward<SetupParams>(setup_params)...);
    }

    /// @brief Запрашивает сообщение из неизменяемого span.
    ///
    /// @tparam T Тип элемента span.
    /// @param[in] span Источник полезной нагрузки.
    /// @return Объект @ref serial_message<std::remove_const_t<T>>.
    template<typename T>
    auto request(
        std::span<const T> span)
    { return request_impl<std::remove_const_t<T>>(std::as_bytes(span)); }

    /// @brief Запрашивает сообщение из неизменяемого span с параметрами
    /// декораторов.
    ///
    /// @tparam T Тип элемента span.
    /// @tparam SetupParams Типы параметров для декораторов.
    /// @param[in] span Источник полезной нагрузки.
    /// @param[in] setup_params Параметры, которые будут переданы
    /// соответствующим декораторам.
    /// @return Объект @ref serial_message<std::remove_const_t<T>>.
    template<typename T, typename... SetupParams>
    // NOLINTNEXTLINE(cppcoreguidelines-missing-std-forward)
    auto request(
        std::span<const T> span, SetupParams &&...setup_params)
    {
        return request_impl<std::remove_const_t<T>>(
            std::as_bytes(span), std::forward<SetupParams>(setup_params)...);
    }

    /// @brief Запрашивает сообщение из POD-структуры.
    ///
    /// @details
    /// Создаёт сообщение, размер которого равен sizeof(UserData). Данные
    /// копируются из переданного объекта. Удобно для передачи типизированных
    /// пакетов.
    ///
    /// @tparam UserData Тип POD-структуры.
    /// @tparam SetupParams Типы параметров для декораторов.
    /// @param[in] user_data Объект, данные которого копируются.
    /// @param[in] setup_params Параметры, которые будут переданы
    /// соответствующим декораторам.
    /// @return Объект @ref serial_message<UserData>.
    template<typename UserData, typename... SetupParams>
        requires(
            !std::integral<UserData>
            && !std::same_as<std::remove_cvref_t<UserData>, std::string_view>
            && !std::same_as<std::remove_cvref_t<UserData>, const char *>
            && !std::same_as<std::remove_cvref_t<UserData>, char *>
            && !stv::contiguous_trivial_container_concept<UserData>)
    auto request(
        const UserData &user_data, SetupParams &&...setup_params)
    {
        return request_impl<UserData>(
            span_type(reinterpret_cast<const std::byte *>(&user_data),
                      sizeof(UserData)),
            std::forward<SetupParams>(setup_params)...);
    }

    /// @brief Запрашивает область памяти под структуру типа UserData.
    ///
    /// @tparam UserData Тип, под хранение которого запрашивается память.
    /// @tparam SetupParams Типы параметров для декораторов.
    ///
    /// @param[in] setup_params Параметры, которые будут переданы
    /// соответствующим декораторам.
    ///
    /// @return Объект @ref serial_message<UserData>. После получения объекта
    /// пользовательский код должен:
    /// - проверить через operator bool() что память успешно выделена.
    /// - в случае успешного выделения памяти заполнить её через оператор
    ///   operator->().
    template<typename UserData, typename... SetupParams>
    auto request(
        SetupParams &&...setup_params)
    {
        return request_impl<UserData>(
            span_type(static_cast<const std::byte *>(nullptr),
                      sizeof(UserData)),
            std::forward<SetupParams>(setup_params)...);
    }

    /// @brief Запрашивает сообщение по паре итераторов.
    ///
    /// @tparam InputIt Тип входных итераторов.
    /// @param[in] cbegin Итератор на начало диапазона.
    /// @param[in] cend Итератор на конец диапазона.
    /// @return Объект @ref serial_message<value_type>.
    template<typename InputIt>
    auto request(
        InputIt cbegin, InputIt cend)
    { return request(std::as_bytes(std::span{cbegin, cend})); }

    /// @brief Возвращает ссылку на связанную очередь.
    ///
    /// @return Ссылка на очередь, в которую помещаются готовые сообщения.
    auto queue_instance() -> decltype(queue_) & { return queue_; }

  private:
    /// @brief Выполняет запрос памяти из кучи под сообщение.
    ///
    /// @tparam UserData Тип данных, под который будет запрошена память.
    /// Обертка над request_impl должна гарантировать, что в случае типа
    /// который является не владеющим (std::string_view, std::span),
    /// pload.size_bytes() будет содержать актуальный размер.
    /// @tparam SetupParams Типы декораторов.
    ///
    /// @param[in] pload span, который может содержать указатель на область
    /// памяти и всегда должен содержать требуемый размер области памяти.
    /// Всегда будет запрошен размер памяти, указанный в
    /// pload.size_in_bytes(). Но, если pload.data() == nullptr, то
    /// пользователь должен самостоятельно заполнить область памяти, которую
    /// вернет request_impl(). Если pload.data() != nullptr, то в случае
    /// успешного выделения памяти данные автоматически будут скопированы из
    /// pload.data() в выделенную область памяти.
    /// @param[in] setup_params Модификаторы декораторов.
    ///
    /// @return Возвращает объект типа UserData. После получения объекта,
    /// пользовательский код должен:
    /// - проверить через operator bool() что память успешно выделена.
    /// - в случае успешного выделения памяти, можно ее заполнить через
    ///   оператор operator->().
    template<typename UserData, typename... SetupParams>
    auto request_impl(
        const span_type &pload, SetupParams &&...setup_params)
    {
        auto decorators_copy = decorators_;

        // Сопоставление каждого аргумента из пачки параметров setup_params с
        // каждым декоратором.
        (apply_param_to_decorators(decorators_copy,
                                   std::forward<SetupParams>(setup_params)),
         ...);

        // Конструирование объекта serial_message из updated_decorators.
        return std::apply(
            [this, &pload](auto &&...decs) {
                return serial_message<UserData, queue_base_type, Decorators...>(
                    queue_, pload, std::forward<decltype(decs)>(decs)...);
            },
            decorators_copy);
    }

    template<typename Param>
    void apply_param_to_decorators(
        auto &tuple_of_decorators, const Param &param)
    {
        std::apply(
            [&param, this](auto &...decorators) {
                (void)this;
                (apply_to_single_decorator(decorators, param), ...);
            },
            tuple_of_decorators);
    }

    // Универсальная проверка - пытаемся сконструировать декоратор из параметра
    template<typename Decorator, typename Param>
    void apply_to_single_decorator(
        Decorator &decorator, const Param &param)
    {
        if constexpr(std::is_constructible_v<Decorator, const Param &>)
        {
            decorator = Decorator(param);
        }
    }
};

/// @brief Буфер серийных сообщений для очередей на базе etl::iqueue.
///
/// @details
/// Наследует функциональность @ref serial_message_buffer_base и адаптирует
/// её для работы с очередью типа etl::iqueue<sim_buff<...>>. Позволяет
/// создавать сообщения, которые затем передаются в последовательный порт,
/// радиоканал или другой транспорт.
///
/// @tparam TQueue Тип очереди, производной от etl::iqueue. Её value_type
/// должен быть совместим с @ref sim_buff.
/// @tparam Decorators Декораторы, применяемые к каждому сообщению.
template<typename TQueue, typename... Decorators>
class serial_message_buffer:
    public serial_message_buffer_base<etl::iqueue<typename TQueue::value_type>,
                                      Decorators...>
{
    /// @brief Тип элемента очереди.
    using queue_item_type = TQueue::value_type;

    /// @brief Тип симуляционного буфера, хранящегося в очереди.
    using sim_buff_type = queue_item_type;

    /// @brief Базовый тип очереди, передаваемый в базовый класс.
    using queue_base_type = etl::iqueue<sim_buff_type>;

    /// @brief Базовый класс.
    using base_type =
        serial_message_buffer_base<queue_base_type, Decorators...>;

  public:
    /// @brief Конструирует буфер сообщений.
    ///
    /// @param[in,out] queue Очередь, в которую будут помещаться готовые
    /// сообщения.
    /// @param[in] decorators Декораторы, применяемые к сообщениям.
    explicit serial_message_buffer(
        queue_base_type &queue, Decorators &&...decorators):
        base_type{queue, std::forward<Decorators>(decorators)...}
    {
    }

    /// @brief Деструктор по умолчанию.
    ~serial_message_buffer() override = default;
};

/// @brief Создаёт объект @ref serial_message_buffer.
///
/// @details
/// Фабричная функция, упрощающая создание буфера сообщений. Тип очереди
/// выводится автоматически из переданного аргумента.
///
/// @tparam TQueue Тип очереди.
/// @tparam Decorators Типы декораторов.
/// @param[in,out] queue Очередь, в которую будут помещаться готовые
/// сообщения.
/// @param[in] decorators Декораторы, применяемые к сообщениям.
/// @return Объект @ref serial_message_buffer.
template<typename TQueue, typename... Decorators>
auto make_serial_message_buffer(
    TQueue &queue, Decorators &&...decorators)
{
    return serial_message_buffer<TQueue, Decorators...>(
        queue, std::forward<Decorators>(decorators)...);
}

} // namespace stv

#endif /* SERIAL_HPP */
