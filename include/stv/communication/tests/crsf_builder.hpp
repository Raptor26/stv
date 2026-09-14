/// @file crsf_builder.hpp
/// @brief Тестовый helper построения кадров протокола crsf. Не зависит от
/// Catch2 и предназначен только для unit-тестов.
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.

#ifndef CRSF_BUILDER_HPP
#define CRSF_BUILDER_HPP

#include "stv/communication/crsf.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

/// @brief Тип входных значений каналов управления helper'а построения кадров
/// crsf.
using crsf_test_channels =
    std::array<std::uint16_t, stv::crsf_rc_channels_count>;

namespace crsf_builder_detail {

/// @brief Значение байта sync, фиксируемое helper'ом.
/// @see Раздел «Frame Details» в crsf.md
inline constexpr std::byte sync_byte{0xC8U};

/// @brief Начинает сборку кадра crsf: проверяет размер буфера и записывает
/// заголовок (sync, frame length, type).
/// @param dst Буфер назначения.
/// @param type Тип кадра crsf.
/// @param payload_size Размер payload кадра в байтах.
/// @return Диапазон байт payload внутри dst либо пустой диапазон, если dst
/// меньше полного кадра.
[[nodiscard]] inline auto begin_frame(
    // Порядок параметров (type, payload_size) зафиксирован существующими
    // вызовами во всех тестах модуля.
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    std::span<std::byte> dst, std::uint8_t type, std::size_t payload_size)
    -> std::span<std::byte>
{
    // sync + frame length + type + payload + crc.
    const std::size_t frame_size{payload_size + 4U};
    if(dst.size() < frame_size)
    {
        return {};
    }

    dst[0U] = sync_byte;
    // frame length = type + payload + crc.
    dst[1U] = static_cast<std::byte>(payload_size + 2U);
    dst[2U] = std::byte{type};
    return dst.subspan(3U, payload_size);
}

/// @brief Завершает сборку кадра crsf: подсчитывает и записывает crc.
/// @param dst Буфер с уже записанными заголовком и payload.
/// @param payload_size Размер payload кадра в байтах.
/// @return Полный размер кадра в байтах.
[[nodiscard]] inline auto end_frame(
    std::span<std::byte> dst, std::size_t payload_size) -> std::size_t
{
    const std::size_t crc_index{payload_size + 3U};
    dst[crc_index] =
        std::byte{stv::crsf_crc8(dst.subspan(2U, payload_size + 1U))};
    return crc_index + 1U;
}

/// @brief Упаковывает значения каналов управления по 11 бит lsb-first в
/// payload кадра rc channels packed.
/// @param payload Диапазон из stv::crsf_rc_channels_payload_size байт.
/// @param channels Значения каналов управления.
/// @see Раздел «0x16 RC Channels Packed Payload» в crsf.md
inline void pack_rc_channels(
    std::span<std::byte> payload, const crsf_test_channels &channels)
{
    std::uint32_t bit_buffer{0U};
    std::size_t   bit_count{0U};
    std::size_t   byte_index{0U};

    for(const std::uint16_t channel: channels)
    {
        bit_buffer |= static_cast<std::uint32_t>(channel & 0x07FFU)
                      << bit_count;
        bit_count  += 11U;
        while(bit_count >= 8U)
        {
            payload[byte_index] = static_cast<std::byte>(bit_buffer & 0xFFU);
            ++byte_index;
            bit_buffer >>= 8U;
            bit_count   -= 8U;
        }
    }
}

} // namespace crsf_builder_detail

/// @brief Строит полный кадр crsf rc channels packed (0x16): sync, frame
/// length, type, 22 байта lsb-first упаковки 11-битных каналов и crc.
/// @param dst Буфер назначения.
/// @param channels Значения 16 каналов управления.
/// @return Число записанных байт (26) либо 0, если dst меньше кадра.
/// @see Раздел «0x16 RC Channels Packed Payload» в crsf.md
[[nodiscard]] inline auto build_rc_channels_frame(
    std::span<std::byte> dst, const crsf_test_channels &channels) -> std::size_t
{
    const std::span<std::byte> payload{
        crsf_builder_detail::begin_frame(dst, stv::crsf_type_rc_channels_packed,
                                         stv::crsf_rc_channels_payload_size),
    };
    if(payload.empty())
    {
        return 0U;
    }

    crsf_builder_detail::pack_rc_channels(payload, channels);
    return crsf_builder_detail::end_frame(dst,
                                          stv::crsf_rc_channels_payload_size);
}

/// @brief Строит полный кадр crsf link statistics (0x14): sync, frame
/// length, type, 10 байт payload и crc.
/// @param dst Буфер назначения.
/// @param link_statistics Поля payload кадра link statistics.
/// @return Число записанных байт (14) либо 0, если dst меньше кадра.
/// @see Раздел «0x14 Link Statistics» в crsf.md
[[nodiscard]] inline auto build_link_statistics_frame(
    std::span<std::byte> dst, const stv::crsf_link_statistics &link_statistics)
    -> std::size_t
{
    const std::span<std::byte> payload{
        crsf_builder_detail::begin_frame(
            dst, stv::crsf_type_link_statistics,
            stv::crsf_link_statistics_payload_size),
    };
    if(payload.empty())
    {
        return 0U;
    }

    payload[0U] = std::byte{link_statistics.up_rssi_ant1};
    payload[1U] = std::byte{link_statistics.up_rssi_ant2};
    payload[2U] = std::byte{link_statistics.up_link_quality};
    payload[3U] = static_cast<std::byte>(
        static_cast<std::uint8_t>(link_statistics.up_snr));
    payload[4U] = std::byte{link_statistics.active_antenna};
    payload[5U] = std::byte{link_statistics.rf_profile};
    payload[6U] = std::byte{link_statistics.up_rf_power};
    payload[7U] = std::byte{link_statistics.down_rssi};
    payload[8U] = std::byte{link_statistics.down_link_quality};
    payload[9U] = static_cast<std::byte>(
        static_cast<std::uint8_t>(link_statistics.down_snr));
    return crsf_builder_detail::end_frame(
        dst, stv::crsf_link_statistics_payload_size);
}

#endif /* CRSF_BUILDER_HPP */
