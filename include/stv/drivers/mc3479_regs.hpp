/// @file mc3479_regs.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.

#ifndef MC3479_REGS_HPP
#define MC3479_REGS_HPP

#include "stv/drivers/mc3479_types.hpp"
#include <cstdint>

namespace stv {

enum class mc3479_state : std::uint8_t {
    sleep = 0,
    wake,
    reserved,
    standby,
};

struct mc3479_device_status_reg {
    static constexpr mc3479_reg_type addr{0x05};

    /// @brief Текущее состояние (режим) акселерометра.
    /// @details Значение кодируется в 2 бита.
    mc3479_state state : 2 {stv::mc3479_state::sleep};

    /// @brief Resolution mode of the current device.
    /// - 0: 16-bit (high) resolution is enabled.
    /// - 1: Reserved.
    bool               res_mode : 1 {false};

    const std::uint8_t zero_1 : 1 {0};

    /// @brief 2C watchdog timeout. This bit is cleared when register 0x05 is
    /// read.
    /// - 0: A watchdog event is not detected.
    /// - 1: A watchdog event has been detected by the hardware and the I2C
    ///   slave state machine is reset to idle
    bool               i2c_wdt : 1 {false};

    const std::uint8_t zero_2 : 2 {0};

    /// @brief One-Time programming (OTP) activity status.
    /// - 0: Internal memory is idle and the device is ready to use.
    /// - 1: Internal memory is active and the device cannot be used.
    bool otp_busy : 1 {false};
};

static_assert(sizeof(mc3479_device_status_reg) == 1, "reg size must be 1");

// -----------------------------------------------------------------------------

struct mc3479_interrupt_enable_reg {
    static constexpr mc3479_reg_type addr{0x06};

    /// @brief Use with the tilt/flip feature in the motion control register
    /// (register 0x09, bit 0) to activate the reporting status of the tilt
    /// interrupt.
    bool tilt_int_en : 1 {false};

    /// @brief Use with the tilt/flip feature in the motion control register
    /// (register 0x09, bit 0) to activate the reporting status of the flip
    /// interrupt.
    bool flip_int_en : 1 {false};

    /// @brief Use with the AnyMotion feature in the motion control register
    /// (register 0x09, bit2) to activate the reporting status of the AnyMotion
    /// interrupt.
    bool anym_int_enable : 1 {false};

    /// @brief Use with the shake feature in the motion control register
    /// (register 0x09, bit 3) and the AnyMotion feature in the motion control
    /// register (register 0x09, bit 2) to activate the reporting status of the
    /// shake interrupt.
    bool shake_int_enable : 1 {false};

    /// @brief Use with the tilt-35 feature in the motion control register
    /// (register 0x09, bit 4) and the AnyMotion feature in the motion control
    /// register (register 0x09, bit 2) to activate the reporting status of the
    /// tilt-35 interrupt.
    bool               tilt_35_int_enable : 1 {false};
    const std::uint8_t reserved           : 1 {0};

    /// @brief Clear pending interrupts automatically or by reading a register.
    /// Enabling more than one interrupt timeout or service feature may produce
    /// unexpected results.
    bool auto_clr_en : 1 {false};

    /// @brief Generate interrupts.
    bool acq_int_en : 1 {false};
};

static_assert(sizeof(mc3479_interrupt_enable_reg) == 1, "reg size must be 1");

// -----------------------------------------------------------------------------

struct mc3479_mode_reg {
    static constexpr mc3479_reg_type addr{0x07};

    /// @brief Accelerometer operational state.
    /// - 00: SLEEP. Clocks are not running and X, Y, and Z-axis data are not
    ///   sampled.
    /// - 01: WAKE. Clocks are running and X, Y, and Z-axis data are acquired at
    ///   he sample rate.
    //  - 10: Reserved.
    //  - 11: STANDBY. Clocks are running but X, Y, and Z-axis data are not
    //    sampled
    mc3479_state       state : 2 {stv::mc3479_state::wake};

    const std::uint8_t zero_1 : 2 {0};

    /// @brief Watchdog timer for negative SCL stalls.
    /// - 0: The I2C watchdog timer for negative SCL stalls is disabled
    /// (default).
    /// - 1: The I2C watchdog timer for negative SCL stalls is enabled.
    bool wtd_neg : 1 {false};

    /// @brief Watchdog timer for positive SCL stalls.
    /// - 0: The I2C watchdog timer for positive SCL stalls is disabled
    /// (default).
    /// - 1: The I2C watchdog timer for positive SCL stalls is enabled
    bool               wtd_pos : 1 {false};

    const std::uint8_t zero_2 : 2 {0};
};

static_assert(sizeof(mc3479_mode_reg) == 1, "reg size must be 1");

// -----------------------------------------------------------------------------

enum class mc3479_sample_rate : uint8_t {
    hz_50   = 0x08,
    hz_100  = 0x09,
    hz_125  = 0x0A,
    hz_200  = 0x0B,
    hz_250  = 0x0C,
    hz_500  = 0x0D,
    hz_1000 = 0x0E,
    hz_2000 = 0x0F,
};

struct mc3479_sample_rate_reg {
    static constexpr mc3479_reg_type addr{0x08};

