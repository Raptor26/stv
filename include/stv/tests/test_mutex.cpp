/// @file test_mutex.cpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.

#include <catch2/catch_test_macros.hpp>
#include <mutex>

#include "stv/mutex_guard.hpp"

TEST_CASE(
    "Mutex", "[stv][moving_average]")
{
    SECTION("Empty mutex")
    {
        stv::empty_mutex      empty_mutex;
        const stv::lock_guard guard{empty_mutex};
    }

    SECTION("std::mutex")
    {
        std::mutex            mutex;
        const stv::lock_guard guard{mutex};
    }

    SECTION("Empty mutex with isr")
    {
        class empty_mutex_isr
        {
          public:
            static auto lock(
                bool is_isr = false) noexcept -> void
            { (void)is_isr; }

            static auto unlock(
                bool is_isr = false) noexcept -> void
            { (void)is_isr; }
        };

        empty_mutex_isr       empty_mutex;
        const stv::lock_guard guard{empty_mutex, false};
    }
}
