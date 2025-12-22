/// @file mutex_.hpp
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

#ifndef MUTEX_HPP
#define MUTEX_HPP

#include "etl/mutex.h"
#include "stv/concepts.hpp"
#include "stv/type_traits.hpp"
#include "stv/utils.hpp"

namespace stv {

/// @brief Класс пустого мьютекса. Используется в качестве заглушки когда нет
/// необходимости избегать состояния гонки данных.
class empty_mutex
{
  public:
    auto lock() noexcept -> void {}

    auto unlock() noexcept -> void {}
};

/// ############################################################################
/// TMutex Guard
/// ############################################################################

/// @brief Объявление для дальнейшей специализации.
template<typename TMutex>
class lock_guard;

/// @brief Специализация RAII класса для использования совместно с
/// std::mutex/std::recursive_mutex.
///
/// TMutex Тип передаваемого в конструктор мьютекса.
template<stv::is_mutex_concept TMutex>
class lock_guard<TMutex> final: public etl::lock_guard<TMutex>
{
  public:
    /// @brief Захватывает мьютекс в конструкторе и освобождает в деструкторе.
    ///
    /// @param[in] mutex Ссылка на мьютекс для управления.
    /// @param[in] is_isr не оказывает никакого эффекта т.к. стандартные
    /// мьютексы не поддерживают вызов из контекста прерывания.
    explicit lock_guard(
        TMutex &mutex, bool is_isr = false):
        etl::lock_guard<TMutex>{mutex}
    {
        (void)is_isr;
    }
};

/// @brief Специализация RAII класса для использования совместно с мьютексами,
/// которые поддерживают вызов из контекста прерывания.
///
/// @tparam TMutex Тип передаваемого в конструктор мьютекса.
template<stv::is_mutex_with_isr_concept TMutex>
class lock_guard<TMutex> final:
    private stv::non_copyable,
    private stv::non_movable
{
    TMutex    &mutex_;
    const bool is_isr_;

  public:
    /// @brief Захватывает мьютекс в конструкторе и освобождает в деструкторе.
    ///
    /// @param[in] mutex Ссылка на мьютекс для управления.
    /// @param[in] is_isr true если lock_guard{} вызывается из контекста
    /// прерывания, false в противном случае.
    explicit lock_guard(
        TMutex &mutex, bool is_isr = false):
        mutex_(mutex),
        is_isr_{is_isr}
    {
        mutex_.lock(is_isr_);
    }

    ~lock_guard() { mutex_.unlock(is_isr_); }
};

template<typename TMutex>
lock_guard(TMutex &) -> lock_guard<TMutex>;

template<typename TMutex>
lock_guard(TMutex &, bool) -> lock_guard<TMutex>;

//------------------------------------------------------------------------------

} // namespace stv

#endif /* MUTEX_HPP */
