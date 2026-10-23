/// @file type_list.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
///
/// NAME
///     stv::type_list -- минимальный typelist и constexpr-хелперы.
///
/// SYNOPSIS
///     template<typename... Ts>
///     struct type_list;
///
///     template<typename List>
///     inline constexpr std::size_t type_list_size_v;
///
///     template<std::size_t Index, typename List>
///     using type_list_element_t = ...;
///
///     template<typename List, typename Fn>
///     constexpr void type_list_for_each(Fn func);
///
///     template<typename T, typename List>
///     using type_list_prepend_t = ...;
///
///     template<typename List, template<typename> typename Pred>
///     using type_list_filter_t = ...;
///
///     template<typename Item, std::size_t N, typename Enum>
///     consteval auto enum_uniquely_covered(const std::array<Item, N> &,
///                                          Enum last) -> bool;
///
/// DESCRIPTION
///     Протоколонезависимые шаблоны-утилиты для типовых списков: список
///     типов type_list, размер type_list_size_v, доступ по индексу
///     type_list_element_t (через std::index_sequence, без рекурсии
///     шаблонов), fold-проход type_list_for_each, добавление в начало
///     type_list_prepend_t и фильтрация предикатом type_list_filter_t.
///
///     Зачем нужны метафункции списка типов: вопросы о составе протокола
///     (сколько сообщений, какое сообщение на позиции i, какие типы
///     удовлетворяют признаку) отвечаются компилятором на этапе сборки
///     при нулевой стоимости времени выполнения. Список типов становится
///     единственным перечнем сущностей: каталоги, таблицы обработчиков
///     и проверки выводятся из него constexpr-кодом, а расхождения
///     (дубликат идентификаторов, непокрытое значение перечисления,
///     обращение за границы) ловятся static_assert до запуска программы.
///     Ручные параллельные перечни (таблицы идентификаторов, проверки
///     размеров в обработчиках) отсутствуют.
///
///     Дополнительно -- consteval-проверка enum_uniquely_covered(): каждое
///     значение перечисления [0, to_underlying(last)) встречается в
///     заданном массиве ровно один раз (инвариант покрытия каталогов
///     сообщений и таблиц обработчиков полезной нагрузки: элементы массива
///     -- либо значения перечисления, либо пары (идентификатор, обработчик)).
///
/// EXAMPLE
///     Протокол обмена сообщениями, описанный единственным списком типов:
///     каталог дескрипторов строится из списка и специализаций
///     message_traits автоматически, метафункции отвечают на вопросы
///     о составе протокола в static_assert'ах, расширение протокола --
///     одна строка в списке.
///     ```cpp
///     #include "stv/communication/message_catalog.hpp"
///     #include "stv/communication/type_list.hpp"
///
///     // Сообщение протокола и его traits: идентификатор сообщения и
///     // минимальный размер полезной нагрузки (контракт описан в
///     // message_traits.hpp).
///     struct my_telemetry {
///         std::uint32_t counter{};
///         std::int16_t  temperature{};
///     };
///
///     template <>
///     struct stv::message_traits<my_telemetry> {
///         static constexpr my_msg_id   msg_id = my_msg_id::telemetry;
///         static constexpr std::size_t min_pload_size_bytes =
///             sizeof(std::uint32_t) + sizeof(std::int16_t);
///     };
///
///     struct my_command {
///         std::uint8_t code{};
///     };
///
///     template <>
///     struct stv::message_traits<my_command> {
///         static constexpr my_msg_id   msg_id = my_msg_id::command;
///         static constexpr std::size_t min_pload_size_bytes =
///             sizeof(my_command);
///     };
///
///     // Единственный перечень сообщений протокола: порядок типов задаёт
///     // порядок дескрипторов каталога.
///     using my_list = stv::type_list<my_telemetry, my_command>;
///
///     // Каталог генерируется из списка и traits -- дескрипторы не
///     // перечисляются вручную. Метафункции списка доступны для любых
///     // constexpr-проверок состава протокола.
///     constexpr auto my_catalog = stv::make_message_catalog<my_list>();
///     static_assert(stv::type_list_size_v<my_list> == my_catalog.size());
///     static_assert(std::same_as<stv::type_list_element_t<1, my_list>,
///                                my_command>);
///     static_assert(my_catalog[0].id == my_msg_id::telemetry);
///
///     // Расширение протокола -- одна строка в списке: каталог, проверки
///     // покрытия и таблица обработчиков обновляются без дополнительных
///     // правок (см. message_catalog.hpp).
///     struct my_status {
///         std::uint8_t flags{};
///     };
///     // ... специализация stv::message_traits<my_status> ...
///     using extended_list = stv::type_list_prepend_t<my_status, my_list>;
///     static_assert(stv::type_list_size_v<extended_list> == 3U);
///     ```
///     Достоверный учебный протокол из четырёх сообщений с разными
///     способами описания полезной нагрузки:
///     stv/communication/tests/demo_messages.hpp; сквозной обмен
///     сообщениями (передатчик и приёмник):
///     tests/test_message_catalog_communication.cpp.
///
/// SEE ALSO
///     message_catalog.hpp (каталог сообщений и сборка таблицы обработчиков
///     из списка типов),
///     stvlinkv2_kit.hpp (набор интеграции протокола stvlinkv2),
///     tests/test_type_list.cpp (проверки хелперов списка типов).

