/// @file status_led.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
///
/// NAME
///     stv::status_led
///
/// DESCRIPTION
///     status_led.hpp предоставляет класс для управления светодиодом
///     состояния устройства с использованием программного таймера обратного
///     вызова. Библиотека включает следующие основные компоненты:
///     - led_api: абстрактный интерфейс низкоуровневого управления
///       светодиодом (включение, выключение, переключение).
///     - status_led_api: абстрактный интерфейс для установки и получения
///       режима индикации.
///     - status_led_states: перечисление режимов работы:
///         - kError:   частое мигание (100 мс), сигнал ошибки;
///         - kNormal:  редкое мигание (1000 мс), нормальный режим;
///         - kFast:    мигание с периодом 300 мс;
///         - kDisable: светодиод выключен.
///     - status_led_init<TCallbackTimer>: структура инициализации,
///       связывающая таймер обратного вызова и реализацию led_api.
///     - status_led<TInit>: шаблонный класс, реализующий заданный режим
///       мигания через регистрацию делегатов в таймере.
///
///     Класс status_led наследуется от status_led_api и запрещает
///     копирование и перемещение. Для корректной работы требуется действующий
///     объект callback_timer и реализация led_api, переданные через
///     status_led_init. Проверить корректность инициализации можно с
///     помощью operator bool().
///
///     Метод set_mode(status_led_states) изменяет режим индикации,
///     возвращает true при успешном переключении. Метод get_mode()
///     возвращает текущий активный режим.
///
///     Все реализации не являются потокобезопасными. Одновременный доступ
///     из нескольких потоков или контекстов прерываний требует внешней
///     синхронизации.
///
/// EXAMPLE
///     Пример использования:
///     ```cpp
///     #include <stv/status_led.hpp>
///     #include <stv/callback_timer.hpp>
///     #include <chrono>
///
///     // Реализация интерфейса светодиода.
///     class my_led : public stv::led_api {
///       public:
///         void enable() const override { /* включить светодиод */ }
///         void disable() const override { /* выключить светодиод */ }
///         void toggle() const override { /* переключить светодиод */ }
///     };
///
///     int main() {
///         using namespace stv;
///
///         // Таймер обратного вызова с периодом 1 мс.
///         callback_timer<10> timer{std::chrono::microseconds(1000)};
///
///         my_led led;
///         status_led_init<callback_timer<10>> init{
///             .callback_timer = &timer,
///             .led = &led,
///         };
///
///         // Создание контроллера светодиода состояния.
///         status_led<decltype(init)> status_led_controller(init);
///
///         if (!status_led_controller) {
///             // Ошибка инициализации: проверьте указатели.
///             return -1;
///         }
///
///         // Установка режима нормальной работы.
///         status_led_controller.set_mode(status_led_states::kNormal);
///
///         // Имитация главного цикла с вызовом таймера.
///         for (int i = 0; i < 100; ++i) {
///             timer.give_notify();
///             timer.run();
///             // Задержка, соответствующая периоду таймера.
///         }
///
///         // Переключение в режим ошибки.
///         status_led_controller.set_mode(status_led_states::kError);
///
///         return 0;
///     }
///     ```
///     В рабочем приложении метод timer.give_notify() обычно вызывается из
///     прерывания аппаратного таймера, а timer.run() — из главного цикла.
///     Реализацию led_api необходимо адаптировать под конкретный порт GPIO.
///
///     См. board/v1_0/board_status_led.hpp и board_status_led.cpp для
///     примера интеграции в реальный проект на микроконтроллере.

#ifndef STATUS_LED_HPP
#define STATUS_LED_HPP

#include "chrono"
#include "etl/callback_timer_atomic.h"
#include "stv/utils.hpp"
#include <array>

namespace stv {

class led_api: public stv::non_copyable, stv::non_movable
{
  public:
    virtual ~led_api() = default;

    virtual void enable() const  = 0;
    virtual void disable() const = 0;
    virtual void toggle() const  = 0;

  protected:
    led_api() = default;
};

enum class status_led_states {
    kError = 0,

    kNormal,

    kFast,

    kDisable,

    kMaxNumber,
};

struct status_led_api: public stv::non_copyable, public stv::non_movable {
    virtual ~status_led_api() = default;

    virtual auto set_mode(status_led_states new_blink_mode) -> bool = 0;

    [[nodiscard]] virtual auto get_mode() const -> status_led_states = 0;

  protected:
    status_led_api() = default;
};

template<typename TCallbackTimer>
struct status_led_init {
    using callback_timer_type       = TCallbackTimer;
    using callback_timer_count_type = TCallbackTimer::count_type;

