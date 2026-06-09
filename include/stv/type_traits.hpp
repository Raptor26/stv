/// @file type_traits.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.

#ifndef TYPE_TRAITS_HPP
#define TYPE_TRAITS_HPP

#include <type_traits>
#include <variant>

namespace stv {

// #############################################################################
// Mutex
// #############################################################################

/// @brief Тег используется в том случае, если объект использует указатель на
/// мьютекс.
struct mutex_ext_tag {
};

/// @brief Тег используется в том случае, если объект использует собственный
/// экземпляр мьютекса.
///
/// @note Обычно, именно этот тег вы захотите использовать в большинстве
/// случаев.
struct mutex_int_tag {
};

/// @brief Вычисляет тип мьютекса на основе тега. Если тег "mutex_int_tag", то
/// создается пустой тип, в противном случае выводится указатель на тип TMutex.
///
/// @note Предназначен для использования в структуре инициализации.
template<typename TMutex, typename TMutexTag>
using mutex_type_setup_v =
    std::conditional_t<std::is_same_v<TMutexTag, stv::mutex_ext_tag>, TMutex *,
                       std::monostate>;

/// @brief Вычисляет тип мьютекса на основе тега. Если тег "mutex_int_tag", то
/// выводится тип TMutex, в противном случае выводится указатель на тип TMutex.
///
/// @note Предназначен для использования в классе.
template<typename TMutex, typename TMutexTag>
using mutex_type_v =
    std::conditional_t<std::is_same_v<TMutexTag, stv::mutex_ext_tag>, TMutex *,
                       TMutex>;
// -----------------------------------------------------------------------------

} // namespace stv

#endif /* TYPE_TRAITS_HPP */
