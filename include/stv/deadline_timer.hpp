/// @file kraslibs_deadline_timer.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// @copyright (c) 2025 Gagaring
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

#ifndef KRASLIBS_DEADLINE_TIMER_HPP
#define KRASLIBS_DEADLINE_TIMER_HPP

#include "kraslibs_runtime.hpp"
#include "kraslibs_utils.hpp"
#include <limits>

namespace kraslibs {

template<std::unsigned_integral TCounter = runtime_type>
struct DeadlineTimerStructInit {
    Runtime<TCounter> *runtime_ptr{nullptr};

    /// @brief Минимальное значение timeout. Если установлено меньше указанного
    /// значения рпи вызове SetTimeoutInMs(), то IsDeadlineElapsed() всегда
    /// будет возвращать false.
    TCounter min_timeout_ms{MIN_TIMEOUT_MS_DEFAULT};

  private:
    static constexpr TCounter MIN_TIMEOUT_MS_DEFAULT{100};
};

/// @brief Класс обеспечивает проверку deadline.
/// @tparam TCounter
template<std::unsigned_integral TCounter = runtime_type>
class DeadlineTimer
{
    /// @brief Указатель на структуру времени с момента старта системы.
    Runtime<TCounter> *runtime_ptr_;

    /// @brief В данную переменную записывается время с момента старта системы
    /// при вызове SetCurrentTimeInMs().
    volatile TCounter current_time_{0};

    /// @brief Период времени относительно current_time_, при превышении
    /// которого IsDeadlineElapsed() начнет возвращать true.
    volatile TCounter deadline_ms_{0};

    volatile TCounter timeout_ms_{0};

    /// @brief Минимальное значение timeout. Если установлено меньше указанного
    /// значения при вызове SetTimeoutInMs(), то IsDeadlineElapsed() всегда
    /// будет возвращать false.
    const TCounter min_timeout_ms_{100};

    /// @brief Остаток времени до истечения deadline. Полезно при отладке.
    volatile TCounter time_before_deadline_ms_{
        std::numeric_limits<decltype(time_before_deadline_ms_)>::max()};

  public:
    explicit DeadlineTimer(
        const DeadlineTimerStructInit<TCounter> &attr):
        runtime_ptr_{attr.runtime_ptr},
        min_timeout_ms_{attr.min_timeout_ms}
    {
    }

    virtual ~DeadlineTimer() = default;

    DeadlineTimer(const DeadlineTimer &other) = default;
    DeadlineTimer(DeadlineTimer &&other)      = delete;

    auto operator=(const DeadlineTimer &other) -> DeadlineTimer & = default;
    auto operator=(DeadlineTimer &&other) -> DeadlineTimer &      = delete;

    /// @brief Возвращает True если класс готов к работе.
    explicit operator bool() const
    {
        auto is_true{true};
        if(!runtime_ptr_)
        {
            is_true = false;
        }
        return is_true;
    }

    /// @brief Метод устанавливает timeout от текущего момента (момента вызова).
    /// В дальнейшем, вы можете провеять истек ли указанный в timeout_ms период
    /// времени с помощью IsDeadlineElapsed().
    /// @note Вызов метода SetTimeoutInMs() эквивалентно запуску таймера.
    /// @param[in] timeout_ms: Период времени, по истечении которого
    /// IsDeadlineElapsed() начет возвращать true.
    /// @return true - если timeout успешно установлен, false в противном
    /// случае.
    KRASLIBS_VIRTUAL auto SetTimeoutInMs(
        TCounter timeout_ms) -> bool
    {
        auto is_timeout_set{false};
        if(timeout_ms >= min_timeout_ms_)
        {
            SetCurrentTimeInMs();

            deadline_ms_   = GiveCurrentTimeInMs() + timeout_ms;
            timeout_ms_    = timeout_ms;
            is_timeout_set = true;
        }

        return is_timeout_set;
    }

    /// @brief Метод вычисляет период времени между вызовами SetTimeoutInMs() и
    /// IsDeadlineElapsed(). Если период времени равен или превышает значение,
    /// указанное при вызове SetTimeoutInMs(), то метод вернет true, в противном
    /// случае вернет false.
    /// @return true если период между вызовами SetTimeoutInMs() и
    /// IsDeadlineElapsed() равен или превышает промежуток, указанный при вызове
    /// SetTimeoutInMs().
    KRASLIBS_VIRTUAL auto IsDeadlineElapsed() -> bool
    {
        if(deadline_ms_ == 0U)
        {
            return false;
        }

        const TCounter current_time = runtime_ptr_->GiveCurrentTimeInMs();

        time_before_deadline_ms_ = deadline_ms_ - current_time;

        // elapsed_time я является целым беззнаковым типом. Переполнение целых
        // беззнаковых чисел является определенным.
        const TCounter elapsed_time = current_time - current_time_;

        auto           is_timeout_elapsed{false};
        if(elapsed_time >= timeout_ms_)
        {
            is_timeout_elapsed       = true;
            time_before_deadline_ms_ = 0U;
        }

        return is_timeout_elapsed;
    }

    /// @brief После вызова метода, IsDeadlineElapsed() всегда будет возвращать
    /// false до тех пор пока не будет вызван SetTimeoutInMs().
    /// @note SetTimeoutInMs() должен вернуть true, это означает что период
    /// успешно установлен.
    KRASLIBS_VIRTUAL void Reset()
    {
        deadline_ms_ = 0U;
        time_before_deadline_ms_ =
            std::numeric_limits<decltype(time_before_deadline_ms_)>::max();
    }

    KRASLIBS_VIRTUAL auto IsActive() -> bool { return deadline_ms_ != 0; }

    /// @brief Возвращает промежуток времени перед наступлением deadline.
    /// @return Количество мс. перед наступлением deadline.
    KRASLIBS_VIRTUAL auto GetTimeBeforeDeadlineMs() -> TCounter
    {
        return time_before_deadline_ms_;
    }

  private:
    /// @brief Запись текущего времени с момента старта системы. Относительно
    /// этого записанного значения будет проверятся истекло ли время.
    void SetCurrentTimeInMs()
    {
        // todo добавить критическую секцию.
        current_time_ = runtime_ptr_->GiveCurrentTimeInMs();
    }

    /// @brief Возвращает значение счетчика, установленное при вызове
    /// SetCurrentTimeInMs().
    /// @return Значение счетчика.
    [[nodiscard]] auto GiveCurrentTimeInMs() const
    {
        // todo добавить критическую секцию.
        return current_time_;
    }
};

} // namespace kraslibs

#endif /* KRASLIBS_DEADLINE_TIMER_HPP */
