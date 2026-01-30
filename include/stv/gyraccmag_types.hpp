/// @file inertial_sensors.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// @copyright (c) 2026 Gagaring
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

#ifndef GYRACCMAG_TYPES_HPP
#define GYRACCMAG_TYPES_HPP

#include <array>
#include <cmath>
#include <concepts>
#include <cstdint>
#include <type_traits>
#include <utility>

namespace stv {

/// @brief Concept for allowed sensor data types (floating-point or integral).
template<typename T>
concept allowed_sensor_type = std::floating_point<T> || std::integral<T>;

/// @brief Tag to indicate sensor data uses local coordinate frame.
///        All real sensors use this frame by default.
struct local_frame_tag {
};

/// @brief Tag to indicate sensor data uses world coordinate frame.
///        Useful in simulation models and orientation systems.
struct world_frame_tag {
};

// NOLINTBEGIN(*-special-member-functions)

template<allowed_sensor_type T = float>
class inertial_sens_storage_proxy
{
    std::array<T, 3> storage_;

  public:
    inertial_sens_storage_proxy(
        T x_axis, T y_axis, T z_axis)
    {
        storage_[0] = x_axis;
        storage_[1] = y_axis;
        storage_[2] = z_axis;
    }

    [[nodiscard]] auto begin() { return storage_.begin(); }

    [[nodiscard]] auto begin() const { return storage_.begin(); }

    [[nodiscard]] auto end() { return storage_.end(); }

    [[nodiscard]] auto end() const { return storage_.end(); }

    [[nodiscard]] auto data() { return storage_.data(); }

    [[nodiscard]] auto operator[](
        int idx)
    {
        return storage_[idx];
    }

    void swap(
        inertial_sens_storage_proxy &other) noexcept
    {
        using std::swap;
        swap(storage_, other.storage_);
    }

    // Дружественная функция для ADL.
    friend void swap(
        inertial_sens_storage_proxy &rhs,
        inertial_sens_storage_proxy &lhs) noexcept
    {
        rhs.swap(lhs);
    }
};

/// @brief Represents a 3D vector, typically used for sensor measurements.
///
/// This struct inherits from std::array<T, 3> and provides named accessors
/// (x, y, z) for the components. It supports common vector operations like
/// addition, subtraction, and scalar multiplication.
///
/// @tparam T The data type for the vector components (must satisfy
///           AllowedSensorType). Defaults to float.
/// @tparam Frame The coordinate frame tag (LocalFrameTag or WorldFrameTag).
///               Defaults to LocalFrameTag.
template<allowed_sensor_type T = float, typename Frame = local_frame_tag>
struct inertial_vector: public inertial_sens_storage_proxy<T> {
    using base_type = inertial_sens_storage_proxy<T>;

    /// @brief Type alias for the coordinate frame tag.
    using frame_tag = Frame;
    /// @brief Type alias for the underlying value type.
    using value_type = T;

    /// @brief Default value for sensor measurements.
    static constexpr value_type default_value{static_cast<value_type>(0)};

    /// @brief Gets the X-axis component (const version).
    ///
    /// @return The value of the X-axis component.
    [[nodiscard]] auto give_x() const { return base_type::operator[](0); }

    /// @brief Gets the Y-axis component (const version).
    ///
    /// @return The value of the Y-axis component.
    [[nodiscard]] auto give_y() const { return base_type::operator[](1); }

    /// @brief Gets the Z-axis component (const version).
    ///
    /// @return The value of the Z-axis component.
    [[nodiscard]] auto give_z() const { return base_type::operator[](2); }

    /// @brief Gets the X-axis component (non-const version).
    ///
    /// @return A reference to the X-axis component.
    [[nodiscard]] auto give_x() -> decltype(auto)
    {
        return base_type::operator[](0);
    }

    /// @brief Gets the Y-axis component (non-const version).
    ///
    /// @return A reference to the Y-axis component.
    [[nodiscard]] auto give_y() -> decltype(auto)
    {
        return base_type::operator[](1);
    }

    /// @brief Gets the Z-axis component (non-const version).
    ///
    /// @return A reference to the Z-axis component.
    [[nodiscard]] auto give_z() -> decltype(auto)
    {
        return base_type::operator[](2);
    }

    // NOLINTBEGIN(readability-identifier-length,
    // bugprone-easily-swappable-parameters)
    /// @brief Constructs sensor measurements with specified values.
    ///
    /// @param[in] x_axis X-axis measurement value.
    /// @param[in] y_axis Y-axis measurement value.
    /// @param[in] z_axis Z-axis measurement value.
    ///
    /// @note Parameters are intentionally short for sensor interface
    ///       compatibility despite readability concerns.
    constexpr inertial_vector(
        T x_axis, T y_axis, T z_axis) noexcept:
        base_type{x_axis, y_axis, z_axis}
    {
    }

