/// @file test_stvlink_dispatcher.cpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.

#include "stv/communication/serial_sender.hpp"
#include "stv/communication/stvlink_dispatcher.hpp"
#include "stv/communication/stvlink_parser.hpp"
#include "stv/communication/stvlink_sender.hpp"
#include "stv/communication/stvlinkv2_parser.hpp"
#include "stv/communication/stvlinkv2_sender.hpp"
#include "stv/containers/simbuff.hpp"
#include <array>
#include <catch2/catch_test_macros.hpp>
#include <cstddef>
#include <cstdint>
#include <etl/queue.h>
#include <etl/queue_spsc_atomic.h>
#include <etl/unordered_map.h>
#include <gsl/gsl>
#include <utility>

namespace {

/// @brief Структура-заглушка полезной нагрузки.
/// @details Диспетчер payload-независим, поэтому вместо реальной
/// payload-структуры интегрирующего проекта (заголовки karavan в тестах
/// stv недоступны — линкуется только stv::stv) используется тривиальная
/// структура подходящего размера.
struct telemetry_stub {
    std::array<std::byte, 40U> raw{};
};

bool telemetry_processing(
    stv::stvlink_message_span pload)
{
    auto success{false};

    if(pload.size_bytes() == sizeof(telemetry_stub))
    {
        success = true;
    }

    return success;
}

constexpr std::pair<int, stv::stvlink_message_handler_fnc_type>
    telemetry_processing_pair{0, telemetry_processing};

/// @brief Минимальная очередь сообщений, удовлетворяющая контракту
///     stv::stvlink_queue_concept.
/// @details Не наследуется от ETL-интерфейсов (etl::iqueue и др.): модуль
///     обязан работать с любой очередью, реализующей контракт, а не только с
///     очередями ETL.
/// @tparam T Тип элемента очереди.
/// @tparam Capacity Максимальное число элементов в очереди.
template<typename T, std::size_t Capacity>
class stub_queue
{
  public:
    /// @brief Тип элемента очереди.
    using value_type = T;

    /// @brief Проверяет, пуста ли очередь.
    /// @return true если очередь пуста, false в противном случае.
    [[nodiscard]] auto empty() const { return size_ == 0U; }

    /// @brief Возвращает ссылку на первый элемент очереди.
    /// @return Ссылку на первый элемент очереди.
    auto front() -> value_type &
    { return gsl::at(buffer_, static_cast<gsl::index>(head_)); }

    /// @brief Удаляет первый элемент из очереди.
    void pop()
    {
        head_ = (head_ + 1U) % Capacity;
        --size_;
    }

    /// @brief Добавляет сообщение в конец очереди (для наполнения в тесте).
    /// @param[in,out] msg Добавляемое сообщение.
    void push(
        value_type &&msg)
    {
        gsl::at(buffer_, static_cast<gsl::index>(tail_)) = std::move(msg);
        tail_ = (tail_ + 1U) % Capacity;
        ++size_;
    }

    /// @brief Возвращает число элементов в очереди.
    /// @return Число элементов в очереди.
    [[nodiscard]] auto size() const { return size_; }

  private:
    std::array<value_type, Capacity> buffer_{};
    std::size_t                      head_{0U};
    std::size_t                      tail_{0U};
    std::size_t                      size_{0U};
};

/// @brief Очередь без метода pop(): контракту stv::stvlink_queue_concept не
///     удовлетворяет (для негативной проверки концепта).
struct queue_without_pop {
    using value_type = stv::sim_buff<stv::empty_mutex>;

    [[nodiscard]] static auto empty() -> bool { return true; }

