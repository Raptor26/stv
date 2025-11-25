/// @file test_mutex.cpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// @copyright (c) 2025 "The Boys"
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

#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "stv/mutex_guard.hpp"
#include <mutex>

TEST_CASE(
    "Mutex", "[stv][moving_average]")
{
    SECTION("Empty mutex")
    {
        stv::EmptyMutex empty_mutex;
        const stv::lock_guard guard{empty_mutex};
    }

    SECTION("std::mutex")
    {
        std::mutex      mutex;
        const stv::lock_guard guard{mutex};
    }

    SECTION("Empty mutex with isr")
    {
        class EmptyMutexIsr
        {
          public:
            static auto lock(
                bool is_isr = false) noexcept -> void
            {
                (void)is_isr;
            }

            static auto unlock(
                bool is_isr = false) noexcept -> void
            {
                (void)is_isr;
            }
        };

        EmptyMutexIsr         empty_mutex;
        const stv::lock_guard guard{empty_mutex, false};
    }
}
