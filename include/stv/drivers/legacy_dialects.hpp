/// @file legacy_dialects.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.

#ifndef STV_DRIVERS_LEGACY_DIALECTS_HPP
#define STV_DRIVERS_LEGACY_DIALECTS_HPP

#include "stv/gyraccmag_types.hpp"
#include <string_view>

namespace stv::drivers {

template<typename T = float>
using magnetic_meas = stv::inertial_vector<T>;

template<typename T = float, typename TIMESTAMP = std::uint32_t>
using sensor3_axis_common = stv::inertial_sensor_common<T, TIMESTAMP>;

template<typename NormalizeMeasType = sensor3_axis_common<float, std::uint32_t>>
class i_magnetic_latest
{
  public:
    using meas_type  = NormalizeMeasType;
    using value_type = typename NormalizeMeasType::value_type;

    virtual ~i_magnetic_latest() = default;

    i_magnetic_latest(const i_magnetic_latest &other) = delete;
    i_magnetic_latest(i_magnetic_latest &&other)      = delete;

    auto operator=(const i_magnetic_latest &other)
        -> i_magnetic_latest &                                       = delete;
    auto operator=(i_magnetic_latest &&other) -> i_magnetic_latest & = delete;

    [[nodiscard]] virtual auto give_latest_meas() const -> meas_type = 0;

    [[nodiscard]] virtual auto noise_level() const -> float
    { return static_cast<value_type>(0); }

  protected:
    i_magnetic_latest() = default;
};

template<typename NormalizeMeasType = sensor3_axis_common<float, std::uint32_t>>
class i_magnetic: public i_magnetic_latest<NormalizeMeasType>
{
  public:
    using meas_type  = NormalizeMeasType;
    using value_type = typename NormalizeMeasType::value_type;

    ~i_magnetic() override = default;

    i_magnetic(const i_magnetic &other) = delete;
    i_magnetic(i_magnetic &&other)      = delete;

    auto         operator=(const i_magnetic &other) -> i_magnetic & = delete;
    auto         operator=(i_magnetic &&other) -> i_magnetic &      = delete;

    virtual auto init() -> bool = 0;

    virtual auto read_meas() -> meas_type = 0;

    [[nodiscard]] virtual auto freq() const -> float
    { return static_cast<value_type>(0); }

    [[nodiscard]] virtual auto give_name() const -> std::string_view
    { return {"Unknown"}; }

    [[nodiscard]] virtual auto is_self_test_valid() const -> bool
    { return false; }

  protected:
    i_magnetic() = default;
};

/// @brief Реализация по умолчанию. Используется для предоставления заглушки
/// интерфейса i_magnetic.
template<typename TBase>
class magnetic_default: public TBase
{
    using meas_type = typename TBase::meas_type;

  public:
    magnetic_default()          = default;
    virtual ~magnetic_default() = default;

    magnetic_default(const magnetic_default &other) = delete;
    magnetic_default(magnetic_default &&other)      = delete;

    auto operator=(const magnetic_default &other)
        -> magnetic_default &                                      = delete;
    auto operator=(magnetic_default &&other) -> magnetic_default & = delete;

    auto init() -> bool override { return false; }

    auto read_meas() -> meas_type override { return meas_type{}; }

    /// @brief Возвращает крайнее буферизированное значение измерений.
    /// @return Буферизированное значение измерения датчика.
    [[nodiscard]] auto give_latest_meas() const -> meas_type override
    { return meas_type{}; }
};

} // namespace stv::drivers

namespace kraslibs::drivers {

template<typename T = float>
using magnetic_meas = stv::drivers::magnetic_meas<T>;

template<typename T = float, typename TIMESTAMP = std::uint32_t>
using sensor3_axis_common = stv::drivers::sensor3_axis_common<T, TIMESTAMP>;

template<typename NormalizeMeasType =
             stv::drivers::sensor3_axis_common<float, std::uint32_t>>
using i_magnetic_latest = stv::drivers::i_magnetic_latest<NormalizeMeasType>;

template<typename NormalizeMeasType =
             stv::drivers::sensor3_axis_common<float, std::uint32_t>>
using i_magnetic = stv::drivers::i_magnetic<NormalizeMeasType>;

template<typename TBase>
using magnetic_default = stv::drivers::magnetic_default<TBase>;

} // namespace kraslibs::drivers

#endif /* STV_DRIVERS_LEGACY_DIALECTS_HPP */
