/// @file mavlink_v2_frame.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
///
/// NAME
///     stv::mavlink_v2_frame -- парсерный декоратор кадра протокола
///     MAVLink v2.
///
/// DESCRIPTION
///     Статический интерфейс декоратора кадра для парсера. Описывает
///     формат MAVLink v2: стартовый байт 0xFD, поле длины полезной
///     нагрузки (1 байт), флаги, заголовок маршрутизации (seq, sysid,
///     compid, msgid), полезная нагрузка, CRC-16/X.25 (2 байта) и
///     необязательная подпись (13 байт). CRC вычисляется по байтам от
///     поля len до конца полезной нагрузки (без стартового байта 0xFD)
///     с досчётом дополнительного байта CRC_EXTRA, специфичного для
///     msgid.
///
/// SEE ALSO
///     crc16.hpp, serial_decorators.hpp, serial_parser.hpp.

#ifndef MAVLINK_V2_FRAME_HPP
#define MAVLINK_V2_FRAME_HPP

#include "stv/communication/crc16.hpp"
#include "stv/communication/serial_decorators.hpp"
#include <bit>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace stv {

/// @brief Парсерный декоратор кадра протокола MAVLink v2.
///
/// @details
/// Предоставляет статический интерфейс, используемый serial_parser для
/// выделения кадров MAVLink v2 из потока байт:
///   - стартовый байт @c 0xFD;
///   - заголовок размером 3 байта (стартовый байт, поле len, поле
///     incompat_flags);
///   - хвост размером 2 байта (CRC-16/X.25);
///   - CRC вычисляется по байтам от поля len до конца полезной нагрузки
///     (то есть без стартового байта @c 0xFD) с досчётом байта CRC_EXTRA,
///     специфичного для msgid;
///   - кадр с установленным битом @c 0x01 в поле incompat_flags содержит
///     в конце 13 байт подписи, расположенных после CRC.
///
/// Полный формат кадра:
///   0xFD | len | incompat_flags | compat_flags | seq | sysid | compid |
///   msgid (3 байта, little-endian) | payload (len байт) |
///   CRC (2 байта) | подпись (13 байт, только при incompat_flags & 0x01).
///
/// После успешного приёма парсер отрезает заголовок (стартовый байт,
/// len, incompat_flags) и хвост CRC. Раскладка оставшегося сообщения:
///   [compat_flags, seq, sysid, compid, msgid (3 байта, LE), payload].
///
/// @note
/// Подписанные кадры (бит @c 0x01 в поле incompat_flags) и кадры с любыми
/// другими ненулевыми битами incompat_flags отклоняются в is_crc_valid():
/// верификация подписи не реализована, а @c trailer_size() учитывает
/// только CRC (2 байта), поэтому принять подписанный кадр без проверки
/// нельзя — 13 байт подписи оказались бы в хвосте отрезанного сообщения.
/// Кадры с неизвестными битами incompat_flags спецификация требует
/// отбрасывать.
///
/// @tparam TCrcExtraProvider Провайдер байта CRC_EXTRA. Должен
///     предоставлять статический метод
///     @c static auto crc_extra_for(std::uint32_t msgid) ->
///     std::optional<std::uint8_t>, возвращающий CRC_EXTRA для
///     известного msgid или @c std::nullopt для неизвестного.
template<typename TCrcExtraProvider>
class mavlink_v2_frame
{
    using length_type = std::uint8_t;
    using crc_type    = std::uint16_t;

    /// @brief Смещение поля len в кадре.
    static constexpr size_t len_offset{1U};

    /// @brief Смещение поля incompat_flags в кадре.
    static constexpr size_t incompat_flags_offset{2U};

    /// @brief Смещение младшего байта поля msgid в кадре.
    static constexpr size_t msgid_offset{7U};

    /// @brief Размер полного заголовка MAVLink v2 (0xFD..msgid).
    static constexpr size_t full_header_size{10U};

    /// @brief Размер подписи в подписанном кадре.
    static constexpr size_t signature_size{13U};

    /// @brief Бит наличия подписи в поле incompat_flags.
    static constexpr std::byte signature_flag{0x01};

  public:
    /// @brief Значение стартового байта кадра.
    static constexpr std::byte first_byte{0xFD};

    /// @brief Проверяет совпадение начала кадра со стартовым байтом.
    ///
    /// @details
    /// Второй байт кадра (len) может принимать любое значение, поэтому
    /// проверяется только первый байт.
    ///
    /// @param[in] first Первый байт peek-нутого заголовка.
    /// @return @c true, если первый байт равен @c 0xFD; @c false в
    ///     противном случае.
    static constexpr bool matches(
        std::byte first, std::byte /*second*/)
    { return first == first_byte; }

