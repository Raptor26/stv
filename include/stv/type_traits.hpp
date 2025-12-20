/// @file type_traits.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// @copyright (c) 2025 "The Boys"
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

#ifndef TYPE_TRAITS_HPP
#define TYPE_TRAITS_HPP

#include <type_traits>

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
