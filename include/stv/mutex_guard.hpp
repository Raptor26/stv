/// @file mutex_guard.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
///
/// NAME
///     stv::mutex_guard
///
/// DESCRIPTION
///     stv::mutex_guard предоставляет RAII обертку для захвата и
///     автоматического освобождения мьютекса. Библиотека включает:
///     - empty_mutex: пустая заглушка мьютекса, используется когда
///       синхронизация не требуется.
///     - lock_guard<TMutex>: шаблонный RAII guard, автоматически
///       захватывает мьютекс в конструкторе и освобождает в деструкторе.
///
///     Специализации lock_guard:
///     - Для стандартных мьютексов (std::mutex, std::recursive_mutex и
///       других, удовлетворяющих is_mutex_concept): наследуется от
///       std::lock_guard<TMutex>. Параметр is_isr не влияет на работу,
///       так как стандартные мьютексы не поддерживают вызовы из контекста
///       прерывания.
///     - Для ISR-совместимых мьютексов (удовлетворяющих
///       is_mutex_with_isr_concept): реализует собственный RAII guard,
///       сохраняет флаг is_isr и передает его в lock()/unlock().
///
///     При передаче is_isr=true для ISR-совместимого мьютекса вызовы
///     lock() и unlock() выполняются с учетом контекста прерывания.
///
///     Объекты lock_guard нельзя копировать и перемещать.
///
/// EXAMPLE
///     Пример с пустым мьютексом:
///     ```cpp
///     #include <stv/mutex_guard.hpp>
///
///     int main() {
///         stv::empty_mutex mutex;
///         stv::lock_guard guard{mutex};
///         // Критическая секция без реальной блокировки.
///         return 0;
///     }
///     ```
///
///     Пример с std::mutex:
///     ```cpp
///     #include <stv/mutex_guard.hpp>
///     #include <iostream>
///     #include <mutex>
///
///     int main() {
///         std::mutex mutex;
///         int counter = 0;
///
///         {
///             stv::lock_guard guard{mutex};
///             ++counter;
///             std::cout << "counter: " << counter << "\n";
///         }
///
///         return 0;
///     }
///     ```
///
///     Пример с ISR-совместимым мьютексом:
///     ```cpp
///     #include <stv/mutex_guard.hpp>
///
///     class isr_mutex {
///       public:
///         void lock(bool is_isr) {
///             if (is_isr) {
///                 // Блокировка из контекста прерывания.
///             } else {
///                 // Обычная блокировка.
///             }
///         }
///
///         void unlock(bool is_isr) {
///             if (is_isr) {
///                 // Разблокировка из контекста прерывания.
///             } else {
///                 // Обычная разблокировка.
///             }
///         }
///     };
///
///     int main() {
///         isr_mutex mutex;
///         const stv::lock_guard guard{mutex, false};
///         // Критическая секция.
///         return 0;
///     }
///     ```
///
///     См. test_mutex.cpp для автоматических тестов.

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
