/// @file test_math.cpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.

#include "stv/math.hpp"
#include "stv/quaternion.hpp"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <numbers>

// NOLINTBEGIN(*-magic-numbers, readability-function-cognitive-complexity)

TEST_CASE(
    "math to_euler tests", "[math]")
{
    SECTION("Identity quaternion gives zero Euler angles")
    {
        const stv::quaternion<float> quat{1.0F, 0.0F, 0.0F, 0.0F};

        REQUIRE(stv::to_euler_roll(quat) == Catch::Approx(0.0F));
        REQUIRE(stv::to_euler_pitch(quat) == Catch::Approx(0.0F));
        REQUIRE(stv::to_euler_yaw(quat) == Catch::Approx(0.0F));
    }

    SECTION("Rotation around Z axis gives yaw only")
    {
        const float                  target_yaw = 0.7F;
        const stv::quaternion<float> quat{std::cos(target_yaw / 2.0F), 0.0F,
                                          0.0F, std::sin(target_yaw / 2.0F)};

        REQUIRE(stv::to_euler_roll(quat) == Catch::Approx(0.0F).margin(1.0e-6F));
        REQUIRE(stv::to_euler_pitch(quat)
                == Catch::Approx(0.0F).margin(1.0e-6F));
        REQUIRE(stv::to_euler_yaw(quat) == Catch::Approx(target_yaw));
    }

    SECTION("Rotation around X axis gives roll only")
    {
        const float                  target_roll = -0.4F;
        const stv::quaternion<float> quat{std::cos(target_roll / 2.0F),
                                          std::sin(target_roll / 2.0F), 0.0F,
                                          0.0F};

        REQUIRE(stv::to_euler_roll(quat) == Catch::Approx(target_roll));
        REQUIRE(stv::to_euler_pitch(quat)
                == Catch::Approx(0.0F).margin(1.0e-6F));
        REQUIRE(stv::to_euler_yaw(quat) == Catch::Approx(0.0F).margin(1.0e-6F));
    }

    SECTION("Rotation around Y axis gives pitch only")
    {
        const float                  target_pitch = 0.5F;
        const stv::quaternion<float> quat{std::cos(target_pitch / 2.0F), 0.0F,
                                          std::sin(target_pitch / 2.0F), 0.0F};

        REQUIRE(stv::to_euler_roll(quat) == Catch::Approx(0.0F).margin(1.0e-6F));
        REQUIRE(stv::to_euler_pitch(quat) == Catch::Approx(target_pitch));
        REQUIRE(stv::to_euler_yaw(quat) == Catch::Approx(0.0F).margin(1.0e-6F));
    }

    SECTION("to_euler returns {roll, pitch, yaw} array")
    {
        const float                  target_yaw = -1.1F;
        const stv::quaternion<float> quat{std::cos(target_yaw / 2.0F), 0.0F,
                                          0.0F, std::sin(target_yaw / 2.0F)};

        const auto euler = stv::to_euler(quat);

        REQUIRE(euler.size() == 3U);
        REQUIRE(euler[0] == Catch::Approx(0.0F).margin(1.0e-6F));
        REQUIRE(euler[1] == Catch::Approx(0.0F).margin(1.0e-6F));
        REQUIRE(euler[2] == Catch::Approx(target_yaw));
    }

    SECTION("Half-turn yaw is recovered with double precision")
    {
        const double                  target_yaw = std::numbers::pi / 2.0;
        const stv::quaternion<double> quat{std::cos(target_yaw / 2.0), 0.0,
                                           0.0, std::sin(target_yaw / 2.0)};

        const auto euler = stv::to_euler(quat);

        REQUIRE(euler[0] == Catch::Approx(0.0).margin(1.0e-12));
        REQUIRE(euler[1] == Catch::Approx(0.0).margin(1.0e-12));
        REQUIRE(euler[2] == Catch::Approx(target_yaw).margin(1.0e-12));
    }
}

// NOLINTEND(*-magic-numbers, readability-function-cognitive-complexity)
