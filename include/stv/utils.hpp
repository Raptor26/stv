/// @file utils.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// @brief Вспомогательные утилиты и базовые классы для библиотеки STV.
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
///
/// @details Этот заголовочный файл содержит набор вспомогательных классов для
/// управления семантикой копирования и перемещения объектов, макросы для
/// управления выравниванием структур в памяти, а также шаблонные функции для
/// работы с булевыми значениями и вычисления математических норм.

#ifndef UTILS_HPP
#define UTILS_HPP

#include "stv/concepts.hpp"
#include <algorithm>
#include <cmath>
#include <concepts>
#include <numeric>
#include <ranges>
#include <type_traits>

namespace stv {

/// @brief Базовый класс для запрета операций копирования объекта.
/// @details Наследование от этого класса (публичное или приватное)
/// запрещает генерацию конструктора копирования и оператора присваивания
/// копированием. Это полезно для классов, управляющих уникальными ресурсами
/// (например, файловые дескрипторы, сетевые сокеты), где копирование
/// владельца ресурса не имеет смысла или опасно. Семантика перемещения
/// (move semantics) остается разрешенной.
///
/// @code
/// class ResourceManager : private stv::non_copyable {
///     // Ресурс не может быть скопирован, но может быть перемещен
/// };
/// @endcode
///
/// @note Конструктор перемещения и оператор присваивания перемещением по
/// умолчанию разрешены.
class non_copyable
{
  public:
    /// @brief Конструктор по умолчанию.
    non_copyable() = default;

    /// @brief Конструктор копирования удален.
    /// @details Попытка копирования объекта этого типа или наследника
    /// приведет к ошибке компиляции.
    non_copyable(const non_copyable &) = delete;

    /// @brief Оператор присваивания копированием удален.
    non_copyable &operator=(const non_copyable &) = delete;

    /// @brief Конструктор перемещения по умолчанию.
    non_copyable(non_copyable &&) = default;

    /// @brief Оператор присваивания перемещением по умолчанию.
    non_copyable &operator=(non_copyable &&) = default;

    /// @brief Виртуальный деструктор по умолчанию.
    /// @details Рекомендуется делать деструктор виртуальным, если класс
    /// предназначен для наследования и полиморфного использования, однако
    /// здесь он оставлен не виртуальным для минимизации накладных расходов
    /// (zero-overhead abstraction).
    ~non_copyable() = default;
};

/// @brief Базовый класс для запрета операций перемещения объекта.
/// @details Наследование от этого класса запрещает конструктор перемещения
/// и оператор присваивания перемещением. Копирование разрешено. Это может
/// быть полезно для объектов, которые содержат указатели на самих себя или
/// зависят от своего адреса в памяти.
///
/// @code
/// class SelfReferencing : private stv::non_movable {
///     // Объект нельзя переместить, так как внутренние указатели станут
///     невалидными
/// };
/// @endcode
///
/// @note Конструктор копирования и оператор присваивания копированием
/// разрешены.
class non_movable
{
  public:
    /// @brief Конструктор по умолчанию.
    non_movable() = default;

    /// @brief Конструктор копирования по умолчанию.
    non_movable(const non_movable &) = default;

    /// @brief Оператор присваивания копированием по умолчанию.
    non_movable &operator=(const non_movable &) = default;

    /// @brief Конструктор перемещения удален.
    non_movable(non_movable &&) = delete;

    /// @brief Оператор присваивания перемещением удален.
    non_movable &operator=(non_movable &&) = delete;

    /// @brief Деструктор по умолчанию.
    ~non_movable() = default;
};

/// @brief Базовый класс для запрета копирования и перемещения объекта.
/// @details Наследование от этого класса делает объект уникальным и
/// неизменяемым в контексте передачи владения. Объект может быть создан
/// только на месте (in-place). Часто используется для реализации паттерна
/// Singleton или классов-владелцев эксклюзивных ресурсов, которые не должны
/// передаваться дальше.
///
/// @code
/// class UniqueSensor : private stv::non_movable_non_copyable {
///     // Объект нельзя скопировать или переместить
/// };
/// @endcode
class non_movable_non_copyable
{
  public:
    /// @brief Конструктор по умолчанию.
    non_movable_non_copyable() = default;

    /// @brief Конструктор копирования удален.
    non_movable_non_copyable(const non_movable_non_copyable &) = delete;

    /// @brief Оператор присваивания копированием удален.
    non_movable_non_copyable &
    operator=(const non_movable_non_copyable &) = delete;

    /// @brief Конструктор перемещения удален.
    non_movable_non_copyable(non_movable_non_copyable &&) = delete;

