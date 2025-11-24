#ifndef CONCEPTS_HPP
#define CONCEPTS_HPP

#include "boost/leaf.hpp"
#include <concepts>

namespace stv {

template<typename TBase>
concept Filterable = requires(TBase base, typename TBase::SetupType setup) {
    typename TBase::ValueType;
    typename TBase::SetupType;
    typename TBase::ContainerType;

    { base.Setup(setup) };
    { base.Reset() };
    { base.GetSetup() };
    { base.GetDefaultParams() };
};

} // namespace stv

#endif /* CONCEPTS_HPP */