    callback_timer_type *callback_timer{nullptr};
    led_api             *led{nullptr};
};

template<typename TInit>
class status_led: public status_led_api
{
  public:
    using init_type           = TInit;
    using callback_timer_type = typename init_type::callback_timer_type;
    using callback_timer_count_type =
        typename init_type::callback_timer_count_type;

  private:
    callback_timer_type *callback_timer_;

    led_api             *led_{nullptr};

    /// @brief Счетчик вызовов для имитации работы проблескового маяка.
    std::size_t          led_cnt_{0};

    etl::timer::id::type id_{etl::timer::id::NO_TIMER};

    /// @brief Счетчик вызовов, по истечение которого необходимо перейти из
    /// режима kLowBatt в kNormal.
    std::size_t       low_bat_to_normal_cnt_{0};

    status_led_states current_blink_mode_{status_led_states::kMaxNumber};

  public:
    explicit status_led(
        const init_type &init):
        callback_timer_{init.callback_timer},
        led_{init.led}
    {
        if(*this)
        {
            set_mode(status_led_states::kNormal);
        }
    }

    ~status_led() override = default;

    status_led(const status_led &other) = delete;
    status_led(status_led &&other)      = delete;

    auto operator=(const status_led &other) -> status_led & = delete;
    auto operator=(status_led &&other) -> status_led &      = delete;

    /// @brief Проверяет валидность интерфейсов управления индикацией и
    /// периодами переключения.
    /// @note Настоятельно рекомендуется вызвать данный оператор после
    /// инициализации объекта чтобы убедиться в корректности инициализации
    /// класса.
    explicit operator bool()
    {
        auto is_object_valid{true};

        if(!callback_timer_)
        {
            is_object_valid = false;
        }

        if(!led_)
        {
            is_object_valid = false;
        }

        return is_object_valid;
    }

    /// @brief Установка режима работы световой индикации.
    /// @warning Не вызывайте данный метод из делегатов:
    /// - Error()
    /// - FatalError()
    /// - WaitTimedFuse()
    /// - LowBat()
    /// - CriticalBattVoltage()
    /// - Normal()
    /// @param[in] new_blink_mode Режим работы световой индикации.
    /// @return true - если установлен новый режим световой индикации.
    auto set_mode(
        status_led_states new_blink_mode) -> bool override
    {
        auto is_new_mode_set{false};

        if((new_blink_mode < status_led_states::kMaxNumber)
           && (new_blink_mode != current_blink_mode_))
        {
            // Отключение световой индикации.
            disable();

            // Обнуление счетчика индикаций.
            low_bat_to_normal_cnt_ = 0U;

            if(callback_timer_->unregister_timer(id_))
            {
                id_ = etl::timer::id::NO_TIMER;
            }
            const auto index = static_cast<std::size_t>(new_blink_mode);
            id_              = callback_timer_->register_timer(
                delegates_[index].delegate, delegates_[index].period.count(),
                delegates_[index].is_continuous);
            assert(id_ != etl::timer::id::NO_TIMER);
            callback_timer_->start(id_, true);

            is_new_mode_set = true;
        }

        return is_new_mode_set;
    }

    [[nodiscard]] auto get_mode() const
        -> decltype(current_blink_mode_) override
    { return current_blink_mode_; }

  private:
    void normal() { led_->toggle(); }

    void disable() { led_->disable(); }

    struct delegate_t {
        typename callback_timer_type::callback_type
            delegate;       ///< Delegate function for the operation.
        callback_timer_count_type period;
        bool is_continuous; ///< Indicates whether the operation is continuous.
    };

    // NOLINTBEGIN(*-magic-numbers)

    /// @brief Массив делегатов световой индикации. Каждый элемент массива
    /// содержит отдельный режим работы световой индикации.
    /// @warning Элементы массива должны быть расположены согласно перечислению
    /// "enum State".
    std::array<delegate_t,
               static_cast<std::size_t>(status_led_states::kMaxNumber)>
        delegates_ = {{{callback_timer_type::callback_type::template create<
                            status_led, &status_led::normal>(*this),
                        std::chrono::milliseconds{100}, true},
                       {callback_timer_type::callback_type::template create<
                            status_led, &status_led::normal>(*this),
                        std::chrono::seconds{1}, true},
                       {callback_timer_type::callback_type::template create<
                            status_led, &status_led::normal>(*this),
                        std::chrono::milliseconds{300}, true},
                       {callback_timer_type::callback_type::template create<
                            status_led, &status_led::disable>(*this),
                        std::chrono::milliseconds{100}, false}}};
};

// NOLINTEND(*-magic-numbers)

} // namespace stv

#endif /* STATUS_LED_HPP */
