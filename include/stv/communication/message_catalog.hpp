/// @file message_catalog.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
///
/// NAME
///     stv::message_catalog -- каталог сообщений и диспетчерная таблица.
///
/// DESCRIPTION
///     Протоколонезависимая механика типового реестра входящих сообщений
///     интегрирующего проекта: генерация каталога дескрипторов
///     make_message_catalog() из списка типов сообщений и специализаций
///     stv::message_traits, проверка полноты приёмной логики стороны
///     check_message_coverage() и сборщик хэш-таблицы обработчиков
///     make_message_dispatcher_table().
///
///     Каждый дескриптор каталога строится из traits своего типа
///     (идентификатор сообщения и минимальный размер полезной нагрузки);
///     ручные перечни дескрипторов отсутствуют. Тип ключа каталога выводится
///     из идентификатора первого типа списка и обязан совпадать у всех
///     типов списка (проверяется static_assert).
///
///     Терминология: обработчик полезной нагрузки -- функция стороны вида
///     bool(stvlink_message_span): указатель на обработчик полезной
///     нагрузки, соответствующей указанному идентификатору сообщения
///     (msg_id); собственного состояния не имеет (состояние хранит сторона:
///     прошивка или приложение) и является единицей приёмной логики стороны.
///     Таблица обработчиков полезной нагрузки -- constexpr-массив пар
///     (msg_id, указатель на обработчик полезной нагрузки), единая точка
///     регистрации приёмной логики: сообщение обрабатывается обработчиком
///     полезной нагрузки, осознанно игнорируется или ловится static_assert
///     покрытия. Сборщик make_message_dispatcher_table() оборачивает каждый
///     обработчик полезной нагрузки проверкой min_pload_size_bytes его
///     дескриптора и регистрирует результат как обработчик кадра диспетчера
///     (обработчик полезной нагрузки -- функция стороны -- не путать с
///     обработчиком кадра: обёрткой guarded_handler, зарегистрированной в
///     таблице диспетчера, тип stvlink_message_handler_fnc_type).
///
///     Правило обратной совместимости по минимальному размеру: новые поля
///     дополняются только в конец структур, старые кадры десериализуются по
///     префиксу, минимальный размер -- смещение последнего обязательного
///     поля (для сообщения без полей -- 0: любой размер принимается, лишние
///     байты больших кадров игнорируются по принципу обратной совместимости
///     из спецификации CRSF).
///
///     Полнота приёмной логики стороны проверяется constexpr-функцией
///     check_message_coverage(): каждый идентификатор каталога должен быть
///     покрыт либо обработчиком полезной нагрузки стороны (пара
///     (идентификатор, обработчик) в таблице обработчиков полезной
///     нагрузки), либо явной записью в списке осознанно игнорируемых
///     идентификаторов стороны -- непокрытый идентификатор ловится
///     static_assert рядом с таблицей обработчиков полезной нагрузки,
///     молчаливая потеря входящих сообщений исключена.
///
///     Сборщик таблицы обработчиков make_message_dispatcher_table(): для
///     каждого дескриптора каталога регистрируется обработчик, оборачивающий
///     обработчик полезной нагрузки стороны (приёмная логика прошивки или
///     приложения) проверкой min_pload_size_bytes; идентификатор без
///     обработчика полезной нагрузки регистрируется безопасным
///     обработчиком-игнором (true).
///
///     Точка расширения: новое входящее сообщение протокола добавляется
///     структурой полезной нагрузки со специализацией stv::message_traits,
///     строкой типа в списке сообщений и обработчиком полезной нагрузки или
///     записью осознанного игнора на каждой стороне -- без правок
///     диспетчеров и без ручных проверок размера полезной нагрузки в
///     обработчиках. Дубликат
///     идентификатора сообщения в списке ловится static_assert на этапе
///     компиляции.
///
/// SEE ALSO
///     type_list.hpp (type_list и constexpr-хелперы списков типов),
///     message_traits.hpp (первичный шаблон и концепты),
///     stvlink_dispatcher.hpp (типы обработчика и полезной нагрузки),
///     stvlinkv2_kit.hpp (тип хэш-таблицы stvlink_handler_table),
///     tests/demo_messages.hpp (учебный протокол: типы, traits, каталог),
///     tests/test_message_catalog_communication.cpp (сквозной пример
///     передающей и принимающей сторон обмена сообщениями).

