/// @file stvlink_sender.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
///
/// NAME
///     stv::stvlink_sender -- передающий декоратор протокола stvlink.
///
/// DESCRIPTION
///     stvlink_sender предоставляет передающую сторону протокола
///     stvlink: единый декоратор stvlink_sender формирует кадр
///     целиком -- канальный уровень (стартовая последовательность
///     0xAA 0xAA, поле frame_size), заголовок сообщения
///     (идентификатор получателя dst_id, идентификатор сообщения
///     msg_id и размер полезной нагрузки pload_size) и CRC-16/MODBUS
///     в хвосте. Формат кадра переиспользуется из stvlink_parser.hpp.
///
///     Байтовый формат кадра на проводе не меняется: поле dst_id
///     отправитель заполняет сам, приёмная сторона его не
///     фильтрует -- поле сохранено в формате кадра для бинарной
///     совместимости протокола.
///
///     stvlink_sender
///         Единый декоратор кадра. В начало сообщения записывает
///         стартовый кадр 0xAA 0xAA, 16-битное поле frame_size,
///         равное размеру оставшейся части кадра, и заголовок
///         сообщения message_header_t (dst_id, msg_id, pload_size).
///         Параметры dst_id и msg_id задаются через структуру
///         setup_t при создании декоратора или при вызове
///         request() буфера серийных сообщений. В конец сообщения
///         записывается CRC-16 (начальное значение 0xFFFF, полином
///         0xA001), вычисленный по всем байтам сообщения, кроме
///         самого CRC. Метод is_crc_valid() проверяет целостность
///         принятого кадра. При превышении размера полезной
///         нагрузки максимального значения uint16_t срабатывает
///         assert.
///
/// EXAMPLE
///     Пример отправки сообщения со стартовым кадром, заголовком
///     сообщения и CRC:
///     ```cpp
///     #include <stv/communication/serial_sender.hpp>
///     #include <stv/communication/stvlink_sender.hpp>
///     #include <stv/containers/simbuff.hpp>
///     #include <etl/queue.h>
///
///     int main() {
///         using namespace stv;
///
///         using sim_buff_type = stv::sim_buff<stv::empty_mutex>;
///         using queue_type    = etl::queue<sim_buff_type, 10>;
///         queue_type queue{};
///
///         auto serial_message_buffer =
///             make_serial_message_buffer(queue, stvlink_sender{});
///
///         {
///             auto msg = serial_message_buffer.request(
///                 "Hello world",
///                 stvlink_sender::setup_t{.dst_id = 1, .msg_id = 7});
///         }
///
///         // Заголовок и CRC заполняются в деструкторе msg,
///         // после чего готовый буфер помещается в queue.
///
///         return 0;
///     }
///     ```
///
/// SEE ALSO
///     stvlink_parser.hpp, serial_decorators.hpp, serial_sender.hpp.

#ifndef STVLINK_SENDER_HPP
#define STVLINK_SENDER_HPP

#include "stv/communication/crc16.hpp"
#include "stv/communication/serial_decorators.hpp"
#include "stv/communication/stvlink_parser.hpp"
#include "stv/utils.hpp"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>

namespace stv {

/// @brief Передающий декоратор, формирующий кадр stvlink целиком.
///
/// @details
/// Единый декоратор кадра протокола stvlink, объединяющий канальный уровень
/// и заголовок сообщения:
///   - в начало записывается стартовый кадр @c 0xAA 0xAA и поле
///     @c frame_size, равное размеру оставшейся части кадра
///     (заголовок сообщения + полезная нагрузка + все хвосты);
///   - следом записывается заголовок сообщения @ref message_header_t:
///     идентификатор получателя @c dst_id, идентификатор сообщения
///     @c msg_id и 16-битный размер полезной нагрузки @c pload_size;
///   - в конец записывается 16-битная контрольная сумма (CRC-16),
///     вычисленная по всем байтам сообщения, кроме самого CRC.
///
/// Такая структура позволяет @c serial_parser находить начало кадра в
/// потоке байт, определять длину сообщения и проверять его целостность.
/// Получатель после парсинга получает сообщение вместе с заголовком
/// сообщения и читает из него идентификатор @c msg_id, а поле @c dst_id
/// не фильтрует -- оно сохранено в формате кадра для бинарной
/// совместимости протокола.
///
/// @note
/// Поле @c frame_size не включает в себя размер стартового кадра. Если
/// сообщение состоит из стартового кадра (4 байта: 0xAA, 0xAA и 16-битное
/// поле размера), заголовка сообщения (4 байта), полезной нагрузки (5
/// байт) и CRC (2 байта), то @c frame_size будет равно 11, а общий размер
/// сообщения -- 15 байт.
class stvlink_sender
{
  public:
    STV_NO_PADDING_NO_OPTIMIZE_BEGIN

