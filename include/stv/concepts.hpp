#ifndef CONCEPTS_HPP
#define CONCEPTS_HPP

#include "boost/leaf.hpp"
#include <chrono>
#include <concepts>
#include <ranges>

namespace stv {

/// @brief
///
/// @tparam TBase
///
template<typename TBase>
concept FilterableConcept =
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
///
/// @tparam TBase
template<typename TBase>
concept SizeableContainerConcept =
    requires(TBase base, typename TBase::setup_type setup) {
        typename TBase::container_type;
    };

/// @brief
///
/// @tparam TMutex
///
template<typename TMutex>
concept IsMutexWithIsrConcept = requires(TMutex &mutex) {
    mutex.lock(bool{});
    mutex.unlock(bool{});
};

/// @brief
///
/// @tparam TMutex
///
template<typename TMutex>
concept IsMutexConcept = requires(TMutex &mutex) {
    mutex.lock();
    mutex.unlock();
} && !IsMutexWithIsrConcept<TMutex>;

template<typename T>
concept contiguous_trivial_container_concept =
    std::ranges::contiguous_range<T>
    && std::is_trivial_v<std::ranges::range_value_t<T>>;

template<typename T>
inline constexpr bool is_duration_v = std::chrono::__is_duration_v<T>;

} // namespace stv

#endif /* CONCEPTS_HPP */