    mc3479_sample_rate rate    : 4 {stv::mc3479_sample_rate::hz_1000};
    std::uint8_t       reserve : 4 {0};
};

static_assert(sizeof(mc3479_sample_rate_reg) == 1, "reg size must be 1");

// -----------------------------------------------------------------------------

struct mc3479_motion_control_reg {
    static constexpr mc3479_reg_type addr{0x09};

    /// @brief Enable or disable the tilt/flip feature. Used with the tilt/flip
    /// features in registers 0x13, 0x14, and 0x0
    /// - 0: Tilt/Flip feature is disabled (default).
    /// - 1: Tilt/Flip feature is enabled.
    bool tf_enable : 1 {false};

    /// If motion interrupts are used, this bit is generally not used.
    /// - 0: Motion block does not latch outputs.
    /// - 1: Motion block latches outputs.
    bool motion_latch : 1 {false};

    /// Enable or disable the AnyMotion feature. Used with the AnyMotion feature
    /// in registers 0x13, 0x14, and 0x06 and the shake and tilt-35 features in
    /// registers 0x14 and 0x06.
    /// - 0: AnyMotion feature is disabled (default).
    /// - 1: AnyMotion feature is enabled
    bool anym_en : 1 {false};

    /// Enable or disable the shake feature. Used with the shake feature in
    /// registers 0x13, 0x14, and 0x06.
    /// - 0: Shake feature is disabled (default).
    /// - 1: Shake feature is enabled. ANYM_EN must also be enabled.
    bool shake_en : 1 {false};

    /// Enable or disable the tilt-35 feature. Used with tilt-35 feature in
    /// registers 0x13, 0x14, and 0x06.
    /// - 0: Tilt-35 feature is disabled (default).
    /// - 1: Tilt-35 feature is enabled. ANYM_EN must also be enabled.
    bool tilt35_en : 1 {false};

    /// Z-axis orientation.
    /// - 0: Z-axis orientation is positive through the top of the package
    ///   (default).
    /// - 1: Z-axis orientation is positive through the bottom of the package.
    bool z_axis_ort : 1 {false};

    /// Enable or disable filtering of motion data.
    /// - 0: Motion flag bits are filtered by debounce and other settings
    ///   (default).
    /// - 1: Motion flag bits are real-time, raw data
    bool raw_proc_stat : 1 {false};

    /// Motion block reset. This bit is not automatically cleared.
    /// - 0: The motion block is not in reset (default).
    /// - 1: The motion block is held in reset. The software must set this bit
    ///   for the reset to be cleared.
    bool motion_reset : 1 {false};
};

static_assert(sizeof(mc3479_motion_control_reg) == 1, "reg size must be 1");

// -----------------------------------------------------------------------------

struct mc3479_status_reg {
    static constexpr mc3479_reg_type addr{0x13};

    bool                             tilt_flag   : 1 {false};
    bool                             flip_flag   : 1 {false};
    bool                             anym_flag   : 1 {false};
    bool                             shake_flag  : 1 {false};
    bool                             tilt35_flag : 1 {false};
    bool                             fifo_flag   : 1 {false};
    std::uint8_t                     reserved    : 1 {0};
    bool                             new_data    : 1 {false};
};

static_assert(sizeof(mc3479_status_reg) == 1, "reg size must be 1");

// -----------------------------------------------------------------------------

enum class mc3479_lpf : uint8_t {
    reserve    = 0,
    IDRdev4255 = 1,
    IDRdev6    = 2,
    IDRdev12   = 3,
    IDRdev16   = 5,
};

enum class mc3479_range : uint8_t {
    g2  = 0,
    g4  = 1,
    g8  = 2,
    g16 = 3,
    g12 = 4,
};

struct mc3479_range_and_scale_control_reg {
    static constexpr mc3479_reg_type addr{0x20};

    mc3479_lpf                       lpf        : 3 {mc3479_lpf::reserve};
    bool                             lpf_enable : 1 {false};
    mc3479_range                     range      : 3 {mc3479_range::g2};
    std::uint8_t                     reserve    : 1 {0};
};

static_assert(sizeof(mc3479_range_and_scale_control_reg) == 1,
              "reg size must be 1");

// -----------------------------------------------------------------------------

struct mc3479_regs_setup {
    stv::mc3479_mode_reg                    mode;
    stv::mc3479_interrupt_enable_reg        interrupt_enable;
    stv::mc3479_sample_rate_reg             sample_rate;
    stv::mc3479_motion_control_reg          motion_control;
    stv::mc3479_range_and_scale_control_reg range_and_scale_control;
};

} // namespace stv

#endif /* MC3479_REGS_HPP */