#ifndef MESSAGE_CATALOG_HPP
#define MESSAGE_CATALOG_HPP

#include "gsl/gsl"
#include "stv/communication/message_traits.hpp"
#include "stv/communication/stvlink_dispatcher.hpp"
#include "stv/communication/type_list.hpp"
#include <array>
#include <concepts>
#include <cstddef>
#include <type_traits>
#include <utility>

namespace stv {

/// @brief Дескриптор сообщения каталога.
///
/// @details Связывает идентификатор сообщения с минимально допустимым
///     размером полезной нагрузки. Кадры меньшего размера отклоняются
///     обёрткой сборщика таблицы обработчиков
///     (@see make_message_dispatcher_table()).
template<typename Key>
struct message_catalog_entry {
    /// @brief Идентификатор сообщения.
    Key id;

    /// @brief Минимальный размер полезной нагрузки в байтах.
    /// @details 0 -- сообщение без обязательной полезной нагрузки: любой
    ///     размер принимается, лишние байты больших кадров игнорируются
    ///     (обратная совместимость по спецификации CRSF).
    std::size_t min_pload_size_bytes;
};

namespace catalog_detail {

/// @brief Тип ключа каталога: тип идентификатора первого типа списка.
///
/// @details Выводится из decltype(message_traits<T>::msg_id) с снятием
///     const (constexpr-член объявлен константным, поле дескриптора
///     каталога -- изменяемым). Требование: список содержит не менее
///     одного типа, тип ключа однороден по всем элементам (проверяется
///     static_assert в make_message_catalog()).
template<typename List>
using catalog_key_t = std::remove_const_t<
    decltype(stv::message_traits<stv::type_list_element_t<0U, List>>::msg_id)>;

/// @brief Считает типы списка с заданным идентификатором сообщения.
///
/// @tparam List Список типов сообщений (type_list).
/// @param[in] msg_id Идентификатор сообщения.
/// @return Число типов списка, чей message_traits::msg_id равен msg_id.
template<typename List, std::size_t... Is>
constexpr auto count_by_id(
    const catalog_key_t<List> msg_id, std::index_sequence<Is...> /*unused*/)
    -> std::size_t
{
    return ((message_traits<type_list_element_t<Is, List>>::msg_id == msg_id
                 ? 1U
                 : 0U)
            + ... + 0U);
}

/// @brief Ищет дескриптор сообщения в каталоге.
///
/// @tparam Catalog Тип каталога (constexpr-массив message_catalog_entry).
/// @tparam Key Тип идентификатора сообщения.
/// @param[in] catalog Каталог сообщений.
/// @param[in] msg_id Идентификатор сообщения.
/// @return Дескриптор сообщения.
/// @pre Идентификатор присутствует в каталоге (сборщик таблицы перебирает
///     только дескрипторы каталога); для чужого идентификатора возвращается
///     дескриптор с нулевым минимальным размером.
template<typename Catalog, typename Key>
constexpr auto find_catalog_entry(
    const Catalog &catalog, const Key msg_id) -> typename Catalog::value_type
{
    for(const auto &descriptor: catalog)
    {
        if(descriptor.id == msg_id)
        {
            return descriptor;
        }
    }

    return
        typename Catalog::value_type{.id = msg_id, .min_pload_size_bytes = 0U};
}

/// @brief Ищет обработчик полезной нагрузки сообщения в таблице обработчиков
///     полезной нагрузки стороны.
///
/// @tparam PloadHandlers Тип таблицы обработчиков полезной нагрузки
///     (массив/кортеж пар (идентификатор сообщения, указатель на обработчик
///     полезной нагрузки)).
/// @tparam Key Тип идентификатора сообщения.
/// @param[in] pload_handlers Таблица обработчиков полезной нагрузки стороны.
/// @param[in] msg_id Идентификатор сообщения.
/// @return Обработчик полезной нагрузки сообщения; nullptr, если сторона
///     обработчик не регистрирует.
template<typename PloadHandlers, typename Key>
constexpr auto find_pload_handler(
    const PloadHandlers &pload_handlers, const Key msg_id)
    -> stvlink_message_handler_fnc_type
{
    for(const auto &[handler_msg_id, handler]: pload_handlers)
    {
        if(handler_msg_id == msg_id)
        {
            return handler;
        }
    }

    return nullptr;
}

/// @brief Обработчик сообщения с проверкой минимального размера полезной
/// нагрузки.
///
/// @details Обёртка обработчика полезной нагрузки стороны: полезная
///     нагрузка меньшего размера отклоняется (false) без вызова обработчика;
///     идентификатор без обработчика полезной нагрузки (PloadHandler ==
///     nullptr) обрабатывается безопасным игнором (true) -- сообщение
///     принимается, лишние байты больших кадров игнорируются.
///
/// @tparam MinPloadSizeBytes Минимальный размер полезной нагрузки
///     дескриптора каталога.
/// @tparam PloadHandler Обработчик полезной нагрузки стороны; nullptr --
///     обработчик-игнор.
///
/// @param[in] pload Полезная нагрузка сообщения.
///
/// @return Результат обработчика полезной нагрузки; false при отклонении по
///     размеру.
template<std::size_t                      MinPloadSizeBytes,
         stvlink_message_handler_fnc_type PloadHandler>
bool guarded_handler(
    stvlink_message_span pload)
{
    if(pload.size_bytes() < MinPloadSizeBytes)
    {
        return false;
    }

    if constexpr(PloadHandler == nullptr)
    {
        return true;
    }
    else
    {
        return PloadHandler(pload);
    }
}

/// @brief Разрешает обработчик сообщения из каталога и таблицы обработчиков
///     полезной нагрузки.
/// @tparam PloadHandlers Таблица обработчиков полезной нагрузки стороны.
/// @tparam Id Идентификатор сообщения.
/// @tparam Catalog Каталог сообщений.
/// @return Обработчик сообщения с проверкой минимального размера полезной
///     нагрузки дескриптора каталога.
template<auto PloadHandlers, auto Id, auto Catalog>
constexpr auto resolve_handler() -> stvlink_message_handler_fnc_type
{
    constexpr auto descriptor    = find_catalog_entry(Catalog, Id);
    constexpr auto pload_handler = find_pload_handler(PloadHandlers, Id);
    return &guarded_handler<descriptor.min_pload_size_bytes, pload_handler>;
}

} // namespace catalog_detail

/// @brief Генерирует каталог сообщений из списка типов.
///
/// @details Дескриптор каждого типа списка строится из его специализации
///     message_traits (идентификатор и минимальный размер полезной
///     нагрузки); ручные перечни дескрипторов отсутствуют. Тип ключа
///     выводится из идентификатора первого типа списка и обязан совпадать у
///     всех типов списка; идентификаторы сообщений обязаны быть уникальны --
///     обе проверки выполняются static_assert на этапе компиляции.
///
/// @tparam List Список типов сообщений (type_list), не менее одного типа.
///
/// @return Массив дескрипторов каталога в порядке типов списка.
///
/// Пример генерации каталога протокола из списка типов (декларации
/// типов и специализаций traits вынесены в заголовок протокола).
/// Протокол содержит два сообщения: телеметрию и команду.
/// @code{.cpp}
/// // Первое сообщение: тип со специализацией traits (в блоке
/// // namespace stv, см. message_traits.hpp): идентификатор сообщения
/// // и минимальный размер полезной нагрузки.
/// struct my_telemetry {
///     std::uint32_t counter{};
///     std::int16_t  temperature{};
/// };
///
/// template <>
/// struct message_traits<my_telemetry> {
///     static constexpr my_msg_id   msg_id = my_msg_id::telemetry;
///     static constexpr std::size_t min_pload_size_bytes =
///         sizeof(std::uint32_t) + sizeof(std::int16_t);
/// };
///
/// // Второе сообщение: тот же протокол, тот же тип ключа идентификатора
/// // my_msg_id, другой минимальный размер полезной нагрузки.
/// struct my_command {
///     std::uint8_t code{};
/// };
///
/// template <>
/// struct message_traits<my_command> {
///     static constexpr my_msg_id   msg_id = my_msg_id::command;
///     static constexpr std::size_t min_pload_size_bytes = sizeof(my_command);
/// };
///
/// // Список типов сообщений одного протокола: телеметрия и команда --
/// // это разные сообщения (команды) в рамках одного протокола, а не
/// // разные протоколы. Порядок типов задаёт порядок дескрипторов
/// // каталога. Точка расширения протокола -- новая строка в списке:
/// // новое сообщение добавляется без правок каталога и диспетчеров.
/// using my_list = type_list<my_telemetry, my_command>;
///
/// // Каталог генерируется из traits одной строкой; дескрипторы править
/// // вручную не нужно. Дубликат идентификатора и несовпадение типа
/// // ключа ловятся static_assert'ами сборщика на этапе компиляции.
/// constexpr auto my_catalog = make_message_catalog<my_list>();
/// static_assert(my_catalog.size() == 2U);
/// static_assert(my_catalog[0].id == my_msg_id::telemetry);
/// static_assert(my_catalog[1].id == my_msg_id::command);
/// static_assert(my_catalog[1].min_pload_size_bytes == sizeof(my_command));
/// @endcode
///
/// Достоверный учебный протокол с четырьмя способами описания полезной
/// нагрузки: stv/communication/tests/demo_messages.hpp.
template<typename List>
consteval auto make_message_catalog()
    -> std::array<message_catalog_entry<catalog_detail::catalog_key_t<List>>,
                  type_list_size_v<List>>
{
    using key           = catalog_detail::catalog_key_t<List>;
    constexpr auto size = type_list_size_v<List>;

    static_assert(
        []<std::size_t... Is>(std::index_sequence<Is...>) {
            return (
                std::same_as<std::remove_const_t<
                                 decltype(message_traits<type_list_element_t<
                                              Is, List>>::msg_id)>,
                             key>
                && ...);
        }(std::make_index_sequence<size>{}),
        "all message ids in the list must have the same key type");

    static_assert(
        []<std::size_t... Is>(std::index_sequence<Is...>) {
            return ((catalog_detail::count_by_id<List>(
                         message_traits<type_list_element_t<Is, List>>::msg_id,
                         std::make_index_sequence<size>{})
                     == 1U)
                    && ...);
        }(std::make_index_sequence<size>{}),
        "message ids in the list must be unique");

    std::array<message_catalog_entry<key>, size> result{};
    [&result]<std::size_t... Is>(std::index_sequence<Is...>) {
        ((result[Is] =
              message_catalog_entry<key>{
                  .id = message_traits<type_list_element_t<Is, List>>::msg_id,
                  .min_pload_size_bytes = message_traits<
                      type_list_element_t<Is, List>>::min_pload_size_bytes,
              }),
         ...);
    }(std::make_index_sequence<size>{});

    return result;
}

/// @brief Возвращает массив идентификаторов сообщений каталога.
///
/// @details Идентификаторы извлекаются из дескрипторов каталога в порядке
///     их следования; тип элемента результата -- тип поля id дескриптора.
///     Хелпер убирает ручную сборку массива идентификаторов перед
///     проверками покрытия перечисления
///     (@see enum_uniquely_covered() в type_list.hpp).
///
/// @tparam Catalog Тип каталога (constexpr-массив message_catalog_entry).
/// @param[in] catalog Каталог сообщений.
/// @return Массив идентификаторов дескрипторов в порядке каталога.
///
/// Пример:
/// @code{.cpp}
/// // Не компилируется, если каталог потерял или продублировал значение
/// // перечисления идентификаторов.
/// static_assert(stv::enum_uniquely_covered(
///     stv::catalog_message_ids(my_catalog), my_msg_id::max));
/// @endcode
template<typename Catalog>
constexpr auto catalog_message_ids(
    const Catalog &catalog)
    -> std::array<
        std::remove_const_t<
            decltype(std::declval<typename Catalog::value_type>().id)>,
        std::tuple_size_v<Catalog>>
{
    using key = std::remove_const_t<
        decltype(std::declval<typename Catalog::value_type>().id)>;

    std::array<key, std::tuple_size_v<Catalog>> result{};
    for(std::size_t i = 0U; i < catalog.size(); ++i)
    {
        gsl::at(result, static_cast<gsl::index>(i)) =
            gsl::at(catalog, static_cast<gsl::index>(i)).id;
    }

    return result;
}

/// @brief Проверяет, что порядок типов списка совпадает с порядком
///     дескрипторов каталога.
///
/// @details Тип на каждой позиции i обязан быть сообщением дескриптора
///     каталога на той же позиции: идентификатор message_traits типа
///     элемента равен идентификатору дескриптора. Позиции не перечисляются
///     вручную: обход выполняется по всем элементам списка через
///     std::index_sequence, добавление сообщения в список не требует правки
///     проверки. Размеры списка и каталога обязаны совпадать.
///
///     Проверка дополняет инвариант покрытия значений перечисления
///     (@see enum_uniquely_covered() в type_list.hpp): покрытие не
///     гарантирует порядок -- дескрипторы могут следовать в другой
///     последовательности, чем типы списка.
///
/// @tparam List Список типов сообщений (type_list).
/// @tparam Catalog Тип каталога (constexpr-массив message_catalog_entry).
/// @param[in] catalog Каталог сообщений.
/// @return true, если размеры совпадают и тип каждой позиции -- сообщение
///     дескриптора той же позиции.
///
/// Пример:
/// @code{.cpp}
/// // Не компилируется, если порядок типов my_list не совпадает с
/// // порядком дескрипторов каталога.
/// static_assert(stv::check_catalog_list_order<my_list>(my_catalog));
/// @endcode
template<typename List, typename Catalog>
consteval auto check_catalog_list_order(
    const Catalog &catalog) -> bool
{
    if(catalog.size() != type_list_size_v<List>)
    {
        return false;
    }

    bool matches = true;
    // NOLINTBEGIN(cppcoreguidelines-pro-bounds-constant-array-index) --
    // ложные срабатывания: индексы Is за пределами каталога недостижимы —
    // guard catalog.size() != type_list_size_v<List> выше возвращает false
    // до выполнения обхода (short-circuit && внутри fold).
    [&matches, &catalog]<std::size_t... Is>(std::index_sequence<Is...>) {
        ((matches = matches
                    && (message_traits<type_list_element_t<Is, List>>::msg_id
                        == catalog[Is].id)),
         ...);
    }(std::make_index_sequence<type_list_size_v<List>>{});
    // NOLINTEND(cppcoreguidelines-pro-bounds-constant-array-index)
    return matches;
}

/// @brief Проверяет полноту приёмной логики стороны.
///
/// @details Каждый дескриптор каталога должен быть покрыт либо обработчиком
///     полезной нагрузки стороны (пара (идентификатор, обработчик) в таблице
///     обработчиков полезной нагрузки), либо явной записью в списке
///     осознанно игнорируемых идентификаторов стороны. Предназначена для
///     static_assert рядом с таблицей обработчиков полезной нагрузки стороны:
///     идентификатор вне обоих списков -- сигнал о молчаливо потерянном
///     входящем сообщении.
///
/// @tparam Catalog Тип каталога (constexpr-массив message_catalog_entry).
/// @tparam PloadHandlers Тип таблицы обработчиков полезной нагрузки стороны
///     (constexpr-массив пар (идентификатор сообщения, обработчик)).
/// @tparam Ignored Тип списка осознанно игнорируемых идентификаторов
///     (constexpr-массив ключей каталога).
///
/// @param[in] catalog Каталог сообщений.
/// @param[in] pload_handlers Таблица обработчиков полезной нагрузки стороны.
/// @param[in] ignored Список осознанно игнорируемых идентификаторов.
///
/// @return true, если каждый идентификатор каталога покрыт.
///
/// Пример фиксации полноты приёмной логики стороны: static_assert
/// размещается рядом с таблицей обработчиков полезной нагрузки стороны
/// и ловит входящее сообщение, для которого не написан ни обработчик
/// полезной нагрузки, ни осознанный игнор:
/// @code{.cpp}
/// // Обработчики полезной нагрузки стороны: свободные функции вида
/// // bool(stvlink_message_span), состояние хранит сторона, а не
/// // обработчик (см. терминологию в шапке файла).
/// bool handle_telemetry(stvlink_message_span pload);
/// bool handle_command(stvlink_message_span pload);
///
/// // Таблица обработчиков полезной нагрузки -- единая точка регистрации
/// // приёмной логики стороны.
/// constexpr auto my_pload_handlers = std::array{
///     std::pair{my_msg_id::telemetry, &handle_telemetry},
///     std::pair{my_msg_id::command, &handle_command},
/// };
///
/// // Осознанно игнорируемые сообщения: в обмене участвуют, стороной не
/// // обрабатываются (например, сообщения другой версии протокола).
/// constexpr auto my_ignored = std::array{
///     my_msg_id::status_text,
/// };
///
/// // Каждый идентификатор каталога покрыт обработчиком полезной нагрузки
/// // или осознанным игнором: молчаливая потеря входящих сообщений
/// // исключена -- непокрытый идентификатор остановит сборку.
/// static_assert(
///     check_message_coverage(my_catalog, my_pload_handlers, my_ignored));
/// @endcode
template<typename Catalog, typename PloadHandlers, typename Ignored>
constexpr auto check_message_coverage(
    const Catalog &catalog, const PloadHandlers &pload_handlers,
    const Ignored &ignored) -> bool
{
    bool covered = true;
    for(const auto &descriptor: catalog)
    {
        bool descriptor_covered = false;
        for(const auto &[msg_id, handler]: pload_handlers)
        {
            descriptor_covered =
                descriptor_covered || (msg_id == descriptor.id);
        }
        for(const auto ignored_id: ignored)
        {
            descriptor_covered =
                descriptor_covered || (ignored_id == descriptor.id);
        }
        covered = covered && descriptor_covered;
    }

    return covered;
}

/// @brief Собирает хэш-таблицу обработчиков входящих сообщений стороны.
///
/// @details Для каждого дескриптора каталога (@see make_message_catalog)
///     регистрируется обработчик, оборачивающий обработчик полезной
///     нагрузки стороны из таблицы обработчиков полезной нагрузки проверкой
///     min_pload_size_bytes. Идентификатор без обработчика полезной
///     нагрузки регистрируется безопасным обработчиком-игнором (true):
///     сообщение принимается, лишние байты больших кадров игнорируются;
///     осознанный игнор обязан быть зафиксирован записью в списке
///     игнорируемых идентификаторов стороны и static_assert
///     (@see check_message_coverage). Ключ диспетчера формируется через
///     std::to_underlying(id), поэтому тип msg_id в traits списка должен
///     быть перечислением (scoped enum с целочисленным underlying).
///
///     Таблица обработчиков полезной нагрузки -- constexpr-массив (или
///     кортеж) пар (идентификатор сообщения, указатель на функцию-обработчик
///     полезной нагрузки) и является единой точкой регистрации приёмной
///     логики стороны. Таблица обработчиков полезной нагрузки и каталог
///     передаются шаблонными параметрами: обработчики хэш-таблицы --
///     указатели на функции без состояния, поэтому обёртка минимального
///     размера встраивается в каждый обработчик на этапе компиляции.
///
/// @tparam HashTable Тип хэш-таблицы обработчиков
///     (@see stv::stvlink_handler_table).
/// @tparam PloadHandlers Таблица обработчиков полезной нагрузки стороны
///     (constexpr-значение).
/// @tparam Catalog Каталог сообщений (constexpr-значение,
///     @see make_message_catalog()).
///
/// @return Заполненная хэш-таблица обработчиков: по ключу каждого
///     дескриптора каталога зарегистрирован обработчик.
///
/// Пример сборки приёмной стороны обмена сообщениями (таблица обработчиков
/// полезной нагрузки и каталог -- примеры выше,
/// @see check_message_coverage()):
/// @code{.cpp}
/// // Ёмкость таблицы -- число значений перечисления идентификаторов
/// // протокола (значение max в конце перечисления).
/// constexpr auto capacity = std::to_underlying(my_msg_id::max);
/// using my_handler_table  = stvlink_handler_table<capacity>;
///
/// // Сборка таблицы: обёртка min_pload_size_bytes применяется сборщиком
/// // автоматически, проверки размера полезной нагрузки в обработчиках
/// // писать не нужно. Функция-аксессор со static -- типичная идиома:
/// // таблица собирается один раз и переиспользуется.
/// const auto &handler_table()
/// {
///     const static auto table = make_message_dispatcher_table<
///         my_handler_table, my_pload_handlers, my_catalog>();
///     return table;
/// }
///
/// // Таблица передаётся модулю диспетчеризации stvlink_module
/// // (см. stvlink_dispatcher.hpp): модуль по идентификатору каждого
/// // принятого кадра вызывает зарегистрированный обработчик.
/// @endcode
///
/// Сквозной учебный пример передающей и принимающей сторон обмена
/// сообщениями по stvlinkv2:
/// stv/communication/tests/test_message_catalog_communication.cpp.
template<typename HashTable, auto PloadHandlers, auto Catalog>
auto make_message_dispatcher_table() -> HashTable
{
    using key_type = typename HashTable::key_type;

    HashTable table{};
    [&table]<std::size_t... Is>(std::index_sequence<Is...>) {
        (table.insert({
             static_cast<key_type>(std::to_underlying(Catalog[Is].id)),
             catalog_detail::resolve_handler<PloadHandlers, Catalog[Is].id,
                                             Catalog>(),
         }),
         ...);
    }(std::make_index_sequence<Catalog.size()>{});

    return table;
}

} // namespace stv

#endif // MESSAGE_CATALOG_HPP