    // NOLINTEND(readability-identifier-length,
    // bugprone-easily-swappable-parameters)
    /// @brief Constructs sensor measurements from array.
    ///
    /// @param[in] array_of_axis Array with measurements.
    explicit inertial_vector(
        const base_type &array_of_axis) noexcept:
        inertial_vector{array_of_axis[0], array_of_axis[1], array_of_axis[2]}
    {
    }

    /// @brief Default constructor initializes with default values.
    constexpr inertial_vector() noexcept:
        inertial_vector{default_value, default_value, default_value}
    {
    }

    /// @brief Virtual destructor for proper inheritance handling.
    virtual ~inertial_vector() = default;

    /// @name Rule of Five
    /// @{
    /// @brief Rule of Five: Copy constructor.
    inertial_vector(
        const inertial_vector &lhs) noexcept:
        base_type{lhs}
    {
    }

    /// @brief Rule of Five: Copy assignment operator with copy-and-swap idiom.
    auto operator=(
        inertial_vector lhs) noexcept -> inertial_vector &
    {
        this->swap(lhs);
        return *this;
    };

#if 0 // NOLINT(*-avoid-unconditional-preprocessor-if)
  /// @brief Rule of Five: Move constructor.
  //   Vector(Vector &&lhs) noexcept = default;

  /// @brief Rule of Five: Move assignment operator.
  //   auto operator=(Vector &&lhs) noexcept -> Vector & = default;
#endif
    /// @}

    void swap(
        inertial_vector &other) noexcept
    {
        base_type::swap(other);
    }

    friend void swap(
        inertial_vector &first, inertial_vector &second) noexcept
    {
        first.swap(second);
    }

    /// @brief Addition operator.
    ///
    /// @param[in] other The other vector to add.
    /// @return A new Vector representing the sum of this vector and the other.
    constexpr auto operator+(
        const inertial_vector &other) const noexcept -> inertial_vector
    {
        return {give_x() + other.give_x(), give_y() + other.give_y(),
                give_z() + other.give_z()};
    }

    /// @brief Adds another vector to this vector (+=).
    ///
    /// @param[in] other The other vector to add.
    /// @return A reference to this vector after the addition.
    constexpr auto operator+=(
        const inertial_vector &other) noexcept -> inertial_vector &
    {
        give_x() += other.give_x();
        give_y() += other.give_y();
        give_z() += other.give_z();
        return *this;
    }

    /// @brief Subtraction operator.
    ///
    /// @param[in] other The other vector to subtract.
    /// @return A new Vector representing the difference of this vector and the
    ///         other.
    constexpr auto operator-(
        const inertial_vector &other) const noexcept -> inertial_vector
    {
        return {give_x() - other.give_x(), give_y() - other.give_y(),
                give_z() - other.give_z()};
    }

    /// @brief Subtracts another vector from this vector (-=).
    ///
    /// @param[in] other The other vector to subtract.
    /// @return A reference to this vector after the subtraction.
    constexpr auto operator-=(
        const inertial_vector &other) noexcept -> inertial_vector &
    {
        give_x() -= other.give_x();
        give_y() -= other.give_y();
        give_z() -= other.give_z();
        return *this;
    }

    /// @brief Vector and scalar multiplication (vector * scalar).
    ///
    /// @tparam U The scalar type (must be arithmetic).
    /// @param[in] scalar The scalar value to multiply by.
    /// @return A new Vector with components multiplied by the scalar.
    template<typename U>
    constexpr auto operator*(
        U scalar) const noexcept
    {
        return inertial_vector{give_x() * scalar, give_y() * scalar,
                               give_z() * scalar};
    }

    /// @brief Scalar and vector multiplication (scalar * vector).
    ///
    /// @tparam U The scalar type.
    /// @param[in] scalar The scalar value.
    /// @param[in] vec The vector.
    /// @return A new Vector with components multiplied by the scalar.
    template<typename U>
    friend constexpr auto operator*(U                      scalar,
                                    const inertial_vector &vec) noexcept;

    /// @brief Equality operator.
    ///
    /// @param[in] other The other vector to compare with.
    /// @return True if all components are equal, false otherwise.
    constexpr auto operator==(
        const inertial_vector &other) const noexcept
    {
        return give_x() == other.give_x() && give_y() == other.give_y()
               && give_z() == other.give_z();
    }

    /// @brief Inequality operator.
    ///
    /// @param[in] other The other vector to compare with.
    /// @return True if any component differs, false otherwise.
    constexpr auto operator!=(
        const inertial_vector &other) const noexcept
    {
        return !(*this == other);
    }

