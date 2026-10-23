/// @file rmp_sender.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
///
/// NAME
///     stv::rmp_sender -- передающий декоратор протокола RMP.
///
/// DESCRIPTION
///     rmp_sender предоставляет передающую сторону протокола RMP
///     (Radio Message Parser): декоратор формирует кадр фиксированной
///     длины (шаблонный параметр TMessageSize, по умолчанию 20 байт) --
///     стартовая последовательность 0xAA 0x55, полезная нагрузка
///     размером TMessageSize - 4 байта и CRC-16/CCITT (полином 0x1021,
///     начальное значение 0xFFFF, без рефлексии) в хвосте, записанный
///     little-endian. CRC вычисляется только по полезной нагрузке.
///     Формат кадра бит-в-бит совместим с библиотекой
///     third_party/radio_message_parser и переиспользуется из
///     rmp_parser.hpp.
///
///     Декоратор полностью статический: состояния, параметров
///     заголовка (setup_t) и маршрутизации (route_t) нет, все методы
///     -- статические. Кадр на проводе всегда имеет размер TMessageSize:
///     полезная нагрузка короче TMessageSize - 4 дополняется нулями,
///     CRC вычисляется по полной полезной нагрузке, включая дополнение.
///
/// EXAMPLE
///     ```cpp
///     #include "stv/communication/rmp_sender.hpp"
///     #include "stv/communication/serial_sender.hpp"
///     #include "stv/containers/simbuff.hpp"
///     #include <etl/queue.h>
///
///     using queue_type = etl::queue<stv::sim_buff<stv::empty_mutex>, 10>;
///     queue_type queue;
///
///     auto buffer =
///         stv::make_serial_message_buffer(queue, stv::rmp_sender<>{});
///     {
///         // Короткая нагрузка дополняется нулями до 16 байт, кадр на
///         // проводе всегда имеет размер 20 байт.
///         const auto msg = buffer.request(3U);
///         if (msg) {
///             // заполнить область полезной нагрузки
///         }
///     } // кадр собирается и помещается в очередь
///     ```
///
/// SEE ALSO
///     crc16.hpp, rmp_parser.hpp, serial_decorators.hpp, serial_sender.hpp,
///     third_party/radio_message_parser/radio_message_parser.h.

#ifndef RMP_SENDER_HPP
#define RMP_SENDER_HPP

#include "stv/communication/crc16.hpp"
#include "stv/communication/rmp_parser.hpp"
#include "stv/communication/serial_decorators.hpp"
#include <bit>
#include <cassert>
#include <cstddef>
#include <cstring>

namespace stv {

/// @brief Передающий декоратор кадра протокола RMP.
///
/// @details
/// Формирует кадр RMP фиксированной длины вокруг полезной нагрузки:
///   - заголовок размером 2 байта: стартовая последовательность
///     @c 0xAA 0x55;
///   - полезная нагрузка размером не более @c max_pload_size() байт;
///   - хвост размером 2 байта: CRC-16/CCITT, вычисленный только по
///     полезной нагрузке и записанный little-endian.
///
/// Декоратор полностью статический и не хранит состояния: параметров
/// @c setup_t и @c route_t нет, единственный параметр кадра -- его
/// фиксированный размер @c TMessageSize. Буфер сообщений отклоняет
/// request() с полезной нагрузкой больше @c max_pload_size(), возвращая
/// невалидное сообщение (см. serial_message_buffer_base), а нагрузка
/// короче @c fixed_pload_size() дополняется нулями: кадр на проводе всегда
/// имеет размер @c TMessageSize.
///
/// Диапазон байт CRC совпадает с rmp_parser::is_crc_valid, поэтому
/// кадр, собранный этим декоратором, проходит проверку CRC парсерным
/// декоратором rmp_parser.
///
/// @tparam TMessageSize Размер кадра в байтах (по умолчанию 20). Должен
///     быть не менее 4 байт: 2 байта стартовой последовательности и
///     2 байта CRC; при меньшем размере полезная нагрузка была бы
///     отрицательной.
template<std::size_t TMessageSize = 20U>
class rmp_sender
{
  public:
    /// @brief Размер кадра должен вмещать стартовую последовательность
    ///     и CRC.
    static_assert(TMessageSize >= 4U,
                  "TMessageSize must be at least 4 bytes: 2 bytes "
                  "of start sequence and 2 bytes of CRC");

