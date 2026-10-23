/// @file crc16.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
///
/// NAME
///     stv::crc16_modbus, stv::crc16_x25, stv::crc16_x25_accumulate,
///     stv::crc16_ccitt -- вычисление CRC-16/MODBUS, CRC-16/X.25 и
///     CRC-16/CCITT.
///
/// DESCRIPTION
///     Предоставляет функцию для вычисления CRC-16 с начальным
///     значением 0xFFFF и полиномом 0xA001 (отражённый вид 0x8005),
///     совместимую с Modbus, а также функции для вычисления
///     CRC-16/X.25 (MCRF4XX, как X25Accumulate в MAVLink): полином
///     0x1021 (отражённый вид 0x8408), начальное значение 0xFFFF,
///     отражение входа и выхода, без финального XOR, и CRC-16/CCITT
///     (CRC-16/CCITT-FALSE): полином 0x1021, начальное значение
///     0xFFFF, без отражения входа и выхода, без финального XOR.
///     Используется декораторами серийных сообщений.
///
/// SEE ALSO
///     serial_decorators.hpp.

#ifndef CRC16_HPP
#define CRC16_HPP

#include <cstddef>
#include <cstdint>

namespace stv {

/// @brief Вычисляет CRC-16/MODBUS по заданному блоку данных.
///
/// @details
/// Используется алгоритм CRC-16 с начальным значением 0xFFFF и
/// полиномом 0xA001 (отраженный вид 0x8005), совместимый с Modbus.
///
/// @param[in] data Указатель на начало данных.
/// @param[in] length Размер данных в байтах.
/// @return Вычисленное значение CRC-16.
[[nodiscard]] inline std::uint16_t crc16_modbus(
    const std::byte *data, std::size_t length)
{
    // NOLINTBEGIN(hicpp-signed-bitwise, bugprone-signed-bitwise)
    std::uint16_t crc = 0xFFFFU;

    for(std::size_t i = 0; i < length; ++i)
    {
        crc ^= static_cast<std::uint8_t>(data[i]);

        for(int j = 0; j < 8; ++j)
        {
            if((crc & 0x0001U) != 0U)
            {
                crc = static_cast<std::uint16_t>((crc >> 1) ^ 0xA001U);
            }
            else
            {
                crc = static_cast<std::uint16_t>(crc >> 1);
            }
        }
    }

    // NOLINTEND(hicpp-signed-bitwise, bugprone-signed-bitwise)
    return crc;
}

/// @brief Досчитывает один байт в значение CRC-16/X.25.
///
/// @details
/// Используется алгоритм CRC-16/MCRF4XX: полином 0x1021 (отражённый вид
/// 0x8408), отражение входа и выхода, без финального XOR. Соответствует
/// одному шагу X25Accumulate из MAVLink; начальное значение 0xFFFF задаёт
/// вызывающая сторона.
///
/// @param[in] crc Текущее значение CRC-16.
/// @param[in] value Досчитываемый байт.
/// @return Обновлённое значение CRC-16.
[[nodiscard]] inline std::uint16_t crc16_x25_accumulate(
    std::uint16_t crc, std::byte value)
{
    // NOLINTBEGIN(hicpp-signed-bitwise, bugprone-signed-bitwise)
    crc ^= static_cast<std::uint8_t>(value);

    for(int i = 0; i < 8; ++i)
    {
        if((crc & 0x0001U) != 0U)
        {
            crc = static_cast<std::uint16_t>((crc >> 1) ^ 0x8408U);
        }
        else
        {
            crc = static_cast<std::uint16_t>(crc >> 1);
        }
    }

    // NOLINTEND(hicpp-signed-bitwise, bugprone-signed-bitwise)
    return crc;
}

/// @brief Вычисляет CRC-16/X.25 (MCRF4XX) по заданному блоку данных.
///
/// @details
/// Используется алгоритм CRC-16/MCRF4XX: полином 0x1021 (отражённый вид
/// 0x8408), начальное значение 0xFFFF, отражение входа и выхода, без
/// финального XOR. Эквивалентно последовательному вызову
/// crc16_x25_accumulate для каждого байта, начиная с 0xFFFF.
///
/// @param[in] data Указатель на начало данных.
/// @param[in] size Размер данных в байтах.
/// @return Вычисленное значение CRC-16.
[[nodiscard]] inline std::uint16_t crc16_x25(
    const std::byte *data, std::size_t size)
{
    std::uint16_t crc = 0xFFFFU;

    for(std::size_t i = 0; i < size; ++i)
    {
        crc = crc16_x25_accumulate(crc, data[i]);
    }

    return crc;
}

/// @brief Вычисляет CRC-16/CCITT (CRC-16/CCITT-FALSE) по блоку данных.
///
/// @details
/// Используется алгоритм CRC-16/CCITT-FALSE: полином 0x1021, начальное
/// значение 0xFFFF, без отражения входа и выхода, без финального XOR.
/// Контрольное значение для строки "123456789" — 0x29B1. Эквивалентно
/// функции CORE_GetCrc16_CCITT_Poly0x1021 из third_party/radio_message_parser.
///
/// @param[in] data Указатель на начало данных.
/// @param[in] size Размер данных в байтах.
/// @return Вычисленное значение CRC-16.
[[nodiscard]] constexpr std::uint16_t crc16_ccitt(
    const std::byte *data, std::size_t size)
{
    // NOLINTBEGIN(hicpp-signed-bitwise, bugprone-signed-bitwise)
    std::uint16_t crc = 0xFFFFU;

    for(std::size_t i = 0; i < size; ++i)
    {
        crc = static_cast<decltype(crc)>(
            crc ^ (static_cast<decltype(crc)>(data[i]) << 8U));

        for(int j = 0; j < 8; ++j)
        {
            const bool is_msb_set = (crc & 0x8000U) != 0U;
            crc                   = static_cast<decltype(crc)>(crc << 1U);
            if(is_msb_set)
            {
                crc = static_cast<decltype(crc)>(crc ^ 0x1021U);
            }
        }
    }

    // NOLINTEND(hicpp-signed-bitwise, bugprone-signed-bitwise)
    return crc;
}

} // namespace stv

#endif /* CRC16_HPP */