    /// @brief Boolean conversion operator.
    ///
    /// @return True if all axes are valid (see IsAxisesValid), false otherwise.
    constexpr explicit operator bool() const noexcept
    {
        return is_axises_valid();
    }

    /// @brief Validates sensor axis values.
    ///
    /// For floating-point types, checks if all components are finite.
    /// For integral types, always returns true.
    ///
    /// @return True if axis values are valid, false otherwise.
    [[nodiscard]] constexpr auto is_axises_valid() const noexcept
    {
        if constexpr(std::is_floating_point_v<T>)
        {
            return std::isfinite(give_x()) && std::isfinite(give_y())
                   && std::isfinite(give_z());
        }
        else if constexpr(std::is_integral_v<T>)
        {
            return true;
        }
        else
        {
            static_assert(
                std::is_arithmetic_v<T>,
                "Support only floating and integral types. If you see this "
                "message, "
                "check if «T» is a floating or integral type (not a pointer or "
                "reference).");
            return false; // Should be unreachable due to static_assert
        }
    }
};

// NOLINTEND(*-special-member-functions)

/// @brief Addition operator for Vector.
///
/// @tparam T The data type for the vector components.
/// @tparam Frame The coordinate frame tag.
/// @param[in] lhs The left-hand side vector.
/// @param[in] rhs The right-hand side vector.
/// @return A new Vector representing the sum.
template<allowed_sensor_type T, typename Frame>
constexpr auto operator+(
    const inertial_vector<T, Frame> &lhs,
    const inertial_vector<T, Frame> &rhs) noexcept -> inertial_vector<T, Frame>
{
    return lhs.operator+(rhs);
}

/// @brief Subtraction operator for Vector.
///
/// @tparam T The data type for the vector components.
/// @tparam Frame The coordinate frame tag.
/// @param[in] lhs The left-hand side vector.
/// @param[in] rhs The right-hand side vector.
/// @return A new Vector representing the difference.
template<allowed_sensor_type T, typename Frame>
constexpr auto operator-(
    const inertial_vector<T, Frame> &lhs,
    const inertial_vector<T, Frame> &rhs) noexcept -> inertial_vector<T, Frame>
{
    return lhs.operator-(rhs);
}

/// @brief Support scalar * vector.
///
/// @tparam U The scalar type.
/// @tparam K The data type for the vector components (must satisfy
///           AllowedSensorType).
/// @tparam FrameFriend The coordinate frame tag.
/// @param[in] scalar The scalar value.
/// @param[in] vec The vector.
/// @return A new Vector with components multiplied by the scalar.
template<typename U, allowed_sensor_type K, typename FrameFriend>
constexpr auto operator*(
    U scalar, const inertial_vector<K, FrameFriend> &vec) noexcept
{
    return vec * scalar;
}

// NOLINTBEGIN(*-special-member-functions)

/// @brief Abstract 3-axis sensor interface base class.
///
/// Extends Vector to include a timestamp. Provides common functionality for
/// 3-axis sensors like gyroscopes, accelerometers, and magnetometers.
///
/// @tparam T Data type for sensor measurements (must satisfy
///           AllowedSensorType). Defaults to float.
/// @tparam TIMESTAMP Type for timestamp storage. Defaults to std::uint32_t.
/// @tparam Frame Coordinate frame tag (LocalFrameTag or WorldFrameTag).
///               Defaults to LocalFrameTag.z
template<allowed_sensor_type T = float, typename TIMESTAMP = std::uint32_t,
         typename Frame = local_frame_tag>
struct inertial_sensor_common: public stv::inertial_vector<T, Frame> {
    /// @brief Type alias for the coordinate frame tag.
    using frame_tag = Frame;
    /// @brief Type alias for the base Vector type.
    using base_type = inertial_vector<T, Frame>;
    /// @brief Type alias for the underlying value type.
    using value_type = typename base_type::value_type;
    /// @brief Type alias for the timestamp type.
    using timestamp_type = TIMESTAMP;
    /// @brief Default value for sensor measurements.
    static constexpr T default_value{static_cast<T>(0)};

    /// @brief Packet timestamp counter.
    TIMESTAMP packstamp{0};

    // NOLINTBEGIN(readability-identifier-length,
    // bugprone-easily-swappable-parameters)
    /// @brief Constructs sensor measurements with specified values.
    ///
    /// @param[in] x_axis X-axis measurement value.
    /// @param[in] y_axis Y-axis measurement value.
    /// @param[in] z_axis Z-axis measurement value.
    /// @param[in] pack_stamp Timestamp for the measurement packet.
    ///
    /// @note Parameters are intentionally short for sensor interface
    ///       compatibility despite readability concerns.
    constexpr inertial_sensor_common(
        T x_axis, T y_axis, T z_axis, TIMESTAMP pack_stamp = 0U) noexcept:
        base_type{x_axis, y_axis, z_axis},
        packstamp{pack_stamp}
    {
    }

