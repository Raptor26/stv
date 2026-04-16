/// @file test_utils.cpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.

#include "stv/utils.hpp"
#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

TEST_CASE(
    "all_true", "[stv][utils]")
{
    REQUIRE(stv::all_true(true, true, true) == true);
    REQUIRE_FALSE(stv::all_true(false, true, true));
    REQUIRE_FALSE(stv::all_true(false, false, true));
    REQUIRE_FALSE(stv::all_true(false, false, false));
}

TEST_CASE(
    "one_true", "[stv][utils]")
{
    REQUIRE(stv::one_true(true, true, true));
    REQUIRE(stv::one_true(false, true, true));
    REQUIRE(stv::one_true(false, false, true));
    REQUIRE_FALSE(stv::one_true(false, false, false));
}

TEST_CASE(
    "Norm")
{
    std::array<float, 3> vector{1.0F, 2.0F, 2.0F};
    constexpr float      expected_norm{3.F};

    float                norm{0};

    SECTION("Like container") { norm = stv::norm(vector); }

    SECTION("Like scalar")
    { norm = stv::norm(vector.at(0), vector.at(1), vector.at(2)); }
    REQUIRE_THAT(norm, Catch::Matchers::WithinRel(
                           static_cast<double>(expected_norm), 0.01));
}

TEST_CASE(
    "clamp_with_result", "[stv][utils]")
{
    SECTION("With clamp")
    {
        float       val{10.0F};
        const float val_min{-1.0F};
        const float val_max{9.99F};

        REQUIRE(stv::clamp_with_result(val, val_min, val_max));
    }

    SECTION("With clamp")
    {
        float       val{-2.0F};
        const float val_min{-1.0F};
        const float val_max{9.99F};

        REQUIRE(stv::clamp_with_result(val, val_min, val_max));
    }

    SECTION("Without clamp")
    {
        float       val{10.0F};
        const float val_min{-1.0F};
        const float val_max{11.99F};

        REQUIRE_FALSE(stv::clamp_with_result(val, val_min, val_max));
    }

    SECTION("Without clamp")
    {
        float       val{10.0F};
        const float val_min{-1.0F};
        const float val_max{10.0F};

        REQUIRE_FALSE(stv::clamp_with_result(val, val_min, val_max));
    }
}
