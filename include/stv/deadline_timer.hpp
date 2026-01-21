/// @file kraslibs_deadline_timer.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// @copyright (c) 2026 Gagaring
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

#ifndef DEADLINE_TIMER_HPP
#define DEADLINE_TIMER_HPP

#include "runtime.hpp"
#include "stv/mutex_guard.hpp"
#include "stv/utils.hpp"
#include <type_traits>

namespace stv {

template<typename TRuntime, typename TMutexOrPtr = stv::empty_mutex>
class deadline_timer_setup
{
    // Определяем базовый тип мьютекса.
    using mutex_base_type = std::remove_pointer_t<TMutexOrPtr>;

    // Если передан указатель на мьютекс, то считаем что пользователь хочет
    // использовать внешний мьютекс.
    static constexpr bool is_external_mutex = std::is_pointer_v<TMutexOrPtr>;

    // Тип для хранения мьютекса. Либо указатель на мьютекс, либо пустой тип.
    using mutex_condition_type =
        std::conditional_t<is_external_mutex, mutex_base_type *,
                           std::monostate>;

  public:
    using runtime_type = TRuntime;
    using counter_type = TRuntime::value_type;
    using mutex_type   = TMutexOrPtr;

    /// @brief Указатель на runtime таймер.
    TRuntime            *runtime{nullptr};

    mutex_condition_type mutex{};
};

template<typename TSetup>
class deadline_timer: public stv::non_copyable, stv::non_movable
{
    using setup_type   = TSetup;
    using runtime_type = TSetup::runtime_type;
    using counter_type = TSetup::counter_type;
    using mutex_type   = typename TSetup::mutex_type;

    /// @brief Указатель на runtime таймер.
    runtime_type      *runtime_;
    counter_type       deadline_{counter_type{0}};
    bool               is_started_{false};
    mutable mutex_type mutex_;

    auto               start() -> void { is_started_ = true; }

    /// @brief Возвращает статус deadline таймера: активен или нет
    ///
    /// @return true - если таймер активен и deadline еще не истек, false в
    /// противном случае.
    [[nodiscard]] auto is_started() const { return is_started_; }

    auto get_mutex_ref() const -> std::remove_pointer_t<mutex_type> &
    {
        if constexpr(std::is_pointer_v<decltype(mutex_)>)
        {
            assert(mutex_);
            return *mutex_;
        }
        else
        {
            return mutex_;
        }
    }

  public:
    explicit deadline_timer(
        const setup_type &setup):
        runtime_(setup.runtime)
    {
        if constexpr(std::is_pointer_v<decltype(mutex_)>)
        {
            mutex_ = setup.mutex;
        }
    }

    virtual ~deadline_timer() = default;

    explicit operator bool() const { return stv::all_true(runtime_); }

    /// @brief Проверяет, истек ли указанный при вызове set_delay() период
    /// времени.
    ///
    /// @note Если set_delay() не был указан, то метод всегда вернет true.
    ///
    /// @return true если истек указанный при вызове set_delay() период времени.
    [[nodiscard]] auto is_elapsed()
    {
        const auto lock = stv::lock_guard{get_mutex_ref()};
        if(is_started())
        {
            const auto current_time = runtime_->get();
            const auto is_deadline_elapsed{
                static_cast<bool>(current_time >= deadline_)};

            if(is_deadline_elapsed)
            {
                this->stop();
            }
            return is_deadline_elapsed;
        }
        return true;
    }

    /// @brief Устанавливает задержку срабатывания и запускает таймер.
    ///
    /// @param[in] delay Задержка относительно момента вызова по истечении
    /// которой метод is_elapsed() вернет true.
    auto set_delay(
        const counter_type &delay) -> void
    {
        if(delay.count()
           != static_cast<std::remove_cvref_t<decltype(delay)>::rep>(0))
        {
            const auto lock         = stv::lock_guard{get_mutex_ref()};
            const auto current_time = runtime_->get();
            deadline_               = current_time + delay;
            this->start();
        }
    }

    /// @brief Принудительно останавливает deadline таймер.
    ///
    /// @note После остановки таймера метод is_elapsed() вернет true.
    auto stop() -> void
    {
        const auto lock = stv::lock_guard{get_mutex_ref()};
        is_started_     = false;
        deadline_       = counter_type{0};
    }
};

} // namespace stv

#endif /* DEADLINE_TIMER_HPP */
