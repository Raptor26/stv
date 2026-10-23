/// @file rmp_parser.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
///
/// NAME
///     stv::rmp_parser -- парсерный декоратор кадра протокола RMP.
///
/// DESCRIPTION
///     Статический интерфейс декоратора кадра для парсера. Описывает
///     формат RMP (Radio Message Parser): кадр фиксированной длины
///     (шаблонный параметр TMessageSize, по умолчанию 20 байт) со
///     стартовой последовательностью 0xAA 0x55, полезной нагрузкой
///     размером TMessageSize - 4 байта и CRC-16/CCITT (полином 0x1021,
///     начальное значение 0xFFFF, без рефлексии) в хвосте, записанным
///     little-endian. CRC вычисляется только по полезной нагрузке.
///     Полей длины, seq и msg_id в кадре нет. Формат бит-в-бит
///     совместим с библиотекой third_party/radio_message_parser и
///     переиспользуется передающей стороной из rmp_sender.hpp.
///
///     После успешного приёма парсер отрезает стартовую
///     последовательность и CRC, поэтому получатель читает из очереди
///     голую полезную нагрузку кадра.
///
/// EXAMPLE
///     ```cpp
///     #include "stv/communication/rmp_parser.hpp"
///
///     // Кадр из 20 байт: 0xAA 0x55 | payload (16 байт) | CRC-16 (2 байта).
///     using frame_type = stv::rmp_parser<>;
///     static_assert(frame_type::total_frame_size(...) == 20U);
///     ```
///
/// SEE ALSO
///     crc16.hpp, rmp_sender.hpp, serial_decorators.hpp, serial_parser.hpp,
///     third_party/radio_message_parser/radio_message_parser.h.

#ifndef RMP_PARSER_HPP
#define RMP_PARSER_HPP

#include "stv/communication/crc16.hpp"
#include "stv/communication/serial_decorators.hpp"
#include "stv/utils.hpp"
#include <bit>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace stv {

/// @brief Парсерный декоратор кадра протокола RMP.
///
/// @details
/// Предоставляет статический интерфейс, используемый serial_parser для
/// выделения кадров RMP из потока байт:
///   - стартовая последовательность @c 0xAA 0x55 (фиксированные
///     константы, не конфигурируются);
///   - заголовок размером 2 байта (стартовая последовательность);
///   - хвост размером 2 байта (CRC-16/CCITT, little-endian);
///   - кадр имеет фиксированную длину @c TMessageSize, поля длины в кадре
///     нет: размер известен заранее;
///   - CRC вычисляется только по полезной нагрузке (байты от 2 до
///     @c TMessageSize - 3), без стартовой последовательности и самого CRC;
///   - после успешного приёма кадра отрезается заголовок и хвост.
///
/// Формат бит-в-бит совместим с эталонной C-библиотекой
/// third_party/radio_message_parser (rmpONE_MESSAGE_SIZE_IN_BYTES = 20).
///
/// @tparam TMessageSize Размер кадра в байтах (по умолчанию 20). Должен
///     быть не менее 4 байт: 2 байта стартовой последовательности и
///     2 байта CRC; при меньшем размере полезная нагрузка была бы
///     отрицательной.
template<std::size_t TMessageSize = 20U>
class rmp_parser
{
    /// @brief Тип контрольной суммы кадра.
    using crc_type = std::uint16_t;

  public:
    /// @brief Размер кадра должен вмещать стартовую последовательность
    ///     и CRC.
    static_assert(TMessageSize >= 4U,
                  "TMessageSize must be at least 4 bytes: 2 bytes "
                  "of start sequence and 2 bytes of CRC");

    STV_NO_PADDING_NO_OPTIMIZE_BEGIN

    /// @brief Стартовый кадр сообщения.
    ///
    /// @details
    /// Поля расположены подряд без выравнивания. Стартовая
    /// последовательность фиксирована и не конфигурируется.
    struct start_frame_t {
        /// @brief Первый байт стартовой последовательности.
        std::byte start_frame_first;

