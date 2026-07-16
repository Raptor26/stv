/// @file critical.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.

///
/// NAME
///     critical
///
/// DESCRIPTION
///     critical — это легковесная реализация вложенной критической секции.
///     Все экземпляры класса разделяют один глобальный атомарный счетчик
///     вложенности, что делает его подходящим для маскирования прерываний
///     и других сценариев, где необходимо отслеживать вложенные блокировки.
///
///     Основные возможности:
///     - Глобальный атомарный счетчик вложенности, общий для всех экземпляров
///     - Функции блокировки/разблокировки вызываются только при входе
///       во внешнюю секцию и выходе из нее
///     - Совместимость со стандартными обертками блокировок (например,
///       std::scoped_lock)
///     - Конфигурация порядка памяти через constexpr
///
///     Параметры шаблона TLock и TUnlock — это вызываемые сущности
///     (функции или указатели на функции), которые вызываются при входе
///     во внешнюю критическую секцию и при выходе из нее соответственно.
///
/// EXAMPLE
///     Базовое использование со std::scoped_lock:
///     ```cpp
///     #include "stv/critical.hpp"
///     #include <mutex>
///
///     void disable_irq()  { /* маскирование прерываний */ }
///     void enable_irq()   { /* размаскирование прерываний */ }
///
///     int main() {
///         stv::critical<disable_irq, enable_irq> cs;
///
///         {
///             std::scoped_lock lock{cs};
///             // прерывания замаскированы
///         }
///         // прерывания размаскированы
///
///         return 0;
///     }
///     ```
///
///     Вложенные критические секции:
///     ```cpp
///     stv::critical<disable_irq, enable_irq> cs1;
///     stv::critical<disable_irq, enable_irq> cs2;
///
///     {
///         std::scoped_lock lock1{cs1};   // вызван disable_irq()
///         {
///             std::scoped_lock lock2{cs2}; // счетчик вложенности == 2
///             // по-прежнему внутри критической секции
///         }                               // счетчик вложенности == 1
///     }                                   // вызван enable_irq()
///     ```
///
///     Использование указателей на функции:
///     ```cpp
///     void lock()   { /* пользовательская блокировка */ }
///     void unlock() { /* пользовательская разблокировка */ }
///
///     stv::critical<&lock, &unlock> cs;
///     std::scoped_lock guard{cs};
///     ```
///
///     См. тестовые случаи в test_critical.cpp для подробных примеров
///     и тестов валидации.

#ifndef CRITICAL_HPP
#define CRITICAL_HPP

#include "utils.hpp"
#include <atomic>
#include <cassert>
#include <concepts>
#include <functional>

namespace stv {

/// @brief Класс критической секции. Предназначен для монопольной блокировки
/// прерываний, поэтому содержит один глобальный счетчик на множество
/// экземпляров.
template<auto TLock, auto TUnlock>
    requires std::invocable<decltype(TLock)>
             && std::invocable<decltype(TUnlock)>
class critical final: stv::non_movable_non_copyable
{
    /// @brief Сквозной счетчик критической секции. Необходим для поддержания
    /// вложенности вызовов входа/выхода.
    inline static std::atomic<std::size_t> nesting_cnt{0};

    /// @brief Порядок памяти, используемый для атомарных операций со
    /// счетчиком вложенности.
    static constexpr std::memory_order expected_memory_order =
        std::memory_order_acq_rel;

  public:
    /// @brief Входит в критическую секцию.
    ///
    /// @note Увеличивает глобальный счетчик вложенности. Если счетчик был
    /// равен нулю до входа, вызывает TLock.
    static void lock() noexcept
    {
        if(nesting_cnt.fetch_add(1, expected_memory_order)
           == static_cast<std::size_t>(0))
        {
            std::invoke(TLock);
        }
    }

    /// @brief Выходит из критической секции.
    ///
    /// @note Уменьшает глобальный счетчик вложенности. Если счетчик стал
    /// равен нулю после выхода, вызывает TUnlock.
    static void unlock() noexcept
    {
#ifndef NDEBUG

        assert(nesting_cnt.load(std::memory_order_relaxed) > 0
               && "unlock() called without matching lock()");
#endif

        // Выполним сравнение с 1 т.к. fetch_sub() возвращает результат
        // предшествующий выполнению операции.
        if(nesting_cnt.fetch_sub(1, expected_memory_order)
           == static_cast<std::size_t>(1))
        {
            std::invoke(TUnlock);
        }
    }

    /// @brief Возвращает глобальное значение счетчика вложенности.
    /// @note Только для отладки и тестов.
    ///
    /// @return Значение глобального счетчика вложенности.
    [[nodiscard]] auto get_glob_nesting_cnt() const
    {
        return static_cast<std::size_t>(
            nesting_cnt.load(std::memory_order_relaxed));
    }
};

} // namespace stv

#endif /* CRITICAL_HPP */
