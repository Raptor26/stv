/// @file serial_decorators.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// @copyright (c) 2025 Gagaring
///
/// MIT License:
///
/// Permission is hereby granted, free of charge, to any person obtaining a copy
/// of this software and associated documentation files (the 'Software'), to
/// deal in the Software without restriction, including without limitation the
/// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
/// sell copies of the Software, and to permit persons to whom the Software is
/// furnished to do so, subject to the following conditions:
///
/// The above copyright notice and this permission notice shall be included in
/// all copies or substantial portions of the Software.
///
/// THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
/// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
/// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
/// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
/// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
/// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
/// IN THE SOFTWARE.

#ifndef SERIAL_DECORATORS_HPP
#define SERIAL_DECORATORS_HPP

#include "stv/utils.hpp"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <span>
#include <limits>

namespace stv {

/// @brief Тип, указывающий на все сообщение в целом.
class total_message_span: public std::span<const std::byte>
{
    using std::span<const std::byte>::span;
};

/// ----------------------------------------------------------------------------

/// @brief Псевдоним типа, указывающий на полезную нагрузку.
using pload_span = std::span<const std::byte>;

/// ----------------------------------------------------------------------------

/// @brief Пустой декоратор. Не добавляет ничего к полезной нагрузке.
struct empty_serial_decorator {
    static constexpr size_t header_size() { return 0U; }

    static constexpr size_t trailer_size() { return 0U; }

    //
    static void setup_header(
        std::byte *const dst, const total_message_span &total,
        const pload_span &pload)
    {
        (void)dst;
        (void)total;
        (void)pload;
    }

    static void setup_trailer(
        std::byte *dst, const total_message_span &total)
    {
        (void)dst;
        (void)total;
    }
};

/// ----------------------------------------------------------------------------

/// @brief Декоратор - добавляет:
/// - 'заголовок кадра + размер всего сообщения без учета страта кадра'
/// - контрольную сумму в конец сообщения.
///
/// @note Предположим, что сообщение имеет следующий формат:
/// - старт кадра (2 байта)
/// - размер фрейма (1 байт)
/// - полезная нагрузка (5 байт)
/// - контрольная сумма (2 байта)
/// Тогда, поле размера фрейма будет иметь значение: 5+2=7. Это позволяет
/// парсеру после нахождения старта фрейма в потоке байт определить конец
/// сообщения.
class start_frame_and_crc_16
{
    using frame_size_type = std::uint8_t;
    using crc_type        = std::uint16_t;

#ifdef UNIT_TEST_ENABLE
  public:
#else
  private:
#endif

    STV_NO_PADDING_NO_OPTIMIZE_BEGIN

    struct start_frame_t {
        std::byte start_frame_first;
        std::byte start_frame_second;

        /// @brief Размер остальной части сообщения (той, которая в потоке байт
        /// расположена после поля frame_size).
        frame_size_type frame_size;
    };

    STV_NO_PADDING_NO_OPTIMIZE_END

  public:
    static constexpr std::byte first_byte{0xAA};
    static constexpr std::byte second_byte{0xAA};

    /// @brief Возвращает размер заголовка который будет добавлен перед полезной
    /// нагрузкой.
    ///
    /// @return Размер в байтах заголовка, который будет добавлен перед полезной
    /// нагрузкой.
    static constexpr size_t header_size() { return sizeof(start_frame_t); }

    /// @brief Возвращает размер хвоста, который будет добавлен после полезной
    /// нагрузки.
    ///
    /// @return Размер в байтах хвоста, который будет добавлен после полезной
    /// нагрузкой.
    static constexpr size_t trailer_size() { return sizeof(crc_type); }

