/// @file moving_average.hpp
///
/// @author Mickle Isaev (mrraptor26@gmail.com)
/// @author Matvey Simakov <simakov.matvey@mail.ru>
///
/// @copyright (c) 2025 "The Boys"
///
/// MIT License:
///
/// Permission is hereby granted, free of charge, to any person obtaining a
/// copy of this software and associated documentation files (the 'Software'),
/// to deal in the Software without restriction, including without limitation
/// the rights to use, copy, modify, merge, publish, distribute, sublicense,
/// and/or sell copies of the Software, and to permit persons to whom the
/// Software is furnished to do so, subject to the following conditions:
///
/// The above copyright notice and this permission notice shall be included in
/// all copies or substantial portions of the Software.
///
/// THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
/// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
/// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
/// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
/// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
/// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
/// DEALINGS IN THE SOFTWARE.
///
/// NAME
///     moving_average
///
/// DESCRIPTION
///     moving_average provides a simple moving average filter implementation
///     for signal processing. The library includes configurable window sizes,
///     thread safety options, and runtime reconfiguration capabilities.
///
///     Main components:
///     - SimpleMovingAverageSetupParams: Configuration structure with window
///       width and mutex settings
///     - ISimpleMovingAverage: Base template class implementing core filtering
///       logic with configurable buffer size
///     - SizeWrapper: Storage wrapper that combines buffer array with filter
///       implementation
///
///     The filter supports:
///     - Integer and floating-point value types
///     - Custom mutex types for thread safety
///     - External mutex references via MutexExtTag
///     - Runtime window width changes
///     - Batch processing of multiple samples
///
///     All implementations require proper buffer initialization. Concurrent
///     access must be synchronized when using non-thread-safe mutex types.
///
/// EXAMPLE
///     Basic usage example:
///     ```cpp
///     #include "moving_average.hpp"
///     #include <iostream>
///
///     int main() {
///         using namespace stv;
///
///         // Define setup parameters with window width of 5
///         using SetupParams = SimpleMovingAverageSetupParams<double>;
///         using FilterBase = ISimpleMovingAverage<SetupParams>;
///
///         // Create filter with maximum buffer size of 20
///         SimpleMovingAverage<FilterBase, 20> filter{
///             SetupParams{.window_width = 5U}
///         };
///
///         // Test data with noise
///         double noisy_data[] = {1.0, 1.1, 0.9, 1.05, 0.95,
///                                1.02, 0.98, 1.01, 0.99, 1.0};
///
///         std::cout << "Original -> Filtered:\n";
///         for (double sample : noisy_data) {
///             double filtered = filter.Filtered(sample);
///             std::cout << sample << " -> " << filtered << "\n";
///         }
///
///         // Check if buffer is full (after 5 samples)
///         std::cout << "Buffer full: " << filter.IsBufferFull() << "\n";
///
///         return 0;
///     }
///     ```
///     Output will show smoothing effect after the buffer fills.
///
///     Thread-safe usage with custom mutex:
///     ```cpp
///     #include "moving_average.hpp"
///     #include <mutex>
///
///     // Use std::recursive_mutex for thread safety
///     using ThreadSafeSetup = SimpleMovingAverageSetupParams<float,
///         std::recursive_mutex>;
///     using ThreadSafeFilter = ISimpleMovingAverage<ThreadSafeSetup>;
///
///     SimpleMovingAverage<ThreadSafeFilter, 10> thread_safe_filter{
///         ThreadSafeSetup{.window_width = 3U}
///     };
///     ```
///
///     External mutex usage:
///     ```cpp
///     struct CustomMutex {
///         void lock() { /* implementation */ }
///         void unlock() { /* implementation */ }
///     };
///
///     CustomMutex custom_mutex;
///     using ExtMutexSetup = SimpleMovingAverageSetupParams<float,
///         CustomMutex, MutexExtTag>;
///     using ExtFilter = ISimpleMovingAverage<ExtMutexSetup>;
///
///     SimpleMovingAverage<ExtFilter, 15> ext_filter{
///         ExtMutexSetup{.window_width = 4U, .mutex = &custom_mutex}
///     };
///     ```
///
///     See test cases in the test suite for comprehensive usage examples and
///     validation tests.

