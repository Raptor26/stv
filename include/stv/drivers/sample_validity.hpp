/// @file sample_validity.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.

#ifndef SAMPLE_VALIDITY_HPP
#define SAMPLE_VALIDITY_HPP

#include <limits>

namespace stv {

/// @brief Проверяет валидность значения одной оси измерения.
///
/// @details Значение оси считается невалидным, если оно равно минимальному
/// или максимальному значению своего типа. Такие значения являются
/// признаком насыщения канала измерения или сбоя при передаче данных
/// по шине.
///
/// @tparam AxisType Тип значения оси (целочисленный без знака или со
/// знаком).
/// @param[in] axis Значение оси для проверки.
/// @return true, если значение оси отлично от min/max своего типа.
template<typename AxisType>
[[nodiscard]] constexpr auto is_axis_valid(
    AxisType axis) -> bool
{
    auto is_valid{true};

    if((axis == std::numeric_limits<AxisType>::max())
       || (axis == std::numeric_limits<AxisType>::min()))
    {
        is_valid = false;
    }

    return is_valid;
}

/// @brief Проверяет валидность трёх осей измерения.
///
/// @details Данные считаются валидными, если ни одна из осей не достигает
/// минимального или максимального значения своего типа (см.
/// `stv::is_axis_valid`).
///
/// @tparam AxisType Тип значений осей (целочисленный без знака или со
/// знаком).
/// @param[in] x_axis Значение по оси X.
/// @param[in] y_axis Значение по оси Y.
/// @param[in] z_axis Значение по оси Z.
/// @return true, если все три оси содержат валидные значения.
template<typename AxisType>
[[nodiscard]] constexpr auto are_axes_valid(
    AxisType x_axis, AxisType y_axis, AxisType z_axis) -> bool
{
    return is_axis_valid(x_axis) && is_axis_valid(y_axis)
           && is_axis_valid(z_axis);
}

} // namespace stv

#endif /* SAMPLE_VALIDITY_HPP */
