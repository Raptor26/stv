/// @file wrappers.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
///
/// NAME
///     stv_wrappers
///
/// DESCRIPTION
///     stv_wrappers предоставляет шаблонные обертки для размещения объектов
///     встроенных (embedded) контейнеров в статической памяти без
///     динамического выделения ресурсов. Библиотека содержит два основных
///     компонента:
///     - memory_storage_proxy<T, SIZE>: вспомогательный класс, хранящий
///       std::array<T, SIZE> и предоставляющий итераторы begin(), end() и
///       указатель data().
///     - container_size_wrapper<TBase, SIZE>: обертка, которая совместно
///       наследуется от memory_storage_proxy и TBase, и автоматически
///       передает в конструктор TBase параметры настройки и span на
///       предварительно выделенный массив.
///
///     TBase должен удовлетворять концепции stv::sizeable_container_concept,
///     определять вложенные типы value_type, setup_type, container_type, а
///     также принимать в конструкторе два аргумента: setup_type по ссылке и
///     container_type, указывающий на область памяти. Шаблонный параметр SIZE
///     задает количество элементов value_type и должен быть больше нуля.
///
///     container_size_wrapper поддерживает конструкторы копирования и
///     перемещения, а также соответствующие операторы присваивания, но только
///     в том случае, если они доступны у базового типа TBase. Деструктор
///     объявлен виртуальным, что позволяет безопасно удалять объекты через
///     указатель на базовый класс. Оператор operator bool() делегирует
///     проверку состояния базовому классу TBase.
///
///     Обеспечивается отсутствие динамического выделения памяти, что делает
///     обертку пригодной для систем с жесткими требованиями к реальному
///     времени и встроенных приложений.
///
/// EXAMPLE
///     Прямое использование container_size_wrapper с базовым классом
///     фильтра скользящего среднего:
///     ```cpp
///     #include "stv/filters/moving_average.hpp"
///     #include <iostream>
///
///     int main() {
///         using namespace stv;
///
///         using Setup = moving_average_setup<float, empty_mutex>;
///         using Base  = moving_average_base<Setup>;
///
///         // Создать фильтр с буфером на 20 элементов напрямую через
///         // обертку. Внутри нее выделяется std::array<float, 20>.
///         container_size_wrapper<Base, 20> filter{
///             Setup{.window_width = 5U}};
///         if (!filter) {
///             return -1;
///         }
///
///         float samples[] = {1.0F, 2.0F, 3.0F, 4.0F, 5.0F};
///         for (auto sample : samples) {
///             std::cout << filter.filt(sample) << "\n";
///         }
///
///         return 0;
///     }
///     ```
///
///     Аналогично можно обернуть и другие базовые классы набора stv,
///     например lwrb_base, передавая в шаблоне размер буфера в байтах.
///
///     Более подробные примеры и проверки находятся в тестах фильтра
///     moving_average и кольцевого буфера lwrb.

#ifndef WRAPPERS_HPP
#define WRAPPERS_HPP

#include "stv/concepts.hpp"
#include <array>

namespace stv {

template<typename T, std::size_t SIZE>
class memory_storage_proxy
{
    std::array<T, SIZE> storage_;

  public:
    [[nodiscard]] auto begin() { return storage_.begin(); }

    [[nodiscard]] auto begin() const { return storage_.begin(); }

    [[nodiscard]] auto end() { return storage_.end(); }

    [[nodiscard]] auto end() const { return storage_.end(); }

    [[nodiscard]] auto data() { return storage_.data(); }
};

/// NOLINTBEGIN(misc-multiple-inheritance)

/// @brief Обертка контейнера, предоставляющая выделенную память.
///
/// @note Конструктор TBase должен принимать 2 аргумента:
/// 1. TBase::setup_type по ссылке;
/// 2. gsl::span<TBase::value_type>
///
/// @note Полный набор требований к TBase вы можете найти в
/// stv::sizeable_container_concept.
///
/// @tparam TBase Тип базового класса для которого необходимо предоставить
/// обертку.
/// @tparam SIZE Количество элементов контейнера, под которые необходимо
/// выделить память.
template<stv::sizeable_container_concept TBase, std::size_t SIZE = 20>
class container_size_wrapper:
    public stv::memory_storage_proxy<typename TBase::value_type, SIZE>,
    public TBase
{
  protected:
    using value_type     = typename TBase::value_type;
    using setup_type     = typename TBase::setup_type;
    using container_type = typename TBase::container_type;

    using buffer_type =
        stv::memory_storage_proxy<typename TBase::value_type, SIZE>;

    static_assert(SIZE > 0, "Template SIZE must be greater then 0");

  public:
    template<typename U>
        requires std::same_as<std::remove_cvref_t<U>, setup_type>
    explicit container_size_wrapper(
        U &&attr):
        buffer_type{},
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
    { return static_cast<bool>(TBase::operator bool()); }
};

/// NOLINTEND(misc-multiple-inheritance)

} // namespace stv

#endif /* WRAPPERS_HPP */
