/// @file callback_timer.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.

/*
/// ############################################################################
/// ПРИМЕР:
/// ############################################################################

#include "callback_timer.hpp" // Предполагаем, что заголовочный файл доступен
#include <iostream>
#include <chrono>
#include <thread>
#include <atomic>

namespace my_app {

// 1. Определяем наш класс задачи
class MyTask {
public:
    // Метод run() будет вызываться таймером
    void run() {
        std::cout << "MyTask::run() executed at " <<
std::chrono::steady_clock::now().time_since_epoch().count() << std::endl;
        counter_++;
        if (counter_ >= 5) {
             std::cout << "MyTask finished its job." << std::endl;
             // Для примера с однократным таймером, можно как-то остановить его
выполнение извне.
             // Здесь просто выводим сообщение.
        }
    }

private:
    int counter_ = 0;
};

// 2. Создаем глобальные/статические переменные для таймера
// (Обычно это делается в отдельном модуле инициализации)
using MyCallbackTimer = stv::callback_timer<5>; // Максимум 5 таймеров
MyCallbackTimer my_system_timer{std::chrono::microseconds(1000)}; // Установим
внутренний период 1 мс (для примера)

// 3. Определяем тип singleton'а для нашего таймерного контекста
using MyTaskContext =
stv::callback_timer_context<stv::callback_timer_context_init<MyCallbackTimer>,
MyTask>; using MyTaskSingleton = etl::singleton<MyTaskContext>;

// 4. Функция инициализации
void init_my_task() {
    stv::callback_timer_context_init<MyCallbackTimer> init;
    init.callback_timer = &my_system_timer; // Передаем ссылку на наш таймер
    init.period         = std::chrono::milliseconds(4000); // Период выполнения
задачи - 4 секунды init.is_continuous  = true; // Повторяющийся таймер

    MyTaskSingleton::create(init); // Создаем singleton с привязкой к таймеру
}

// 5. Функция для симуляции ISR или внешнего источника тиков
void simulate_timer_isr() {
    // Эта функция должна вызываться каждую 1 мс (или как часто задан period у
my_system_timer) my_system_timer.give_notify(); // Сообщаем таймеру, что прошла
1 мс
}

} // namespace my_app

int main() {
    std::cout << "Initializing task..." << std::endl;
    my_app::init_my_task();

    std::cout << "Starting main loop..." << std::endl;
    auto start_time = std::chrono::steady_clock::now();
    while (std::chrono::steady_clock::now() - start_time <
std::chrono::seconds(25)) { // Работаем 25 секунд
        // Основной цикл программы
        my_app::my_system_timer.run(); // Обрабатываем делегаты, если нужно
        std::this_thread::sleep_for(std::chrono::microseconds(100)); //
Симулируем полезную работу my_app::simulate_timer_isr(); // Симулируем внешнее
прерывание таймера (раз в 1 мс)
    }

    std::cout << "Main loop ended." << std::endl;

    // Singleton будет уничтожен автоматически при выходе из main,
    // что вызовет деструктор MyTaskContext и отмену регистрации таймера.
    return 0;
} */

#ifndef CALLBACK_TIMER_HPP
#define CALLBACK_TIMER_HPP

#include "etl/callback_timer_atomic.h"
#include "stv/concepts.hpp"
#include "stv/utils.hpp"
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
    explicit callback_timer(
        decltype(period_) period)
    { set_period(period); }

    explicit operator bool()
    {
        auto is_ready_to_run{true};

        if(!is_period_valid())
        {
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
        if(is_notify_given_ && is_period_valid())
        {
            // Сброс флага, следующая обработка будет выполнена только после
            // повторного вызова GiveNotify(). Данный флаг сбрасывается до
            // начала обработки делегатов. Это позволяет в случае длительной
            // обработки, не пропустить нового вызова GiveNotify().
            is_notify_given_ = false;

            if(this->tick(nticks_))
            {
                nticks_ = period_.count();
            }
            else
            {
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

        if(period.count() > 0)
        {
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
    auto is_period_valid() { return period_ > count_type{0U}; }
};

/// @brief Структура данных для инициализации callback_timer_context.
///
/// Содержит параметры, необходимые для создания экземпляра
/// callback_timer_context.
///
/// @tparam TCallbackTimer Тип используемого callback_timer.
template<typename TCallbackTimer>
struct callback_timer_context_init {
    using callback_timer_type =
        etl::icallback_timer_atomic<std::atomic_uint32_t>;

    using count_type = typename TCallbackTimer::count_type;

    callback_timer_type *callback_timer{nullptr};

    count_type           period{0};

    /// @brief Указывает, является ли операция непрерывной.
    bool is_continuous{true};

    bool immediately{false};
};

/// @brief Класс для привязки объекта с методом run() к callback_timer.
///
/// Этот класс автоматически регистрирует метод run() объекта типа TBase
/// как делегат в указанном callback_timer. При достижении заданного периода
/// метод run() будет вызываться автоматически.
///
/// @tparam TDelegateInit Тип структуры инициализации (обычно
/// callback_timer_context_init).
/// @tparam TBase Тип класса, объект которого нужно привязать к таймеру. Должен
/// иметь метод run().
template<typename TDelegateInit, typename TBase>
    requires(stv::runable_concept<TBase>)
class callback_timer_context: public TBase
{
    using delegate_init_type = TDelegateInit;
    using base_type          = TBase;
    using callback_timer_type =
        typename delegate_init_type::callback_timer_type;
    using count_type = typename delegate_init_type::count_type;

    callback_timer_type *callback_timer_{nullptr};

    /// @brief Период выполнения в микросекундах.
    count_type period_{};

    /// @brief Указывает, является ли операция непрерывной.
    const bool is_continuous_{true};

    /// @brief Делегат функции для выполнения операции.
    typename callback_timer_type::callback_type delegate_{
        callback_timer_type::callback_type::template create<base_type,
                                                            &base_type::run>(
            *this)};

    /// @brief Идентификатор таймера.
    etl::timer::id::type id_{etl::timer::id::NO_TIMER};

  public:
    template<typename... TArgs>
    explicit callback_timer_context(
        const delegate_init_type &delegate, TArgs &&...args):
        base_type{std::forward<TArgs>(args)...},
        callback_timer_{delegate.callback_timer},
        period_{delegate.period},
        is_continuous_{delegate.is_continuous}
    {
        if(callback_timer_)
        {
            id_ = callback_timer_->register_timer(delegate_, period_.count(),
                                                  is_continuous_);
            assert(id_ != etl::timer::id::NO_TIMER);
            callback_timer_->start(id_, delegate.immediately);
        }
    }

    virtual ~callback_timer_context()
    {
        if(callback_timer_->unregister_timer(id_))
        {
            id_ = etl::timer::id::NO_TIMER;
        }
    }
};

} // namespace stv

#endif /* CALLBACK_TIMER_HPP */
