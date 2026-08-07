/// @file mavlink_v2_sender.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
///
/// NAME
///     stv::mavlink_v2_sender -- передающий декоратор кадра протокола
///     MAVLink v2.
///
/// DESCRIPTION
///     mavlink_v2_sender предоставляет передающую сторону протокола
///     MAVLink v2: декоратор mavlink_v2_frame_tx формирует кадр
///     (стартовый байт 0xFD, поле длины len, флаги, заголовок
///     маршрутизации seq/sysid/compid/msgid, полезная нагрузка,
///     CRC-16/X.25), а mavlink_v2_seq_counter ведёт сквозной атомарный
///     счётчик последовательности seq. Формат кадра переиспользуется из
///     mavlink_v2_frame.hpp.
///
/// SEE ALSO
///     mavlink_v2_frame.hpp, serial_decorators.hpp, serial_sender.hpp.

#ifndef MAVLINK_V2_SENDER_HPP
#define MAVLINK_V2_SENDER_HPP

#include "stv/communication/crc16.hpp"
#include "stv/communication/mavlink_v2_frame.hpp"
#include "stv/communication/serial_decorators.hpp"
#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>

namespace stv {

/// @brief Сквозной счётчик последовательности MAVLink v2 (поле seq).
///
/// @details
/// Хранит атомарный счётчик, общий для всех кадров одного отправителя.
/// Начальное значение — 0. Каждый сформированный кадр получает
/// уникальное значение seq, даже если формирование происходит из
/// нескольких потоков. Счётчик заворачивается через 255, как того
/// требует однобайтовое поле seq MAVLink.
///
/// @note
/// Гарантируются только уникальность и полнота диапазона seq (без
/// пропусков на стороне счётчика), но НЕ порядок seq в очереди при
/// нескольких потоках-производителях: seq присваивается до помещения
/// кадра в очередь, поэтому кадр с бо́льшим seq может попасть в очередь
/// раньше кадра с меньшим. Для строго монотонного порядка seq на
/// проводе используйте одного производителя на буфер сообщений.
///
/// @warning
/// Счётчик передаётся в декоратор как невладеющий указатель и
/// разыменовывается в деструкторе каждого сообщения. Счётчик (как и
/// очередь) обязан жить дольше всех объектов serial_message, созданных
/// буфером, — не только дольше самого буфера.
class mavlink_v2_seq_counter
{
  public:
    /// @brief Возвращает текущее значение и инкрементирует счётчик.
    ///
    /// @details
    /// После значения 255 счётчик заворачивается в 0.
    ///
    /// @return Значение счётчика до инкремента.
    std::uint8_t next() { return counter_.fetch_add(1U); }