    /// @brief Возвращает размер заголовка кадра.
    ///
    /// @return Размер заголовка в байтах (2).
    static constexpr size_t header_size()
    { return rmp_parser<TMessageSize>::header_size(); }

    /// @brief Возвращает размер хвоста кадра.
    ///
    /// @return Размер хвоста в байтах (CRC-16 = 2).
    static constexpr size_t trailer_size()
    { return rmp_parser<TMessageSize>::trailer_size(); }

    /// @brief Возвращает максимальный размер полезной нагрузки кадра.
    ///
    /// @details
    /// Буфер сообщений отклоняет request() с бо́льшей полезной нагрузкой,
    /// возвращая невалидное сообщение (см. serial_message_buffer_base).
    ///
    /// @return Максимальный размер полезной нагрузки в байтах
    ///     (TMessageSize - 4).
    static constexpr size_t max_pload_size()
    { return TMessageSize - header_size() - trailer_size(); }

    /// @brief Возвращает фиксированный размер полезной нагрузки кадра.
    ///
    /// @details
    /// Буфер сообщений выделяет под полезную нагрузку ровно столько байт
    /// независимо от запрошенного размера: запрошенная нагрузка короче
    /// дополняется нулями до этого размера, поэтому кадр RMP на проводе
    /// всегда имеет размер TMessageSize, а CRC вычисляется по полной
    /// полезной нагрузке, включая нулевое дополнение (см.
    /// composite_serial_message).
    ///
    /// @return Фиксированный размер полезной нагрузки в байтах
    ///     (TMessageSize - 4).
    static constexpr size_t fixed_pload_size() { return max_pload_size(); }

    /// @brief Заполняет стартовую последовательность заголовка кадра.
    ///
    /// @param[out] dst Указатель на начало заголовка в собранном
    ///     сообщении.
    /// @param[in] total Границы всего сообщения.
    /// @param[in] pload Границы полезной нагрузки.
    static void setup_header(
        std::byte *const dst, const total_message_span &total,
        const pload_span &pload)
    {
        // Параметры используются только в assert, поэтому в релизной
        // сборке (NDEBUG) предупреждение об неиспользуемых параметрах
        // подавляется явным приведением к void.
        (void)total;
        (void)pload;

        // Гарантируется проверками на уровне request(): max_pload_size()
        // ограничивает полезную нагрузку, общий размер кадра фиксирован.
        assert(pload.size_bytes() <= max_pload_size());
        assert(total.size_bytes()
               == (header_size() + pload.size_bytes() + trailer_size()));

        dst[0U] = rmp_parser<TMessageSize>::first_byte;
        dst[1U] = rmp_parser<TMessageSize>::second_byte;
    }

    /// @brief Записывает CRC-16/CCITT в хвост кадра.
    ///
    /// @details
    /// CRC вычисляется только по полезной нагрузке (байты от начала
    /// @c total со смещения @c header_size() до конца хвоста) и
    /// записывается little-endian. Диапазон байт CRC совпадает с
    /// rmp_parser::is_crc_valid.
    ///
    /// @param[out] dst Указатель на начало хвоста в собранном сообщении.
    /// @param[in] total Границы всего сообщения.
    static void setup_trailer(
        std::byte *dst, const total_message_span &total)
    {
        // Запись CRC через memcpy предполагает, что порядок байт CRC на
        // проводе (little-endian) совпадает с порядком байт платформы.
        static_assert(std::endian::native == std::endian::little,
                      "rmp_sender::setup_trailer requires a little-endian"
                      " platform");

        assert(total.size_bytes() >= (header_size() + trailer_size()));

        const auto computed_crc = stv::crc16_ccitt(
            total.data() + header_size(),
            total.size_bytes() - header_size() - trailer_size());
        std::memcpy(dst, &computed_crc, sizeof(computed_crc));
    }
};

} // namespace stv

#endif // RMP_SENDER_HPP