    /// @brief Параметры заголовка сообщения, задаваемые при создании
    ///     декоратора.
    struct setup_t {
        /// @brief Идентификатор получателя сообщения.
        ///
        /// @details
        /// Заполняется отправителем; приёмная сторона его не фильтрует.
        /// Поле сохранено в формате кадра для бинарной совместимости
        /// протокола.
        std::uint8_t dst_id{0};

        /// @brief Идентификатор сообщения.
        ///
        /// @details
        /// Получатель выбирает обработчик сообщения по этому идентификатору.
        std::uint8_t msg_id{0};
    };

    /// @brief Заголовок сообщения с полем размера полезной нагрузки.
    ///
    /// @details
    /// Именно эта структура записывается в кадр сразу после стартового
    /// кадра. Раскладка полей в памяти и размер (4 байта) соответствуют
    /// бинарному формату заголовка сообщения stvlink.
    struct message_header_t {
        /// @brief Идентификатор получателя сообщения.
        ///
        /// @details
        /// Заполняется отправителем; приёмная сторона его не фильтрует.
        std::uint8_t dst_id{std::numeric_limits<decltype(dst_id)>::min()};

        /// @brief Идентификатор сообщения.
        ///
        /// @details
        /// Получатель выбирает обработчик сообщения по этому идентификатору.
        std::uint8_t msg_id{std::numeric_limits<decltype(msg_id)>::min()};

        /// @brief Размер полезной нагрузки в байтах.
        std::uint16_t pload_size{
            std::numeric_limits<decltype(pload_size)>::min(),
        };
    };

    STV_NO_PADDING_NO_OPTIMIZE_END

    static_assert(sizeof(message_header_t) == 4U,
                  "message_header_t должен занимать 4 байта согласно "
                  "формату заголовка сообщения stvlink");

    /// @brief Конструирует декоратор с заданными параметрами заголовка.
    ///
    /// @param[in] setup Параметры заголовка сообщения. По умолчанию оба
    ///     поля равны нулю.
    explicit stvlink_sender(
        const setup_t &setup = setup_t{.dst_id = 0, .msg_id = 0}):
        setup_{setup}
    {
    }

    /// @brief Возвращает размер заголовка.
    ///
    /// @return Размер заголовка в байтах (старотовый кадр + заголовок
    ///     сообщения = 8).
    static constexpr size_t header_size()
    { return stvlink_parser::header_size() + sizeof(message_header_t); }

    /// @brief Возвращает размер заголовка сообщения.
    ///
    /// @return Размер заголовка сообщения в байтах (4).
    static constexpr size_t message_header_size()
    { return sizeof(message_header_t); }

    /// @brief Возвращает размер хвоста.
    ///
    /// @return Размер хвоста в байтах (CRC = 2).
    static constexpr size_t trailer_size()
    { return stvlink_parser::trailer_size(); }

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
            reinterpret_cast<stvlink_parser::start_frame_t *>(dst);
        start_frame->start_frame_first  = stvlink_parser::first_byte;
        start_frame->start_frame_second = stvlink_parser::second_byte;

        // Неизвестно, есть ли другие декораторы, поэтому вычислим размер
        // кадра из поля total. Поле frame_size не включает стартовый кадр.
        start_frame->frame_size =
            static_cast<decltype(start_frame->frame_size)>(
                total.size_bytes() - stvlink_parser::header_size());

        auto *header = reinterpret_cast<message_header_t *>(
            dst + stvlink_parser::header_size());
        header->dst_id = setup_.dst_id;
        header->msg_id = setup_.msg_id;
        header->pload_size =
            static_cast<decltype(header->pload_size)>(pload.size_bytes());

        // Нужно убедиться, что запись размера полезной нагрузки помещается
        // в переменную pload_size без сужающих преобразований.
        assert(pload.size_bytes()
               <= std::numeric_limits<decltype(header->pload_size)>::max());
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
    { return stvlink_parser::is_crc_valid(total); }

  private:
    /// @brief Сохраненные параметры заголовка сообщения.
    setup_t setup_;
};

/// ----------------------------------------------------------------------------

} // namespace stv

#endif /* STVLINK_SENDER_HPP */
