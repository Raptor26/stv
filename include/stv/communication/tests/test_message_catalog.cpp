/// @file test_message_catalog.cpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.

#include "stv/communication/message_catalog.hpp"
#include "stv/communication/stvlinkv2_kit.hpp"
#include "stv/communication/tests/demo_messages.hpp"
#include <array>
#include <catch2/catch_test_macros.hpp>
#include <concepts>
#include <cstddef>
#include <span>
#include <utility>

using stv::demo_catalog;
using stv::demo_command;
using stv::demo_list;
using stv::demo_msg_id;
using stv::demo_registers;
using stv::demo_status_text;
using stv::demo_telemetry;

namespace {

constexpr auto table_capacity = std::to_underlying(demo_msg_id::max);
using table_type              = stv::stvlink_handler_table<table_capacity>;

/// @brief Флаг вызова тестового обработчика полезной нагрузки.
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
bool pload_handler_called{false};

/// @brief Результат, который возвращает тестовый обработчик полезной
///     нагрузки при вызове.
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
bool pload_handler_result{true};

/// @brief Тестовый обработчик полезной нагрузки приёма сообщения.
bool test_pload_handler(
    stv::stvlink_message_span pload)
{
    pload_handler_called = true;
    return (pload.size_bytes() > 0U) && pload_handler_result;
}

/// @brief Таблица обработчиков полезной нагрузки с тестовым обработчиком
///     сообщения command.
constexpr auto test_pload_handlers = std::array{
    std::pair{demo_msg_id::command, &test_pload_handler},
};

/// @brief Осознанно игнорируемые сообщения положительного сценария
///     check_message_coverage (весь каталог без command).
constexpr auto test_ignored = std::array{
    demo_msg_id::telemetry,
    demo_msg_id::status_text,
    demo_msg_id::registers,
};

/// @brief Пустой список осознанного игнора для негативного сценария
///     check_message_coverage.
constexpr auto test_empty_ignored = std::array<demo_msg_id, 0U>{};

/// @brief Полностью игнорируемый каталог: обработчики полезной нагрузки не
///     нужны.
constexpr auto test_all_ignored = std::array{
    demo_msg_id::telemetry,
    demo_msg_id::command,
    demo_msg_id::status_text,
    demo_msg_id::registers,
};

constexpr auto test_empty_pload_handlers =
    std::array<std::pair<demo_msg_id, stv::stvlink_message_handler_fnc_type>,
               0U>{};

/// @brief Собирает тестовую таблицу обработчиков из заданных обработчиков
///     полезной нагрузки.
template<auto PloadHandlers>
auto make_table() -> table_type
{
    return stv::make_message_dispatcher_table<table_type, PloadHandlers,
                                              demo_catalog>();
}

/// @brief Ищет обработчик сообщения в таблице по идентификатору.
auto find_handler(
    const table_type &table, const demo_msg_id msg_id)
    -> table_type::const_iterator
{ return table.find(std::to_underlying(msg_id)); }

} // namespace

TEST_CASE(
    "message_catalog: каталог генерируется из списка типов")
{
    // Тип ключа выводится из traits первого типа списка; дескрипторы равны
    // traits своих типов с тем же индексом.
    static_assert(demo_catalog.size() == 4U);
    static_assert(std::same_as<typename decltype(demo_catalog)::value_type,
                               stv::message_catalog_entry<demo_msg_id>>);

    static_assert(demo_catalog[0].id == demo_msg_id::telemetry);
    static_assert(demo_catalog[0].min_pload_size_bytes
                  == stv::message_traits<demo_telemetry>::min_pload_size_bytes);
    static_assert(demo_catalog[1].id == demo_msg_id::command);
    static_assert(demo_catalog[1].min_pload_size_bytes
                  == stv::message_traits<demo_command>::min_pload_size_bytes);
    static_assert(demo_catalog[2].id == demo_msg_id::status_text);
    static_assert(
        demo_catalog[2].min_pload_size_bytes
        == stv::message_traits<demo_status_text>::min_pload_size_bytes);
    static_assert(demo_catalog[3].id == demo_msg_id::registers);
    static_assert(demo_catalog[3].min_pload_size_bytes
                  == sizeof(demo_registers));

    REQUIRE(demo_catalog.size() == stv::type_list_size_v<demo_list>);
    REQUIRE(demo_catalog[1].min_pload_size_bytes == 1U);
}

