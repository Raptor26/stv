/// @file mmc3630kj_types.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.

#ifndef MMC3630KJ_TYPES_HPP
#define MMC3630KJ_TYPES_HPP

#include <cstdint>

namespace stv {

/// @brief Тип данных для представления регистра MMC3630KJ.
///
/// @details Используется для всех регистровых операций датчика MMC3630KJ.
/// Ширина регистра — 8 бит. Все регистры датчика (STATUS, CTRL0–CTRL2,
/// PRODUCT_ID и др.) адресуются и читаются/пишутся через этот тип.
using mmc3630kj_reg_type = std::uint8_t;

} // namespace stv

#endif /* MMC3630KJ_TYPES_HPP */
