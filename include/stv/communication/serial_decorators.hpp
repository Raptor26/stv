/// @file serial_decorators.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
///
/// NAME
///     stv::serial_decorators -- декораторы служебных
///     полей серийных сообщений.
///
/// DESCRIPTION
///     serial_decorators предоставляет типы и классы,
///     формирующие служебные поля (заголовки и хвосты)
///     серийных сообщений. Каждый декоратор реализует
///     интерфейс из статических методов header_size(),
///     trailer_size() и методов setup_header()/
///     setup_trailer(), которые заполняют соответствующие
///     области собранного кадра.
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
///     start_frame_and_crc_16
///         Добавляет в начало сообщения стартовый кадр
///         0xAA 0xAA и 16-битное поле frame_size, равное
///         размеру оставшейся части кадра. В конец
///         записывается CRC-16 (начальное значение 0xFFFF,
///         полином 0xA001), вычисленный по всем байтам
///         сообщения, кроме самого CRC. Метод
///         is_crc_valid() проверяет целостность принятого
///         кадра.
///
///     head_route
///         Добавляет маршрутизацию: идентификатор
///         получателя dst_id, порядковый номер пакета
///         pack_id и 16-битный размер полезной нагрузки
///         pload_size. Параметры задаются через структуру
///         head_route_setup_t. При превышении размера
///         полезной нагрузки максимального значения
///         uint16_t срабатывает assert.
///
/// EXAMPLE
///     Пример формирования кадра со стартовой
///     последовательностью и CRC:
///     ```cpp
///     #include <stv/communication/serial_decorators.hpp>
///     #include <array>
///     #include <span>
///
///     int main() {
///         using namespace stv;
///
///         std::array<std::byte, 11> buffer{};
///         std::array<std::byte, 5> pload{
///             std::byte{0x01}, std::byte{0x02},
///             std::byte{0x03}, std::byte{0x04},
///             std::byte{0x05}
///         };
///
///         start_frame_and_crc_16 decorator;
///         decorator.setup_header(
///             buffer.data(),
///             total_message_span(buffer.data(), buffer.size()),
///             pload_span(pload.data(), pload.size()));
///
///         std::memcpy(
///             buffer.data()
///                 + start_frame_and_crc_16::header_size(),
///             pload.data(), pload.size());
///
///         decorator.setup_trailer(
///             buffer.data()
///                 + start_frame_and_crc_16::header_size()
///                 + pload.size(),
///             total_message_span(buffer.data(), buffer.size()));
///
///         bool ok = start_frame_and_crc_16::is_crc_valid(
///             total_message_span(buffer.data(), buffer.size()));
///
///         return ok ? 0 : 1;
///     }
///     ```
///
///     Пример маршрутизации:
///     ```cpp
///     stv::head_route::head_route_setup_t route{
///         .dst_id = 1, .pack_id = 7};
///     stv::head_route route_decorator(route);
///     route_decorator.setup_header(dst, total, pload);
///     ```
///
/// SEE ALSO
///     test_serial_parser.cpp, test_serial_sender.cpp.

#ifndef SERIAL_DECORATORS_HPP
#define SERIAL_DECORATORS_HPP

#include "stv/utils.hpp"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
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
/// без стартового кадра, маршрутизации или контрольной суммы. Может
/// использоваться как заглушка в шаблонах @c serial_message_buffer и
/// @c serial_parser, где требуется пустой пакет декораторов.
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

/// @brief Декоратор, добавляющий стартовый кадр и CRC-16.
///
/// @details
/// Формирует канальный уровень серийного сообщения:
///   - в начало записывается стартовый кадр @c 0xAA 0xAA и поле
///     @c frame_size, равное размеру оставшейся части сообщения
///     (полезная нагрузка + все хвосты);
///   - в конец записывается 16-битная контрольная сумма (CRC-16),
///     вычисленная по всем байтам сообщения, кроме самого CRC.
///
/// Такая структура позволяет @c serial_parser находить начало кадра в
/// потоке байт, определять длину сообщения и проверять его целостность.
///
/// @note
/// Поле @c frame_size не включает в себя размер стартового кадра. Если
/// сообщение состоит из стартового кадра (4 байта: 0xAA, 0xAA и 16-битное
/// поле размера), полезной нагрузки (5 байт) и CRC (2 байта), то
/// @c frame_size будет равно 7, а общий размер сообщения — 11 байт.
class start_frame_and_crc_16
{
    using frame_size_type = std::uint16_t;
    using crc_type        = std::uint16_t;

  public:
    STV_NO_PADDING_NO_OPTIMIZE_BEGIN