    // NOLINTEND(readability-identifier-length,
    // bugprone-easily-swappable-parameters)
    /// @brief Constructs sensor measurements from a Vector and timestamp.
    ///
    /// @param[in] vector The vector containing axis measurements.
    /// @param[in] pack_stamp Timestamp for the measurement packet.
    explicit inertial_sensor_common(
        const base_type &vector, TIMESTAMP pack_stamp = 0U) noexcept:
        inertial_sensor_common{vector.give_x(), vector.give_y(),
                               vector.give_z(), pack_stamp}
    {
    }

    /// @brief Default constructor initializes with default values.
    constexpr inertial_sensor_common() noexcept:
        inertial_sensor_common{default_value, default_value, default_value, 0U}
    {
    }

    /// @brief Virtual destructor for proper inheritance handling.
    ~inertial_sensor_common() override = default;

    /// @name Rule of Five
    /// @{
    /// @brief Rule of Five: Copy constructor.
    inertial_sensor_common(
        const inertial_sensor_common &lhs) noexcept:
        base_type{lhs},
        packstamp{lhs.packstamp}
    {
    }

    /// @brief Rule of Five: Move constructor.
    inertial_sensor_common(inertial_sensor_common &&lhs) noexcept = default;

    /// @brief Rule of Five: Copy assignment operator with copy-and-swap idiom.
    auto operator=(
        inertial_sensor_common lhs) noexcept -> inertial_sensor_common &
    {
        this->swap(lhs);
        return *this;
    };

// To use beauty Copy assignment operator with copy-and-swap idiom, needs to
// delete Move assignment operator definition.
#if 0 // NOLINT(*-avoid-unconditional-preprocessor-if)
  /// @brief Rule of Five: Move assignment operator.
  auto operator=(Sensor3AxisCommon &&lhs) noexcept
      -> Sensor3AxisCommon & = default;
#endif

    /// @}

    void swap(
        inertial_sensor_common &other) noexcept
    {
        using std::swap;
        base_type::swap(other);
        std::swap(packstamp, other.packstamp);
    }

    friend void swap(
        inertial_sensor_common &first, inertial_sensor_common &second) noexcept
    {
        first.swap(second);
    }

    /// @brief Equality comparison operator (compares timestamps only).
    ///
    /// @param[in] other The other sensor data to compare with.
    /// @return True if timestamps are equal, false otherwise.
    constexpr auto operator==(
        const inertial_sensor_common<T> &other) const noexcept
    {
        return packstamp == other.packstamp;
    }

    /// @brief Inequality comparison operator (compares timestamps only).
    ///
    /// @param[in] other The other sensor data to compare with.
    /// @return True if timestamps differ, false otherwise.
    constexpr auto operator!=(
        const inertial_sensor_common<T> &other) const noexcept
    {
        return !(*this == other);
    }

    /// @brief Addition operator.
    ///
    /// Adds the vector components and uses the timestamp of this object.
    ///
    /// @param[in] other The other sensor data to add.
    /// @return A new Sensor3AxisCommon representing the vector sum, using the
    ///         timestamp of this object.
    constexpr auto operator+(
        const inertial_sensor_common &other) const noexcept
        -> inertial_sensor_common
    {
        return inertial_sensor_common{
            static_cast<const base_type &>(*this)
                + static_cast<const base_type &>(other),
            packstamp};
    }

    /// @brief Subtraction operator.
    ///
    /// Subtracts the vector components and uses the timestamp of this object.
    ///
    /// @param[in] other The other sensor data to subtract.
    /// @return A new Sensor3AxisCommon representing the vector difference,
    /// using
    ///         the timestamp of this object.
    constexpr auto operator-(
        const inertial_sensor_common &other) const noexcept
        -> inertial_sensor_common
    {
        return inertial_sensor_common{
            static_cast<const base_type &>(*this)
                - static_cast<const base_type &>(other),
            packstamp};
    }

    /// @brief Boolean conversion operator.
    ///
    /// @return True if the timestamp is valid (non-zero) and axis values are
    ///         valid (see IsAxisesValid), false otherwise.
    constexpr explicit operator bool() const noexcept
    {
        return is_timestamp_valid() && this->is_axises_valid();
    }

