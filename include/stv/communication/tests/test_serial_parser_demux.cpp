/// @file test_serial_parser_demux.cpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.

#include "etl/unordered_map.h"
#include "stv/communication/crc16.hpp"
#include "stv/communication/mavlink_v2_frame.hpp"
#include "stv/communication/mavlink_v2_route.hpp"
#include "stv/communication/mavlink_v2_sender.hpp"
#include "stv/communication/parsed_queue.hpp"
#include "stv/communication/serial_decorators.hpp"
#include "stv/communication/serial_parser.hpp"
#include "stv/communication/serial_sender.hpp"
#include "stv/communication/stvlink_frame.hpp"
#include "stv/communication/stvlink_route.hpp"
#include "stv/communication/stvlink_sender.hpp"
#include "stv/containers/lwrb.hpp"
#include "stv/containers/simbuff.hpp"
#include <array>
#include <catch2/catch_test_macros.hpp>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <etl/queue.h>
#include <mutex>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

// NOLINTBEGIN(*-magic-numbers)

// Catch2 SECTION-макросы резко завышают когнитивную сложность теста;
// разбивать единый тестовый сценарий ради метрики нецелесообразно.
// NOLINTBEGIN(readability-function-cognitive-complexity)

namespace {

using sim_buffer_type = stv::sim_buff<stv::empty_mutex>;
using queue_type      = etl::queue<sim_buffer_type, 10>;
using lwrb_setup_type = stv::lwrb_setup<stv::empty_mutex>;
using lwrb_base_type  = stv::lwrb_base<lwrb_setup_type>;
using hash_type       = etl::iunordered_map<int, queue_type *>;

/// @brief Тестовый провайдер CRC_EXTRA с маленькой таблицей на 2 msgid.
struct test_crc_extra_provider {
    static auto crc_extra_for(
        std::uint32_t msgid) -> std::optional<std::uint8_t>
    {
        switch(msgid)
        {
            case 0U:
                return std::uint8_t{50U};  // HEARTBEAT
            case 1U:
                return std::uint8_t{124U}; // SYS_STATUS
            default:
                return std::nullopt;
        }
    }
};

using mavlink_frame_type    = stv::mavlink_v2_frame<test_crc_extra_provider>;
using mavlink_frame_tx_type = stv::mavlink_v2_frame_tx<test_crc_extra_provider>;

using stv_route_setup_type = stv::stvlink_route_setup<queue_type, hash_type>;
using stv_router_type      = stv::stvlink_route<stv_route_setup_type>;

using mavlink_route_setup_type =
    stv::mavlink_v2_route_setup<mavlink_frame_type, queue_type, hash_type>;
using mavlink_router_type = stv::mavlink_v2_route<mavlink_route_setup_type>;

/// @brief Идентификатор получателя stv_rx_v2 (dst_id в stvlink_route_tx).
constexpr std::uint8_t stv_dst_id{7U};

/// @brief Идентификатор системы-отправителя MAVLink v2 (sysid).
constexpr std::uint8_t mavlink_sysid{1U};

/// @brief Идентификатор компонента-отправителя MAVLink v2 (compid).
constexpr std::uint8_t mavlink_compid{3U};

/// @brief Идентификатор сообщения HEARTBEAT.
constexpr std::uint32_t mavlink_msgid_heartbeat{0U};

/// @brief Идентификатор сообщения SYS_STATUS.
constexpr std::uint32_t mavlink_msgid_sys_status{1U};

/// @brief Смещение младшего байта поля msgid в кадре MAVLink v2.
constexpr std::size_t mavlink_msgid_offset{7U};

/// @brief Размер заголовка маршрутизации MAVLink v2 в отрезанном сообщении
///     (compat_flags, seq, sysid, compid, msgid).
constexpr std::size_t mavlink_routing_header_size{7U};

/// @brief Собирает кадр stv_rx_v2 с заданной полезной нагрузкой.
auto make_stv_frame(
    std::string_view payload, std::uint8_t dst_id) -> std::vector<std::byte>
{
    queue_type tx_queue;
    auto       buffer = stv::make_serial_message_buffer(
        tx_queue, stv::stvlink_frame_tx{}, stv::stvlink_route_tx{});

    {
        auto msg = buffer.request(payload, stv::stvlink_route_tx::setup_t{
                                               .dst_id = dst_id, .pack_id = 0});
        (void)msg;
    }

    REQUIRE(!tx_queue.empty());

    const auto            &frame = tx_queue.front();
    std::vector<std::byte> result(frame.begin(), frame.end());
    tx_queue.pop();

    return result;
}

/// @brief Собирает кадр MAVLink v2 с заданной полезной нагрузкой через
///     передающий декоратор mavlink_v2_frame_tx.
auto make_mavlink_frame(
    std::string_view payload, std::uint8_t compid,
    const mavlink_frame_tx_type::route_t &route) -> std::vector<std::byte>
{
    queue_type                  tx_queue;
    stv::mavlink_v2_seq_counter counter;

    auto                        buffer = stv::make_serial_message_buffer(
        tx_queue,
        mavlink_frame_tx_type{
            {.sysid = mavlink_sysid, .compid = compid, .counter = &counter}});

    {
        auto msg = buffer.request(payload, route);
        (void)msg;
    }

    REQUIRE(!tx_queue.empty());

    const auto            &frame = tx_queue.front();
    std::vector<std::byte> result(frame.begin(), frame.end());
    // Очередь освобождается до разрушения счётчика seq.
    tx_queue.pop();

    return result;
}

/// @brief Возвращает ожидаемое содержимое распарсенного кадра stv_rx_v2
///     (заголовок stvlink_route_tx: dst_id, pack_id, pload_size + нагрузка).
auto make_expected_stv_payload(
    std::string_view payload, std::uint8_t dst_id) -> std::vector<std::byte>
{
    const auto pload_size = static_cast<std::uint32_t>(payload.size());

    std::vector<std::byte> result;
    result.reserve(stv::stvlink_route_tx::header_size() + payload.size());
    result.push_back(std::byte{dst_id});
    result.push_back(std::byte{0x00}); // pack_id
    // pload_size записывается little-endian (хостовая платформа — LE).
    result.push_back(static_cast<std::byte>(pload_size & 0xFFU));
    result.push_back(static_cast<std::byte>((pload_size >> 8U) & 0xFFU));

    const auto bytes = std::as_bytes(std::span(payload.data(), payload.size()));
    result.insert(result.end(), bytes.begin(), bytes.end());

    return result;
}

/// @brief Возвращает ожидаемое содержимое распарсенного кадра MAVLink v2:
///     [compat_flags, seq, sysid, compid, msgid (3 байта, LE), payload].
auto make_expected_mavlink_payload(
    std::string_view payload, std::uint8_t compid,
    const mavlink_frame_tx_type::route_t &route) -> std::vector<std::byte>
{
    const auto             msgid = route.msgid;
    std::vector<std::byte> result;
    result.reserve(mavlink_routing_header_size + payload.size());
    result.push_back(std::byte{0x00}); // compat_flags
    // Каждый кадр собирается со свежим счётчиком, поэтому seq всегда 0.
    result.push_back(std::byte{0x00}); // seq
    result.push_back(std::byte{mavlink_sysid});
    result.push_back(std::byte{compid});
    // msgid записывается little-endian: младший байт первым.
    result.push_back(static_cast<std::byte>(msgid & 0xFFU));
    result.push_back(static_cast<std::byte>((msgid >> 8U) & 0xFFU));
    result.push_back(static_cast<std::byte>((msgid >> 16U) & 0xFFU));

    const auto bytes = std::as_bytes(std::span(payload.data(), payload.size()));
    result.insert(result.end(), bytes.begin(), bytes.end());

    return result;
}

/// @brief Проверяет, что сообщение в голове очереди совпадает с ожидаемым,
///     и удаляет его из очереди.
void require_front_equals(
    queue_type &queue, const std::vector<std::byte> &expected)
{
    REQUIRE(!queue.empty());

    const auto msg = std::move(queue.front());
    REQUIRE(msg.size_bytes() == expected.size());
    REQUIRE(std::memcmp(msg.data<std::byte>(), expected.data(), expected.size())
            == 0);
    queue.pop();
}

} // namespace