    /// @brief Возвращает размер заголовка кадра.
    ///
    /// @return Размер заголовка в байтах (0xFD + len + incompat_flags = 3).
    static constexpr size_t header_size() { return 3U; }

    /// @brief Возвращает размер хвоста кадра.
    ///
    /// @return Размер хвоста в байтах (CRC = 2).
    static constexpr size_t trailer_size() { return sizeof(crc_type); }

    /// @brief Вычисляет полный размер кадра по заголовку.
    ///
    /// @details
    /// Заголовок должен содержать как минимум @c header_size() байт.
    /// Полный размер кадра равен 10 байтам полного заголовка MAVLink,
    /// len байтам полезной нагрузки, 2 байтам CRC и 13 байтам подписи
    /// (только при установленном бите @c 0x01 в поле incompat_flags).
    ///
    /// @param[in] header Span, содержащий peek-нутый заголовок кадра.
    /// @return Полный размер кадра в байтах.
    static size_t total_frame_size(
        const stv::total_message_span &header)
    {
        assert(header.size_bytes() >= header_size());

        const bool is_signed =
            (header[incompat_flags_offset] & signature_flag) != std::byte{0x00};

        return full_header_size + static_cast<length_type>(header[len_offset])
               + trailer_size() + (is_signed ? signature_size : 0U);
    }

    /// @brief Проверяет корректность CRC принятого кадра.
    ///
    /// @details
    /// CRC вычисляется по байтам от поля len до конца полезной нагрузки
    /// (то есть без стартового байта @c 0xFD) с досчётом байта CRC_EXTRA,
    /// предоставляемого @c TCrcExtraProvider для msgid кадра. Кадр с
    /// неизвестным провайдеру msgid считается невалидным.
    ///
    /// Кадр с любым ненулевым полем incompat_flags отклоняется: единственный
    /// определённый спецификацией бит @c 0x01 означает наличие подписи, а
    /// её верификация не реализована; остальные биты спецификация требует
    /// отбрасывать как неизвестные. Когда поддержка подписи будет
    /// реализована, здесь нужно будет разрешить бит @c 0x01 и отклонять
    /// только неизвестные биты (@c incompat_flags & ~0x01).
    ///
    /// @param[in] total Границы всего принятого кадра.
    /// @return @c true, если CRC совпадает; @c false в противном случае.
    static bool is_crc_valid(
        const stv::total_message_span &total)
    {
        // Чтение CRC через memcpy предполагает, что порядок байт CRC на
        // проводе (little-endian) совпадает с порядком байт платформы.
        static_assert(std::endian::native == std::endian::little,
                      "mavlink_v2_frame::is_crc_valid requires a little-endian"
                      " platform");

        // Кадр должен содержать как минимум заголовок и хвост, иначе
        // вычисление размера данных для CRC приведёт к underflow size_t.
        if(total.size_bytes() < (header_size() + trailer_size()))
        {
            return false;
        }

        // Подпись не верифицируется, а неизвестные биты incompat_flags
        // спецификация требует отбрасывать: принимаются только кадры с
        // нулевым полем incompat_flags (см. документацию класса).
        if(total[incompat_flags_offset] != std::byte{0x00})
        {
            return false;
        }

        // Для чтения msgid и CRC кадр должен содержать полный заголовок
        // MAVLink и хвост CRC.
        if(total.size_bytes() < (full_header_size + trailer_size()))
        {
            return false;
        }

        // Сборка msgid из 3 байт, little-endian.
        const auto msgid =
            static_cast<std::uint32_t>(total[msgid_offset])
            | (static_cast<std::uint32_t>(total[msgid_offset + 1U]) << 8U)
            | (static_cast<std::uint32_t>(total[msgid_offset + 2U]) << 16U);

        const auto crc_extra = TCrcExtraProvider::crc_extra_for(msgid);
        if(!crc_extra.has_value())
        {
            return false;
        }

        // Данные для CRC начинаются с байта len (сразу после 0xFD) и
        // заканчиваются перед CRC.
        const auto data_for_crc = total.subspan(len_offset);
        auto       expected_crc = stv::crc16_x25(
            data_for_crc.data(), data_for_crc.size_bytes() - trailer_size());
        expected_crc =
            stv::crc16_x25_accumulate(expected_crc, std::byte{*crc_extra});

        crc_type         received_crc{};
        const std::byte *crc_pos =
            total.data() + total.size_bytes() - trailer_size();
        std::memcpy(&received_crc, crc_pos, sizeof(received_crc));

        return received_crc == expected_crc;
    }
};

} // namespace stv

#endif /* MAVLINK_V2_FRAME_HPP */