  private:
    /// @brief Validates timestamp value.
    ///
    /// @return True if the timestamp is non-zero, false otherwise.
    [[nodiscard]] constexpr auto is_timestamp_valid() const noexcept
    {
        return static_cast<TIMESTAMP>(packstamp) != static_cast<TIMESTAMP>(0);
    }
};

// NOLINTEND(*-special-member-functions)
/// @brief Addition operator for Sensor3AxisCommon.
///
/// @tparam T The data type for the sensor measurements.
/// @tparam TIMESTAMP The timestamp type.
/// @tparam Frame The coordinate frame tag.
/// @param[in] lhs The left-hand side sensor data.
/// @param[in] rhs The right-hand side sensor data.
/// @return A new Sensor3AxisCommon representing the vector sum, using the
///         timestamp of the left operand.
template<allowed_sensor_type T, typename TIMESTAMP, typename Frame>
constexpr auto operator+(
    const inertial_sensor_common<T, TIMESTAMP, Frame> &lhs,
    const inertial_sensor_common<T, TIMESTAMP, Frame> &rhs) noexcept
    -> inertial_sensor_common<T, TIMESTAMP, Frame>
{
    return lhs.operator+(rhs);
}

/// @brief Subtraction operator for Sensor3AxisCommon.
///
/// @tparam T The data type for the sensor measurements.
/// @tparam TIMESTAMP The timestamp type.
/// @tparam Frame The coordinate frame tag.
/// @param[in] lhs The left-hand side sensor data.
/// @param[in] rhs The right-hand side sensor data.
/// @return A new Sensor3AxisCommon representing the vector difference, using
///         the timestamp of the left operand.
template<allowed_sensor_type T, typename TIMESTAMP, typename Frame>
constexpr auto operator-(
    const inertial_sensor_common<T, TIMESTAMP, Frame> &lhs,
    const inertial_sensor_common<T, TIMESTAMP, Frame> &rhs) noexcept
    -> inertial_sensor_common<T, TIMESTAMP, Frame>
{
    return lhs.operator-(rhs);
}

/// NOLINTBEGIN(*-special-member-functions)

/// @brief Gyroscope measurement data structure.
///
/// Extends Sensor3AxisCommon to include the integration period for angular
/// velocity measurements.
///
/// @tparam T Data type for measurements (float, double). Defaults to float.
/// @tparam TIMESTAMP Type for timestamp storage. Defaults to std::uint32_t.
/// @tparam Frame Coordinate frame tag (LocalFrameTag or WorldFrameTag).
///               Defaults to LocalFrameTag.
template<allowed_sensor_type T = float, typename TIMESTAMP = std::uint32_t,
         typename Frame = local_frame_tag>
struct gyr: public inertial_sensor_common<T, TIMESTAMP, Frame> {
    /// @brief Type alias for the base Sensor3AxisCommon type.
    using base_type = inertial_sensor_common<T, TIMESTAMP, Frame>;

    /// @brief Type alias for the coordinate frame tag.
    using frame_tag = typename base_type::frame_tag;

    /// @brief Type alias for the underlying value type.
    using value_type = T;

    /// @brief Type alias for the timestamp type.
    using timestamp_type = TIMESTAMP;

    /// @brief Type alias for the integration period type.
    using period_type = T;

    /// @brief Constructs gyroscope measurement with specified values.
    ///
    /// @param[in] x_axis X-axis angular velocity.
    /// @param[in] y_axis Y-axis angular velocity.
    /// @param[in] z_axis Z-axis angular velocity.
    /// @param[in] pack_stamp Measurement timestamp.
    /// @param[in] gyr_dt_sec Integration period for angular velocity.
    constexpr gyr(
        T x_axis, T y_axis, T z_axis, TIMESTAMP pack_stamp = 0U,
        period_type gyr_dt_sec = 0):
        base_type{x_axis, y_axis, z_axis, pack_stamp},
        dT_sec{gyr_dt_sec}
    {
    }

    /// @brief Constructs gyroscope measurement from common sensor type.
    ///
    /// @param[in] sens Common sensor struct.
    /// @param[in] dt_sec Integration period for angular velocity.
    constexpr explicit gyr(
        const inertial_sensor_common<T, TIMESTAMP, local_frame_tag> &sens,
        period_type dt_sec = 0):
        base_type{sens},
        dT_sec{dt_sec}
    {
    }

    /// @brief Constructs gyroscope measurement from a Vector and timestamp.
    ///
    /// @param[in] sens The vector containing axis measurements.
    /// @param[in] pack_stamp Measurement timestamp.
    /// @param[in] dt_sec Integration period for angular velocity.
    constexpr explicit gyr(
        const inertial_vector<T, local_frame_tag> &sens,
        TIMESTAMP pack_stamp = 0, period_type dt_sec = 0):
        base_type{sens, pack_stamp},
        dT_sec{dt_sec}
    {
    }

    /// @brief Default constructor initializes with zero values.
    constexpr gyr():
        base_type{},
        dT_sec{static_cast<T>(0)}
    {
    }

    /// @brief Virtual destructor for proper inheritance handling.
    ~gyr() override = default;

