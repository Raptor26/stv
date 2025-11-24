#ifndef WRAPPERS_HPP
#define WRAPPERS_HPP

#include "filters_concepts.hpp"

namespace stv {

/// @brief
///
/// @note Конструктор TBase должен принимать 2 аргумента:
/// 1. TBase::SetupType по ссылке;
/// 2. gsl::span<TBase::ValueType>
///
/// @note Полный набор требований к TBase вы можете найти в stv::Filterable.
///
/// @tparam TBase
/// @tparam SIZE
template<stv::Filterable TBase, std::size_t SIZE = 20>
class SizeWrapper:
    private std::array<typename TBase::ValueType, SIZE>,
    public TBase
{
  protected:
    using ValueType     = typename TBase::ValueType;
    using SetupType     = typename TBase::SetupType;
    using ContainerType = typename TBase::ContainerType;

  public:
    template<typename U>
        requires std::same_as<std::remove_cvref_t<U>, SetupType>
    SizeWrapper(
        U &&attr):
        std::array<ValueType, SIZE>{},
        TBase{std::forward<U>(attr), ContainerType{this->begin(), this->end()}}
    {
    }

    /// @brief Деструктор.
    ///
    /// @note Этот деструктор виртуальный, чтобы обеспечить правильную очистку
    /// объектов производного класса при удалении через указатель базового
    /// класса.
    ~SizeWrapper() override = default;

    // Конструктор перемещения - доступен только если TBase перемещаемый
    SizeWrapper(SizeWrapper &&other)
        noexcept(std::is_nothrow_move_constructible_v<TBase>)
        requires std::is_move_constructible_v<TBase>
    = default;

    // Оператор присваивания перемещения - доступен только если TBase
    // перемещаемый
    auto operator=(SizeWrapper &&other)
        noexcept(std::is_nothrow_move_assignable_v<TBase>) -> SizeWrapper &
        requires std::is_move_assignable_v<TBase>
    = default;

    // Конструктор копирования - доступен только если TBase копируемый
    SizeWrapper(const SizeWrapper &other)
        requires std::is_copy_constructible_v<TBase>
    = default;

    // Оператор присваивания копирования - доступен только если TBase копируемый
    auto operator=(const SizeWrapper &other) -> SizeWrapper &
        requires std::is_copy_assignable_v<TBase>
    = default;

    operator bool() const { return static_cast<bool>(TBase::operator bool()); }
};

} // namespace stv

#endif /* WRAPPERS_HPP */