#ifndef STVF_MOVING_AVERAGE_HPP
#define STVF_MOVING_AVERAGE_HPP

#include "GSL/gsl"
#include "boost/leaf.hpp"
#include "etl/mutex.h"
#include "stv/concepts.hpp"
#include "stv/mutex_guard.hpp"
#include "stv/wrappers.hpp"
#include <algorithm>
#include <array>
#include <concepts>
#include <cstdint>
#include <variant>

namespace stv {

struct ZeroWindowWidth {
};

struct MaxWindowWidthLimitError {
};

/// @brief Параметры фильтра скользящего среднего.
template<typename T, typename TMutex = stv::EmptyMutex,
         typename TMutexTag = stv::MutexIntTag>
struct SimpleMovingAverageSetupParams {
    using ValueType          = T;
    using MutexTag           = TMutexTag;
    using MutexType          = TMutex;
    using MutexConditionType = stv::mutex_type_setup_v<TMutex, TMutexTag>;

    static constexpr std::uint16_t DEFAULT_WINDOW_WIDTH{1};

    /// @brief Ширина окна фильтра.
    ///
    /// @note Значение должно быть положительным и не превышать максимальную
    /// ширину окна.
    std::uint16_t window_width{DEFAULT_WINDOW_WIDTH};

    /// @brief Если указан внешний мьютекс, то mutex будет указателем на тип
    /// TMutex, в противном случае тип будет пустым.
    MutexConditionType mutex{};

    /// @brief Оператор сравнения на равенство.
    /// @param[in] other: Другой объект параметров.
    /// @return true, если параметры равны, иначе false.
    auto operator==(
        const SimpleMovingAverageSetupParams &other) const -> bool
    {
        return (this->window_width == other.window_width);
    }

    /// @brief Оператор сравнения на неравенство.
    /// @param[in] other: Другой объект параметров.
    /// @return true, если параметры не равны, иначе false.
    auto operator!=(
        const SimpleMovingAverageSetupParams &other) const -> bool
    {
        return !(*this == other);
    }

    /// @brief Возвращает true если window_width находится в допустимом
    /// диапазоне.
    ///
    /// @param max_window_width: Максимально допустима ширина окна, которую
    /// задает вызывающий код.
    ///
    /// @return true если window_width находится в допустимом диапазоне, false
    /// в противном случае.
    auto IsValid(
        std::size_t max_window_width) const -> boost::leaf::result<void>
    {
        if(window_width == 0) {
            return boost::leaf::new_error(stv::ZeroWindowWidth{});
        }

        if(window_width > max_window_width) {
            return boost::leaf::new_error(stv::MaxWindowWidthLimitError{});
        }

        return boost::leaf::result<void>{};
    }
};

/// @brief Структура, содержащая атрибуты фильтра скользящего среднего.
///
/// @note Наследуется от SimpleMovingAverageSetupParams для включения ширины
/// окна.
template<typename T, typename TMutex = stv::EmptyMutex>
using SimpleMovingAverageFilterAttr = SimpleMovingAverageSetupParams<T, TMutex>;

/// @brief Базовый класс, представляющий интерфейсы для фильтрации значений с
/// помощью скользящего среднего без указания максимальной ширины окна в списке
/// параметров шаблона. В качестве дополнительной функции ISimpleMovingAverage
/// предлагает изменение фактической ширины окна во время выполнения.
template<typename TSetup>
class ISimpleMovingAverage
{
  public:
    using ValueType = typename TSetup::ValueType;
    using MutexTag  = typename TSetup::MutexTag;
    using MutexType = stv::mutex_type_v<typename TSetup::MutexType, MutexTag>;
    using SetupType = TSetup;
    using ContainerType = gsl::span<ValueType>;

#ifdef UNIT_TEST_ENABLE
  public:
#else
  private:
#endif

    /// @brief Представление ввода и вывода в память для хранения отсчетов.
    ///
    /// @note Это поле сделано защищенным для тестирования защищенных
    /// операторов перемещения и копирования.
    ContainerType buffer;

