/// @file stvlinkv2_sender.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
///
/// NAME
///     stv::stvlinkv2_sender -- передающий декоратор протокола
///     stvlinkv2.
///
/// DESCRIPTION
///     stvlinkv2_sender предоставляет передающую сторону протокола
///     stvlinkv2: единый декоратор stvlinkv2_sender формирует кадр
///     целиком -- канальный уровень (стартовая последовательность
///     0x55 0x55, поле frame_size), заголовок сообщения (16-битный
///     идентификатор сообщения msg_id и 16-битный размер полезной
///     нагрузки pload_size) и CRC-32 в хвосте. Формат кадра
///     переиспользуется из stvlinkv2_parser.hpp.
///
///     По сравнению с stvlink из заголовка сообщения убрано поле
///     dst_id, идентификатор сообщения msg_id расширен до 16 бит, а
///     контрольная сумма заменена с CRC-16/MODBUS на CRC-32.
///
/// SEE ALSO
///     crc32.hpp, serial_decorators.hpp, stvlinkv2_parser.hpp.

#ifndef STVLINKV2_SENDER_HPP
#define STVLINKV2_SENDER_HPP

#include "stv/communication/crc32.hpp"
#include "stv/communication/serial_decorators.hpp"
#include "stv/communication/stvlinkv2_parser.hpp"
#include "stv/utils.hpp"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>

namespace stv {

/// @brief Передающий декоратор, формирующий кадр stvlinkv2 целиком.
///
/// @details
/// Единый декоратор кадра протокола stvlinkv2, объединяющий канальный
/// уровень и заголовок сообщения:
///   - в начало записывается стартовый кадр @c 0x55 0x55 и поле
///     @c frame_size, равное размеру оставшейся части кадра
///     (заголовок сообщения + полезная нагрузка + все хвосты);
///   - следом записывается заголовок сообщения @ref message_header_t:
///     16-битный идентификатор сообщения @c msg_id и 16-битный размер
///     полезной нагрузки @c pload_size;
///   - в конец записывается 32-битная контрольная сумма (CRC-32),
///     вычисленная по всем байтам сообщения, кроме самого CRC.
///
/// Такая структура позволяет @c serial_parser находить начало кадра в
/// потоке байт, определять длину сообщения и проверять его целостность.
/// Получатель после парсинга получает сообщение вместе с заголовком
/// сообщения и читает из него идентификатор @c msg_id.
///
/// @note
/// Поле @c frame_size не включает в себя размер стартового кадра. Если
/// сообщение состоит из стартового кадра (4 байта: 0x55, 0x55 и
/// 16-битное поле размера), заголовка сообщения (4 байта), полезной
/// нагрузки (5 байт) и CRC-32 (4 байта), то @c frame_size будет равно
/// 13, а общий размер сообщения -- 17 байт.
class stvlinkv2_sender
{
  public:
    STV_NO_PADDING_NO_OPTIMIZE_BEGIN

    /// @brief Параметры заголовка сообщения, задаваемые при создании
    ///     декоратора.
    struct setup_t {
        /// @brief Идентификатор сообщения.
        ///
        /// @details
        /// Получатель выбирает обработчик сообщения по этому идентификатору.
        std::uint16_t msg_id{std::numeric_limits<decltype(msg_id)>::min()};
    };

    /// @brief Заголовок сообщения с полем размера полезной нагрузки.
    ///
    /// @details
    /// Алиас на структуру заголовка сообщения из stvlinkv2_parser: формат
    /// заголовка -- собственность протокола, а не передающего декоратора.
    /// Раскладка полей в памяти и размер (4 байта) соответствуют
    /// бинарному формату заголовка сообщения stvlinkv2.
    using message_header_t = stvlinkv2_parser::message_header_t;

    STV_NO_PADDING_NO_OPTIMIZE_END

    static_assert(sizeof(message_header_t) == 4U,
                  "message_header_t must be 4 bytes according to "
                  "the stvlinkv2 message header format");

    /// @brief Конструирует декоратор с заданными параметрами заголовка.
    ///
    /// @param[in] setup Параметры заголовка сообщения. По умолчанию поле
    ///     @c msg_id равно нулю.
    explicit stvlinkv2_sender(
        const setup_t &setup = setup_t{.msg_id = 0}):
        setup_{setup}
    {
    }

    /// @brief Возвращает размер заголовка.
    ///
    /// @return Размер заголовка в байтах (стартовый кадр + заголовок
    ///     сообщения = 8).
    static constexpr size_t header_size()
    { return stvlinkv2_parser::header_size() + sizeof(message_header_t); }

    /// @brief Возвращает размер заголовка сообщения.
    ///
    /// @return Размер заголовка сообщения в байтах (4).
    static constexpr size_t message_header_size()
    { return sizeof(message_header_t); }

    /// @brief Возвращает размер хвоста.
    ///
    /// @return Размер хвоста в байтах (CRC-32 = 4).
    static constexpr size_t trailer_size()
    { return stvlinkv2_parser::trailer_size(); }

    /// @brief Заполняет стартовый кадр, поле размера и заголовок сообщения.
    ///
    /// @param[out] dst Указатель на начало заголовка в собранном
    /// сообщении.
    /// @param[in] total Границы всего сообщения.
    /// @param[in] pload Границы полезной нагрузки.
    void setup_header(
        std::byte *const dst, const total_message_span &total,
        const pload_span &pload) const
    {
        auto *start_frame =
            reinterpret_cast<stvlinkv2_parser::start_frame_t *>(dst);
        start_frame->start_frame_first  = stvlinkv2_parser::first_byte;
        start_frame->start_frame_second = stvlinkv2_parser::second_byte;

        // Неизвестно, есть ли другие декораторы, поэтому вычислим размер
        // кадра из поля total. Поле frame_size не включает стартовый кадр.
        start_frame->frame_size =
            static_cast<decltype(start_frame->frame_size)>(
                total.size_bytes() - stvlinkv2_parser::header_size());

        auto *header = reinterpret_cast<message_header_t *>(
            dst + stvlinkv2_parser::header_size());
        header->msg_id = setup_.msg_id;
        header->pload_size =
            static_cast<decltype(header->pload_size)>(pload.size_bytes());

        // Нужно убедиться, что запись размера полезной нагрузки помещается
        // в переменную pload_size без сужающих преобразований.
        assert(pload.size_bytes()
               <= std::numeric_limits<decltype(header->pload_size)>::max());
    }

    /// @brief Записывает CRC-32 в хвост сообщения.
    ///
    /// @param[out] dst Указатель на начало хвоста в собранном
    /// сообщении.
    /// @param[in] total Границы всего сообщения.
    static void setup_trailer(
        std::byte *dst, const total_message_span &total)
    {
        const auto computed_crc =
            stv::crc32(total.data(), total.size_bytes() - trailer_size());
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
    { return stvlinkv2_parser::is_crc_valid(total); }

  private:
    /// @brief Сохраненные параметры заголовка сообщения.
    setup_t setup_;
};

} // namespace stv

#endif // STVLINKV2_SENDER_HPP
