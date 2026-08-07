/// @file parsed_queue.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
///
/// NAME
///     stv::parsed_queue -- типизированная связка декоратора кадра и очереди.
///
/// DESCRIPTION
///     parsed_queue оборачивает указатель на очередь, связывая её с
///     конкретным декоратором кадра на уровне типа. Это позволяет
///     serial_parser и маршрутизаторам проверять соответствие очереди
///     декоратору на этапе компиляции.
///
/// SEE ALSO
///     stvlink_frame.hpp, serial_parser.hpp.

#ifndef PARSED_QUEUE_HPP
#define PARSED_QUEUE_HPP

#include <cstddef>

namespace stv {

/// @brief Типизированная обёртка указателя на очередь.
///
/// @details
/// Хранит указатель на очередь и связывает его с тегом декоратора кадра
/// @c FrameDecorator. Параметр @c QueueType позволяет зафиксировать тип
/// очереди на этапе компиляции; по умолчанию используется @c void, при
/// котором тип очереди не контролируется обёрткой.
///
/// @tparam FrameDecorator Декоратор кадра, с которым связана очередь.
/// @tparam QueueType Тип очереди, хранимой обёрткой. По умолчанию @c void.
template<typename FrameDecorator, typename QueueType = void>
struct parsed_queue {
    /// @brief Тип декоратора кадра — тег очереди.
    using frame_decorator_type = FrameDecorator;

    /// @brief Тип очереди, зафиксированный обёрткой.
    using queue_type = QueueType;

    /// @brief Конструирует обёртку с @c nullptr.
    parsed_queue() = default;

    /// @brief Конструирует обёртку из ссылки на очередь.
    ///
    /// @tparam Q Тип очереди, выводимый из аргумента.
    /// @param[in,out] queue Очередь, в которую будут помещаться распарсенные
    ///     кадры, соответствующие декоратору @c FrameDecorator.
    template<typename Q>
    explicit parsed_queue(
        Q &queue):
        queue_{&queue}
    {
    }

    /// @brief Конструирует обёртку из указателя на очередь.
    ///
    /// @tparam Q Тип очереди, выводимый из аргумента.
    /// @param[in] queue Указатель на очередь. Может быть @c nullptr.
    template<typename Q>
    explicit parsed_queue(
        Q *queue):
        queue_{queue}
    {
    }

    /// @brief Возвращает сохранённый указатель на очередь.
    ///
    /// @return Указатель на очередь или @c nullptr.
    [[nodiscard]] void *queue() const noexcept { return queue_; }

  private:
    /// @brief Указатель на очередь.
    void *queue_{nullptr};
};

} // namespace stv

#endif /* PARSED_QUEUE_HPP */