    /// @name Rule of Five
    /// @{
    /// @brief Rule of Five: Copy constructor.
    gyr(
        const gyr &lhs) noexcept:
        base_type{lhs},
        dT_sec{lhs.dT_sec}
    {
    }

    /// @brief Rule of Five: Move constructor.
    gyr(gyr &&lhs) = default;

    /// @brief Rule of Five: Copy assignment operator with copy-and-swap idiom.
    auto operator=(
        gyr lhs) noexcept -> gyr &
    {
        this->swap(lhs);
        return *this;
    }

// To use beauty Copy assignment operator with copy-and-swap idiom, needs to
// delete Move assignment operator definition.
#if 0 // NOLINT(*-avoid-unconditional-preprocessor-if)
  /// @brief Rule of Five: Move assignment operator.
  auto operator=(SensorGyr &&lhs) -> SensorGyr & = default;
#endif
    /// @}

    /// @brief Boolean conversion operator.
    ///
    /// @return True if the integration period is non-zero and the base sensor
    ///         data is valid, false otherwise.
    constexpr explicit operator bool() const noexcept
    {
        return (dT_sec != static_cast<value_type>(0))
               && static_cast<const base_type &>(*this);
    }

    void swap(
        gyr &other) noexcept
    {
        using std::swap;
        base_type::swap(other);
        std::swap(dT_sec, other.dT_sec);
    }

    /// @brief Swaps the contents of two SensorGyr objects.
    ///
    /// @param[in,out] first The first SensorGyr object.
    /// @param[in,out] second The second SensorGyr object.
    friend void swap(
        gyr &first, gyr &second) noexcept
    {
        first.swap(second);
    }

    /// @brief Integration period for angular velocity measurements (seconds).
    ///
    /// NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
    period_type dT_sec;
};

// NOLINTEND(*-special-member-functions)

/// @brief Accelerometer measurement data structure.
///
/// Type alias for Sensor3AxisCommon, representing linear acceleration
/// measurements along three axes.
///
/// @tparam T Data type for measurements (float, double).
/// @tparam TIMESTAMP Type for timestamp storage. Defaults to std::uint32_t.
/// @tparam Frame Coordinate frame tag (LocalFrameTag or WorldFrameTag).
///               Defaults to LocalFrameTag.
template<allowed_sensor_type T, typename TIMESTAMP = std::uint32_t,
         typename Frame = local_frame_tag>
using acc = inertial_sensor_common<T, TIMESTAMP, Frame>;

/// @brief Magnetometer measurement data structure.
///
/// Type alias for Sensor3AxisCommon, representing magnetic field strength
/// measurements along three axes.
///
/// @tparam T Data type for measurements (float, double).
/// @tparam TIMESTAMP Type for timestamp storage. Defaults to std::uint32_t.
/// @tparam Frame Coordinate frame tag (LocalFrameTag or WorldFrameTag).
///               Defaults to LocalFrameTag.
template<allowed_sensor_type T, typename TIMESTAMP = std::uint32_t,
         typename Frame = local_frame_tag>
using mag = inertial_sensor_common<T, TIMESTAMP, Frame>;

/// @brief Interface for gyroscope data acquisition.
///
/// Defines the contract for classes that provide gyroscope data.
///
/// @tparam T Data type for measurements (float, double). Defaults to float.
/// @tparam TIMESTAMP Type for timestamp storage. Defaults to std::uint32_t.
template<allowed_sensor_type T = float, typename TIMESTAMP = std::uint32_t>
class igyr
{
  public:
    /// @brief Type alias for the measurement value type.
    using value_type = T;

    /// @brief Type alias for the timestamp value type.
    using timestamp_value = TIMESTAMP;

    using gyr_type = stv::gyr<value_type, timestamp_value>;

    /// @brief Reads gyroscope data from the sensor.
    ///
    /// @return Filled gyroscope measurement structure.
    [[nodiscard]] virtual auto get_gyr() const -> gyr<T, TIMESTAMP> = 0;

    /// @brief Virtual destructor for proper inheritance handling.
    virtual ~igyr() = default;

  protected:
    /// @name Rule of Five (Protected)
    /// @{
    /// @brief Default constructor for inheritance.
    igyr() = default;

    /// @brief Rule of Five: Copy constructor.
    igyr(const igyr &lhs) = default;

    /// @brief Rule of Five: Copy assignment operator.
    auto operator=(const igyr &lhs) -> igyr & = default;

    /// @brief Rule of Five: Move constructor.
    igyr(igyr &&lhs) = default;