    /// @brief Заполняет заголовок.
    ///
    /// @param[out] dst Указатель на заголовок, который необходимо
    /// заполнить.
    /// @param[in] total Границы всего сообщения.
    /// @param[in] pload Границы полезной нагрузки.
    ///
    static void setup_header(
        std::byte *const dst, const total_message_span &total,
        const pload_span &pload)
    {
        (void)pload;
        auto *start_frame              = reinterpret_cast<start_frame_t *>(dst);
        start_frame->start_frame_first = first_byte;
        start_frame->start_frame_second = second_byte;

        // Неизвестно, есть ли другие декораторы, поэтому вычислим размер кадра
        // из поля total.
        start_frame->frame_size =
            static_cast<decltype(start_frame->frame_size)>(total.size_bytes()
                                                           - header_size());
    }

    /// @brief Заполняет хвост.
    ///
    /// @param[out] dst Указатель на хвост сообщения. С этой области памяти
    /// начинается контрольная сумма.
    /// @param[in] total Память, выделенная под все сообщение.
    static void setup_trailer(
        std::byte *dst, const total_message_span &total)
    {
        auto *crc = reinterpret_cast<crc_type *>(&dst[0]);
        *crc = calculate_crc(total.data(), total.size_bytes() - trailer_size());
    }

  private:
    static uint16_t calculate_crc(
        const std::byte *data, size_t length)
    {
        // NOLINTBEGIN(hicpp-signed-bitwise)
        // Простая реализация Crc для примера
        uint16_t crc = 0xFFFF;
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

/// @brief Декоратор - добавляет данные для маршрутизации пакета данных на
/// принимающей стороне. Также явно содержит размер полезной нагрузки.
struct head_route {
    /// ########################################################################
    /// Арбитраж сообщения.
    /// ########################################################################
    STV_NO_PADDING_NO_OPTIMIZE_BEGIN

    struct head_route_setup_t {
        /// @brief Идентификатор получателя сообщения.
        std::uint8_t dst_id{std::numeric_limits<decltype(dst_id)>::min()};

        /// @brief Порядковый номер, по которому получатель может
        /// идентифицировать полученное сообщение.
        std::uint8_t pack_id{std::numeric_limits<decltype(pack_id)>::min()};
    };

    STV_NO_PADDING_NO_OPTIMIZE_END

    /// ------------------------------------------------------------------------

    explicit head_route(
        const head_route_setup_t &setup = head_route_setup_t{.dst_id  = 0,
                                                             .pack_id = 0}):
        setup_{setup}
    {
    }

    static constexpr size_t header_size()
    {
        return sizeof(head_route_setup_with_pload_t);
    }

    static constexpr size_t trailer_size() { return 0; }

    /// @brief Заполняет заголовок сообщения.
    void setup_header(
        std::byte *const dst, const total_message_span &total,
        const pload_span &pload) const
    {
        (void)total;
        auto *header = reinterpret_cast<head_route_setup_with_pload_t *>(dst);
        *header = static_cast<std::remove_pointer_t<decltype(header)>>(setup_);
        header->pload_size =
            static_cast<decltype(header->pload_size)>(pload.size_bytes());

        // Нужно убедиться, что запись размера полезной нагрузки помещается в
        // переменную pload_size без сужающих преобразований.
        assert(pload.size_bytes()
               <= std::numeric_limits<decltype(header->pload_size)>::max());
    }

    static void setup_trailer(
        std::byte *dst, const total_message_span &total, size_t total_size)
    {
        (void)dst;
        (void)total;
        (void)total_size;
    }

#ifdef UNIT_TEST_ENABLE
  public:
#else
  private:
#endif
    STV_NO_PADDING_NO_OPTIMIZE_BEGIN

    struct head_route_setup_with_pload_t: public head_route_setup_t {
        uint8_t pload_size{std::numeric_limits<decltype(pload_size)>::min()};
    };

    STV_NO_PADDING_NO_OPTIMIZE_END

  private:
    head_route_setup_t setup_;
};

/// ----------------------------------------------------------------------------

} // namespace stv

#endif /* SERIAL_DECORATORS_HPP */