    auto                      front() -> value_type &;
};

static_assert(stv::stvlink_queue_concept<
                  stub_queue<stv::sim_buff<stv::empty_mutex>, 10U>>,
              "stub_queue must satisfy stvlink_queue_concept");
static_assert(stv::stvlink_queue_concept<
                  etl::queue<stv::sim_buff<stv::empty_mutex>, 10U>>,
              "etl::queue must satisfy stvlink_queue_concept");
static_assert(stv::stvlink_queue_concept<
                  etl::queue_spsc_atomic<stv::sim_buff<stv::empty_mutex>, 10U>>,
              "etl::queue_spsc_atomic must satisfy stvlink_queue_concept");
static_assert(!stv::stvlink_queue_concept<queue_without_pop>,
              "a queue without pop() must not satisfy stvlink_queue_concept");

/// @brief Очередь сообщений теста диспетчера stvlinkv2.
using v2_queue = etl::queue<stv::sim_buff<stv::empty_mutex>, 10>;

/// @brief Настройки диспетчера stvlinkv2 для теста.
using v2_setup = stv::stvlink_setup<
    etl::iunordered_map<int, stv::stvlink_message_handler_fnc_type>>;

/// @brief Тип диспетчера сообщений stvlinkv2 с передатчиком stvlinkv2.
using v2_module =
    stv::stvlink_module<v2_queue, v2_setup, stv::stvlinkv2_sender>;

/// @brief Тип передатчика сериализованных сообщений stvlinkv2.
using v2_serial_sender = decltype(make_serial_message_buffer(
    std::declval<v2_queue &>(), stv::stvlinkv2_sender{}));

/// @brief Общие объекты теста диспетчера stvlinkv2.
///
/// @details Идентификатор сообщения больше 0xFF: при чтении заголовка не
///     тем протоколом (например, stvlink вместо stvlinkv2) значение msg_id
///     читалось бы со смещения на 1 байт и обработчик не был бы найден.
struct v2_dispatcher_fixture {
    static constexpr std::uint16_t telemetry_msg_id{0x0123U};

    etl::unordered_map<int, stv::stvlink_message_handler_fnc_type, 1U>
                     processing_hash_table;
    v2_queue         serial_sender_queue;
    v2_serial_sender serial_sender;
    v2_queue         module_queue;
    v2_module        module;

    v2_dispatcher_fixture():
        processing_hash_table(
            etl::make_unordered_map<int, stv::stvlink_message_handler_fnc_type>(
                std::make_pair(static_cast<int>(telemetry_msg_id),
                               telemetry_processing))),
        serial_sender(make_serial_message_buffer(serial_sender_queue,
                                                 stv::stvlinkv2_sender{})),
        module(module_queue, v2_setup{.hash_table = &processing_hash_table})
    {
    }
};

/// @brief Перемещает сериализованные сообщения из очереди передатчика в
///     очередь обработчика.
///
/// @details Парсер на приёмной стороне отрезает стартовый кадр и CRC,
///     а заголовок сообщения оставляет в начале сообщения -- эмулируем
///     это здесь.
///
/// @param[in,out] serial_sender_queue Очередь сериализованных сообщений.
/// @param[in,out] module_queue Очередь обработчика.
// NOLINTBEGIN(bugprone-easily-swappable-parameters) -- очереди разного
// назначения, порядок подсказан именами.
void move_serialized_to_module_queue(
    v2_queue &serial_sender_queue, v2_queue &module_queue)
// NOLINTEND(bugprone-easily-swappable-parameters)
{
    while(!serial_sender_queue.empty())
    {
        decltype(auto) msg = serial_sender_queue.front();
        msg.trim_head(stv::stvlinkv2_parser::header_size());
        msg.trim_tail(stv::stvlinkv2_parser::trailer_size());
        module_queue.push(std::move(msg));
        serial_sender_queue.pop();
    }
}

/// @brief Проверяет перемещенное сообщение: обработано и удалено из
///     очереди.
///
/// @param[in,out] fixture Объекты теста.
/// @param[in] expected_status Ожидаемый результат process_one().
void require_moved_message_processed(
    v2_dispatcher_fixture &fixture, bool expected_status)
{
    REQUIRE(!fixture.serial_sender_queue.empty());
    move_serialized_to_module_queue(fixture.serial_sender_queue,
                                    fixture.module_queue);

    // После вызова process_one() размер очереди должен уменьшиться
    // т.к. обработанное сообщение удаляется из очереди.
    const auto queue_size = fixture.module_queue.size();
    REQUIRE(fixture.module.process_one() == expected_status);
    REQUIRE(fixture.module_queue.size() == queue_size - 1);
}

/// @brief Проверяет обработку одного сообщения (валидный и невалидный
///     идентификаторы).
void require_v2_processing_messages()
{
    v2_dispatcher_fixture fixture{};
    REQUIRE(fixture.module);

    auto expected_queue_processing_status{false};
    SECTION("Send telemetry")
    {
        auto msg = fixture.serial_sender.request<telemetry_stub>(
            stv::stvlinkv2_sender::setup_t{
                .msg_id = v2_dispatcher_fixture::telemetry_msg_id,
            });
        msg->raw[0] = std::byte{10};

        expected_queue_processing_status = true;
        // В деструкторе msg выполняется сериализация сообщения.
    }

    SECTION("Send invalid msg id")
    {
        const auto msg = fixture.serial_sender.request<telemetry_stub>(
            stv::stvlinkv2_sender::setup_t{.msg_id = 10U});

        expected_queue_processing_status = false;
        // В деструкторе msg выполняется сериализация сообщения.
    }

    require_moved_message_processed(fixture, expected_queue_processing_status);
}

/// @brief Проверяет, что process_all обрабатывает очередь из нескольких
///     сообщений.
void require_v2_process_all_drains_queue()
{
    constexpr auto        messages_count{3U};

    v2_dispatcher_fixture fixture{};
    REQUIRE(fixture.module);

    for(std::size_t i{0U}; i < messages_count; ++i)
    {
        auto msg = fixture.serial_sender.request<telemetry_stub>(
            stv::stvlinkv2_sender::setup_t{
                .msg_id = v2_dispatcher_fixture::telemetry_msg_id,
            });
        msg->raw[0] = std::byte{10};
        // В деструкторе msg выполняется сериализация сообщения.
    }

    move_serialized_to_module_queue(fixture.serial_sender_queue,
                                    fixture.module_queue);
    REQUIRE(fixture.module_queue.size() == messages_count);

    fixture.module.process_all();

    REQUIRE(fixture.module_queue.empty());
}

/// @brief Проверяет, что process_all с пустой очередью ничего не
///     обрабатывает.
void require_v2_process_all_with_empty_queue()
{
    v2_dispatcher_fixture fixture{};
    REQUIRE(fixture.module);

    REQUIRE(fixture.module_queue.empty());

    fixture.module.process_all();

    REQUIRE(fixture.module_queue.empty());
}

} // namespace

