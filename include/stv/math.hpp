/// @file math.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.

#ifndef MATH_HPP
#define MATH_HPP

#include <array>
#include <boost/qvm/quat_access.hpp>
#include <cmath>
#include <concepts>
#include <numbers>
#include <type_traits>

namespace stv {

// NOLINTBEGIN(*-magic-numbers)

// NOLINTBEGIN(*readability-identifier-length)
/// @brief Число pi.
/// @tparam T Тип с плавающей точкой.
template<std::floating_point T>
inline constexpr T pi = std::numbers::pi_v<T>;

/// @brief Число pi, делённое на 2.
/// @tparam T Тип с плавающей точкой.
template<std::floating_point T>
inline constexpr T pi_over_2 = std::numbers::pi_v<T> / T(2);

/// @brief Удвоенное число pi (2*pi).
/// @tparam T Тип с плавающей точкой.
template<std::floating_point T>
inline constexpr T two_pi = T(2) * std::numbers::pi_v<T>;

// NOLINTEND(*readability-identifier-length)

/// @brief Угол крена (roll, вращение вокруг оси X) из кватерниона.
/// @tparam Q Тип кватерниона, адаптированный под boost::qvm::quat_traits
/// (например, stv::quaternion или boost::qvm::quat).
/// @param quat Кватернион ориентации с порядком компонент (w, x, y, z).
/// @return Угол крена в радианах.
inline auto to_euler_roll(
    const auto &quat)
{
    using scalar_t = typename boost::qvm::quat_traits<
        std::remove_cvref_t<decltype(quat)>>::scalar_type;

    constexpr auto two = static_cast<scalar_t>(2);
    constexpr auto one = static_cast<scalar_t>(1);

    return std::atan2(two
                          * (boost::qvm::S(quat) * boost::qvm::X(quat)
                             + boost::qvm::Y(quat) * boost::qvm::Z(quat)),
                      one
                          - (two
                             * (boost::qvm::X(quat) * boost::qvm::X(quat)
                                + boost::qvm::Y(quat) * boost::qvm::Y(quat))));
}

/// @brief Угол тангажа (pitch, вращение вокруг оси Y) из кватерниона.
/// @tparam Q Тип кватерниона, адаптированный под boost::qvm::quat_traits
/// (например, stv::quaternion или boost::qvm::quat).
/// @param quat Кватернион ориентации с порядком компонент (w, x, y, z).
/// @return Угол тангажа в радианах.
inline auto to_euler_pitch(
    const auto &quat)
{
    using scalar_t = typename boost::qvm::quat_traits<
        std::remove_cvref_t<decltype(quat)>>::scalar_type;

    return std::asin(static_cast<scalar_t>(2)
                     * (boost::qvm::S(quat) * boost::qvm::Y(quat)
                        - boost::qvm::Z(quat) * boost::qvm::X(quat)));
}

/// @brief Угол рыскания (yaw, вращение вокруг оси Z) из кватерниона.
/// @tparam Q Тип кватерниона, адаптированный под boost::qvm::quat_traits
/// (например, stv::quaternion или boost::qvm::quat).
/// @param quat Кватернион ориентации с порядком компонент (w, x, y, z).
/// @return Угол рыскания в радианах.
inline auto to_euler_yaw(
    const auto &quat)
{
    using scalar_t = typename boost::qvm::quat_traits<
        std::remove_cvref_t<decltype(quat)>>::scalar_type;

    constexpr auto two = static_cast<scalar_t>(2);
    constexpr auto one = static_cast<scalar_t>(1);

    return std::atan2(two
                          * (boost::qvm::S(quat) * boost::qvm::Z(quat)
                             + boost::qvm::X(quat) * boost::qvm::Y(quat)),
                      one
                          - (two
                             * (boost::qvm::Y(quat) * boost::qvm::Y(quat)
                                + boost::qvm::Z(quat) * boost::qvm::Z(quat))));
}

/// @brief Углы Эйлера (roll, pitch, yaw) из кватерниона.
/// @tparam Q Тип кватерниона, адаптированный под boost::qvm::quat_traits
/// (например, stv::quaternion или boost::qvm::quat).
/// @param quat Кватернион ориентации с порядком компонент (w, x, y, z).
/// @return Массив {roll, pitch, yaw} в радианах.
inline auto to_euler(
    const auto &quat)
{
    using scalar_t = typename boost::qvm::quat_traits<
        std::remove_cvref_t<decltype(quat)>>::scalar_type;

    return std::array<scalar_t, 3>{
        to_euler_roll(quat),
        to_euler_pitch(quat),
        to_euler_yaw(quat),
    };
}

/// @brief Обертывает угол в диапазон [0; 2*pi).
///
/// @param[in] radian Угол в радианах.
/// @return Эквивалентный угол в диапазоне [0; 2*pi).
///
/// @note Для входного значения, кратного -2*pi, результатом является
/// отрицательный ноль (-0.0).
inline auto wrap_2pi(
    const std::floating_point auto radian)
{
    auto res = std::fmod(radian, stv::two_pi<decltype(radian)>);
    if(res < 0)
    {
        res += stv::two_pi<decltype(radian)>;
    }
    return res;
}

/// @brief Обертывает угол в диапазон (-pi; +pi].
///
/// @param[in] radian Угол в радианах.
/// @return Эквивалентный угол в диапазоне (-pi; +pi].
///
/// @note Граница разреза углового пространства смещена: wrap_pi(-pi)
/// возвращает +pi.
inline auto wrap_pi(
    const std::floating_point auto radian)
{
    auto res = wrap_2pi(radian);
    if(res > stv::pi<decltype(radian)>)
    {
        res -= stv::two_pi<decltype(radian)>;
    }
    return res;
}

// NOLINTEND(*-magic-numbers)

} // namespace stv

#endif // MATH_HPP
