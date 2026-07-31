/// @file while_timeout.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
///
/// NAME
///     stv::while_timeout
///
/// DESCRIPTION
///     stv::while_timeout реализует блокирующее ожидание выполнения
///     условия с ограничением по времени. Условие опрашивается в цикле,
///     пока не вернет true либо пока не истечет указанный таймаут.
///
///     Метод is_elapsed() позволяет различить два исхода ожидания:
///     - false: условие выполнилось до истечения таймаута;
///     - true: условие не выполнилось, ожидание прервано по таймауту.
///
/// EXAMPLE
///     Пример использования:
///     ```cpp
///     #include <stv/while_timeout.hpp>
///
///     // Ожидание готовности устройства не дольше 200 мс.
///     stv::while_timeout wait{runtime, 200ms,
///                             [&device]() { return device.is_ready(); }};
///     if (wait.is_elapsed()) {
///         // Устройство не ответило за отведенное время.
///     }
///     ```
///
///     См. test_while_timeout.cpp для автоматических тестов.

#ifndef WHILE_TIMEOUT_HPP
#define WHILE_TIMEOUT_HPP

#include "stv/deadline_timer.hpp"
#include "stv/runtime.hpp"
#include <concepts>

namespace stv {

template<typename TRuntime, typename TMutexOrPtr = stv::empty_mutex>
class while_timeout
{
    using runtime_type = TRuntime;
    using deadline_setup_type =
        stv::deadline_timer_setup<runtime_type, TMutexOrPtr>;
    using deadline_type = stv::deadline_timer<deadline_setup_type>;

    deadline_type deadline_;

  public:
    /// @brief Ожидает выполнения условия не дольше указанного таймаута.
    ///
    /// @param[in] runtime Источник монотонного времени.
    /// @param[in] timeout Максимальное время ожидания.
    /// @param[in] condition Опрашиваемое условие. Ожидание завершается,
    /// когда condition() возвращает true.
    template<typename Fn>
        requires std::predicate<Fn>
    while_timeout(
        runtime_type &runtime, typename runtime_type::value_type timeout,
        Fn condition):
        deadline_{deadline_setup_type{.runtime                   = &runtime,
                                      .delay                     = timeout,
                                      .is_elapsed_if_not_started = true}}
    {
        while(!condition())
        {
            if(deadline_.is_elapsed())
            {
                break;
            }
        }
    }

    /// @brief Проверяет, истек ли таймаут ожидания.
    ///
    /// @return true если ожидание прервано по таймауту, false если условие
    /// выполнилось.
    [[nodiscard]] auto is_elapsed() const { return deadline_.is_elapsed(); }
};

} // namespace stv

#endif /* WHILE_TIMEOUT_HPP */