TEST_CASE(
    "stvlink_dispatcher", "[stv][stvlink]")
{
    const auto processing_hash_table =
        etl::make_unordered_map<int, stv::stvlink_message_handler_fnc_type>(
            telemetry_processing_pair);

    using sim_buff_type = stv::sim_buff<stv::empty_mutex>;
    using hash_table_type =
        etl::iunordered_map<int, stv::stvlink_message_handler_fnc_type>;
    using queue_type = etl::queue<sim_buff_type, 10>;
    queue_type serial_sender_queue{};

    auto       serial_sender =
        make_serial_message_buffer(serial_sender_queue, stv::stvlink_sender{});

    using setup_type  = stv::stvlink_setup<hash_table_type>;
    using module_type = stv::stvlink_module<queue_type, setup_type>;

    queue_type module_queue;

    SECTION("Create invalid object with default setup")
    { REQUIRE_FALSE(module_type{module_queue, setup_type{}}); }

    module_type module{module_queue,
                       setup_type{.hash_table = &processing_hash_table}};
    REQUIRE(module);

    SECTION("Processing messages")
    {
        auto expected_queue_processing_status{false};
        SECTION("Send telemetry")
        {
            constexpr auto msg_id{std::get<0>(telemetry_processing_pair)};

            auto           msg = serial_sender.request<telemetry_stub>(
                stv::stvlink_sender::setup_t{
                    .dst_id = 111, ///< в данном тесте не имеет значения,
                                   ///< т.к. приёмник по dst_id не
                                   ///< фильтрует.
                    .msg_id = static_cast<
                        decltype(stv::stvlink_sender::setup_t::msg_id)>(msg_id),
                });
            msg->raw[0] = std::byte{10};

            expected_queue_processing_status = true;
            // В деструкторе msg выполняется сериализация сообщения.
        }

        SECTION("Send invalid msg id")
        {
            const auto msg = serial_sender.request<telemetry_stub>(
                stv::stvlink_sender::setup_t{
                    .dst_id = 111, ///< в данном тесте не имеет значения,
                                   ///< т.к. приёмник по dst_id не
                                   ///< фильтрует.
                    .msg_id = static_cast<
                        decltype(stv::stvlink_sender::setup_t::msg_id)>(10),
                });

            expected_queue_processing_status = false;
            // В деструкторе msg выполняется сериализация сообщения.
        }

        REQUIRE(!serial_sender_queue.empty());

        /// Перемещение сериализованного сообщения из очереди передатчика в
        /// очередь обработчика. Парсер на приёмной стороне отрезает
        /// стартовый кадр и CRC, а заголовок сообщения оставляет в начале
        /// сообщения -- эмулируем это здесь.
        decltype(auto) msg = serial_sender_queue.front();
        msg.trim_head(stv::stvlink_parser::header_size());
        msg.trim_tail(stv::stvlink_parser::trailer_size());
        module_queue.push(std::move(msg));
        serial_sender_queue.pop();

        // После вызова process_one() размер очереди должен уменьшиться
        // т.к. обработанное сообщение удаляется из очереди.
        const auto queue_size = module_queue.size();
        REQUIRE(module.process_one() == expected_queue_processing_status);
        REQUIRE(module_queue.size() == queue_size - 1);
    }
}

