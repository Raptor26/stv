/// @file pid.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.

///
/// NAME
///     pid
///
/// DESCRIPTION
///     pid provides an abstract interface for PID (proportional-integral-
///     derivative) controllers.
///
///     Main components:
///     - pid_interface: abstract interface with configurable value type
///
///     The interface defines:
///     - update(): computes the control output from setpoint, measurement
///       and the elapsed time step
///     - reset(): clears the internal state (integral accumulator,
///       previous error)
///
///     Implementations are expected to be free of dynamic memory allocation
///     and suitable for real-time control loops.
///
/// EXAMPLE
///     ```cpp
///     class my_pid final: public stv::pid_interface<float>
///     {
///         float update(float setpoint, float measurement,
///                      float dt) override { /* ... */ }
///         void reset() override { /* ... */ }
///     };
///     ```
///

#ifndef STV_CONTROLLERS_PID_HPP
#define STV_CONTROLLERS_PID_HPP

namespace stv {

/// @brief Abstract interface of a PID controller.
///
/// @tparam T Value type of the control signal (float by default).
template<typename T = float>
class pid_interface
{
  public:
    using value_type = T;

    virtual ~pid_interface() = default;

    /// @brief Computes the control output.
    ///
    /// @param[in] setpoint Desired value of the controlled variable.
    /// @param[in] measurement Measured value of the controlled variable.
    /// @param[in] dt Elapsed time since the previous update (same time
    /// unit as used by the implementation, e.g. seconds).
    ///
    /// @return Control output.
    virtual value_type update(value_type setpoint, value_type measurement,
                              value_type dt) = 0;

    /// @brief Clears the internal state (integral accumulator, previous
    /// error).
    virtual void reset() = 0;

  protected:
    pid_interface() = default;
};

} // namespace stv

#endif /* STV_CONTROLLERS_PID_HPP */
