/// @file runtime.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.

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

template<typename TValueType>
class runtime_interface
{
  public:
    using value_type = TValueType;

    virtual ~runtime_interface() = default;

    /// @brief Возвращает прошедшее с момента запуска системы время.
    [[nodiscard]] virtual auto get() const -> value_type = 0;

  protected:
    runtime_interface() = default;
};

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

template<typename TSetup>
class runtime: public runtime_interface<typename TSetup::value_type>
{
  public:
    using setup_type   = TSetup;
    using value_type   = typename setup_type::value_type;
    using counter_type = value_type;
    using mutex_type   = typename setup_type::mutex_type;

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

    ~runtime() override = default;

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
    auto get() const -> value_type override
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