TEST_CASE(
    "message_catalog: порядок типов списка совпадает с порядком "
    "дескрипторов каталога")
{
    // Позитивный сценарий: тип на каждой позиции -- сообщение дескриптора
    // той же позиции. Позиции не перечисляются вручную.
    static_assert(stv::check_catalog_list_order<demo_list>(demo_catalog));
    REQUIRE(stv::check_catalog_list_order<demo_list>(demo_catalog));

    // Негативный сценарий: те же дескрипторы в другом порядке.
    constexpr auto shuffled = std::array{
        demo_catalog[3],
        demo_catalog[0],
        demo_catalog[1],
        demo_catalog[2],
    };
    static_assert(!stv::check_catalog_list_order<demo_list>(shuffled));
    REQUIRE_FALSE(stv::check_catalog_list_order<demo_list>(shuffled));

    // Негативный сценарий: размер каталога меньше размера списка.
    constexpr auto truncated = std::array{demo_catalog[0], demo_catalog[1]};
    static_assert(!stv::check_catalog_list_order<demo_list>(truncated));
    REQUIRE_FALSE(stv::check_catalog_list_order<demo_list>(truncated));
}

TEST_CASE(
    "message_catalog: check_message_coverage принимает обработчики полезной "
    "нагрузки вместе со списком игнора")
{
    constexpr bool covered = stv::check_message_coverage(
        demo_catalog, test_pload_handlers, test_ignored);
    static_assert(covered);
    REQUIRE(covered);
}

TEST_CASE(
    "message_catalog: check_message_coverage принимает полностью игнорируемый "
    "каталог")
{
    constexpr bool covered = stv::check_message_coverage(
        demo_catalog, test_empty_pload_handlers, test_all_ignored);
    static_assert(covered);
    REQUIRE(covered);
}

TEST_CASE(
    "message_catalog: check_message_coverage отклоняет непокрытый "
    "идентификатор")
{
    // command покрыт обработчиком полезной нагрузки; остальной каталог без
    // записей игнора остаётся непокрытым.
    REQUIRE_FALSE(stv::check_message_coverage(demo_catalog, test_pload_handlers,
                                              test_empty_ignored));
}

TEST_CASE(
    "message_catalog: диспетчер регистрирует обработчик каждого дескриптора")
{
    const auto table = make_table<test_pload_handlers>();

    for(const auto &descriptor: demo_catalog)
    {
        CAPTURE(std::to_underlying(descriptor.id));
        REQUIRE(find_handler(table, descriptor.id) != table.end());
    }
}

TEST_CASE(
    "message_catalog: диспетчер передаёт полезную нагрузку достаточного "
    "размера обработчику полезной нагрузки")
{
    const auto table      = make_table<test_pload_handlers>();
    const auto handler_it = find_handler(table, demo_msg_id::command);
    REQUIRE(handler_it != table.end());

    std::array<std::byte, 1U> pload{};
    pload_handler_called = false;
    pload_handler_result = true;

    REQUIRE(handler_it->second(std::span{pload}));
    REQUIRE(pload_handler_called);
}

TEST_CASE(
    "message_catalog: диспетчер отклоняет полезную нагрузку меньше "
    "минимального размера")
{
    const auto table      = make_table<test_pload_handlers>();
    const auto handler_it = find_handler(table, demo_msg_id::command);
    REQUIRE(handler_it != table.end());

    // Минимальный размер command -- 1 байт: пустой кадр отклоняется
    // без вызова обработчика полезной нагрузки.
    pload_handler_called = false;

    REQUIRE_FALSE(handler_it->second({}));
    REQUIRE_FALSE(pload_handler_called);
}

TEST_CASE(
    "message_catalog: диспетчер пробрасывает результат обработчика полезной "
    "нагрузки")
{
    const auto table      = make_table<test_pload_handlers>();
    const auto handler_it = find_handler(table, demo_msg_id::command);
    REQUIRE(handler_it != table.end());

    std::array<std::byte, 1U> pload{};
    pload_handler_called = false;
    pload_handler_result = false;

    REQUIRE_FALSE(handler_it->second(std::span{pload}));
    REQUIRE(pload_handler_called);

    pload_handler_result = true;
}

TEST_CASE(
    "message_catalog: диспетчер ставит безопасный игнор для идентификатора без "
    "обработчика полезной нагрузки")
{
    const auto table = make_table<test_pload_handlers>();

    // status_text в тестовой таблице обработчиков полезной нагрузки не
    // регистрируется, минимальный
    // размер его дескриптора -- 0: обработчик-игнор принимает любой кадр.
    const auto handler_it = find_handler(table, demo_msg_id::status_text);
    REQUIRE(handler_it != table.end());

    pload_handler_called = false;

    // Без полезной нагрузки и с лишними байтами большого кадра: всегда true.
    REQUIRE(handler_it->second({}));
    std::array<std::byte, 4U> extra{};
    REQUIRE(handler_it->second(std::span{extra}));
    REQUIRE_FALSE(pload_handler_called);
}
