/// @file stvlink_frame.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
///
/// NAME
///     stv::stvlink_frame -- парсерный декоратор кадра протокола
///     stvlink.
///
/// DESCRIPTION
///     Статический интерфейс декоратора кадра для парсера. Описывает
///     формат stvlink: стартовая последовательность 0xAA 0xAA, поле
///     длины кадра frame_size (2 байта), полезная нагрузка,
///     CRC-16/MODBUS (2 байта). CRC вычисляется по всему кадру,
///     включая стартовую последовательность. Формат кадра
///     переиспользуется передающей стороной из stvlink_sender.hpp.
///
/// SEE ALSO
///     crc16.hpp, serial_decorators.hpp, serial_parser.hpp,
///     stvlink_sender.hpp.

#ifndef STVLINK_FRAME_HPP
#define STVLINK_FRAME_HPP

#include "stv/communication/crc16.hpp"
#include "stv/communication/serial_decorators.hpp"
#include "stv/utils.hpp"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace stv {

/// @brief Парсерный декоратор кадра протокола stvlink.
///
/// @details
/// Предоставляет статический интерфейс, используемый serial_parser для
/// выделения кадров из потока байт:
///   - стартовая последовательность @c 0xAA 0xAA;
///   - заголовок размером 4 байта (2 байта стартовой последовательности +
///     2 байта поля размера);
///   - хвост размером 2 байта (CRC-16/MODBUS);
///   - CRC вычисляется по всему кадру, включая стартовую последовательность;
///   - после успешного приёма кадра отрезается заголовок и хвост.
class stvlink_frame
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

    /// @brief Первый байт стартовой последовательности.
    static constexpr std::byte first_byte{0xAA};

    /// @brief Второй байт стартовой последовательности.
    static constexpr std::byte second_byte{0xAA};

    /// @brief Проверяет совпадение начала кадра со стартовой
    ///     последовательностью.
    ///
    /// @param[in] first Первый байт peek-нутого заголовка.
    /// @param[in] second Второй байт peek-нутого заголовка.
    /// @return @c true, если байты совпадают со стартовой
    ///     последовательностью @c 0xAA 0xAA; @c false в противном случае.
    static constexpr bool matches(
        std::byte first, std::byte second)
    { return (first == first_byte) && (second == second_byte); }

    /// @brief Возвращает размер заголовка кадра.
    ///
    /// @return Размер заголовка в байтах (S + L = 4).
    static constexpr size_t header_size() { return sizeof(start_frame_t); }

    /// @brief Возвращает размер хвоста кадра.
    ///
    /// @return Размер хвоста в байтах (CRC = 2).
    static constexpr size_t trailer_size() { return sizeof(crc_type); }

    /// @brief Вычисляет полный размер кадра по заголовку.
    ///
    /// @details
    /// Заголовок должен содержать как минимум @c header_size() байт.
    /// Полный размер кадра равен размеру заголовка плюс значение поля
    /// @c frame_size, записанного в заголовке.
    ///
    /// @param[in] header Span, содержащий peek-нутый заголовок кадра.
    /// @return Полный размер кадра в байтах.
    static size_t total_frame_size(
        const stv::total_message_span &header)
    {
        assert(header.size_bytes() >= header_size());
        const auto *start_frame =
            reinterpret_cast<const start_frame_t *>(header.data());
        return header_size() + start_frame->frame_size;
    }

    /// @brief Проверяет корректность CRC принятого кадра.
    ///
    /// @details
    /// Вычисляет ожидаемый CRC по всем байтам кадра, кроме хвоста,
    /// и сравнивает его со значением, записанным в конце буфера.
    ///
    /// @param[in] total Границы всего принятого кадра.
    /// @return @c true, если CRC совпадает; @c false в противном случае.
    static auto is_crc_valid(
        const stv::total_message_span &total)
    {
        // Вычисляем ожидаемый CRC по данным (без трейлера).
        const auto expected_crc = stv::crc16_modbus(
            total.data(), total.size_bytes() - trailer_size());

        // Безопасно читаем полученный CRC из конца буфера.
        crc_type         received_crc{};
        const std::byte *crc_pos =
            total.data() + total.size_bytes() - trailer_size();
        std::memcpy(&received_crc, crc_pos, sizeof(received_crc));

        return received_crc == expected_crc;
    }
};

} // namespace stv

#endif /* STVLINK_FRAME_HPP */
