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
    requires(TBase base, typename TBase::SetupType setup) {
        typename TBase::ValueType;
        typename TBase::SetupType;
        typename TBase::ContainerType;

        { base.Setup(setup) };
        { base.Reset() };
        { base.GetSetup() };
        { base.GetDefaultParams() };
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