  private:
    /// @brief Накопленная сумма значений в буфере.
    ValueType sum_{};

    /// @brief Счетчик, указывающий текущую позицию в буфере.
    std::uint32_t cnt_{};

    /// @brief Флаг, указывающий, был ли буфер заполнен хотя бы один раз.
    /// Может быть сброшен в SetBiggerWindowWidth().
    bool is_buffer_full_{false};

    /// @brief Фактические параметры фильтра скользящего среднего.
    SetupType setup_actual_;

    /// @brief Параметры по умолчанию фильтра скользящего среднего.
    ///
    /// @note Используются невалидные параметры которые вернут ошибку при вызове
    /// IsValid().
    SetupType setup_default_;

    /// @brief Обратное значение текущей ширины окна для более быстрых операций
    /// деления.
    ///
    /// @note Если T является целочисленным типом, то используется прямое
    /// значение вместо обратного.
    ValueType window_width_inv_{};

    /// @brief Используется для обеспечения атомарности обновления данных в
    /// многопоточном приложении.
    mutable MutexType mutex_;

  public:
    /// @brief Деструктор.
    ///
    /// @note Этот деструктор виртуальный, чтобы обеспечить правильную очистку
    /// объектов производного класса при удалении через указатель базового
    /// класса.
    virtual ~ISimpleMovingAverage() = default;

    /// @brief Защищенный конструктор перемещения по умолчанию.
    /// @note Член "правила 5" в С++.
    ISimpleMovingAverage(ISimpleMovingAverage &&other) = default;

    /// @brief Защищенный оператор присваивания перемещения по умолчанию.
    /// @note Член "правила 5" в С++.
    auto operator=(ISimpleMovingAverage &&other)
        -> ISimpleMovingAverage & = default;

    /// @brief Защищенный оператор присваивания копирования по умолчанию.
    /// @note Член "правила 5" в С++.
    auto operator=(const ISimpleMovingAverage &other)
        -> ISimpleMovingAverage & = default;

    /// @brief Защищенный конструктор копирования по умолчанию.
    /// @note Член "правила 5" в С++.
    ISimpleMovingAverage(const ISimpleMovingAverage &other) = default;

    explicit operator boost::leaf::result<void>() const noexcept(
        noexcept(setup_default_.IsValid(buffer.size()))
        && noexcept(setup_actual_.IsValid(buffer.size())))
    {
        BOOST_LEAF_CHECK(setup_default_.IsValid(buffer.size()));
        BOOST_LEAF_CHECK(setup_actual_.IsValid(buffer.size()));
        return boost::leaf::result<void>{};
    }

    explicit operator bool() const noexcept
    {
        auto is_mutex_ptr_valid{true};
        if constexpr(std::is_pointer_v<decltype(mutex_)>) {
            if(!mutex_) {
                is_mutex_ptr_valid = false;
            }
        }
        return static_cast<bool>(static_cast<boost::leaf::result<void>>(*this))
               && is_mutex_ptr_valid;
    }

    /// @brief Установить параметры фильтра скользящего среднего во время
    /// выполнения.
    ///
    /// @note Корректирует внутренний буфер и сумму на основе новой ширины
    /// окна.
    ///
    /// @param[in] params: Параметры фильтра.
    template<typename U>
    auto Setup(
        U &&params, bool is_isr = false)
        noexcept(
            noexcept(params.IsValid(buffer.size()))
            && noexcept(ChangeWindowWidthAndUpdateCounter(params.window_width))
            && noexcept(UpdateWindowWithInverse())) -> boost::leaf::result<void>
    {
        BOOST_LEAF_CHECK(params.IsValid(buffer.size()));

        const stv::lock_guard critical{GetMutexRef(), is_isr};

        ChangeWindowWidthAndUpdateCounter(params.window_width);

        setup_actual_ = std::forward<U>(params);

        // Обновить обратную ширину окна для оптимизации времени
        // вычислений.
        UpdateWindowWithInverse();

        return boost::leaf::result<void>{};
    }

    /// @brief Получить конфигурацию фильтра по умолчанию.
    ///
    /// @return Значения по умолчанию для параметров фильтра.
    [[nodiscard]] auto GetDefaultParams() const noexcept -> SetupType
    {
        return setup_default_;
    }

