#ifndef CONCEPTS_HPP
#define CONCEPTS_HPP

#include "boost/leaf.hpp"
#include <concepts>

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

} // namespace stv

#endif /* CONCEPTS_HPP */