TEST_CASE(
    "serial_parser demultiplexes stv_rx_v2 and mavlink_v2 frames",
    "[stv][communication]")
{
    constexpr std::size_t                       lwrb_buffer_size{256};
    stv::lwrb<lwrb_base_type, lwrb_buffer_size> lwrb{lwrb_setup_type{}};
    queue_type                                  stv_parsed_queue;
    queue_type                                  mavlink_parsed_queue;

    using parser_setup_type =
        stv::serial_parser_setup<lwrb_base_type, queue_type, stv::empty_mutex,
                                 stv::stvlink_frame, mavlink_frame_type>;

    parser_setup_type setup;
    setup.lwrb = &lwrb;
    setup.set_queue<stv::stvlink_frame>(stv_parsed_queue);
    setup.set_queue<mavlink_frame_type>(mavlink_parsed_queue);

    auto parser = stv::make_serial_parser<parser_setup_type, stv::stvlink_frame,
                                          mavlink_frame_type>(setup);
    REQUIRE(parser);

    // Маршрутизатор stv_rx_v2 по dst_id из заголовка stvlink_route_tx.
    queue_type                                stv_routed_queue;
    etl::unordered_map<int, queue_type *, 10> stv_hash;
    stv_hash.insert({stv_dst_id, &stv_routed_queue});

    stv_route_setup_type stv_route_setup;
    stv_route_setup.queue_to_read = &stv_parsed_queue;
    stv_route_setup.hash_to_write = &stv_hash;

    stv_router_type stv_router{stv_route_setup};
    REQUIRE(stv_router);

    // Маршрутизатор MAVLink v2 по compid источника.
    queue_type                                mavlink_routed_queue;
    etl::unordered_map<int, queue_type *, 10> mavlink_hash;
    mavlink_hash.insert({mavlink_compid, &mavlink_routed_queue});

    mavlink_route_setup_type mavlink_route_setup;
    mavlink_route_setup.queue_to_read =
        stv::parsed_queue<mavlink_frame_type, queue_type>{mavlink_parsed_queue};
    mavlink_route_setup.hash_to_write = &mavlink_hash;

    mavlink_router_type mavlink_router{mavlink_route_setup};
    REQUIRE(mavlink_router);

    SECTION("mavlink frame is selected by single 0xFD byte")
    {
        // MAVLink v2 выбирается по одному стартовому байту 0xFD: второй
        // байт (len) может принимать любое значение.
        STATIC_REQUIRE(
            mavlink_frame_type::matches(std::byte{0xFD}, std::byte{0x00}));
        STATIC_REQUIRE(
            mavlink_frame_type::matches(std::byte{0xFD}, std::byte{0xAA}));
        STATIC_REQUIRE_FALSE(
            mavlink_frame_type::matches(std::byte{0xAA}, std::byte{0xAA}));
        STATIC_REQUIRE(
            stv::stvlink_frame::matches(std::byte{0xAA}, std::byte{0xAA}));
        STATIC_REQUIRE_FALSE(
            stv::stvlink_frame::matches(std::byte{0xFD}, std::byte{0xFD}));

        constexpr std::string_view payload{"Hi"};
        const auto                 frame = make_mavlink_frame(
            payload, mavlink_compid, {.msgid = mavlink_msgid_heartbeat});
        lwrb.write(frame);

        REQUIRE(parser.run());
        REQUIRE(stv_parsed_queue.empty());
        REQUIRE(mavlink_parsed_queue.size() == 1U);

        REQUIRE(mavlink_router.run() == 1U);
        REQUIRE(mavlink_parsed_queue.empty());
        require_front_equals(
            mavlink_routed_queue,
            make_expected_mavlink_payload(payload, mavlink_compid,
                                          {.msgid = mavlink_msgid_heartbeat}));
    }

    SECTION("mixed stream")
    {
        constexpr std::string_view stv_payload_one{"A"};
        constexpr std::string_view mav_payload_one{"Hi"};
        constexpr std::string_view stv_payload_two{"BC"};
        constexpr std::string_view mav_payload_two{"Yo"};

        lwrb.write(make_stv_frame(stv_payload_one, stv_dst_id));
        lwrb.write(make_mavlink_frame(mav_payload_one, mavlink_compid,
                                      {.msgid = mavlink_msgid_heartbeat}));
        lwrb.write(make_stv_frame(stv_payload_two, stv_dst_id));
        lwrb.write(make_mavlink_frame(mav_payload_two, mavlink_compid,
                                      {.msgid = mavlink_msgid_sys_status}));

        REQUIRE(parser.run());
        REQUIRE(stv_parsed_queue.size() == 2U);
        REQUIRE(mavlink_parsed_queue.size() == 2U);

        REQUIRE(stv_router.run() == 2U);
        REQUIRE(mavlink_router.run() == 2U);

        require_front_equals(
            stv_routed_queue,
            make_expected_stv_payload(stv_payload_one, stv_dst_id));
        require_front_equals(
            stv_routed_queue,
            make_expected_stv_payload(stv_payload_two, stv_dst_id));
        require_front_equals(
            mavlink_routed_queue,
            make_expected_mavlink_payload(mav_payload_one, mavlink_compid,
                                          {.msgid = mavlink_msgid_heartbeat}));
        require_front_equals(
            mavlink_routed_queue,
            make_expected_mavlink_payload(mav_payload_two, mavlink_compid,
                                          {.msgid = mavlink_msgid_sys_status}));
    }

    SECTION("garbage between frames")
    {
        constexpr std::string_view stv_payload{"stv"};
        constexpr std::string_view mav_payload{"mav"};
        const auto stv_frame = make_stv_frame(stv_payload, stv_dst_id);
        const auto mav_frame = make_mavlink_frame(
            mav_payload, mavlink_compid, {.msgid = mavlink_msgid_heartbeat});

        // Мусор не должен содержать стартовые последовательности 0xAA 0xAA
        // или 0xFD, в том числе на границе со следующим кадром.
        constexpr std::array<std::byte, 6> garbage{
            std::byte{0x00}, std::byte{0x11}, std::byte{0x22},
            std::byte{0x33}, std::byte{0x44}, std::byte{0x66}};

        lwrb.write(garbage);
        lwrb.write(stv_frame);
        lwrb.write(garbage);
        lwrb.write(mav_frame);
        lwrb.write(garbage);

        REQUIRE(parser.run());
        REQUIRE(stv_parsed_queue.size() == 1U);
        REQUIRE(mavlink_parsed_queue.size() == 1U);

        REQUIRE(stv_router.run() == 1U);
        REQUIRE(mavlink_router.run() == 1U);

        require_front_equals(stv_routed_queue, make_expected_stv_payload(
                                                   stv_payload, stv_dst_id));
        require_front_equals(
            mavlink_routed_queue,
            make_expected_mavlink_payload(mav_payload, mavlink_compid,
                                          {.msgid = mavlink_msgid_heartbeat}));
    }

    SECTION("corrupted crc of each protocol")
    {
        constexpr std::string_view stv_payload_one{"first"};
        constexpr std::string_view stv_payload_bad{"bad"};
        constexpr std::string_view stv_payload_two{"second"};
        constexpr std::string_view mav_payload_one{"ok"};
        constexpr std::string_view mav_payload_bad{"err"};
        constexpr std::string_view mav_payload_two{"fin"};

        auto stv_frame_bad = make_stv_frame(stv_payload_bad, stv_dst_id);
        auto mav_frame_bad =
            make_mavlink_frame(mav_payload_bad, mavlink_compid,
                               {.msgid = mavlink_msgid_heartbeat});

        // Портим последний байт CRC каждого кадра, не трогая их размер.
        stv_frame_bad.back() ^= std::byte{0xFF};
        mav_frame_bad.back() ^= std::byte{0xFF};

        lwrb.write(make_stv_frame(stv_payload_one, stv_dst_id));
        lwrb.write(stv_frame_bad);
        lwrb.write(make_mavlink_frame(mav_payload_one, mavlink_compid,
                                      {.msgid = mavlink_msgid_heartbeat}));
        lwrb.write(mav_frame_bad);
        lwrb.write(make_stv_frame(stv_payload_two, stv_dst_id));
        lwrb.write(make_mavlink_frame(mav_payload_two, mavlink_compid,
                                      {.msgid = mavlink_msgid_sys_status}));

        // Битые кадры отброшены, синхронизация потока не потеряна.
        REQUIRE(parser.run());
        REQUIRE(stv_parsed_queue.size() == 2U);
        REQUIRE(mavlink_parsed_queue.size() == 2U);

        REQUIRE(stv_router.run() == 2U);
        REQUIRE(mavlink_router.run() == 2U);

        require_front_equals(
            stv_routed_queue,
            make_expected_stv_payload(stv_payload_one, stv_dst_id));
        require_front_equals(
            stv_routed_queue,
            make_expected_stv_payload(stv_payload_two, stv_dst_id));
        require_front_equals(
            mavlink_routed_queue,
            make_expected_mavlink_payload(mav_payload_one, mavlink_compid,
                                          {.msgid = mavlink_msgid_heartbeat}));
        require_front_equals(
            mavlink_routed_queue,
            make_expected_mavlink_payload(mav_payload_two, mavlink_compid,
                                          {.msgid = mavlink_msgid_sys_status}));
    }

    SECTION("unknown msgid frame dropped without losing stream sync")
    {
        // Кадр с неизвестным провайдеру msgid нельзя собрать через
        // mavlink_v2_frame_tx (отправитель обязан знать CRC_EXTRA), поэтому
        // берём валидный кадр и подменяем в нём msgid на неизвестный.
        auto unknown_frame = make_mavlink_frame(
            "ok", mavlink_compid, {.msgid = mavlink_msgid_heartbeat});
        unknown_frame[mavlink_msgid_offset] = std::byte{0x2A}; // msgid = 42

        constexpr std::string_view stv_payload{"next"};
        constexpr std::string_view mav_payload{"last"};

        lwrb.write(unknown_frame);
        lwrb.write(make_stv_frame(stv_payload, stv_dst_id));
        lwrb.write(make_mavlink_frame(mav_payload, mavlink_compid,
                                      {.msgid = mavlink_msgid_heartbeat}));

        // Кадр с неизвестным msgid отброшен, следующие кадры распарсены.
        REQUIRE(parser.run());
        REQUIRE(stv_parsed_queue.size() == 1U);
        REQUIRE(mavlink_parsed_queue.size() == 1U);

        REQUIRE(stv_router.run() == 1U);
        REQUIRE(mavlink_router.run() == 1U);

        require_front_equals(stv_routed_queue, make_expected_stv_payload(
                                                   stv_payload, stv_dst_id));
        require_front_equals(
            mavlink_routed_queue,
            make_expected_mavlink_payload(mav_payload, mavlink_compid,
                                          {.msgid = mavlink_msgid_heartbeat}));
    }

    SECTION("frame split across runs")
    {
        constexpr std::string_view payload{"split"};
        const auto                 frame = make_mavlink_frame(
            payload, mavlink_compid, {.msgid = mavlink_msgid_heartbeat});

        REQUIRE(frame.size() > 5U);
        const std::size_t first_part_size = 5U;

        lwrb.write(frame.data(), first_part_size);
        REQUIRE_FALSE(parser.run());
        REQUIRE(mavlink_parsed_queue.empty());

        lwrb.write(frame.data() + first_part_size,
                   frame.size() - first_part_size);
        REQUIRE(parser.run());
        REQUIRE(mavlink_parsed_queue.size() == 1U);
        REQUIRE(stv_parsed_queue.empty());

        REQUIRE(mavlink_router.run() == 1U);
        require_front_equals(
            mavlink_routed_queue,
            make_expected_mavlink_payload(payload, mavlink_compid,
                                          {.msgid = mavlink_msgid_heartbeat}));
    }

    SECTION("false mavlink header discarded by timeout")
    {
        // Ложный заголовок 0xFD с len=200: парсер ждет кадр из 212 байт.
        // Байт следующего валидного кадра недостаточно для сборки ложного
        // кадра целиком, поэтому путь восстановления через ошибку CRC не
        // срабатывает - выход из ожидания возможен только по таймауту.
        constexpr std::array<std::byte, 3> false_header{
            std::byte{0xFD}, std::byte{200U}, std::byte{0x00}};

        constexpr std::string_view payload{"Hi"};
        const auto                 frame = make_mavlink_frame(
            payload, mavlink_compid, {.msgid = mavlink_msgid_heartbeat});

        lwrb.write(false_header);
        lwrb.write(frame);

        // Первый вызов находит ложный заголовок и переходит в ожидание
        // остатка кадра, фиксируя число байт в буфере.
        REQUIRE_FALSE(parser.run());
        REQUIRE(mavlink_parsed_queue.empty());

        // Таймаут: после max_wait_message_ready_polls опросов без новых
        // байт ложный заголовок отбрасывается.
        for(std::size_t i{0};
            i < decltype(parser)::max_wait_message_ready_polls; ++i)
        {
            REQUIRE_FALSE(parser.run());
        }

        // Валидный кадр, шедший в потоке следом за ложным заголовком,
        // распарсен после отбрасывания заголовка.
        REQUIRE(parser.run());
        REQUIRE(stv_parsed_queue.empty());
        REQUIRE(mavlink_parsed_queue.size() == 1U);

        REQUIRE(mavlink_router.run() == 1U);
        require_front_equals(
            mavlink_routed_queue,
            make_expected_mavlink_payload(payload, mavlink_compid,
                                          {.msgid = mavlink_msgid_heartbeat}));
    }

    SECTION("signed mavlink frame parsed through demux")
    {
        // Подписанный кадр: бит 0x01 в incompat_flags, после CRC идут 13
        // байт подписи. Подпись не верифицируется и остаётся в хвосте
        // отрезанного сообщения (задокументированное ограничение
        // mavlink_v2_frame), маршрутизация по compid при этом работает.
        constexpr std::string_view payload{"sig"};

        // Кадр собирается вручную: передающий декоратор подпись не
        // формирует. CRC считается по байтам len..payload + CRC_EXTRA
        // и НЕ покрывает подпись.
        std::vector<std::byte> signed_frame{
            std::byte{0xFD},          static_cast<std::byte>(payload.size()),
            std::byte{0x01},  // incompat_flags: кадр подписан
            std::byte{0x00},  // compat_flags
            std::byte{0x00},  // seq
            std::byte{mavlink_sysid}, std::byte{mavlink_compid},
            std::byte{0x00},          std::byte{0x00},
            std::byte{0x00}}; // msgid = 0

        const auto pload_bytes =
            std::as_bytes(std::span(payload.data(), payload.size()));
        signed_frame.insert(signed_frame.end(), pload_bytes.begin(),
                            pload_bytes.end());

        auto crc =
            stv::crc16_x25(signed_frame.data() + 1U, signed_frame.size() - 1U);
        crc = stv::crc16_x25_accumulate(crc, std::byte{50U});
        signed_frame.push_back(static_cast<std::byte>(crc & 0xFFU));
        signed_frame.push_back(static_cast<std::byte>(
            (static_cast<std::uint32_t>(crc) >> 8U) & 0xFFU));
        signed_frame.insert(signed_frame.end(), 13U, std::byte{0x5A});

        lwrb.write(signed_frame);
        lwrb.write(make_mavlink_frame("ok", mavlink_compid,
                                      {.msgid = mavlink_msgid_heartbeat}));

        // Подписанный кадр и следующий за ним обычный кадр распарсены.
        REQUIRE(parser.run());
        REQUIRE(stv_parsed_queue.empty());
        REQUIRE(mavlink_parsed_queue.size() == 2U);

        REQUIRE(mavlink_router.run() == 2U);

        // Парсер отрезает заголовок (3) и хвост trailer_size() = 2 байта
        // от конца кадра, поэтому в отрезанном сообщении остаются 2 байта
        // CRC и первые 11 байт подписи (задокументированное ограничение
        // mavlink_v2_frame: подпись не отделяется от сообщения).
        auto expected_signed = make_expected_mavlink_payload(
            payload, mavlink_compid, {.msgid = mavlink_msgid_heartbeat});
        expected_signed.push_back(static_cast<std::byte>(crc & 0xFFU));
        expected_signed.push_back(static_cast<std::byte>(
            (static_cast<std::uint32_t>(crc) >> 8U) & 0xFFU));
        expected_signed.insert(expected_signed.end(), 11U, std::byte{0x5A});

        require_front_equals(mavlink_routed_queue, expected_signed);
        require_front_equals(
            mavlink_routed_queue,
            make_expected_mavlink_payload("ok", mavlink_compid,
                                          {.msgid = mavlink_msgid_heartbeat}));
    }

    SECTION("oversized mavlink frame")
    {
        // Кадр MAVLink v2 с нагрузкой в 1 байт имеет общий размер 13 байт,
        // что больше установленного ограничения.
        setup.max_one_message_size = 12U;
        auto oversized_parser =
            stv::make_serial_parser<parser_setup_type, stv::stvlink_frame,
                                    mavlink_frame_type>(setup);
        REQUIRE(oversized_parser);

        constexpr std::string_view mav_payload{"A"};
        constexpr std::string_view stv_payload{"B"};
        const auto                 mav_frame = make_mavlink_frame(
            mav_payload, mavlink_compid, {.msgid = mavlink_msgid_heartbeat});
        const auto stv_frame = make_stv_frame(stv_payload, stv_dst_id);

        lwrb.write(mav_frame);
        lwrb.write(stv_frame);

        // Первый вызов отбрасывает слишком большой кадр MAVLink.
        REQUIRE_FALSE(oversized_parser.run());
        // Следующий вызов продолжает поиск и находит кадр stv_rx_v2.
        REQUIRE(oversized_parser.run());
        REQUIRE(mavlink_parsed_queue.empty());
        REQUIRE(stv_parsed_queue.size() == 1U);

        REQUIRE(stv_router.run() == 1U);
        require_front_equals(stv_routed_queue, make_expected_stv_payload(
                                                   stv_payload, stv_dst_id));
    }

    SECTION("external lwrb reset while parser waits for frame")
    {
        constexpr std::string_view payload{"reset"};
        const auto                 frame = make_mavlink_frame(
            payload, mavlink_compid, {.msgid = mavlink_msgid_heartbeat});

        REQUIRE(frame.size() > 5U);
        constexpr std::size_t first_part_size = 5U;

        lwrb.write(frame.data(), first_part_size);
        REQUIRE_FALSE(parser.run());
        REQUIRE(mavlink_parsed_queue.empty());

        // Внешний сброс буфера должен вернуть парсер в состояние поиска
        // начала кадра; следующий валидный кадр принимается корректно.
        lwrb.reset();
        lwrb.write(frame);

        REQUIRE(parser.run());
        REQUIRE(mavlink_parsed_queue.size() == 1U);
        REQUIRE(stv_parsed_queue.empty());

        REQUIRE(mavlink_router.run() == 1U);
        require_front_equals(
            mavlink_routed_queue,
            make_expected_mavlink_payload(payload, mavlink_compid,
                                          {.msgid = mavlink_msgid_heartbeat}));
    }
}

