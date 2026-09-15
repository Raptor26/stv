/// @file icm42688p_fifo.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.

#ifndef ICM42688P_FIFO_HPP
#define ICM42688P_FIFO_HPP

#include "icm42688p_types.hpp"
#include <cstddef>
#include <cstdint>
#include <span>

namespace stv {

/// @brief Размер 20-байтного FIFO-пакета в режиме расширенного разрешения
/// (20-битные данные акселерометра и гироскопа).
inline constexpr std::size_t icm42688p_fifo_packet_size{20U};

/// @brief Структура 20-байтного FIFO-пакета ICM-42688-P в 20-битном режиме.
///
/// @details Содержит 20-битные знаковые значения осей акселерометра и
///          гироскопа (в LSB при фиксированных диапазонах ±16 g и
///          ±2000 dps), 16-битное значение температуры и 16-битную метку
///          времени, см. раздел 6.1 (Packet 4) даташита DS-000347 rev 1.6.
struct icm42688p_fifo_packet {
    /// @brief 20-битное значение оси X акселерометра, LSB.
    std::int32_t accel_x{0};

    /// @brief 20-битное значение оси Y акселерометра, LSB.
    std::int32_t accel_y{0};

    /// @brief 20-битное значение оси Z акселерометра, LSB.
    std::int32_t accel_z{0};

    /// @brief 20-битное значение оси X гироскопа, LSB.
    std::int32_t gyro_x{0};

    /// @brief 20-битное значение оси Y гироскопа, LSB.
    std::int32_t gyro_y{0};

    /// @brief 20-битное значение оси Z гироскопа, LSB.
    std::int32_t gyro_z{0};

    /// @brief 16-битное значение температуры.
    std::int16_t temp{0};

    /// @brief 16-битная метка времени пакета.
    std::uint16_t timestamp{0};

    /// @brief Флаг валидности: бит 4 заголовка (HEADER_20) установлен, пакет
    ///        содержит новую валидную 20-битную выборку.
    bool valid{false};

