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
    using counter_type = typename TRuntime::value_type;
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
class deadline_timer: public stv::non_movable_non_copyable
{
    using setup_type = TSetup;

  public:
    using runtime_type = typename setup_type::runtime_type;
    using counter_type = typename setup_type::counter_type;

  private:
    using mutex_type = typename setup_type::mutex_type;

    /// @brief Указатель на runtime таймер.
    runtime_type *runtime_;
    counter_type  start_time_{counter_type{0}};
    counter_type  delay_{counter_type{0}};
    bool          is_started_{false};
    mutable bool  is_deadline_elapsed_{false};

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

    virtual ~deadline_timer() { stop(); }

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

        // Если уже истек указанный период времени, то повторно проверка не
        // выполняется. Это позволяет всегда возвращать корректный статус
        // таймера если он не запущен."
        if(is_started() && !is_deadline_elapsed_)
        {
            const auto now = runtime_->get();
            is_deadline_elapsed_ =
                static_cast<bool>((now - start_time_) >= delay_);
        }
        else if(is_elapsed_if_not_started_)
        {
            is_deadline_elapsed_ = true;
        }

        return is_deadline_elapsed_;
    }

    /// @brief Проверяет не возникнет ли переполнения счетчика при установке
    /// указанного timeout.
    ///
    /// @param[in] delay Задержка, которую планируется использовать в
    /// set_delay()
    ///
    /// @return true если переполнение не обнаружено, false в противном случае.
    static constexpr bool can_set_delay(
        const auto &delay)
    {
        using deadline_type        = counter_type;
        using deadline_rep_type    = typename deadline_type::rep;
        using deadline_period_type = typename deadline_type::period;
        using compare_counter_type = std::uint64_t;

        constexpr auto max_deadline_val =
            std::numeric_limits<deadline_rep_type>::max();
        constexpr auto max_compare_val =
            std::numeric_limits<compare_counter_type>::max();

        if constexpr(max_deadline_val < max_compare_val)
        {
            const auto timeout_with_compare_type =
                std::chrono::duration<compare_counter_type,
                                      deadline_period_type>{delay};

            return timeout_with_compare_type.count()
                   < static_cast<compare_counter_type>(max_deadline_val);
        }

        return true;
    }

    /// @brief Устанавливает задержку срабатывания и запускает таймер.
    ///
    /// @param[in] delay Задержка относительно момента вызова по
    /// истечении которой метод is_elapsed() вернет true.
    auto set_delay(
        const auto &delay)
    {
        auto is_set_delay{false};

        if(can_set_delay(delay))
        {
            if(delay.count()
               != static_cast<std::remove_cvref_t<decltype(delay)>::rep>(0))
            {
                const auto lock = stv::lock_guard{get_mutex_ref()};
                start_time_     = runtime_->get();
                delay_          = delay;
                this->start();
                is_set_delay         = true;
                is_deadline_elapsed_ = false;
            }
        }

        return is_set_delay;
    }

    /// @brief Принудительно останавливает deadline таймер.
    ///
    /// @note После остановки таймера метод is_elapsed() вернет true.
    auto stop() -> void
    {
        const auto lock      = stv::lock_guard{get_mutex_ref()};
        is_started_          = false;
        delay_               = counter_type{0};
        start_time_          = counter_type{0};
        is_deadline_elapsed_ = is_elapsed_if_not_started_;
    }

    /// @brief Возвращает время, прошедшее с момента запуска таймера.
    ///
    /// @return Время с момента вызова set_delay(). Если таймер не запущен,
    /// вернет 0.
    [[nodiscard]] auto get_time_after_start() const -> counter_type
    {
        const auto lock = stv::lock_guard{get_mutex_ref()};
        if (!is_started_)
        {
            return counter_type{0};
        }
        return runtime_->get() - start_time_;
    }

    /// @brief Возвращает оставшееся время до наступления deadline.
    ///
    /// @return Количество времени перед наступлением deadline. Если таймер не
    /// запущен или deadline уже истек, вернет 0.
    [[nodiscard]] auto get_time_before_deadline() const -> counter_type
    {
        const auto lock = stv::lock_guard{get_mutex_ref()};
        if (!is_started_ || is_deadline_elapsed_)
        {
            return counter_type{0};
        }
        const auto now     = runtime_->get();
        const auto elapsed = now - start_time_;
        if (elapsed >= delay_)
        {
            return counter_type{0};
        }
        return delay_ - elapsed;
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