    /// @brief Получить фактические параметры настройки фильтра.
    [[nodiscard]] auto GetSetup(
        bool is_isr = false) const
    {
        const stv::lock_guard critical{GetMutexRef(), is_isr};
        return setup_actual_;
    }

    /// @brief Сбросить коэффициенты фильтра в значения "по умолчанию".
    void Reset(
        bool is_isr = false)
    {
        const stv::lock_guard critical{GetMutexRef(), is_isr};

        // Строка ниже гарантировано завершиться успешно т.к. setup_default_
        // устанавливается в конструкторе и содержит достоверные значения.
        const auto error = Setup(setup_default_);
        (void)error;
    }

    /// @brief Возвращает статус буфера.
    /// @return true - если буфер заполен и Filtered() возвращает среднее
    /// арифметическое значение, false - если буфер еще не заполнен и
    /// Filtered() возвращает исходное значение.
    [[nodiscard]] auto IsBufferFull(
        bool is_isr = false) const noexcept
    {
        const stv::lock_guard critical{GetMutexRef(), is_isr};
        return is_buffer_full_;
    }

    /// @brief Очищает все значения буфера скользящего среднего.
    void Clear(
        bool is_isr = false)
        noexcept(
            noexcept(Setup()))
    {
        const stv::lock_guard critical{GetMutexRef(), is_isr};
        cnt_            = 0;
        is_buffer_full_ = false;
    }

    /// @brief Получить арифметическое среднее из буфера.
    ///
    /// @param[in] new_sample: Новый отсчет для добавления в буфер.
    ///
    /// @note Если буфер содержит меньше значений, чем было задано при вызове
    /// Setup(), Filtered() вернет new_sample.
    ///
    /// @return Среднее значение, если в буфере достаточно элементов.
    [[nodiscard]] auto Filtered(
        ValueType new_sample, bool is_isr = false)
        noexcept(
            noexcept(IsBufferFull()))
    {
        const stv::lock_guard critical{GetMutexRef(), is_isr};

        sum_          += new_sample - buffer[cnt_];
        buffer[cnt_]   = new_sample;
        auto filtered  = new_sample;

        if(!IsBufferFull(is_isr)) {
            if((cnt_ + 1) == setup_actual_.window_width) {
                is_buffer_full_ = true;
            }
        }

        if(IsBufferFull(is_isr)) {
            // ... вычислить среднее.
            if constexpr(std::is_integral_v<ValueType>) {
                filtered = sum_ / window_width_inv_;
            } else {
                filtered = sum_ * window_width_inv_;
            }
        }

        cnt_ = (cnt_ + 1) % setup_actual_.window_width;

        return filtered;
    }

    template<typename... TSamples>
    auto Filtered(
        TSamples... samples)
    {
        ValueType filtered;

        ((filtered = Filtered(std::forward<TSamples>(samples))), ...);

        return filtered;
    }

    template<typename TInputIt>
    auto Filtered(
        TInputIt cbegin, TInputIt cend, bool is_isr = false)
    {
        ValueType filtered;
        std::for_each(cbegin, cend, [&](const auto &item) {
            filtered = Filtered(item, is_isr);
        });
        return filtered;
    }

  protected:
    /// @brief Защищенный конструктор для предотвращения прямого создания
    /// экземпляра.
    ///
    /// @param[in] attr: Атрибуты конфигурации фильтра.
    ///
    /// @param[in] buffer_span: Span, просматривающий предварительно выделенный
    /// массив для хранения отсчетов.
    ISimpleMovingAverage(
        const SetupType &attr, ContainerType buffer_span):
        buffer{buffer_span},
        setup_actual_(static_cast<bool>(attr.IsValid(buffer.size()))
                          ? attr
                          : SetupType{.window_width = 0U}),
        setup_default_{setup_actual_}
    {
        UpdateWindowWithInverse();

        if constexpr(std::is_same_v<MutexTag, stv::MutexExtTag>) {
            mutex_ = attr.mutex;
        }
    }

