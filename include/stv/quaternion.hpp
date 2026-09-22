/// @file quaternion.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.

#ifndef QUATERNION_HPP
#define QUATERNION_HPP

#include "boost/qvm/quat_traits.hpp"
#include <array>
#include <cmath>
#include <cstddef>
#include <type_traits>

namespace stv {

/// @brief Кватернион с порядком компонент (w, x, y, z): скалярная часть идёт
/// первой, что совпадает с раскладкой boost::qvm (элемент 0 — S).
/// @tparam T Скалярный тип с плавающей точкой.
template<typename T = float>
struct quaternion {
    using value_type = T;

    static_assert(std::is_floating_point_v<T>,
                  "quaternion supports only floating point scalar types");

    /// @brief Компоненты (w, x, y, z). Публичный доступ нужен адаптеру
    /// boost::qvm::quat_traits.
    /// NOLINTNEXTLINE(readability-identifier-length)
    std::array<T, 4> a;

    constexpr quaternion() noexcept:
        a{}
    {
    }

    constexpr quaternion(
        T w_q, T x_q, T y_q, T z_q) noexcept:
        a{w_q, x_q, y_q, z_q}
    {
    }

    explicit constexpr quaternion(
        const std::array<T, 4> &quat) noexcept:
        a{quat}
    {
    }

    [[nodiscard]] constexpr auto give_w() const noexcept { return a[0]; }

    [[nodiscard]] constexpr auto give_x() const noexcept { return a[1]; }

    [[nodiscard]] constexpr auto give_y() const noexcept { return a[2]; }

    [[nodiscard]] constexpr auto give_z() const noexcept { return a[3]; }

    /// @brief Все компоненты конечны (не NaN и не inf).
    [[nodiscard]] explicit operator bool() const noexcept
    { return is_finite(); }

    /// @brief Все компоненты равны нулю.
    [[nodiscard]] constexpr auto is_zero() const noexcept
    { return a == std::array<T, 4>{}; }

  private:
    [[nodiscard]] auto is_finite() const noexcept
    {
        return std::isfinite(give_w()) && std::isfinite(give_x())
               && std::isfinite(give_y()) && std::isfinite(give_z());
    }
};

} // namespace stv

namespace boost::qvm {

/// @brief Адаптер boost::qvm для stv::quaternion: элемент 0 — скалярная часть
/// (S), элементы 1..3 — векторная часть (X, Y, Z).
/// @tparam T Скалярный тип с плавающей точкой.
template<typename T>
struct quat_traits<stv::quaternion<T>> {
    using scalar_type = T;

    template<int I>
    [[nodiscard]] static constexpr auto read_element(
        const stv::quaternion<T> &quat) noexcept -> T
    { return quat.a[static_cast<std::size_t>(I)]; }

    template<int I>
    [[nodiscard]] static constexpr auto write_element(
        stv::quaternion<T> &quat) noexcept -> T &
    { return quat.a[static_cast<std::size_t>(I)]; }
};

} // namespace boost::qvm

#endif // QUATERNION_HPP