    /// @brief Стартовый кадр сообщения.
    ///
    /// @details
    /// Поля расположены подряд без выравнивания. Порядок байт в памяти
    /// определяется архитектурой целевой платформы.
    struct start_frame_t {
        /// @brief Первый байт стартовой последовательности.
        std::byte start_frame_first;

        /// @brief Второй байт стартовой последовательности.
        std::byte start_frame_second;

        /// @brief Размер оставшейся части сообщения.
        ///
        /// @details
        /// 16-битное поле. Равно размеру полезной нагрузки плюс размер
        /// всех хвостов, расположенных после поля @c frame_size.
        frame_size_type frame_size;
    };

    STV_NO_PADDING_NO_OPTIMIZE_END

    /// @brief Значение первого байта стартового кадра.
    static constexpr std::byte first_byte{0xAA};

    /// @brief Значение второго байта стартового кадра.
    static constexpr std::byte second_byte{0xAA};

    /// @brief Возвращает размер заголовка.
    ///
    /// @return Размер заголовка в байтах.
    static constexpr size_t header_size() { return sizeof(start_frame_t); }

    /// @brief Возвращает размер хвоста.
    ///
    /// @return Размер хвоста в байтах.
    static constexpr size_t trailer_size() { return sizeof(crc_type); }

    /// @brief Заполняет стартовый кадр и поле размера сообщения.
    ///
    /// @param[out] dst Указатель на начало заголовка в собранном
    /// сообщении.
    /// @param[in] total Границы всего сообщения.
    /// @param[in] pload Границы полезной нагрузки.
    static void setup_header(
        std::byte *const dst, const total_message_span &total,
        const pload_span &pload)
    {
        (void)pload;
        auto *start_frame              = reinterpret_cast<start_frame_t *>(dst);
        start_frame->start_frame_first = first_byte;
        start_frame->start_frame_second = second_byte;

        // Неизвестно, есть ли другие декораторы, поэтому вычислим размер
        // кадра из поля total.
        start_frame->frame_size =
            static_cast<decltype(start_frame->frame_size)>(total.size_bytes()
                                                           - header_size());
    }

    /// @brief Записывает CRC-16 в хвост сообщения.
    ///
    /// @param[out] dst Указатель на начало хвоста в собранном
    /// сообщении.
    /// @param[in] total Границы всего сообщения.
    static void setup_trailer(
        std::byte *dst, const total_message_span &total)
    {
        crc_type computed_crc =
            calculate_crc(total.data(), total.size_bytes() - trailer_size());
        std::memcpy(dst, &computed_crc, sizeof(computed_crc));
    }

    /// @brief Проверяет корректность CRC принятого сообщения.
    ///
    /// @details
    /// Вычисляет ожидаемый CRC по всем байтам сообщения, кроме хвоста,
    /// и сравнивает его со значением, записанным в конце буфера.
    ///
    /// @param[in] total Границы всего принятого сообщения.
    /// @return @c true, если CRC совпадает; @c false в противном случае.
    static auto is_crc_valid(
        const total_message_span &total)
    {
        // Вычисляем ожидаемый CRC по данным (без трейлера).
        auto expected_crc =
            calculate_crc(total.data(), total.size_bytes() - trailer_size());

        // Безопасно читаем полученный CRC из конца буфера.
        crc_type         received_crc{};
        const std::byte *crc_pos = total.end().base() - trailer_size();
        std::memcpy(&received_crc, crc_pos, sizeof(received_crc));

        return received_crc == expected_crc;
    }

  private:
    /// @brief Вычисляет CRC-16 по заданному блоку данных.
    ///
    /// @details
    /// Используется алгоритм CRC-16 с начальным значением 0xFFFF и
    /// полиномом 0xA001 (отраженный вид 0x8005), совместимый с Modbus.
    ///
    /// @param[in] data Указатель на начало данных.
    /// @param[in] length Размер данных в байтах.
    /// @return Вычисленное значение CRC-16.
    static crc_type calculate_crc(
        const std::byte *data, size_t length)
    {
        // NOLINTBEGIN(hicpp-signed-bitwise)
        // Простая реализация Crc для примера
        crc_type crc = 0xFFFF;
        for(size_t i = 0; i < length; ++i)
        {
            crc ^= static_cast<std::uint8_t>(data[i]);
            for(int j = 0; j < 8; ++j)
            {
                if(crc & 0x0001)
                {
                    crc = (crc >> 1) ^ 0xA001;
                }
                else
                {
                    crc >>= 1;
                }
            }
        }
        // NOLINTEND(hicpp-signed-bitwise)
        return crc;
    }
};

/// ----------------------------------------------------------------------------