TEST_CASE(
    "serial_parser with external std::mutex", "[stv][communication]")
{
    constexpr std::size_t                       lwrb_buffer_size{256};
    stv::lwrb<lwrb_base_type, lwrb_buffer_size> lwrb{lwrb_setup_type{}};
    queue_type                                  stv_queue;

    std::mutex                                  parser_mutex;

    using parser_setup_type =
        stv::serial_parser_setup<lwrb_base_type, queue_type, std::mutex *,
                                 stv::stvlink_frame>;

    parser_setup_type setup;
    setup.lwrb  = &lwrb;
    setup.mutex = &parser_mutex;
    setup.set_queue<stv::stvlink_frame>(stv_queue);

    auto parser =
        stv::make_serial_parser<parser_setup_type, stv::stvlink_frame>(setup);
    REQUIRE(parser);

    constexpr std::string_view payload{"external mutex"};
    const auto                 frame = make_stv_frame(payload, stv_dst_id);
    lwrb.write(frame);

    REQUIRE(parser.run());
    REQUIRE(stv_queue.size() == 1U);

    require_front_equals(stv_queue,
                         make_expected_stv_payload(payload, stv_dst_id));
}

TEST_CASE(
    "serial_parser decorator order does not matter", "[stv][communication]")
{
    constexpr std::size_t                       lwrb_buffer_size{256};
    stv::lwrb<lwrb_base_type, lwrb_buffer_size> lwrb{lwrb_setup_type{}};
    queue_type                                  stv_queue;
    queue_type                                  mavlink_queue;

    // Setup задаёт декораторов в одном порядке, парсер — в другом.
    using parser_setup_type =
        stv::serial_parser_setup<lwrb_base_type, queue_type, stv::empty_mutex,
                                 stv::stvlink_frame, mavlink_frame_type>;

    parser_setup_type setup;
    setup.lwrb = &lwrb;
    setup.set_queue<stv::stvlink_frame>(stv_queue);
    setup.set_queue<mavlink_frame_type>(mavlink_queue);

    auto parser = stv::make_serial_parser<parser_setup_type, mavlink_frame_type,
                                          stv::stvlink_frame>(setup);
    REQUIRE(parser);

    constexpr std::string_view stv_payload{"order test"};
    constexpr std::string_view mav_payload{"mav"};
    const auto stv_frame = make_stv_frame(stv_payload, stv_dst_id);
    const auto mav_frame = make_mavlink_frame(
        mav_payload, mavlink_compid, {.msgid = mavlink_msgid_heartbeat});

    lwrb.write(stv_frame);
    lwrb.write(mav_frame);

    REQUIRE(parser.run());
    REQUIRE(stv_queue.size() == 1U);
    REQUIRE(mavlink_queue.size() == 1U);

    require_front_equals(stv_queue,
                         make_expected_stv_payload(stv_payload, stv_dst_id));
    require_front_equals(
        mavlink_queue,
        make_expected_mavlink_payload(mav_payload, mavlink_compid,
                                      {.msgid = mavlink_msgid_heartbeat}));
}

// NOLINTEND(readability-function-cognitive-complexity)

// NOLINTEND(*-magic-numbers)