    /// @brief Оператор присваивания перемещением удален.
    non_movable_non_copyable &operator=(non_movable_non_copyable &&) = delete;

    /// @brief Деструктор по умолчанию.
    ~non_movable_non_copyable() = default;
};

/// ----------------------------------------------------------------------------
/// @defgroup packing_macros Макросы управления выравниванием (Packing)
/// @{
///
/// @brief Макросы для предотвращения оптимизации выравнивания полей в
/// структурах и классах с шаблонными параметрами.
///
/// @details Эти макросы используют директивы компилятора для установки
/// выравнивания структур в 1 байт (pack(1)). Это гарантирует, что память
/// будет распределена плотно без выравнивания по границам слов процессора.
/// Полезно при сериализации данных, работе с сетевыми пакетами или
/// взаимодействии с аппаратным обеспечением.
///
/// @par Пример использования:
/// @code
/// STV_NO_PADDING_NO_OPTIMIZE_BEGIN
/// template <typename T>
/// struct MyStruct {
///   char a;
///   int b;
///   T t;
/// };
/// STV_NO_PADDING_NO_OPTIMIZE_END
/// @endcode
///
/// @warning Поддерживаются только компиляторы GCC, Clang и MSVC.
/// При использовании другого компилятора генерация кода будет прервана
/// ошибкой (#error).
///
/// @note Использование плотной упаковки может привести к снижению
/// производительности при доступе к полям структуры на некоторых
/// архитектурах (например, ARM), требующих выравнивания данных.

#if __GNUC__ || __clang__
    #define STV_NO_PADDING_NO_OPTIMIZE_BEGIN _Pragma("pack(push, 1)")
    #define STV_NO_PADDING_NO_OPTIMIZE_END   _Pragma("pack(pop)")
#elif _MSC_VER
    #define STV_NO_PADDING_NO_OPTIMIZE_BEGIN __pragma(pack(push, 1))
    #define STV_NO_PADDING_NO_OPTIMIZE_END   __pragma(pack(pop))
#else
    #error "Compiler does not support packing directives"
    #define STV_NO_PADDING_NO_OPTIMIZE_BEGIN
    #define STV_NO_PADDING_NO_OPTIMIZE_END
#endif
/// @}

/// @defgroup test_macros Макросы для модульного тестирования
/// @{

/// @brief Условный макрос ключевого слова virtual.
/// @details Если определен макрос `UNIT_TEST_ENABLE`, ключевое слово
/// `virtual` подставляется в код. В противном случае оно исчезает. Это
/// позволяет делать методы виртуальными только в сборках для тестирования,
/// чтобы иметь возможность создавать моки (mocks), сохраняя при этом
/// производительность и семантику финальных классов в релизной сборке.
///
/// @par Пример использования:
/// @code
/// class Service {
/// public:
///     STV_VIRTUAL void doWork(); // Виртуально только в тестах
/// };
/// @endcode
#ifdef UNIT_TEST_ENABLE
    #define STV_VIRTUAL virtual
#else
    #define STV_VIRTUAL
#endif
/// @}

/// @defgroup logic_utils Логические утилиты
/// @{

/// @brief Проверка истинности всех аргументов (логическое И).
/// @details Функция использует fold expression (выражение свертки) C++17
/// для вычисления логического conjunction над всеми переданными
/// аргументами.
///
/// @see Шаблоны C++. Справочник разработчика, 2-е издание - 2018, 4.2.
/// Выражения свертки.
///
/// @tparam Args Вариативный шаблонный параметр. Типы аргументов должны быть
/// явно или неявно приводимы к bool.
///
/// @param[in] args Входная последовательность булевых аргументов.
///
/// @return `true`, если все аргументы в пакете `Args` истинны.
/// @return `false`, если хотя бы один аргумент ложен.
///
/// @code
/// bool result = stv::all_true(true, true, false); // false
/// @endcode
template<typename... Args>
auto all_true(
    Args... args)
{ return (... && args); }

/// @brief Проверка истинности хотя бы одного аргумента (логическое ИЛИ).
/// @details Функция использует fold expression для вычисления логического
/// disjunction над всеми переданными аргументами.
///
/// @tparam Args Вариативный шаблонный параметр. Типы аргументов должны быть
/// явно или неявно приводимы к bool.
///
/// @param[in] args Входная последовательность аргументов.
///
/// @return `true`, если хотя бы один из входных параметров равен `true`.
/// @return `false`, если все аргументы ложны.
///
/// @code
/// bool result = stv::one_true(false, false, true); // true
/// @endcode
template<typename... Args>
auto one_true(
    Args... args)
{ return (... || args); }

/// @}

/// @defgroup math_utils Математические утилиты
/// @{

/// @brief Вычисление Евклидовой нормы (L2 norm) для контейнера.
/// @details Функция вычисляет квадратный корень из суммы квадратов
/// элементов контейнера. Формула: \f$ \sqrt{\sum_{i=0}^{n} x_i^2} \f$
///
/// @tparam Container Тип контейнера, удовлетворяющий требованию
/// `std::ranges::range`. Элементы контейнера должны поддерживать операции
/// умножения и сложения.
///
/// @param[in] container Ссылка на контейнер с данными.
///
/// @return Вычисленное значение нормы. Тип возвращаемого значения
/// определяется результатом работы `std::sqrt` над типом элемента
/// контейнера.
///
/// @note Начальное значение суммы устанавливается в `value_type{0}`.
/// Убедитесь, что тип элемента поддерживает конструирование из нуля.
/// @warning Возможна потеря точности или переполнение при возведении в
/// квадрат больших значений перед суммированием.
template<std::ranges::range Container>
auto norm(
    const Container &container)
{
    using value_type = std::ranges::range_value_t<Container>;
    auto sum_sq      = std::accumulate(
        std::begin(container), std::end(container), value_type{0},
        [](const auto &acc, const auto &axis) { return acc + (axis * axis); });
    return std::sqrt(sum_sq);
}

/// @brief Вычисление Евклидовой нормы (L2 norm) для вариативного списка
/// аргументов.
/// @details Перегруженная версия функции norm, принимающая отдельные
/// аргументы. Вычисляет квадратный корень из суммы квадратов переданных
/// аргументов. Формула: \f$ \sqrt{a^2 + b^2 + ... + n^2} \f$
///
/// @tparam Args Вариативный шаблонный параметр.
///
/// @param[in] args Набор аргументов, представляющих координаты или
/// значения.
///
/// @return Вычисленное значение нормы.
///
/// @warning Функция содержит static_assert проверки:
/// - Требуется хотя бы один аргумент.
/// - Все аргументы должны быть арифметическими типами (arithmetic types).
///
/// @code
/// double len = stv::norm(3.0, 4.0); // 5.0
/// @endcode
template<typename... Args>
auto norm(
    Args... args)
{
    static_assert(sizeof...(Args) > 0, "At least one argument required");
    static_assert((std::is_arithmetic_v<Args> && ...),
                  "All arguments must be arithmetic types");

    return std::sqrt(((args * args) + ...));
}

template<std::floating_point T>
constexpr bool is_equal(
    const T first, const T second,
    const T epsilon = std::numeric_limits<T>::epsilon()) noexcept
{ return std::abs(first - second) < epsilon; }

template<typename T>
    requires(!std::floating_point<T> && std::integral<T>)
constexpr bool is_equal(
    const T first, const T second) noexcept
{ return first == second; }

template<typename T>
    requires(!std::floating_point<T>)
constexpr bool is_equal(
    const T &first, const T &second) noexcept
{ return first == second; }

/// @brief Re-maps a number from one range to another. That is, a value of
/// in_min would get mapped to out_min, a value of in_max to out_max, values
/// in-between to values in-between, etc.
///
/// @tparam T_IN: Type of input range. Compile, since C++17, can deduced this
/// type.
/// @tparam T_OUT: Type of output range. Compile, since C++17, can deduced this
/// type.
///
/// @param[in] x: Input value.
/// @param[in] in_min: Minimum range value for x.
/// @param[in] in_max: Maximum range value for x.
/// @param[in] out_min: Minimum range for output value.
/// @param[in] out_max: Maximum range for output value.
///
/// @return Return mapped x value, from [in_min; in_max] range to [out_min;
/// out_max].
template<typename T_IN, typename T_OUT>
    requires std::common_with<T_IN, T_OUT>
constexpr auto map(
    const T_IN input, const T_IN in_min, const T_IN in_max, const T_OUT out_min,
    const T_OUT out_max) -> T_OUT
{
    using common_t = std::common_type_t<T_IN, T_OUT>;
    return (static_cast<common_t>(input - in_min)
            * static_cast<common_t>(out_max - out_min)
            / static_cast<common_t>(in_max - in_min))
           + static_cast<common_t>(out_min);
}

/// @}

template<typename T>
[[nodiscard]] auto clamp_with_result(
    T &val, const T &min, const T &max)
{
    const T old_val = val;
    val             = std::clamp(val, min, max);

    if constexpr(std::is_floating_point_v<T>)
    {
        return std::abs(val - old_val) > static_cast<T>(0.001);
    }
    else
    {
        return old_val != val;
    }
}

template<typename T, typename U>
concept map_supported_types = std::common_with<T, U>;

} // namespace stv

#endif /* UTILS_HPP */