/// @brief Декоратор, добавляющий маршрутизацию и размер полезной нагрузки.
///
/// @details
/// Записывает в заголовок идентификатор получателя @c dst_id, порядковый
/// номер пакета @c pack_id и размер полезной нагрузки @c pload_size.
/// Приемная сторона может использовать @c dst_id для выбора очереди
/// обработчика, а @c pack_id — для обнаружения потерь или дублирования.
///
/// @note
/// Размер полезной нагрузки сохраняется в 16-битном поле. Передача
/// сообщения с полезной нагрузкой, превышающей максимальное значение
/// @c uint16_t, приведет к срабатыванию assert.
struct head_route {
    /// @brief Параметры маршрутизации, задаваемые при создании декоратора.
    /// ########################################################################
    /// Арбитраж сообщения.
    /// ########################################################################
    STV_NO_PADDING_NO_OPTIMIZE_BEGIN

    struct head_route_setup_t {
        /// @brief Идентификатор получателя сообщения.
        ///
        /// @details
        /// Используется @c serial_parser_route для выбора очереди, в
        /// которую будет направлен пакет.
        std::uint8_t dst_id{std::numeric_limits<decltype(dst_id)>::min()};

        /// @brief Порядковый номер пакета.
        ///
        /// @details
        /// Позволяет получателю различать сообщения и отслеживать их
        /// порядок доставки.
        std::uint8_t pack_id{std::numeric_limits<decltype(pack_id)>::min()};
    };

    STV_NO_PADDING_NO_OPTIMIZE_END

    /// ------------------------------------------------------------------------

    /// @brief Конструирует декоратор с заданными параметрами маршрутизации.
    ///
    /// @param[in] setup Параметры маршрутизации. По умолчанию оба поля
    /// равны нулю.
    explicit head_route(
        const head_route_setup_t &setup = head_route_setup_t{.dst_id  = 0,
                                                             .pack_id = 0}):
        setup_{setup}
    {
    }

    /// @brief Возвращает размер заголовка.
    ///
    /// @return Размер заголовка в байтах.
    static constexpr size_t header_size()
    { return sizeof(head_route_setup_with_pload_t); }

    /// @brief Возвращает размер хвоста.
    ///
    /// @return Размер хвоста в байтах (всегда 0).
    static constexpr size_t trailer_size() { return 0; }

    /// @brief Заполняет заголовок маршрутизации.
    ///
    /// @param[out] dst Указатель на начало заголовка в собранном
    /// сообщении.
    /// @param[in] total Границы всего сообщения.
    /// @param[in] pload Границы полезной нагрузки.
    void setup_header(
        std::byte *const dst, const total_message_span &total,
        const pload_span &pload) const
    {
        (void)total;
        auto *header   = reinterpret_cast<head_route_setup_with_pload_t *>(dst);
        header->dst_id = setup_.dst_id;
        header->pack_id = setup_.pack_id;
        header->pload_size =
            static_cast<decltype(header->pload_size)>(pload.size_bytes());

        // Нужно убедиться, что запись размера полезной нагрузки помещается
        // в переменную pload_size без сужающих преобразований.
        assert(pload.size_bytes()
               <= std::numeric_limits<decltype(header->pload_size)>::max());
    }

    /// @brief Заглушка для заполнения хвоста.
    ///
    /// @note Так как хвост отсутствует, метод не выполняет никаких
    /// действий. Дополнительный параметр @c total_size оставлен для
    /// совместимости с внутренними вызовами.
    ///
    /// @param[out] dst Указатель на область хвоста.
    /// @param[in] total Границы всего сообщения.
    /// @param[in] total_size Полный размер сообщения в байтах.
    static void setup_trailer(
        std::byte *dst, const total_message_span &total, size_t total_size)
    {
        (void)dst;
        (void)total;
        (void)total_size;
    }

    STV_NO_PADDING_NO_OPTIMIZE_BEGIN

    /// @brief Поле размера полезной нагрузки.
    struct pload_size_t {
        /// @brief Размер полезной нагрузки в байтах.
        uint16_t pload_size{std::numeric_limits<decltype(pload_size)>::min()};
    };

    /// @brief Заголовок маршрутизации с полем размера полезной нагрузки.
    ///
    /// @details
    /// Объединяет @c head_route_setup_t и @c pload_size_t. Именно эта
    /// структура записывается в начало сообщения декоратором.
    // NOLINTNEXTLINE(*-multiple-inheritance)
    struct head_route_setup_with_pload_t:
        public head_route_setup_t,
        public pload_size_t {
    };

    STV_NO_PADDING_NO_OPTIMIZE_END

  private:
    /// @brief Сохраненные параметры маршрутизации.
    head_route_setup_t setup_;
};

/// ----------------------------------------------------------------------------

} // namespace stv

#endif /* SERIAL_DECORATORS_HPP */
