/// @file mutex_guard.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.

#ifndef MUTEX_HPP
#define MUTEX_HPP

#include "stv/concepts.hpp"
#include "stv/type_traits.hpp"
#include "stv/utils.hpp"
#include <mutex>

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
class lock_guard<TMutex> final: public std::lock_guard<TMutex>
{
  public:
    /// @brief Захватывает мьютекс в конструкторе и освобождает в деструкторе.
    ///
    /// @param[in] mutex Ссылка на мьютекс для управления.
    /// @param[in] is_isr не оказывает никакого эффекта т.к. стандартные
    /// мьютексы не поддерживают вызов из контекста прерывания.
    explicit lock_guard(
        TMutex &mutex, bool is_isr = false):
        std::lock_guard<TMutex>{mutex}
    { (void)is_isr; }
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
    { mutex_.lock(is_isr_); }

    ~lock_guard() { mutex_.unlock(is_isr_); }
};

template<typename TMutex>
lock_guard(TMutex &) -> lock_guard<TMutex>;

template<typename TMutex>
lock_guard(TMutex &, bool) -> lock_guard<TMutex>;

//------------------------------------------------------------------------------

} // namespace stv

#endif /* MUTEX_HPP */