#ifndef TYPE_LIST_HPP
#define TYPE_LIST_HPP

#include <array>
#include <cstddef>
#include <type_traits>
#include <utility>

namespace stv {

/// @brief Список типов: основа типовых реестров и каталогов.
///
/// @details Сам по себе -- пустая структура-агрегат типов; всю работу
///     выполняют хелперы ниже (размер, доступ по индексу, проход,
///     фильтрация).
///
/// @par Пример
/// Типовой сценарий -- список типов сообщений одного протокола: каталог
/// сообщений, проверки покрытия и диспетчеризация сторон строятся из
/// списка автоматически (@see message_catalog.hpp).
/// @code{.cpp}
/// // Порядок типов задаёт порядок дескрипторов каталога. Точка
/// // расширения протокола -- новая строка в списке.
/// using my_list = stv::type_list<my_telemetry, my_command>;
/// @endcode
template<typename... Ts>
struct type_list {
};

namespace type_list_detail {

/// @brief Связывает тип элемента с его индексом в списке.
/// @tparam Index Индекс элемента.
/// @tparam T Тип элемента.
template<std::size_t Index, typename T>
struct indexed_entry {
    using type = T;

    /// @brief Выборка элемента по тегу-индексу: разрешается наследованием
    ///     (@see indexed_list).
    static auto get(std::integral_constant<std::size_t, Index> /*unused*/)
        -> indexed_entry<Index, T>;
};

/// @brief Список типов с пронумерованными индексами (наследование выборки).
///
/// @details Каждый элемент списка наследует от indexed_entry<Is, Ts>;
///     объявление using ...::get собирает перегрузки выборки всех
///     элементов в одну область видимости: вызов get() с тегом-индексом
///     разрешается уникальной перегрузкой элемента с этим индексом.
template<typename Sequence, typename List>
struct indexed_list;

template<std::size_t... Is, typename... Ts>
struct indexed_list<std::index_sequence<Is...>, type_list<Ts...>>:
    indexed_entry<Is, Ts>... {
    using indexed_entry<Is, Ts>::get...;
};

} // namespace type_list_detail

/// @brief Число элементов в списке типов.
///
/// Пример:
/// @code{.cpp}
/// using my_list = stv::type_list<my_telemetry, my_command>;
/// static_assert(stv::type_list_size_v<my_list> == 2U);
/// @endcode
template<typename List>
inline constexpr std::size_t type_list_size_v = 0U;

template<typename... Ts>
inline constexpr std::size_t type_list_size_v<type_list<Ts...>> = sizeof...(Ts);

/// @brief Тип элемента списка с заданным индексом.
///
/// @details Доступ выполняется нумерацией элементов списка через
///     std::index_sequence и выборкой перегрузкой: рекурсия шаблонов
///     не используется.
///
/// @tparam Index Индекс элемента (должен быть в границах списка).
/// @tparam List Список типов (type_list).
template<std::size_t Index, typename List>
struct type_list_element {
  private:
    using indexed = type_list_detail::indexed_list<
        std::make_index_sequence<type_list_size_v<List>>, List>;