  private:
    /// @brief Атомарное значение счётчика.
    std::atomic<std::uint8_t> counter_{0};
};

/// ----------------------------------------------------------------------------

/// @brief Передающий декоратор кадра протокола MAVLink v2.
///
/// @details
/// Формирует кадр MAVLink v2 вокруг полезной нагрузки:
///   - заголовок размером 10 байт: стартовый байт @c 0xFD, поле len
///     (1 байт, равен размеру полезной нагрузки), incompat_flags и
///     compat_flags (всегда 0), seq из сквозного счётчика, sysid,
///     compid и msgid (3 байта, little-endian);
///   - хвост размером 2 байта: CRC-16/X.25, вычисленный по байтам от
///     поля len до конца полезной нагрузки (без стартового байта
///     @c 0xFD и без самого CRC) с досчётом байта CRC_EXTRA,
///     специфичного для msgid.
///
/// Формирование подписанных кадров не поддерживается: поля флагов
/// всегда равны нулю.
///
/// Параметры, постоянные для отправителя (sysid, compid и счётчик),
/// задаются один раз при создании буфера через @ref setup_t.
/// Идентификатор сообщения конкретного кадра передаётся в request()
/// через @ref route_t: декоратор реализует apply_setup(), поэтому
/// параметр запроса обновляет только msgid, не затрагивая sysid/compid
/// и счётчик — см. документацию apply_to_single_decorator.
///
/// @note
/// Счётчик seq инкрементируется в setup_header(), то есть до помещения
/// кадра в очередь. Если очередь переполнена и кадр потерян, в
/// последовательности seq образуется дыра — это осознанное решение,
/// позволяющее приёмной стороне обнаруживать потери.
///
/// @warning
/// Декоратор ДОЛЖЕН быть единственным декоратором буфера сообщений:
///   make_serial_message_buffer(queue, mavlink_v2_frame_tx{...})
/// Поле len вычисляется от размера полезной нагрузки всего сообщения,
/// а CRC — по всем байтам после стартового байта, поэтому при
/// добавлении других декораторов в буфер поля len и CRC будут
/// сформированы неверно.
///
/// @note
/// При вызове request() из нескольких потоков кадр с бо́льшим seq может
/// попасть в очередь раньше кадра с меньшим (см. mavlink_v2_seq_counter).
///
/// @tparam TCrcExtraProvider Провайдер байта CRC_EXTRA. Должен
///     предоставлять статический метод
///     @c static auto crc_extra_for(std::uint32_t msgid) ->
///     std::optional<std::uint8_t>, возвращающий CRC_EXTRA для
///     известного msgid или @c std::nullopt для неизвестного.
template<typename TCrcExtraProvider>
class mavlink_v2_frame_tx
{
    /// @brief Максимальное значение идентификатора сообщения (3 байта).
    static constexpr std::uint32_t max_message_id{0xFF'FFFFU};

    /// @brief Смещение поля len в кадре (сразу после стартового байта).
    static constexpr std::size_t len_offset{1U};

  public:
    /// @brief Параметры отправителя, задаваемые при создании буфера.
    struct setup_t {
        /// @brief Идентификатор системы-отправителя (sysid).
        std::uint8_t sysid{};

        /// @brief Идентификатор компонента-отправителя (compid).
        std::uint8_t compid{};

        /// @brief Указатель на сквозной счётчик последовательности (seq).
        ///
        /// @warning
        /// Невладеющий указатель: счётчик обязан жить дольше всех
        /// объектов serial_message, созданных буфером с этим
        /// декоратором, — не только дольше самого буфера.
        mavlink_v2_seq_counter *counter{nullptr};
    };

    /// @brief Параметры маршрутизации, задаваемые на каждый запрос.
    struct route_t {
        /// @brief Идентификатор сообщения (msgid), не более 0xFFFFFF.
        std::uint32_t msgid;
    };

    /// @brief Конструирует декоратор по умолчанию.
    ///
    /// @details
    /// Требуется конструктором невалидного сообщения (request_null()):
    /// такое сообщение не выделяет память, поэтому setup_header() и
    /// setup_trailer() для него не вызываются.
    mavlink_v2_frame_tx() = default;

    /// @brief Конструирует декоратор с параметрами отправителя.
    ///
    /// @param[in] setup Параметры отправителя (sysid, compid и счётчик
    ///     seq).
    explicit mavlink_v2_frame_tx(
        const setup_t &setup):
        setup_{setup}
    { assert(setup.counter != nullptr); }

    /// @brief Возвращает размер заголовка кадра.
    ///
    /// @return Размер заголовка в байтах (0xFD..msgid = 10).
    static constexpr std::size_t header_size() { return 10U; }

    /// @brief Возвращает размер хвоста кадра.
    ///
    /// @return Размер хвоста в байтах (CRC = 2).
    static constexpr std::size_t trailer_size()
    { return sizeof(std::uint16_t); }

    /// @brief Обновляет идентификатор сообщения из параметра запроса.
    ///
    /// @details
    /// Обновляет только msgid, сохраняя sysid, compid и счётчик,
    /// заданные при создании буфера. Вызывается из request() буфера
    /// сообщений для каждого параметра @ref route_t.
    ///
    /// @param[in] route Параметры маршрутизации сообщения.
    void apply_setup(
        const route_t &route)
    {
        assert(route.msgid <= max_message_id);
        route_ = route;
    }

    /// @brief Заполняет заголовок кадра MAVLink v2.
    ///
    /// @details
    /// Записывает стартовый байт, поле len, нулевые флаги и заголовок
    /// маршрутизации (seq, sysid, compid, msgid). Счётчик seq
    /// инкрементируется здесь, до помещения кадра в очередь.
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
        assert(route_.msgid <= max_message_id);
        assert(pload.size_bytes() <= std::numeric_limits<std::uint8_t>::max());

        dst[0U] = mavlink_v2_frame<TCrcExtraProvider>::first_byte;
        dst[1U] = static_cast<std::byte>(pload.size_bytes());
        dst[2U] = std::byte{0x00}; // incompat_flags: подпись не
                                   // поддерживается.
        dst[3U] = std::byte{0x00}; // compat_flags

        assert(setup_.counter != nullptr);
        dst[4U] = std::byte{setup_.counter->next()};

        dst[5U] = std::byte{setup_.sysid};
        dst[6U] = std::byte{setup_.compid};

        // msgid записывается little-endian: младший байт первым.
        dst[7U] = static_cast<std::byte>(route_.msgid & 0xFFU);
        dst[8U] = static_cast<std::byte>((route_.msgid >> 8U) & 0xFFU);
        dst[9U] = static_cast<std::byte>((route_.msgid >> 16U) & 0xFFU);
    }

    /// @brief Записывает CRC-16/X.25 в хвост кадра.
    ///
    /// @details
    /// CRC вычисляется по байтам от поля len до конца полезной нагрузки
    /// (без стартового байта @c 0xFD и без самого CRC) с досчётом байта
    /// CRC_EXTRA, предоставляемого @c TCrcExtraProvider для msgid кадра.
    /// Диапазон байт CRC совпадает с mavlink_v2_frame::is_crc_valid.
    ///
    /// @param[out] dst Указатель на начало хвоста в собранном сообщении.
    /// @param[in] total Границы всего сообщения.
    void setup_trailer(
        std::byte *dst, const total_message_span &total) const
    {
        // Отправитель обязан знать CRC_EXTRA для передаваемого msgid.
        const auto crc_extra = TCrcExtraProvider::crc_extra_for(route_.msgid);
        assert(crc_extra.has_value());

        // Данные для CRC начинаются с байта len (сразу после 0xFD) и
        // заканчиваются перед CRC.
        const auto data_for_crc = total.subspan(len_offset);
        auto       computed_crc = stv::crc16_x25(
            data_for_crc.data(), data_for_crc.size_bytes() - trailer_size());
        computed_crc =
            stv::crc16_x25_accumulate(computed_crc, std::byte{*crc_extra});

        std::memcpy(dst, &computed_crc, sizeof(computed_crc));
    }

  private:
    /// @brief Параметры отправителя (sysid, compid и счётчик seq).
    setup_t setup_;

    /// @brief Параметры маршрутизации текущего сообщения (msgid).
    route_t route_{};
};

} // namespace stv

#endif /* MAVLINK_V2_SENDER_HPP */