    /// @brief Rule of Five: Move assignment operator.
    auto operator=(igyr &&lhs) -> igyr & = default;
    /// @}
};

/// @brief Default gyroscope implementation that always returns zero values.
///
/// A concrete implementation of IGyrGetter that provides default
/// (zero-initialized) gyroscope measurements.
///
/// @tparam T Data type for measurements (float, double). Defaults to float.
/// @tparam TIMESTAMP Type for timestamp storage. Defaults to std::uint32_t.
template<allowed_sensor_type T = float, typename TIMESTAMP = std::uint32_t>
class gyr_getter_default final: public igyr<T>
{
  public:
    /// @brief Type alias for the measurement value type.
    using typename igyr<T, TIMESTAMP>::value_type;

    /// @brief Type alias for the timestamp value type.
    using typename igyr<T, TIMESTAMP>::timestamp_value;

    /// @brief Default constructor.
    constexpr gyr_getter_default() = default;

    /// @brief Virtual destructor.
    ~gyr_getter_default() override = default;

    /// @brief Returns default (zero) gyroscope measurements.
    ///
    /// @return Default-initialized gyroscope measurement structure.
    [[nodiscard]] constexpr auto get_gyr() const -> gyr<T, TIMESTAMP> override
    {
        return gyr<T, TIMESTAMP>{};
    }

    /// @name Rule of Five
    /// @{
    /// @brief Rule of Five: Copy constructor.
    gyr_getter_default(const gyr_getter_default &lhs) = default;

    /// @brief Rule of Five: Copy assignment operator.
    auto operator=(const gyr_getter_default &lhs)
        -> gyr_getter_default & = default;

    /// @brief Rule of Five: Move constructor.
    gyr_getter_default(gyr_getter_default &&lhs) = default;

    /// @brief Rule of Five: Move assignment operator.
    auto operator=(gyr_getter_default &&lhs) -> gyr_getter_default & = default;
    /// @}
};

/// @brief Interface for accelerometer data acquisition.
///
/// Defines the contract for classes that provide accelerometer data.
///
/// @tparam T Data type for measurements (float, double). Defaults to float.
/// @tparam TIMESTAMP Type for timestamp storage. Defaults to std::uint32_t.
template<allowed_sensor_type T = float, typename TIMESTAMP = std::uint32_t>
class iacc
{
  public:
    /// @brief Type alias for the measurement value type.
    using value_type = T;

    /// @brief Type alias for the timestamp value type.
    using timestamp_type = TIMESTAMP;

    using acc_type = stv::acc<value_type, timestamp_type>;

    /// @brief Reads accelerometer data from the sensor.
    ///
    /// @return Filled accelerometer measurement structure.
    [[nodiscard]] virtual auto get_acc() const -> acc_type = 0;

    /// @brief Virtual destructor for proper inheritance handling.
    virtual ~iacc() = default;

  protected:
    /// @name Rule of Five (Protected)
    /// @{
    /// @brief Default constructor for inheritance.
    iacc() = default;

    /// @brief Rule of Five: Copy constructor.
    iacc(const iacc &lhs) = default;

    /// @brief Rule of Five: Copy assignment operator.
    auto operator=(const iacc &lhs) -> iacc & = default;

    /// @brief Rule of Five: Move constructor.
    iacc(iacc &&lhs) = default;

    /// @brief Rule of Five: Move assignment operator.
    auto operator=(iacc &&lhs) -> iacc & = default;

    /// @}
};

/// @brief Default accelerometer implementation that always returns zero
/// values.
///
/// A concrete implementation of IAccGetter that provides default
/// (zero-initialized) accelerometer measurements.
///
/// @tparam T Data type for measurements (float, double). Defaults to float.
/// @tparam TIMESTAMP Type for timestamp storage. Defaults to std::uint32_t.
template<allowed_sensor_type T = float, typename TIMESTAMP = std::uint32_t>
class acc_getter_default final: public iacc<T>
{
    using base_type = iacc<T>;

  public:
    /// @brief Type alias for the timestamp value type.
    using typename base_type::timestamp_value;

    /// @brief Type alias for the measurement value type.
    using typename base_type::value_type;

    using typename base_type::acc_type;

    /// @brief Default constructor.
    constexpr acc_getter_default() = default;

    /// @brief Virtual destructor.
    ~acc_getter_default() override = default;

    /// @brief Returns default (zero) accelerometer measurements.
    ///
    /// @return Default-initialized accelerometer measurement structure.
    [[nodiscard]] constexpr auto get_acc() const -> acc_type override
    {
        return acc_type{};
    }

    /// @name Rule of Five
    /// @{
    /// @brief Rule of Five: Copy constructor.
    acc_getter_default(const acc_getter_default &lhs) = default;

    /// @brief Rule of Five: Copy assignment operator.
    auto operator=(const acc_getter_default &lhs)
        -> acc_getter_default & = default;

    /// @brief Rule of Five: Move constructor.
    acc_getter_default(acc_getter_default &&lhs) = default;