        /// @brief Второй байт стартовой последовательности.
        std::byte start_frame_second;
    };

    STV_NO_PADDING_NO_OPTIMIZE_END

    static_assert(sizeof(start_frame_t) == 2U,
                  "start_frame_t must be 2 bytes according to the "
                  "RMP frame format");

    /// @brief Первый байт стартовой последовательности.
    static constexpr std::byte first_byte{0xAA};

    /// @brief Второй байт стартовой последовательности.
    static constexpr std::byte second_byte{0x55};

    /// @brief Проверяет совпадение начала кадра со стартовой
    ///     последовательностью.
    ///
    /// @param[in] first Первый байт peek-нутого заголовка.
    /// @param[in] second Второй байт peek-нутого заголовка.
    /// @return @c true, если байты совпадают со стартовой
    ///     последовательностью @c 0xAA 0x55; @c false в противном случае.
    static constexpr bool matches(
        std::byte first, std::byte second)
    { return (first == first_byte) && (second == second_byte); }

    /// @brief Возвращает размер заголовка кадра.
    ///
    /// @return Размер заголовка в байтах (2).
    static constexpr size_t header_size() { return sizeof(start_frame_t); }

    /// @brief Возвращает размер хвоста кадра.
    ///
    /// @return Размер хвоста в байтах (CRC-16 = 2).
    static constexpr size_t trailer_size() { return sizeof(crc_type); }

    /// @brief Вычисляет полный размер кадра по заголовку.
    ///
    /// @details
    /// Кадр RMP имеет фиксированную длину, не содержащуюся в самом кадре,
    /// поэтому размер не читается из заголовка, а возвращается как
    /// шаблонный параметр @c TMessageSize.
    ///
    /// @param[in] header Span, содержащий peek-нутый заголовок кадра.
    /// @return Полный размер кадра в байтах (TMessageSize).
    static size_t total_frame_size(
        [[maybe_unused]] const stv::total_message_span &header)
    {
        assert(header.size_bytes() >= header_size());
        return TMessageSize;
    }

    /// @brief Проверяет корректность CRC принятого кадра.
    ///
    /// @details
    /// Вычисляет ожидаемый CRC-16/CCITT по полезной нагрузке кадра (байты
    /// от @c header_size() до @c TMessageSize - trailer_size()) и сравнивает
    /// его со значением, записанным в конце кадра little-endian.
    ///
    /// @param[in] total Границы всего принятого кадра.
    /// @return @c true, если CRC совпадает; @c false в противном случае
    ///     или если размер кадра меньше @c TMessageSize.
    static bool is_crc_valid(
        const stv::total_message_span &total)
    {
        // Чтение CRC через memcpy предполагает, что порядок байт CRC на
        // проводе (little-endian) совпадает с порядком байт платформы.
        static_assert(std::endian::native == std::endian::little,
                      "rmp_parser::is_crc_valid requires a little-endian"
                      " platform");

        // Кадр короче фиксированного размера приводил бы к чтению за
        // пределами буфера.
        if(total.size_bytes() < TMessageSize)
        {
            return false;
        }

        // Вычисляем ожидаемый CRC только по полезной нагрузке (без
        // стартовой последовательности и хвоста).
        const auto expected_crc =
            stv::crc16_ccitt(total.data() + header_size(),
                             TMessageSize - header_size() - trailer_size());
        static_assert(sizeof(expected_crc) == sizeof(crc_type),
                      "returned CRC size must match "
                      "crc_type");

        // Безопасно читаем полученный CRC из конца кадра.
        crc_type         received_crc{};
        const std::byte *crc_pos = total.data() + TMessageSize - trailer_size();
        std::memcpy(&received_crc, crc_pos, sizeof(received_crc));

        return received_crc == expected_crc;
    }
};

} // namespace stv

#endif // RMP_PARSER_HPP