  public:
    using type = typename decltype(indexed::get(
        std::integral_constant<std::size_t, Index>{}))::type;
};

/// @brief Тип элемента списка с заданным индексом (синоним).
///
/// Пример:
/// @code{.cpp}
/// using my_list = stv::type_list<my_telemetry, my_command>;
/// static_assert(
///     std::same_as<stv::type_list_element_t<0, my_list>, my_telemetry>);
/// static_assert(
///     std::same_as<stv::type_list_element_t<1, my_list>, my_command>);
/// @endcode
///
/// @tparam Index Индекс элемента.
/// @tparam List Список типов (type_list).
template<std::size_t Index, typename List>
using type_list_element_t = typename type_list_element<Index, List>::type;

/// @brief Применяет функцию к каждому типу списка (fold-проход).
///
/// @par Пример
/// Обобщённая лямбда получает каждый тип списка через шаблонный
/// operator(); пример -- суммарный минимальный размер полезной нагрузки
/// всех сообщений протокола:
/// @code{.cpp}
/// using my_list = stv::type_list<my_telemetry, my_command>;
///
/// constexpr auto total_min_size = [] {
///     std::size_t sum = 0U;
///     stv::type_list_for_each<my_list>([&sum]<typename T> {
///         sum += stv::message_traits<T>::min_pload_size_bytes;
///     });
///     return sum;
/// }();
/// static_assert(total_min_size > 0U);
/// @endcode
///
/// @tparam List Список типов (type_list).
/// @tparam Fn Тип функции: вызывается как func.template operator()<T>().
/// @param[in] func Функция, применяемая к каждому типу списка.
template<typename List, typename Fn>
constexpr void type_list_for_each(
    Fn func)
{
    []<typename... Ts>(type_list<Ts...>, Fn inner_func) {
        (inner_func.template operator()<Ts>(), ...);
    }(List{}, func);
}

/// @brief Добавляет тип в начало списка типов.
/// @tparam T Добавляемый тип.
/// @tparam List Список типов (type_list).
template<typename T, typename List>
struct type_list_prepend;

template<typename T, typename... Ts>
struct type_list_prepend<T, type_list<Ts...>> {
    using type = type_list<T, Ts...>;
};

/// @brief Добавляет тип в начало списка типов (синоним).
///
/// Пример:
/// @code{.cpp}
/// using my_list = stv::type_list<my_command>;
/// using extended = stv::type_list_prepend_t<my_telemetry, my_list>;
/// static_assert(std::same_as<extended,
///                            stv::type_list<my_telemetry, my_command>>);
/// @endcode
///
/// @tparam T Добавляемый тип.
/// @tparam List Список типов (type_list).
template<typename T, typename List>
using type_list_prepend_t = typename type_list_prepend<T, List>::type;

/// @brief Фильтрует список типов предикатом.
/// @tparam List Список типов (type_list).
/// @tparam Pred Мета-предикат: шаблон с членом static constexpr bool value.
template<typename List, template<typename> typename Pred>
struct type_list_filter {
    using type = type_list<>;
};

template<typename Head, typename... Tail, template<typename> typename Pred>
struct type_list_filter<type_list<Head, Tail...>, Pred> {
  private:
    using rest = typename type_list_filter<type_list<Tail...>, Pred>::type;