    /// @brief Rule of Five: Move assignment operator.
    auto operator=(acc_getter_default &&lhs) -> acc_getter_default & = default;

    /// @}
};

/// @brief Interface for magnetometer data acquisition.
///
/// Defines the contract for classes that provide magnetometer data.
///
/// @tparam T Data type for measurements (float, double). Defaults to float.
/// @tparam TIMESTAMP Type for timestamp storage. Defaults to std::uint32_t.
template<allowed_sensor_type T = float, typename TIMESTAMP = std::uint32_t>
class imag
{
  public:
    /// @brief Type alias for the measurement value type.
    using value_type = T;

    /// @brief Type alias for the timestamp value type.
    using timestamp_value = TIMESTAMP;

    using mag_type = stv::mag<T, TIMESTAMP>;

    /// @brief Reads magnetometer data from the sensor.
    ///
    /// @return Filled magnetometer measurement structure.
    [[nodiscard]] virtual auto get_mag() const -> mag<T, TIMESTAMP> = 0;

    /// @brief Virtual destructor for proper inheritance handling.
    virtual ~imag() = default;

  protected:
    /// @name Rule of Five (Protected)
    /// @{
    /// @brief Default constructor for inheritance.
    imag() = default;

    /// @brief Rule of Five: Copy constructor.
    imag(const imag &lhs) = default;

    /// @brief Rule of Five: Copy assignment operator.
    auto operator=(const imag &lhs) -> imag & = default;

    /// @brief Rule of Five: Move constructor.
    imag(imag &&lhs) = default;

    /// @brief Rule of Five: Move assignment operator.
    auto operator=(imag &&lhs) -> imag & = default;

    /// @}
};

/// @brief Default magnetometer implementation that always returns zero
/// values.
///
/// A concrete implementation of IMagGetter that provides default
/// (zero-initialized) magnetometer measurements.
///
/// @tparam T Data type for measurements (float, double). Defaults to float.
/// @tparam TIMESTAMP Type for timestamp storage. Defaults to std::uint32_t.
template<allowed_sensor_type T = float, typename TIMESTAMP = std::uint32_t>
class mag_getter_default final: public stv::imag<T, TIMESTAMP>
{
    using base_type = stv::imag<T, TIMESTAMP>;

  public:
    /// @brief Type alias for the timestamp value type.
    using typename imag<T, TIMESTAMP>::timestamp_value;

    /// @brief Type alias for the measurement value type.
    using typename imag<T, TIMESTAMP>::value_type;

    using mag_type = typename base_type::mag_type;

    /// @brief Default constructor.
    constexpr mag_getter_default() = default;

    /// @brief Virtual destructor.
    ~mag_getter_default() override = default;

    /// @brief Returns default (zero) magnetometer measurements.
    ///
    /// @return Default-initialized magnetometer measurement structure.
    [[nodiscard]] constexpr auto get_mag() const -> mag_type override
    {
        return mag_type{};
    }

    /// @name Rule of Five
    /// @{
    /// @brief Rule of Five: Copy constructor.
    mag_getter_default(const mag_getter_default &lhs) = default;

    /// @brief Rule of Five: Copy assignment operator.
    auto operator=(const mag_getter_default &lhs)
        -> mag_getter_default & = default;

    /// @brief Rule of Five: Move constructor.
    mag_getter_default(mag_getter_default &&lhs) = default;

    /// @brief Rule of Five: Move assignment operator.
    auto operator=(mag_getter_default &&lhs) -> mag_getter_default & = default;

    /// @}
};

/// @brief Concept to check if a type is a 3-axis sensor.
///
/// A type satisfies this concept if it is derived from
/// stv::Vector<T::value_type, T::frame_tag>.
template<typename T>
concept Is3AxisSensor = std::is_base_of_v<
    stv::inertial_vector<typename T::value_type, typename T::frame_tag>, T>;

/// @brief Concept to check if a type has a timestamp_type member type.
template<class T>
concept HasTimestampType = requires { typename T::timestamp_type; };

/// @brief Concept to check if a type has a period_type member type.
template<class T>
concept HasPeriodType = requires { typename T::period_type; };

/// @brief Concept for 3-axis sensors with a timestamp.
///
/// Combines Is3AxisSensor and HasTimestampType.
template<class T>
concept Is3AxisSensorWithTimestamp = Is3AxisSensor<T> && HasTimestampType<T>;

/// @brief Concept for 3-axis sensors with a timestamp and period.
///
/// Combines Is3AxisSensor, HasTimestampType, and HasPeriodType.
template<class T>
concept Is3AxisSensorWithTimestampAndPeriod =
    Is3AxisSensor<T> && HasTimestampType<T> && HasPeriodType<T>;

} // namespace stv

#endif /* GYRACCMAG_TYPES_HPP */
