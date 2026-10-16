/// @file test_utils.cpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.

#include <array>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <cstddef>
#include <type_traits>

#include "stv/utils.hpp"

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

TEST_CASE(
    "stv::Map from 'input' range to 'out' range", "[stv][utils]")
{
    SECTION("If double")
    {
        using in_type_t = double;
        constexpr double abs{0.001};
        using out_type_t = in_type_t;

        SECTION("Symmetric ranges")
        {
            constexpr in_type_t  in_min{-1.0};
            constexpr in_type_t  in_max{1.0};

            constexpr out_type_t out_min{-2.0};
            constexpr out_type_t out_max{2.0};

            SECTION("If input zero")
            {
                constexpr in_type_t input{0.0};
                const auto          out =
                    ::stv::map(input, in_min, in_max, out_min, out_max);
                REQUIRE_THAT(0.0, Catch::Matchers::WithinRel(out, abs));

                static_assert(std::is_same_v<std::remove_const_t<decltype(out)>,
                                             out_type_t>,
                              "Compile must deduced return type as out_type_t");
            }

            SECTION("If input max")
            {
                constexpr in_type_t input{in_max};
                const auto          out =
                    ::stv::map(input, in_min, in_max, out_min, out_max);
                REQUIRE_THAT(out_max, Catch::Matchers::WithinRel(out, abs));

                static_assert(std::is_same_v<std::remove_const_t<decltype(out)>,
                                             out_type_t>,
                              "Compile must deduced return type as out_type_t");
            }

            SECTION("If input min")
            {
                constexpr in_type_t input{in_min};
                const auto          out =
                    ::stv::map(input, in_min, in_max, out_min, out_max);
                REQUIRE_THAT(out_min, Catch::Matchers::WithinRel(out, abs));

                static_assert(std::is_same_v<std::remove_const_t<decltype(out)>,
                                             out_type_t>,
                              "Compile must deduced return type as out_type_t");
            }

            SECTION("If input half of max")
            {
                constexpr in_type_t input{in_max * 0.5};
                const auto          out =
                    ::stv::map(input, in_min, in_max, out_min, out_max);
                REQUIRE_THAT(out_max * 0.5,
                             Catch::Matchers::WithinRel(out, abs));

                static_assert(std::is_same_v<std::remove_const_t<decltype(out)>,
                                             out_type_t>,
                              "Compile must deduced return type as out_type_t");
            }
        }

        SECTION("Asymmetric ranges")
        {
            constexpr in_type_t  in_min{-1.0};
            constexpr in_type_t  in_max{1.0};

            constexpr out_type_t out_min{0.0};
            constexpr out_type_t out_max{10.0};

            SECTION("If input middle")
            {
                constexpr in_type_t input{(in_min + in_max) * 0.5};
                const auto          out =
                    ::stv::map(input, in_min, in_max, out_min, out_max);
                REQUIRE_THAT((out_min + out_max) * 0.5,
                             Catch::Matchers::WithinRel(out, abs));

                static_assert(std::is_same_v<std::remove_const_t<decltype(out)>,
                                             out_type_t>,
                              "Compile must deduced return type as out_type_t");
            }

            SECTION("If input max")
            {
                constexpr in_type_t input{in_max};
                const auto          out =
                    ::stv::map(input, in_min, in_max, out_min, out_max);
                REQUIRE_THAT(out_max, Catch::Matchers::WithinRel(out, abs));

                static_assert(std::is_same_v<std::remove_const_t<decltype(out)>,
                                             out_type_t>,
                              "Compile must deduced return type as out_type_t");
            }

            SECTION("If input min")
            {
                constexpr in_type_t input{in_min};
                const auto          out =
                    ::stv::map(input, in_min, in_max, out_min, out_max);
                REQUIRE_THAT(out_min, Catch::Matchers::WithinRel(out, abs));

                static_assert(std::is_same_v<std::remove_const_t<decltype(out)>,
                                             out_type_t>,
                              "Compile must deduced return type as out_type_t");
            }
        }
    }

    SECTION("if input range double, output range int")
    {
        using in_type_t  = double;
        using out_type_t = int;
        SECTION("Symmetric ranges")
        {
            constexpr in_type_t  in_min{-1.0};
            constexpr in_type_t  in_max{1.0};

            constexpr out_type_t out_min{-10};
            constexpr out_type_t out_max{10};

            SECTION("If input middle")
            {
                constexpr auto input{(in_min + in_max) * 0.5};
                const auto     out =
                    ::stv::map(input, in_min, in_max, out_min, out_max);
                REQUIRE_THAT(((out_min + out_max) * 0.5),
                             Catch::Matchers::WithinAbs(out, 1e-6));

                static_assert(std::is_same_v<std::remove_const_t<decltype(out)>,
                                             out_type_t>,
                              "Compile must deduced return type as out_type_t");
            }

            SECTION("If input max")
            {
                constexpr auto input{in_max};
                const auto     out =
                    ::stv::map(input, in_min, in_max, out_min, out_max);
                REQUIRE(out_max == out);

                static_assert(std::is_same_v<std::remove_const_t<decltype(out)>,
                                             out_type_t>,
                              "Compile must deduced return type as out_type_t");
            }
        }

        SECTION("Asymmetric ranges")
        {
            constexpr in_type_t  in_min{-1.0};
            constexpr in_type_t  in_max{1.0};

            constexpr out_type_t out_min{0};
            constexpr out_type_t out_max{10};

            SECTION("If input middle")
            {
                constexpr auto input{(in_min + in_max) * 0.5};
                const auto     out =
                    ::stv::map(input, in_min, in_max, out_min, out_max);
                REQUIRE_THAT(((out_min + out_max) * 0.5),
                             Catch::Matchers::WithinAbs(out, 1e-6));

                static_assert(std::is_same_v<std::remove_const_t<decltype(out)>,
                                             out_type_t>,
                              "Compile must deduced return type as out_type_t");
            }

            SECTION("If input max")
            {
                constexpr auto input{in_max};
                const auto     out =
                    ::stv::map(input, in_min, in_max, out_min, out_max);
                REQUIRE(out_max == out);

                static_assert(std::is_same_v<std::remove_const_t<decltype(out)>,
                                             out_type_t>,
                              "Compile must deduced return type as out_type_t");
            }
        }
    }

    SECTION("Convert normalized control input dshot")
    {
        using in_type_t = float;
        constexpr in_type_t in_min{0.0};
        constexpr in_type_t in_max{1.0};

        using out_type_t = std::size_t;
        constexpr out_type_t out_min{48};
        constexpr out_type_t out_max{2047};

        SECTION("If middle")
        {
            constexpr in_type_t input{(in_min + in_max) * 0.5F};

            const out_type_t    out =
                ::stv::map(input, in_min, in_max, out_min, out_max);
            REQUIRE(((out_min + out_max) / 2) == out);
        }

        SECTION("If min")
        {
            constexpr in_type_t input{in_min};

            const out_type_t    out =
                ::stv::map(input, in_min, in_max, out_min, out_max);
            REQUIRE(out_min == out);
        }

        SECTION("If little more than min")
        {
            constexpr in_type_t input{0.2F};

            const out_type_t    out =
                ::stv::map(input, in_min, in_max, out_min, out_max);
            REQUIRE(out > out_min);
        }

        SECTION("If max")
        {
            constexpr in_type_t input{in_max};

            const out_type_t    out =
                ::stv::map(input, in_min, in_max, out_min, out_max);
            REQUIRE(out_max == out);
        }
    }
}
