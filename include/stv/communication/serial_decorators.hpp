/// @file serial_decorators.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
///
/// NAME
///     stv::serial_decorators -- обобщённые типы декораторов
///     служебных полей серийных сообщений.
///
/// DESCRIPTION
///     serial_decorators предоставляет обобщённые типы,
///     используемые декораторами, формирующими служебные поля
///     (заголовки и хвосты) серийных сообщений. Каждый декоратор
///     реализует интерфейс из статических методов header_size(),
///     trailer_size() и методов setup_header()/
///     setup_trailer(), которые заполняют соответствующие
///     области собранного кадра. Конкретные декораторы
///     протоколов вынесены в отдельные заголовки (например,
///     stvlink_sender.hpp, mavlink_v2_sender.hpp).
///
///     total_message_span
///         Границы всего собранного сообщения. Наследуется
///         от std::span<const std::byte> и передается
///         декораторам в методы setup_header() и
///         setup_trailer().
///
///     pload_span
///         Псевдоним std::span<const std::byte>,
///         представляющий полезную нагрузку сообщения.
///
///     empty_serial_decorator
///         Заглушка, не добавляющая служебных полей.
///         Используется, когда сообщение передается "как
///         есть".
///
/// SEE ALSO
///     serial_sender.hpp, serial_parser.hpp, stvlink_sender.hpp.

#ifndef SERIAL_DECORATORS_HPP
#define SERIAL_DECORATORS_HPP

#include <cstddef>
#include <span>

namespace stv {

/// @brief Тип, представляющий границы всего собранного сообщения.
///
/// @details
/// Используется декораторами в качестве аргумента @c total. Содержит
/// указатель на начало сообщения и его полный размер в байтах, включая
/// все заголовки, полезную нагрузку и хвосты. Наследует конструкторы
/// от @c std::span<const std::byte>.
class total_message_span: public std::span<const std::byte>
{
    using std::span<const std::byte>::span;
};

/// ----------------------------------------------------------------------------

/// @brief Тип, представляющий полезную нагрузку серийного сообщения.
///
/// @details
/// Содержит неизменяемый диапазон байт, которые будут размещены между
/// заголовками и хвостами. Передается декораторам, например, в метод
/// @c setup_header(), для заполнения служебных полей.
using pload_span = std::span<const std::byte>;

/// ----------------------------------------------------------------------------

/// @brief Пустой декоратор, не добавляющий служебных полей.
///
/// @details
/// Применяется, когда серийное сообщение должно передаваться "как есть",
/// без стартового кадра, заголовка сообщения или контрольной суммы. Может
/// использоваться как заглушка в шаблоне @c serial_message_buffer, где
/// требуется пустой пакет декораторов. В @c serial_parser заглушкой служить
/// не может: парсер требует от декоратора кадра методы matches() и
/// total_frame_size(), а суммарный размер заголовка — не менее 2 байт.
struct empty_serial_decorator {
    /// @brief Возвращает размер заголовка.
    ///
    /// @return Размер заголовка в байтах (всегда 0).
    static constexpr size_t header_size() { return 0U; }

    /// @brief Возвращает размер хвоста.
    ///
    /// @return Размер хвоста в байтах (всегда 0).
    static constexpr size_t trailer_size() { return 0U; }

    /// @brief Заполняет заголовок.
    ///
    /// @note Так как заголовок отсутствует, метод не выполняет никаких
    /// действий.
    ///
    /// @param[out] dst Указатель на область заголовка.
    /// @param[in] total Границы всего сообщения.
    /// @param[in] pload Границы полезной нагрузки.
    static void setup_header(
        std::byte *const dst, const total_message_span &total,
        const pload_span &pload)
    {
        (void)dst;
        (void)total;
        (void)pload;
    }

    /// @brief Заполняет хвост.
    ///
    /// @note Так как хвост отсутствует, метод не выполняет никаких
    /// действий.
    ///
    /// @param[out] dst Указатель на область хвоста.
    /// @param[in] total Границы всего сообщения.
    static void setup_trailer(
        std::byte *dst, const total_message_span &total)
    {
        (void)dst;
        (void)total;
    }
};

/// ----------------------------------------------------------------------------

} // namespace stv

#endif /* SERIAL_DECORATORS_HPP */