  public:
    using type = std::conditional_t<Pred<Head>::value,
                                    type_list_prepend_t<Head, rest>, rest>;
};

/// @brief Фильтрует список типов предикатом (синоним).
///
/// @par Пример
/// Мета-предикат -- шаблон с членом static constexpr bool value.
/// Пример: отбор тривиально копируемых сообщений протокола (читаемых
/// декодером побайтовым копированием):
/// @code{.cpp}
/// template<typename T>
/// struct is_trivially_stored {
///     static constexpr bool value = std::is_trivially_copyable_v<T>;
/// };
///
/// // my_telemetry -- тривиально копируемая структура, my_status_text
/// // содержит std::string_view и тривиально копируемой не является.
/// using my_list = stv::type_list<my_telemetry, my_status_text>;
/// using trivial = stv::type_list_filter_t<my_list, is_trivially_stored>;
/// static_assert(std::same_as<trivial, stv::type_list<my_telemetry>>);
/// @endcode
///
/// @tparam List Список типов (type_list).
/// @tparam Pred Мета-предикат: шаблон с членом static constexpr bool value.
template<typename List, template<typename> typename Pred>
using type_list_filter_t = typename type_list_filter<List, Pred>::type;

namespace type_list_detail {

/// @brief Возвращает идентификатор покрытия элемента проверяемого массива.
///
/// @details Элемент принимает две формы: само значение перечисления
///     (массив идентификаторов) или пара (идентификатор, обработчик)
///     таблицы обработчиков полезной нагрузки стороны -- проверяется поле
///     .first. Форма выбирается концептом на этапе компиляции.
///
/// @tparam Enum Перечислимый тип идентификаторов.
/// @tparam Item Тип элемента массива: Enum или пара (Enum, ...).
/// @param[in] item Элемент проверяемого массива.
/// @return Идентификатор сообщения элемента.
template<typename Enum, typename Item>
consteval auto coverage_key(
    const Item &item) -> Enum
{
    if constexpr(requires { item.first; })
    {
        return static_cast<Enum>(item.first);
    }
    else
    {
        return static_cast<Enum>(item);
    }
}

} // namespace type_list_detail

/// @brief Проверяет, что каждое значение перечисления встречается один раз.
///
/// @details Каждое значение [0, to_underlying(last)) должно встречаться в
///     values ровно один раз: ни одно значение не потеряно и не
///     продублировано. Проверка выполняется на этапе компиляции
///     (consteval) циклом по raw-значениям с подсчётом вхождений.
///
///     Элементы values принимают две формы: значения перечисления (массив
///     идентификаторов каталога) или пары (идентификатор, обработчик)
///     таблицы обработчиков полезной нагрузки стороны -- ключи извлекаются
///     автоматически (@see type_list_detail::coverage_key()). Таблица,
///     покрывающая каждое значение перечисления (сторона обрабатывает
///     каждое сообщение протокола), проверяется напрямую, без построения
///     промежуточного массива идентификаторов.
///
/// @tparam Item Тип элемента массива: Enum или пара (Enum, ...).
/// @tparam N Размер массива значений.
/// @tparam Enum Перечислимый тип.
/// @param[in] values Массив значений перечисления или массив пар
///     (идентификатор, обработчик).
/// @param[in] last Граница покрытия: значение, следующее за последним
///     проверяемым (обычно элемент max перечисления).
/// @return true, если каждое значение [0, to_underlying(last)) встречается
///     в values ровно один раз.
///
/// @par Пример
/// Фиксация инварианта каталога сообщений протокола: каждое значение
/// перечисления идентификаторов имеет ровно один дескриптор в каталоге
/// (ни одно сообщение не потеряно и не продублировано). Идентификаторы
/// каталога извлекаются хелпером catalog_message_ids()
/// (см. message_catalog.hpp):
/// @code{.cpp}
/// // Не компилируется, если каталог потерял или продублировал значение
/// // перечисления идентификаторов.
/// static_assert(stv::enum_uniquely_covered(
///     stv::catalog_message_ids(my_catalog), my_msg_id::max));
/// @endcode
///
/// Таблица обработчиков полезной нагрузки стороны, покрывающая каждое
/// значение перечисления (сторона обрабатывает каждое сообщение
/// протокола), проверяется напрямую:
/// @code{.cpp}
/// static_assert(
///     stv::enum_uniquely_covered(my_pload_handlers, my_msg_id::max));
/// @endcode
///
/// Достоверный пример из интегрирующего проекта:
/// kraslibs/tablet/message_registry.hpp.
template<typename Item, std::size_t N, typename Enum>
    requires std::is_enum_v<Enum>
consteval auto enum_uniquely_covered(
    const std::array<Item, N> &values, const Enum last) -> bool
{
    for(std::underlying_type_t<Enum> raw = 0; raw < std::to_underlying(last);
        ++raw)
    {
        std::size_t count = 0U;
        for(const auto &value: values)
        {
            if(std::to_underlying(type_list_detail::coverage_key<Enum>(value))
               == raw)
            {
                ++count;
            }
        }
        if(count != 1U)
        {
            return false;
        }
    }

    return true;
}

} // namespace stv

#endif // TYPE_LIST_HPP
