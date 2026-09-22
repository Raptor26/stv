/// @file test_quaternion.cpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.

#include "boost/qvm/all.hpp"
#include "stv/quaternion.hpp"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>

// NOLINTBEGIN(*-magic-numbers, readability-function-cognitive-complexity)

TEST_CASE(
    "quaternion class tests", "[quaternion]")
{
    SECTION("Default constructed quaternion is zero")
    {
        const stv::quaternion<float> quat;

        REQUIRE(quat.is_zero());
        REQUIRE(quat.give_w() == Catch::Approx(0.0F));
        REQUIRE(quat.give_x() == Catch::Approx(0.0F));
        REQUIRE(quat.give_y() == Catch::Approx(0.0F));
        REQUIRE(quat.give_z() == Catch::Approx(0.0F));
    }

    SECTION("Component constructor stores values in (w, x, y, z) order")
    {
        const stv::quaternion<float> quat{1.0F, 2.0F, 3.0F, 4.0F};

        REQUIRE(quat.give_w() == Catch::Approx(1.0F));
        REQUIRE(quat.give_x() == Catch::Approx(2.0F));
        REQUIRE(quat.give_y() == Catch::Approx(3.0F));
        REQUIRE(quat.give_z() == Catch::Approx(4.0F));
        REQUIRE_FALSE(quat.is_zero());
    }

    SECTION("Array constructor stores values in (w, x, y, z) order")
    {
        const stv::quaternion<float> quat{
            std::array<float, 4>{5.0F, 6.0F, 7.0F, 8.0F}};

        REQUIRE(quat.give_w() == Catch::Approx(5.0F));
        REQUIRE(quat.give_x() == Catch::Approx(6.0F));
        REQUIRE(quat.give_y() == Catch::Approx(7.0F));
        REQUIRE(quat.give_z() == Catch::Approx(8.0F));
    }

    SECTION("operator bool is true when all components are finite")
    {
        const stv::quaternion<float> quat{1.0F, 0.0F, 0.0F, 0.0F};

        REQUIRE(quat);
    }

    SECTION("operator bool is false when a component is NaN")
    {
        const stv::quaternion<float> quat{
            std::numeric_limits<float>::quiet_NaN(), 0.0F, 0.0F, 0.0F};

        REQUIRE_FALSE(quat);
    }

    SECTION("boost::qvm writes scalar and vector parts of the quaternion")
    {
        stv::quaternion<float> quat;
        const float            target_yaw = 0.7F;

        boost::qvm::S(quat) = std::cos(target_yaw / 2.0F);
        boost::qvm::X(quat) = 0.0F;
        boost::qvm::Y(quat) = 0.0F;
        boost::qvm::Z(quat) = std::sin(target_yaw / 2.0F);

        REQUIRE(quat.give_w() == Catch::Approx(std::cos(target_yaw / 2.0F)));
        REQUIRE(quat.give_x() == Catch::Approx(0.0F));
        REQUIRE(quat.give_y() == Catch::Approx(0.0F));
        REQUIRE(quat.give_z() == Catch::Approx(std::sin(target_yaw / 2.0F)));
    }

    SECTION("boost::qvm reads scalar and vector parts of the quaternion")
    {
        const stv::quaternion<float> quat{1.0F, 2.0F, 3.0F, 4.0F};

        REQUIRE(boost::qvm::S(quat) == Catch::Approx(1.0F));
        REQUIRE(boost::qvm::X(quat) == Catch::Approx(2.0F));
        REQUIRE(boost::qvm::Y(quat) == Catch::Approx(3.0F));
        REQUIRE(boost::qvm::Z(quat) == Catch::Approx(4.0F));
    }

    SECTION("boost::qvm quaternion operations work on stv::quaternion")
    {
        const float                  target_yaw = 0.7F;
        const stv::quaternion<float> quat{std::cos(target_yaw / 2.0F), 0.0F,
                                          0.0F, std::sin(target_yaw / 2.0F)};

        REQUIRE(boost::qvm::mag(quat) == Catch::Approx(1.0F));
        REQUIRE(boost::qvm::mag(boost::qvm::normalized(quat))
                == Catch::Approx(1.0F));
    }
}

// NOLINTEND(*-magic-numbers, readability-function-cognitive-complexity)
