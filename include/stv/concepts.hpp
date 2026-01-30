#ifndef CONCEPTS_HPP
#define CONCEPTS_HPP

#include <chrono>
#include <concepts>
#include <ranges>
#include <type_traits>

namespace stv {

/// @brief Проверяет что переданный тип поддерживает общий для фильтров API.
///
/// TBase Тип данных который нужно проверить на наличие необходимого API.
template<typename TBase>
concept filterable_concept =
    requires(TBase base, typename TBase::setup_type setup) {
        typename TBase::value_type;
        typename TBase::setup_type;
        typename TBase::container_type;

        { base.setup(setup) };
        { base.reset() };
        { base.get_setup() };
        { base.get_default_setup() };
    };

/// @brief TBase должен предоставлять псевдоним TBase::container_type для того
/// чтобы в базовый класс TBase передать указатели на выделенную область памяти.
template<typename TBase>
concept sizeable_container_concept =
    requires(TBase base, typename TBase::setup_type setup) {
        typename TBase::container_type;
    };

/// @brief Проверяет, является ли тип TMutex типом, поддерживающим вызов в
/// контексте прерывания.
template<typename TMutex>
concept is_mutex_with_isr_concept = requires(TMutex &mutex) {
    mutex.lock(bool{});
    mutex.unlock(bool{});
};

/// @brief Проверяет, поддерживает ли TMutex вызов из контекста прерываний.
template<typename TMutex>
concept is_mutex_concept = requires(TMutex &mutex) {
    mutex.lock();
    mutex.unlock();
} && !is_mutex_with_isr_concept<TMutex>;

/// @brief Концепт проверяет, что тип Т является контейнером, который хранит
/// тривиальные объекты в непрерывном фрагменте памяти.
template<typename T>
concept contiguous_trivial_container_concept =
    std::ranges::contiguous_range<T>
    && std::is_trivial_v<std::ranges::range_value_t<T>> && requires(T src) {
           src.data();
           src.size();
           src.begin();
           src.end();
       };
;

#ifndef __APPLE__
template<typename T>
// NOLINTNEXTLINE(readability-identifier-naming)
inline constexpr bool is_duration_v = std::chrono::__is_duration_v<T>;
#else
template<typename T>
// NOLINTNEXTLINE(readability-identifier-naming)
inline constexpr bool is_duration_v = true;
#endif

/// @brief Концепт проверяет, что тип T является контейнером с непрерывно
/// выделенной памятью.
template<typename T>
concept contiguous_container_concept =
    std::ranges::contiguous_range<T> && requires(T src) {
        src.data();
        src.size();
        src.begin();
        src.end();
    };

} // namespace stv

#endif /* CONCEPTS_HPP */
