#ifndef WRAPPERS_HPP
#define WRAPPERS_HPP

#include "stv/concepts.hpp"

namespace stv {

/// @brief
///
/// @note Конструктор TBase должен принимать 2 аргумента:
/// 1. TBase::setup_type по ссылке;
/// 2. gsl::span<TBase::value_type>
///
/// @note Полный набор требований к TBase вы можете найти в
/// stv::sizeable_container_concept.
///
/// @tparam TBase
/// @tparam SIZE
template<stv::sizeable_container_concept TBase, std::size_t SIZE = 20>
class container_size_wrapper:
    private std::array<typename TBase::value_type, SIZE>,
    public TBase
{
  protected:
    using value_type     = typename TBase::value_type;
    using setup_type     = typename TBase::setup_type;
    using container_type = typename TBase::container_type;

  public:
    template<typename U>
        requires std::same_as<std::remove_cvref_t<U>, setup_type>
    explicit container_size_wrapper(
        U &&attr):
        std::array<value_type, SIZE>{},
        TBase{std::forward<U>(attr), container_type{this->begin(), this->end()}}
    {
    }

    /// @brief Деструктор.
    ///
    /// @note Этот деструктор виртуальный, чтобы обеспечить правильную очистку
    /// объектов производного класса при удалении через указатель базового
    /// класса.
    ~container_size_wrapper() override = default;

    // Конструктор перемещения - доступен только если TBase перемещаемый
    container_size_wrapper(container_size_wrapper &&other)
        noexcept(std::is_nothrow_move_constructible_v<TBase>)
        requires std::is_move_constructible_v<TBase>
    = default;

    // Оператор присваивания перемещения - доступен только если TBase
    // перемещаемый
    auto operator=(container_size_wrapper &&other)
        noexcept(std::is_nothrow_move_assignable_v<TBase>)
            -> container_size_wrapper &
        requires std::is_move_assignable_v<TBase>
    = default;

    // Конструктор копирования - доступен только если TBase копируемый
    container_size_wrapper(const container_size_wrapper &other)
        requires std::is_copy_constructible_v<TBase>
    = default;

    // Оператор присваивания копирования - доступен только если TBase копируемый
    auto operator=(const container_size_wrapper &other)
        -> container_size_wrapper &
        requires std::is_copy_assignable_v<TBase>
    = default;

    explicit operator bool() const
    {
        return static_cast<bool>(TBase::operator bool());
    }
};

} // namespace stv

#endif /* WRAPPERS_HPP */
