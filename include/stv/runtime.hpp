/// @file runtime.hpp
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

#ifndef RUNTIME_HPP
#define RUNTIME_HPP

#include "stv/mutex_guard.hpp"
#include "stv/utils.hpp"
#include <cassert>
#include <chrono>
#include <concepts>
#include <cstdint>
#include <type_traits>
#include <variant>

namespace stv {

using runtime_counter_type = std::chrono::duration<std::uint32_t, std::micro>;

template<typename TCounter    = runtime_counter_type,
         typename TMutexOrPtr = stv::empty_mutex>
    requires std::unsigned_integral<typename TCounter::rep>
class runtime_setup
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
    using value_type = TCounter;
    using mutex_type = TMutexOrPtr;

    static_assert(std::unsigned_integral<typename value_type::rep>,
                  "TCounter must be std::chrono::duration with unsigned type");

    /// @brief Период времени в мс на которе увеличивается счетчик при
    /// вызове runtime::inc().
    value_type increment_period{static_cast<decltype(increment_period)>(0)};

    /// @brief Если указан внешний мьютекс (stv::mutex_ext_tag), то mutex будет
    /// указателем на тип TMutex, в противном случае тип будет пустым.
    mutex_condition_type mutex{};
};

template<typename TSetup = runtime_counter_type>
class runtime
{
  public:
    using value_type = typename TSetup::value_type;
    using mutex_type = typename TSetup::mutex_type;
    using setup_type = TSetup;

  private:
    /// @brief Счетчик времени с момента запуска системы.
    volatile value_type::rep counter_{static_cast<value_type::rep>(0)};

    /// @brief Период времени в мс на которе увеличивается счетчик при вызове
    /// runtime::inc().
    value_type increment_period_{};

    /// @brief Используется для обеспечения атомарности обновления данных в
    /// многопоточном приложении.
    mutable mutex_type mutex_;

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
    explicit runtime(
        const setup_type &setup):
        increment_period_{setup.increment_period}
    {
        if constexpr(std::is_pointer_v<decltype(mutex_)>)
        {
            mutex_ = setup.mutex;
        }
    }

    virtual ~runtime() = default;

    explicit operator bool() const
    {
        auto is_valid{true};

        if(increment_period_.count() == 0)
        {
            is_valid = false;
        }
        return is_valid;
    }

    /// @brief Увеличивает счетчик тиков на величину increment_period_.
    ///
    /// @note Ожидается, что данный метод вызывается из контекста прерывания,
    /// например из прерывания systic.
    auto inc(
        bool is_isr = false) -> void
    {
        const auto lock  = stv::lock_guard{get_mutex_ref(), is_isr};
        counter_        += increment_period_.count();
    }

    /// @brief Возвращает прошедшее с момента запуска системы время.
    ///
    /// @return Объект std::chrono.
    virtual auto get() -> value_type
    {
        const auto lock = stv::lock_guard{get_mutex_ref()};
        return value_type{counter_};
    }

    void set_period(
        auto period)
    {
        const auto lock   = stv::lock_guard{get_mutex_ref()};
        increment_period_ = std::chrono::duration_cast<value_type>(period);
    }
};

} // namespace stv

#endif /* RUNTIME_HPP */