    /// @brief Оператор проверки валидности пакета.
    explicit operator bool() const { return valid; }
};

/// @brief Чувствительность акселерометра в 20-битном режиме, LSB/g.
///
/// @details В 20-битном режиме FIFO диапазон измерений фиксирован на ±16 g
///          независимо от настроек ACCEL_UI_FS_SEL, см. раздел 6.1 даташита
///          DS-000347 rev 1.6.
inline constexpr float icm42688p_fifo_accel_lsb_per_g{8192.0F};

/// @brief Чувствительность гироскопа в 20-битном режиме, LSB/dps.
///
/// @details В 20-битном режиме FIFO диапазон измерений фиксирован на
///          ±2000 dps независимо от настроек GYRO_UI_FS_SEL.
inline constexpr float icm42688p_fifo_gyro_lsb_per_dps{131.0F};

namespace detail {

inline constexpr std::uint8_t nibble_mask{0x0F};

/// @brief Собирает 20-битное знаковое значение оси из старших 16 бит
/// (два байта big-endian) и младшего ниббла.
///
/// @param[in] data_hi Байт [19:12].
/// @param[in] data_lo Байт [11:4].
/// @param[in] nibble Байт, младшие 4 бита которого содержат [3:0].
/// @return Знаковое 20-битное значение оси.
[[nodiscard]] constexpr auto assemble_axis_20bit(
    std::uint8_t data_hi, std::uint8_t data_lo, std::uint8_t nibble)
    -> std::int32_t
{
    constexpr unsigned      hi_shift{12U};
    constexpr unsigned      lo_shift{4U};
    constexpr std::uint32_t sign_bit{1U << 19U};
    constexpr auto          sign_offset =
        static_cast<std::int32_t>(std::uint32_t{1U} << 20U);

    const auto value = (static_cast<std::uint32_t>(data_hi) << hi_shift)
                       | (static_cast<std::uint32_t>(data_lo) << lo_shift)
                       | (static_cast<std::uint32_t>(nibble) & nibble_mask);

    auto       signed_value = static_cast<std::int32_t>(value);
    if((value & sign_bit) != 0U)
    {
        signed_value -= sign_offset;
    }

    return signed_value;
}

} // namespace detail

/// @brief Распаковывает 20-байтный FIFO-пакет в 20-битном режиме.
///
/// @details Формат пакета (раздел 6.1, Packet 4, даташит DS-000347 rev 1.6):
///          байт 0 — заголовок (бит 4 — признак валидных 20-битных данных,
///          бит 7 — признак пустого FIFO); байты 0x01–0x0C — старшие 16 бит
///          осей акселерометра и гироскопа (по два байта big-endian на ось);
///          байты 0x0D–0x0E — температура; байты 0x0F–0x10 — метка времени;
///          байты 0x11–0x13 — младшие нибблы шести осей (AccelX[3:0] +
///          AccelY[3:0], AccelZ[3:0] + GyroX[3:0], GyroY[3:0] + GyroZ[3:0]).
///
/// @param[in] data Буфер из 20 байт FIFO-пакета.
/// @return Структура icm42688p_fifo_packet с распакованными значениями.
[[nodiscard]] inline auto icm42688p_parse_fifo_packet(
    std::span<const std::uint8_t, icm42688p_fifo_packet_size> data)
    -> icm42688p_fifo_packet
{
    constexpr std::size_t header_idx{0U};
    constexpr std::size_t temp_hi_idx{0x0DU};
    constexpr std::size_t timestamp_hi_idx{0x0FU};
    constexpr std::size_t nibble_0_idx{0x11U};
    constexpr std::size_t nibble_1_idx{0x12U};
    constexpr std::size_t nibble_2_idx{0x13U};
    constexpr unsigned    header_msg_bit{7U};
    constexpr unsigned    header_20bit_bit{4U};

    icm42688p_fifo_packet packet{};

    const auto            header = data[header_idx];

    constexpr auto        header_20bit_mask =
        static_cast<std::uint8_t>(std::uint32_t{1U} << header_20bit_bit);
    constexpr auto header_msg_mask =
        static_cast<std::uint8_t>(std::uint32_t{1U} << header_msg_bit);

    packet.valid = ((header & header_20bit_mask) != 0U)
                   && ((header & header_msg_mask) == 0U);

    constexpr std::size_t accel_x_hi_idx{0x01U};
    constexpr std::size_t accel_y_hi_idx{0x03U};
    constexpr std::size_t accel_z_hi_idx{0x05U};
    constexpr std::size_t gyro_x_hi_idx{0x07U};
    constexpr std::size_t gyro_y_hi_idx{0x09U};
    constexpr std::size_t gyro_z_hi_idx{0x0BU};

    constexpr unsigned    nibble_shift{4U};

    packet.accel_x = detail::assemble_axis_20bit(
        data[accel_x_hi_idx], data[accel_x_hi_idx + 1U],
        static_cast<std::uint8_t>(static_cast<std::uint32_t>(data[nibble_0_idx])
                                  >> nibble_shift));
    packet.accel_y = detail::assemble_axis_20bit(
        data[accel_y_hi_idx], data[accel_y_hi_idx + 1U], data[nibble_0_idx]);
    packet.accel_z = detail::assemble_axis_20bit(
        data[accel_z_hi_idx], data[accel_z_hi_idx + 1U],
        static_cast<std::uint8_t>(static_cast<std::uint32_t>(data[nibble_1_idx])
                                  >> nibble_shift));
    packet.gyro_x = detail::assemble_axis_20bit(
        data[gyro_x_hi_idx], data[gyro_x_hi_idx + 1U], data[nibble_1_idx]);
    packet.gyro_y = detail::assemble_axis_20bit(
        data[gyro_y_hi_idx], data[gyro_y_hi_idx + 1U],
        static_cast<std::uint8_t>(static_cast<std::uint32_t>(data[nibble_2_idx])
                                  >> nibble_shift));
    packet.gyro_z = detail::assemble_axis_20bit(
        data[gyro_z_hi_idx], data[gyro_z_hi_idx + 1U], data[nibble_2_idx]);

    packet.temp = static_cast<std::int16_t>(static_cast<std::uint16_t>(
        (static_cast<std::uint32_t>(data[temp_hi_idx]) << 8U)
        | static_cast<std::uint32_t>(data[temp_hi_idx + 1U])));

    packet.timestamp = static_cast<std::uint16_t>(
        (static_cast<std::uint32_t>(data[timestamp_hi_idx]) << 8U)
        | static_cast<std::uint32_t>(data[timestamp_hi_idx + 1U]));

    return packet;
}

} // namespace stv

#endif /* ICM42688P_FIFO_HPP */
