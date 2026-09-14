/// @file register_field.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// @brief Шаблонные функции для работы с битовыми полями регистров.
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
///
/// @details Этот заголовочный файл содержит набор constexpr-функций для
/// типобезопасной сериализации и парсинга битовых полей регистров
/// периферийных устройств (датчиков). Функции инкапсулируют повторяющиеся
/// конструкции вида `(reg >> offset) & mask` и `field << offset`, выполняя
/// все операции в домене `std::uint32_t` с беззнаковыми операндами. Это
/// исключает предупреждения анализаторов `bugprone-signed-bitwise` и
/// `hicpp-signed-bitwise` и гарантирует одинаковый результат независимо от
/// ширины типа регистра (`std::uint8_t` или `std::byte`) благодаря
/// integer promotion.

#ifndef REGISTER_FIELD_HPP
#define REGISTER_FIELD_HPP

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace stv {

namespace detail {

/// @brief Приводит значение регистра, маски или поля к `std::uint32_t`.
/// @details Поддерживает беззнаковые целые типы и перечисления (scoped enum
/// приводится через `static_cast` к беззнаковому целому). Специальный случай
/// для `std::byte` обрабатывается через `std::to_integer`.
/// @tparam T Тип приводимого значения.
/// @param value Значение для приведения.
/// @return Значение, приведённое к `std::uint32_t`.
template<typename T>
[[nodiscard]] constexpr std::uint32_t to_u32(
    T value) noexcept
{
    if constexpr(std::is_same_v<std::remove_cv_t<T>, std::byte>)
    {
        return std::to_integer<std::uint32_t>(value);
    }
    else
    {
        return static_cast<std::uint32_t>(value);
    }
}

} // namespace detail

/// @brief Проверяет установленность одного бита в значении регистра.
/// @details Эквивалентна выражению
/// `(reg & (std::uint32_t{1} << offset)) != 0`, но выполняется полностью в
/// беззнаковом домене `std::uint32_t`.
/// @tparam Reg Тип значения регистра (беззнаковое целое или `std::byte`).
/// @param reg Значение регистра.
/// @param offset Позиция проверяемого бита (0–31).
/// @return true, если бит в позиции offset установлен.
template<typename Reg>
[[nodiscard]] constexpr bool is_bit_set(
    Reg reg, unsigned offset) noexcept
{ return (detail::to_u32(reg) & (std::uint32_t{1} << offset)) != 0U; }

/// @brief Извлекает сырое значение поля регистра по смещению и маске.
/// @details Эквивалентна выражению `(reg >> offset) & mask`, но выполняется
/// полностью в беззнаковом домене `std::uint32_t`.
/// @tparam Reg Тип значения регистра (беззнаковое целое или `std::byte`).
/// @tparam Mask Тип маски (беззнаковое целое или `std::byte`).
/// @param reg Значение регистра.
/// @param offset Позиция младшего бита поля (0–31).
/// @param mask Маска поля (без смещения).
/// @return Извлечённое сырое значение поля.
template<typename Reg, typename Mask>
[[nodiscard]] constexpr std::uint32_t field_raw(
    Reg reg, unsigned offset, Mask mask) noexcept
{ return (detail::to_u32(reg) >> offset) & detail::to_u32(mask); }

/// @brief Извлекает типизированное поле регистра по смещению и маске.
/// @details Комбинация `field_raw` с приведением результата к целевому
/// типу поля (обычно scoped enum). Значение бит-в-бит совпадает с выражением
/// `static_cast<Field>((reg >> offset) & mask)`.
/// @tparam Field Тип извлекаемого поля (перечисление или беззнаковое целое).
/// @tparam Reg Тип значения регистра (беззнаковое целое или `std::byte`).
/// @tparam Mask Тип маски (беззнаковое целое или `std::byte`).
/// @param reg Значение регистра.
/// @param offset Позиция младшего бита поля (0–31).
/// @param mask Маска поля (без смещения).
/// @return Извлечённое значение поля, приведённое к типу Field.
template<typename Field, typename Reg, typename Mask>
[[nodiscard]] constexpr Field extract_field(
    Reg reg, unsigned offset, Mask mask) noexcept
{ return static_cast<Field>(field_raw(reg, offset, mask)); }

/// @brief Парсит однобитовый флаг регистра в перечисление.
/// @details Выбирает значение `on`, если бит в позиции offset установлен,
/// иначе — значение `off`. Эквивалентна выражению
/// `(reg & (std::uint32_t{1} << offset)) ? on : off`.
/// @tparam Reg Тип значения регистра (беззнаковое целое или `std::byte`).
/// @tparam Enum Тип перечисления поля.
/// @param reg Значение регистра.
/// @param offset Позиция бита флага (0–31).
/// @param enabled Значение перечисления для установленного бита.
/// @param disabled Значение перечисления для сброшенного бита.
/// @return Значение перечисления, соответствующее состоянию бита.
template<typename Reg, typename Enum>
[[nodiscard]] constexpr Enum parse_flag(
    Reg reg, unsigned offset, Enum enabled, Enum disabled) noexcept
{ return is_bit_set(reg, offset) ? enabled : disabled; }

/// @brief Сериализует значение поля для записи в регистр.
/// @details Эквивалентна выражению `field << offset` в домене
/// `std::uint32_t`. Результат предназначен для объединения с другими полями
/// операцией `|` с последующим приведением к типу регистра.
/// @tparam Field Тип поля (перечисление или беззнаковое целое).
/// @param field Значение поля.
/// @param offset Позиция младшего бита поля (0–31).
/// @return Значение поля, сдвинутое на offset бит.
template<typename Field>
[[nodiscard]] constexpr std::uint32_t field_to_raw(
    Field field, unsigned offset) noexcept
{ return detail::to_u32(field) << offset; }

} // namespace stv

#endif /* REGISTER_FIELD_HPP */
