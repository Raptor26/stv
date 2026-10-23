/// @file crc32.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
///
/// NAME
///     stv::crc32 -- вычисление CRC-32 (zlib).
///
/// DESCRIPTION
///     Предоставляет функцию для вычисления 32-битной CRC: полином
///     0x04C11DB7 (отражённый вид 0xEDB88320), начальное значение
///     0xFFFFFFFF, отражение входа и выхода, финальный XOR 0xFFFFFFFF.
///     Является обёрткой над etl::crc32_t16 (ETL). Таблица ETL является
///     константой времени компиляции (static constexpr) и размещается
///     компилятором в .rodata, т.е. в памяти программ (FLASH), а не в ОЗУ.
///
/// SEE ALSO
///     crc16.hpp, stvlinkv2_parser.hpp.

#ifndef CRC32_HPP
#define CRC32_HPP

#include "etl/crc32.h"

#include <cstddef>
#include <cstdint>

namespace stv {

/// @brief Вычисляет CRC-32 (zlib) по заданному блоку данных.
///
/// @details
/// Обёртка над etl::crc32_t16 (ETL): полином 0x04C11DB7 (отражённый вид
/// 0xEDB88320), начальное значение 0xFFFFFFFF, отражение входа и выхода,
/// финальный XOR 0xFFFFFFFF. Таблица CRC (64 байта) — константа времени
/// компиляции и размещается в памяти программ (.rodata → FLASH).
///
/// @param[in] data Указатель на начало данных.
/// @param[in] length Размер данных в байтах.
/// @return Вычисленное значение CRC-32.
[[nodiscard]] inline std::uint32_t crc32(
    const std::byte *data, std::size_t length)
{
    return etl::crc32_t16{reinterpret_cast<const std::uint8_t *>(data),
                          reinterpret_cast<const std::uint8_t *>(data)
                              + length};
}

} // namespace stv

#endif /* CRC32_HPP */
