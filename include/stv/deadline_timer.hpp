/// @file deadline_timer.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.

#ifndef DEADLINE_TIMER_HPP
#define DEADLINE_TIMER_HPP

#include "runtime.hpp"
#include "stv/mutex_guard.hpp"
#include "stv/utils.hpp"
#include "utils.hpp"
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
    TRuntime *runtime{nullptr};

    /// @brief Задержка deadline таймера. Если не равна 0, то таймер начинает
    /// отсчет сразу после создания.
    counter_type delay{0};

    /// @brief Если флаг true, то метод deadline_timer::is_elapsed() возвращает
    /// true если:
    /// - таймер не запущен;
    /// - таймер был запущен ранее, но уже истек указанный период.
    bool                 is_elapsed_if_not_started{true};

    mutex_condition_type mutex{};
};

template<typename TSetup>
class deadline_timer: virtual public stv::non_movable_non_copyable
{
    using setup_type = TSetup;

  public:
    using runtime_type = typename setup_type::runtime_type;
    using counter_type = typename setup_type::counter_type;

  private:
    using mutex_type = typename setup_type::mutex_type;

    /// @brief Указатель на runtime таймер.
    runtime_type *runtime_;
    counter_type  deadline_{counter_type{0}};
    bool          is_started_{false};

    /// @brief Если флаг true, то метод deadline_timer::is_elapsed() возвращает
    /// true если:
    /// - таймер не запущен;
    /// - таймер был запущен ранее, но уже истек указанный период.
    bool               is_elapsed_if_not_started_;

    mutable mutex_type mutex_;

    auto               start() -> void { is_started_ = true; }

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
        runtime_(setup.runtime),
        is_elapsed_if_not_started_{setup.is_elapsed_if_not_started}
    {
        if constexpr(std::is_pointer_v<decltype(mutex_)>)
        {
            mutex_ = setup.mutex;
        }

        set_delay(setup.delay);
    }

    virtual ~deadline_timer() = default;

    explicit operator bool() const { return stv::all_true(runtime_); }

    /// @brief Проверяет, истек ли указанный при вызове set_delay() период
    /// времени.
    ///
    /// @note Если set_delay() не был указан, то метод всегда вернет true.
    ///
    /// @return true если истек указанный при вызове set_delay() период времени.
    [[nodiscard]] auto is_elapsed() const
    {
        const auto lock = stv::lock_guard{get_mutex_ref()};
        if(is_started())
        {
            const auto current_time = runtime_->get();
            const auto is_deadline_elapsed{
                static_cast<bool>(current_time >= deadline_)};
            return is_deadline_elapsed;
        }

        if(is_elapsed_if_not_started_)
        {
            return true;
        }

        return false;
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

    /// @brief Возвращает статус deadline таймера: активен или нет
    ///
    /// @return true - если таймер активен и deadline еще не истек, false в
    /// противном случае.
    [[nodiscard]] auto is_started() const
    {
        const auto lock = stv::lock_guard{get_mutex_ref()};
        return is_started_;
    }
};

} // namespace stv

#endif /* DEADLINE_TIMER_HPP */
