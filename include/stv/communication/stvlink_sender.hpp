/// @file stvlink_sender.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
///
/// NAME
///     stv::stvlink_sender -- передающие декораторы протокола stvlink.
///
/// DESCRIPTION
///     stvlink_sender предоставляет передающую сторону протокола
///     stvlink: декоратор stvlink_frame_tx формирует канальный уровень
///     кадра (стартовая последовательность 0xAA 0xAA, поле frame_size,
///     CRC-16/MODBUS в хвосте), а stvlink_route_tx добавляет заголовок
///     маршрутизации (идентификатор получателя dst_id, порядковый номер
///     пакета pack_id и размер полезной нагрузки pload_size). Формат
///     кадра переиспользуется из stvlink_frame.hpp.
///
///     stvlink_frame_tx
///         Добавляет в начало сообщения стартовый кадр
///         0xAA 0xAA и 16-битное поле frame_size, равное
///         размеру оставшейся части кадра. В конец
///         записывается CRC-16 (начальное значение 0xFFFF,
///         полином 0xA001), вычисленный по всем байтам
///         сообщения, кроме самого CRC. Метод
///         is_crc_valid() проверяет целостность принятого
///         кадра.
///
///     stvlink_route_tx
///         Добавляет маршрутизацию: идентификатор
///         получателя dst_id, порядковый номер пакета
///         pack_id и 16-битный размер полезной нагрузки
///         pload_size. Параметры задаются через структуру
///         setup_t. При превышении размера
///         полезной нагрузки максимального значения
///         uint16_t срабатывает assert.
///
/// EXAMPLE
///     Пример формирования кадра со стартовой
///     последовательностью и CRC:
///     ```cpp
///     #include <stv/communication/stvlink_sender.hpp>
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
///         stvlink_frame_tx decorator;
///         decorator.setup_header(
///             buffer.data(),
///             total_message_span(buffer.data(), buffer.size()),
///             pload_span(pload.data(), pload.size()));
///
///         std::memcpy(
///             buffer.data()
///                 + stvlink_frame_tx::header_size(),
///             pload.data(), pload.size());
///
///         decorator.setup_trailer(
///             buffer.data()
///                 + stvlink_frame_tx::header_size()
///                 + pload.size(),
///             total_message_span(buffer.data(), buffer.size()));
///
///         bool ok = stvlink_frame_tx::is_crc_valid(
///             total_message_span(buffer.data(), buffer.size()));
///
///         return ok ? 0 : 1;
///     }
///     ```
///
///     Пример маршрутизации:
///     ```cpp
///     stv::stvlink_route_tx::setup_t route{
///         .dst_id = 1, .pack_id = 7};
///     stv::stvlink_route_tx route_decorator(route);
///     route_decorator.setup_header(dst, total, pload);
///     ```
///
/// SEE ALSO
///     stvlink_frame.hpp, serial_decorators.hpp, serial_sender.hpp.

#ifndef STVLINK_SENDER_HPP
#define STVLINK_SENDER_HPP

#include "stv/communication/crc16.hpp"
#include "stv/communication/serial_decorators.hpp"
#include "stv/communication/stvlink_frame.hpp"
#include "stv/utils.hpp"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>

namespace stv {

/// @brief Передающий декоратор, добавляющий стартовый кадр и CRC-16.
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
class stvlink_frame_tx
{
  public:
    /// @brief Возвращает размер заголовка.
    ///
    /// @return Размер заголовка в байтах.
    static constexpr size_t header_size()
    { return stvlink_frame::header_size(); }

    /// @brief Возвращает размер хвоста.
    ///
    /// @return Размер хвоста в байтах.
    static constexpr size_t trailer_size()
    { return stvlink_frame::trailer_size(); }

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
        auto *start_frame =
            reinterpret_cast<stvlink_frame::start_frame_t *>(dst);
        start_frame->start_frame_first  = stvlink_frame::first_byte;
        start_frame->start_frame_second = stvlink_frame::second_byte;

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
        const auto computed_crc = stv::crc16_modbus(
            total.data(), total.size_bytes() - trailer_size());
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
    { return stvlink_frame::is_crc_valid(total); }
};

/// ----------------------------------------------------------------------------

/// @brief Передающий декоратор, добавляющий маршрутизацию и размер
///     полезной нагрузки.
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
class stvlink_route_tx
{
  public:
    STV_NO_PADDING_NO_OPTIMIZE_BEGIN

    /// @brief Параметры маршрутизации, задаваемые при создании декоратора.
    struct setup_t {
        /// @brief Идентификатор получателя сообщения.
        ///
        /// @details
        /// Используется @c stvlink_route для выбора очереди, в
        /// которую будет направлен пакет.
        std::uint8_t dst_id{std::numeric_limits<decltype(dst_id)>::min()};

        /// @brief Порядковый номер пакета.
        ///
        /// @details
        /// Позволяет получателю различать сообщения и отслеживать их
        /// порядок доставки.
        std::uint8_t pack_id{std::numeric_limits<decltype(pack_id)>::min()};
    };

    /// @brief Заголовок маршрутизации с полем размера полезной нагрузки.
    ///
    /// @details
    /// Именно эта структура записывается в начало сообщения декоратором.
    /// Раскладка полей в памяти и размер (4 байта) соответствуют
    /// бинарному формату заголовка маршрутизации stvlink.
    struct routing_header_t {
        /// @brief Идентификатор получателя сообщения.
        std::uint8_t dst_id{std::numeric_limits<decltype(dst_id)>::min()};

        /// @brief Порядковый номер пакета.
        std::uint8_t pack_id{std::numeric_limits<decltype(pack_id)>::min()};

        /// @brief Размер полезной нагрузки в байтах.
        std::uint16_t pload_size{
            std::numeric_limits<decltype(pload_size)>::min(),
        };
    };

    STV_NO_PADDING_NO_OPTIMIZE_END

    static_assert(sizeof(routing_header_t) == 4U,
                  "routing_header_t должен занимать 4 байта согласно "
                  "формату заголовка маршрутизации stvlink");

    /// @brief Конструирует декоратор с заданными параметрами маршрутизации.
    ///
    /// @param[in] setup Параметры маршрутизации. По умолчанию оба поля
    /// равны нулю.
    explicit stvlink_route_tx(
        const setup_t &setup = setup_t{.dst_id = 0, .pack_id = 0}):
        setup_{setup}
    {
    }

    /// @brief Возвращает размер заголовка.
    ///
    /// @return Размер заголовка в байтах.
    static constexpr size_t header_size() { return sizeof(routing_header_t); }

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
        auto *header    = reinterpret_cast<routing_header_t *>(dst);
        header->dst_id  = setup_.dst_id;
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

  private:
    /// @brief Сохраненные параметры маршрутизации.
    setup_t setup_;
};

/// ----------------------------------------------------------------------------

} // namespace stv

#endif /* STVLINK_SENDER_HPP */