  private:
    /// @brief Изменить ширину окна и обновить счетчик, если новая ширина
    /// отличается от ширины из setup_actual.
    ///
    /// @param[in] new_width: Новая ширина окна.
    void ChangeWindowWidthAndUpdateCounter(
        std::size_t new_width)
        noexcept(
            noexcept(SetSmallerWindowWidth(new_width))
            && noexcept(SetBiggerWindowWidth(new_width)))
    {
        if(new_width < setup_actual_.window_width) {
            SetSmallerWindowWidth(new_width);
        } else if(new_width > setup_actual_.window_width) {
            SetBiggerWindowWidth(new_width);
        }
    }

    /// @brief Уменьшить ширину окна и удалить старейшие элементы из текущего
    /// <cnt_>.
    ///
    /// @param[in] new_width: Новая меньшая ширина окна фильтра.
    ///
    /// @note Этот метод
    void SetSmallerWindowWidth(
        std::size_t new_width) noexcept
    {
        auto old_width      = setup_actual_.window_width;
        auto size_decrement = old_width - new_width;

        // Уменьшить накопленную сумму.
        for(std::size_t i = 0; i < size_decrement; ++i) {
            // Вычислить индекс элемента, который должен быть удален из
            // суммы.
            auto idx  = (cnt_ + i) % old_width;
            sum_     -= buffer[idx];
        }

        // Обновить буфер, оставляя только последние элементы.

        // Вычислить максимальный индекс элемента старого буфера. Индекс должен
        // быть больше нуля
        auto old_max_element_idx = old_width - 1;

        // Добавить old_width к текущему счетчику, чтобы иметь возможность
        // итерироваться справа налево. Счетчик должен быть больше нуля.
        auto last_element_idx = cnt_ + old_max_element_idx;

        for(std::size_t i = 0, new_max_element_idx = new_width - 1;
            i < new_width; ++i) {
            // Вычислить индекс элемента, который должен остаться в буфере.
            // Начать с последнего элемента, который должен остаться.
            auto idx = ((last_element_idx - i) % old_width);

            // Обновить буфер
            buffer[new_max_element_idx - i] = buffer[idx];
        }

        // Установить все элементы за пределами new_width в 0.
        std::fill(buffer.begin() + new_width, buffer.end(),
                  static_cast<ValueType>(0));

        // Обновить счетчик и проверить, что он не становится отрицательным.
        if(cnt_ < size_decrement) {
            cnt_ = 0;
        } else {
            cnt_ -= size_decrement;
        }
    }

    /// @brief Увеличить ширину окна и обновить счетчик соответствующим
    /// образом.
    ///
    /// @note Счетчик обновляется только если буфер уже полон.
    ///
    /// @param[in] new_width: Новая большая ширина окна фильтра.
    void SetBiggerWindowWidth(
        std::size_t new_width) noexcept
    {
        if(IsBufferFull()) {
            cnt_ = (cnt_ + (new_width - setup_actual_.window_width) + 1)
                   % new_width;

            is_buffer_full_ = false;
        }
    }

    /// @brief Обновить обратное значение ширины окна. Это значение
    /// используется для более быстрого вычисления в Filtered().
    auto UpdateWindowWithInverse() noexcept -> void
    {
        if(setup_actual_.window_width > 0) {
            if constexpr(std::is_integral_v<ValueType>) {
                window_width_inv_ = setup_actual_.window_width;
            } else {
                window_width_inv_ =
                    static_cast<ValueType>(1)
                    / static_cast<ValueType>(setup_actual_.window_width);
            }
        }
    }

    auto GetMutexRef() const -> std::remove_pointer_t<MutexType> &
    {
        if constexpr(std::is_same_v<MutexTag, stv::MutexExtTag>) {
            assert(mutex_ != nullptr);
            return *mutex_;
        } else {
            return mutex_;
        }
    }
};

template<stv::FilterableConcept TBase, std::size_t MAX_WINDOW_WIDTH = 20>
using SimpleMovingAverage = stv::SizeWrapper<TBase, MAX_WINDOW_WIDTH>;

} // namespace stv
#endif /* STVF_MOVING_AVERAGE_HPP */
