/// @file callback_timer.hpp
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

#ifndef CALLBACK_TIMER_HPP
#define CALLBACK_TIMER_HPP

#include "etl/callback_timer_atomic.h"
#include "stv/concepts.hpp"
#include <atomic>
#include <chrono>

namespace stv {

/// @brief Класс главного таймера используется для вызова пользовательских
/// функций и методов из контекста основной программы.
template<uint_least8_t CallbackTimersNumb = 10>
class callback_timer:
    public etl::callback_timer_atomic<CallbackTimersNumb, std::atomic_uint32_t>
{
  public:
    using base_type  = etl::icallback_timer_atomic<std::atomic_uint32_t>;
    using count_type = std::chrono::duration<std::uint32_t, std::micro>;

  private:
    count_type period_{0};

    ///@brief Number of ticks since last notification.
    std::uint32_t nticks_{count_type{period_}.count()};

    /// @brief Флаг готовности обработки делегатов в run().
    volatile bool is_notify_given_{false};

  public:
    callback_timer(
        decltype(period_) period)
    {
        set_period(period);
    }

    operator bool()
    {
        auto is_ready_to_run{true};

        if(!is_period_valid()) {
            is_ready_to_run = false;
        }

        return is_ready_to_run;
    }

    /// @brief Метод отвечает за обработку зарегистрированных делегатов.
    /// @note Обработка будет выполнена только в том случае, если
    /// пользовательский код вызвал GiveNotify().
    /// @note Рекомендуется данный метод вызывать из main().
    void run()
    {
        if(is_notify_given_ && is_period_valid()) {
            // Сброс флага, следующая обработка будет выполнена только после
            // повторного вызова GiveNotify(). Данный флаг сбрасывается до
            // начала обработки делегатов. Это позволяет в случае длительной
            // обработки, не пропустить нового вызова GiveNotify().
            is_notify_given_ = false;

            if(this->tick(nticks_)) {
                nticks_ = period_.count();
            } else {
                nticks_ += period_.count();
            }
        }
    }

    /// @brief Устанавливает период в микросекундах между вызовами GiveNotify().
    /// @note Пользовательский код отвечате за то, чтобы период между вызовами
    /// GiveNotify() и значение, указанное в period_in_us соответствовали.
    /// @param[in] period_in_us: Период в микросекундах между вызовами
    /// GiveNotify().
    /// @return true если period_in_us находится в допустимом диапазоне, false в
    /// противном случа.
    template<typename U>
        requires(stv::is_duration_v<U>)
    auto set_period(
        U period)
    {
        auto is_period_valid{false};

        if(period.count() > 0) {
            is_period_valid = true;

            period_ = std::chrono::duration_cast<count_type>(period);
        }

        return is_period_valid;
    }

    /// @brief При получении уведомления о то что прошел период времени,
    /// указанный в period_, вызовите данный метод. Это разблокирует в run()
    /// выполнение зарегистрированных делегатов.
    void give_notify() { is_notify_given_ = true; }

  private:
    auto is_period_valid()
    {
        if(period_ > count_type{0U}) {
            return true;
        }

        return false;
    }
};

} // namespace stv

#endif /* CALLBACK_TIMER_HPP */