TEST_CASE(
    "stvlink_dispatcher with stvlinkv2_sender", "[stv][stvlink][stvlinkv2]")
{
    SECTION("Create invalid object with default setup")
    {
        v2_queue module_queue;
        REQUIRE_FALSE(v2_module{module_queue, v2_setup{}});
    }

    SECTION("Processing messages") { require_v2_processing_messages(); }

    SECTION("process_all drains a queue of several messages")
    { require_v2_process_all_drains_queue(); }

    SECTION("process_all with empty queue processes nothing")
    { require_v2_process_all_with_empty_queue(); }
}

TEST_CASE(
    "stvlink_module with non-ETL queue", "[stv][stvlink]")
{
    const auto processing_hash_table =
        etl::make_unordered_map<int, stv::stvlink_message_handler_fnc_type>(
            telemetry_processing_pair);

    using sim_buff_type = stv::sim_buff<stv::empty_mutex>;
    using hash_table_type =
        etl::iunordered_map<int, stv::stvlink_message_handler_fnc_type>;

    // Очередь передатчика остаётся ETL, очередь модуля -- не-ETL заглушка:
    // модуль обязан принимать любую очередь по контракту
    // stv::stvlink_queue_concept.
    etl::queue<sim_buff_type, 10> serial_sender_queue{};
    auto                          serial_sender =
        make_serial_message_buffer(serial_sender_queue, stv::stvlink_sender{});

    using setup_type  = stv::stvlink_setup<hash_table_type>;
    using queue_type  = stub_queue<sim_buff_type, 10U>;
    using module_type = stv::stvlink_module<queue_type, setup_type>;

    queue_type  module_queue;
    module_type module{module_queue,
                       setup_type{.hash_table = &processing_hash_table}};
    REQUIRE(module);

    {
        constexpr auto msg_id{std::get<0>(telemetry_processing_pair)};

        auto           msg =
            serial_sender.request<telemetry_stub>(stv::stvlink_sender::setup_t{
                .dst_id = 111, ///< в данном тесте не имеет значения,
                               ///< т.к. приёмник по dst_id не фильтрует.
                .msg_id =
                    static_cast<decltype(stv::stvlink_sender::setup_t::msg_id)>(
                        msg_id),
            });
        msg->raw[0] = std::byte{10};
        // В деструкторе msg выполняется сериализация сообщения.
    }

    /// Перемещение сериализованного сообщения из очереди передатчика в
    /// очередь обработчика. Парсер на приёмной стороне отрезает стартовый
    /// кадр и CRC, а заголовок сообщения оставляет в начале сообщения --
    /// эмулируем это здесь.
    decltype(auto) msg = serial_sender_queue.front();
    msg.trim_head(stv::stvlink_parser::header_size());
    msg.trim_tail(stv::stvlink_parser::trailer_size());
    module_queue.push(std::move(msg));
    serial_sender_queue.pop();

    // Сообщение обработано и удалено из очереди.
    REQUIRE(module.process_one());
    REQUIRE(module_queue.empty());
}
