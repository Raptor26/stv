/// @file test_serial_parser.cpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.

#include <array>
#include <catch2/catch_test_macros.hpp>
#include <cstddef>
#include <cstring>
#include <etl/queue.h>
#include <etl/utility.h>
#include <new>
#include <string_view>
#include <utility>

#include "stv/communication/parsed_queue.hpp"
#include "stv/communication/serial_decorators.hpp"
#include "stv/communication/serial_parser.hpp"
#include "stv/communication/serial_sender.hpp"
#include "stv/communication/stvlink_parser.hpp"
#include "stv/communication/stvlink_sender.hpp"
#include "stv/containers/lwrb.hpp"
#include "stv/containers/simbuff.hpp"
#include "stv/mutex_guard.hpp"

// NOLINTBEGIN(*-magic-numbers, google-build-using-namespace)
// NOLINTBEGIN(readability-function-cognitive-complexity)
// NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables)

namespace {

template<typename T>
class custom_allocator
{
    inline static std::size_t alloc_size{};

  public:
    using value_type = T;

    custom_allocator() = default;

    template<typename U>
    explicit custom_allocator(
        const custom_allocator<U> & /*unused*/)
    {
    }

    T *allocate(
        std::size_t n)
    {
        alloc_size = n;
        ++alloc_cnt;
        return reinterpret_cast<T *>(memory_to_serial_parser.data());
    }

    void deallocate(
        T *ptr, std::size_t n)
    {
        (void)ptr;
        (void)n;
        --alloc_cnt;
    }

    static auto           get_allocator_cnt() { return alloc_cnt; }

    static constexpr auto get_mem_ptr()
    { return memory_to_serial_parser.data(); }

    static auto data() { return memory_to_serial_parser.data(); }

    static auto get_last_alloc_size() { return alloc_size; }

  private:
    inline static std::array<char, 64> memory_to_serial_parser{};
    inline static int                  alloc_cnt{};
};

} // namespace

TEST_CASE(
    "Serial Parser", "[stv][communication]")
{
    // using custom_allocator = custom_allocator<std::byte>;
    // using sim_buffer_with_custom_allocator_type =
    //     stv::sim_buff<stv::empty_mutex, custom_allocator>;
    using sim_buffer_type = stv::sim_buff<stv::empty_mutex>;
    using queue_type      = etl::queue<sim_buffer_type, 10>;
    queue_type parsed_msg_queue;
    queue_type serial_msg_queue;

    using lwrb_setup_type = stv::lwrb_setup<stv::empty_mutex>;
    using lwrb_base_type  = stv::lwrb_base<lwrb_setup_type>;
    constexpr std::size_t                       lwrb_buffer_size{128};
    stv::lwrb<lwrb_base_type, lwrb_buffer_size> lwrb{lwrb_setup_type{}};
    SECTION("Serial Parser")
    {
        /// @brief Объект используется для создания сообщений требуемой для
        /// проверки парсера структуры.
        auto serial_message_buffer =
            make_serial_message_buffer(serial_msg_queue, stv::stvlink_sender{});

        using serial_parser_setup_type =
            stv::serial_parser_setup<lwrb_base_type, queue_type,
                                     stv::empty_mutex, stv::stvlink_parser>;

        SECTION("Invalid Setup")
        {
            const serial_parser_setup_type setup;
            auto parser = stv::make_serial_parser<serial_parser_setup_type,
                                                  stv::stvlink_parser>(setup);
            REQUIRE_FALSE(parser);
        }

        SECTION("Valid Setup")
        {
            serial_parser_setup_type setup;
            setup.lwrb = &lwrb;
            setup.set_queue<stv::stvlink_parser>(parsed_msg_queue);

            auto parser = stv::make_serial_parser<serial_parser_setup_type,
                                                  stv::stvlink_parser>(setup);
            REQUIRE(parser);

            SECTION("Parse per message")
            {
                constexpr std::string_view test_message{"Hello world"};

                SECTION("Invalid message only")
                {
                    lwrb.write(test_message);
                    REQUIRE_FALSE(parser.run());
                }

                SECTION("Without offset")
                {
                    {
                        const auto msg =
                            serial_message_buffer.request(test_message);

                        // В деструкторе msg будет записан вызван декоратор,
                        // который вычислит CRC.
                    }

                    decltype(auto) queue_instance =
                        serial_message_buffer.queue_instance();
                    auto msg = queue_instance.front();
                    lwrb.write(msg.begin(), msg.end());
                    REQUIRE(parser.run());
                    queue_instance.pop();
                }

                SECTION("With offset")
                {
                    // Вначале запишем просто тексТ, без начала кадра и
                    // контрольной суммы.
                    lwrb.write(test_message);

                    {
                        // А это уже запись обернутого с помощью декоратора
                        // сообщения.
                        const auto msg =
                            serial_message_buffer.request(test_message);

                        // В деструкторе msg будет записан вызван декоратор,
                        // который вычислит CRC.
                    }

                    decltype(auto) queue_instance =
                        serial_message_buffer.queue_instance();
                    auto msg = queue_instance.front();
                    lwrb.write(msg.begin(), msg.end());
                    REQUIRE(parser.run());
                    queue_instance.pop();
                }

                if(!parsed_msg_queue.empty())
                {
                    auto msg = parsed_msg_queue.front();

                    // Парсер отрезает стартовый кадр и CRC, а заголовок
                    // сообщения (dst_id, msg_id, pload_size) оставляет в
                    // начале сообщения: срежем его перед сверкой нагрузки.
                    msg.trim_head(stv::stvlink_sender::message_header_size());

                    REQUIRE(memcmp(test_message.data(), msg.data(),
                                   test_message.size())
                            == 0);

                    REQUIRE(msg.begin() == msg.data<std::byte>());
                    REQUIRE(msg.end() == msg.data<std::byte>() + msg.size());

                    parsed_msg_queue.pop();
                }
            }

            SECTION("Parse chain of a messages")
            {
                constexpr std::string_view test_message_one{"Hello"};
                constexpr std::string_view test_message_two{"World"};

                {
                    const auto msg =
                        serial_message_buffer.request(test_message_one);

                    // В деструкторе msg будет записан вызван декоратор, который
                    // вычислит CRC.
                }

                {
                    const auto msg =
                        serial_message_buffer.request(test_message_two);

                    // В деструкторе msg будет записан вызван декоратор, который
                    // вычислит CRC.
                }

                decltype(auto) queue_instance =
                    serial_message_buffer.queue_instance();

                {
                    decltype(auto) msg = queue_instance.front();
                    lwrb.write(msg.begin(), msg.end());
                    queue_instance.pop();
                }

                {
                    decltype(auto) msg = queue_instance.front();
                    lwrb.write(msg.begin(), msg.end());
                    queue_instance.pop();
                }

                REQUIRE(parser.run());

                {
                    auto msg = parsed_msg_queue.front();
                    msg.trim_head(stv::stvlink_sender::message_header_size());
                    REQUIRE(memcmp(test_message_one.data(), msg.data(),
                                   test_message_one.size())
                            == 0);
                    parsed_msg_queue.pop();
                }

                {
                    auto msg = std::move(parsed_msg_queue.front());
                    msg.trim_head(stv::stvlink_sender::message_header_size());
                    REQUIRE(memcmp(test_message_two.data(), msg.data(),
                                   test_message_two.size())
                            == 0);
                    parsed_msg_queue.pop();
                }
            }

            SECTION("Parse chain of a messages if first invalid")
            {
                constexpr std::string_view test_message_one{"Hello"};
                constexpr std::string_view test_message_two{"World"};

                {
                    const auto msg =
                        serial_message_buffer.request(test_message_one);

                    // В деструкторе msg будет записан вызван декоратор, который
                    // вычислит CRC.
                }

                {
                    const auto msg =
                        serial_message_buffer.request(test_message_two);

                    // В деструкторе msg будет записан вызван декоратор, который
                    // вычислит CRC.
                }

                decltype(auto) queue_instance =
                    serial_message_buffer.queue_instance();

                {
                    decltype(auto) msg = queue_instance.front();

                    // Внесем ошибку в сообщение.
                    auto iter  = msg.begin();
                    iter      += 2;
                    *iter      = std::byte{0xFF};
                    lwrb.write(msg.begin(), msg.end());
                    queue_instance.pop();
                }

                {
                    decltype(auto) msg = queue_instance.front();
                    lwrb.write(msg.begin(), msg.end());
                    queue_instance.pop();
                }

                // В первом сообщении испорчен размер сообщения, поэтому парсер
                // сброситься в поиск начала кадра.
                REQUIRE_FALSE(parser.run());
                REQUIRE(parser.run());

                {
                    auto msg = std::move(parsed_msg_queue.front());
                    msg.trim_head(stv::stvlink_sender::message_header_size());
                    REQUIRE(memcmp(test_message_two.data(), msg.data(),
                                   test_message_two.size())
                            == 0);
                    parsed_msg_queue.pop();
                }
            }

            SECTION("False start frame does not consume valid next message")
            {
                // В потоке байт перед валидным сообщением встречается
                // последовательность, похожая на начало кадра (0xAA 0xAA),
                // за которой следует размер и достаточное количество байт.
                // CRC для этой последовательности не сойдется. Если при
                // этом байты были удалены из буфера через read(), то
                // следующее валидное сообщение будет потеряно.

                constexpr std::array<std::byte, 14> false_frame{
                    std::byte{0xAA}, std::byte{0xAA}, std::byte{0x0C},
                    std::byte{0x00}, std::byte{0x01}, std::byte{0x02},
                    std::byte{0x03}, std::byte{0x04}, std::byte{0x05},
                    std::byte{0x06}, std::byte{0x07}, std::byte{0x08},
                    std::byte{0x09}, std::byte{0x0A},
                };

                constexpr std::string_view test_message{"B"};

                {
                    const auto msg =
                        serial_message_buffer.request(test_message);
                    (void)msg;
                }

                decltype(auto) queue_instance =
                    serial_message_buffer.queue_instance();

                REQUIRE(queue_instance.size() == 1);

                // Записываем вначале мусор, чтобы парсер не сразу нашел
                // валидный заголовок, а наткнулся на ложный кадр.
                constexpr std::array<std::byte, 1> garbage{std::byte{0x00}};
                lwrb.write(garbage);

                lwrb.write(false_frame);

                {
                    decltype(auto) msg = queue_instance.front();
                    lwrb.write(msg.begin(), msg.end());
                    queue_instance.pop();
                }

                // Первый вызов должен обнаружить ложный заголовок, не съесть
                // при этом валидное сообщение и в итоге его распарсить.
                REQUIRE(parser.run());

                REQUIRE(parsed_msg_queue.size() == 1);

                {
                    auto msg = std::move(parsed_msg_queue.front());
                    // В распарсенном сообщении перед нагрузкой остаётся
                    // заголовок сообщения (dst_id, msg_id, pload_size).
                    msg.trim_head(stv::stvlink_sender::message_header_size());
                    REQUIRE(msg.size() == test_message.size());
                    REQUIRE(memcmp(test_message.data(), msg.data(),
                                   test_message.size())
                            == 0);
                    parsed_msg_queue.pop();
                }
            }

            SECTION("Incomplete frame waits for remaining bytes")
            {
                constexpr std::string_view test_message{"Hello world"};

                {
                    const auto msg =
                        serial_message_buffer.request(test_message);
                }

                decltype(auto) queue_instance =
                    serial_message_buffer.queue_instance();
                auto msg = queue_instance.front();

                // Записываем только часть кадра: заголовок и часть полезной
                // нагрузки.
                constexpr std::size_t partial_size{8};
                lwrb.write(msg.begin(), msg.begin() + partial_size);

                // Парсер должен дождаться остатка кадра, не помещая
                // сообщение в очередь и не блокируясь внутри run().
                REQUIRE_FALSE(parser.run());
                REQUIRE(parsed_msg_queue.empty());

                // Дописываем остаток кадра: сообщение должно быть
                // распарсено на следующем вызове run().
                lwrb.write(msg.begin() + partial_size, msg.end());
                queue_instance.pop();

                REQUIRE(parser.run());
                REQUIRE(parsed_msg_queue.size() == 1);

                {
                    auto parsed_msg = std::move(parsed_msg_queue.front());
                    parsed_msg.trim_head(
                        stv::stvlink_sender::message_header_size());
                    REQUIRE(memcmp(test_message.data(), parsed_msg.data(),
                                   test_message.size())
                            == 0);
                    parsed_msg_queue.pop();
                }
            }

            SECTION("Stalled frame recovered by timeout")
            {
                constexpr std::string_view test_message{"Hello world"};

                {
                    const auto msg =
                        serial_message_buffer.request(test_message);
                }

                decltype(auto) queue_instance =
                    serial_message_buffer.queue_instance();

                // Записываем только заголовок кадра: отправитель «оборвал»
                // передачу посреди кадра.
                {
                    decltype(auto) msg = queue_instance.front();
                    lwrb.write(msg.begin(),
                               msg.begin()
                                   + static_cast<std::ptrdiff_t>(
                                       stv::stvlink_sender::header_size()));
                    queue_instance.pop();
                }

                // Каждый вызов run() - это опрос неполного кадра. Первый
                // вызов фиксирует число байт в буфере, каждый следующий
                // без новых байт - тик таймаута: после
                // max_wait_message_ready_polls тиков парсер отбрасывает
                // стартовый кадр и возвращается к поиску заголовка.
                for(std::size_t i{0};
                    i < (decltype(parser)::max_wait_message_ready_polls + 1U);
                    ++i)
                {
                    REQUIRE_FALSE(parser.run());
                }

                // Восстановление произошло именно по таймауту, а не по
                // приходу новых байт: отброшен один байт ложного заголовка,
                // остальные байты будут отброшены при обработке следующего
                // валидного кадра.
                REQUIRE(lwrb.get_full()
                        == (stv::stvlink_sender::header_size() - 1U));

                // После восстановления следующее валидное сообщение должно
                // быть распарсено.
                {
                    const auto msg =
                        serial_message_buffer.request(test_message);
                }

                {
                    decltype(auto) msg = queue_instance.front();
                    lwrb.write(msg.begin(), msg.end());
                    queue_instance.pop();
                }

                REQUIRE(parser.run());
                REQUIRE(parsed_msg_queue.size() == 1);

                {
                    auto parsed_msg = std::move(parsed_msg_queue.front());
                    parsed_msg.trim_head(
                        stv::stvlink_sender::message_header_size());
                    REQUIRE(memcmp(test_message.data(), parsed_msg.data(),
                                   test_message.size())
                            == 0);
                    parsed_msg_queue.pop();
                }
            }

            SECTION("Full output queue drops message without corruption")
            {
                constexpr std::string_view test_message{"Hello world"};

                decltype(auto)             queue_instance =
                    serial_message_buffer.queue_instance();

                // Заполняем выходную очередь до ее емкости.
                for(std::size_t i{0}; i < parsed_msg_queue.capacity(); ++i)
                {
                    {
                        const auto msg =
                            serial_message_buffer.request(test_message);
                    }

                    decltype(auto) msg = queue_instance.front();
                    lwrb.write(msg.begin(), msg.end());
                    queue_instance.pop();

                    REQUIRE(parser.run());
                    REQUIRE(parsed_msg_queue.size() == i + 1);
                }

                // Очередь заполнена: следующее сообщение должно быть
                // отброшено без порчи очереди.
                {
                    const auto msg =
                        serial_message_buffer.request(test_message);
                }

                {
                    decltype(auto) msg = queue_instance.front();
                    lwrb.write(msg.begin(), msg.end());
                    queue_instance.pop();
                }

                REQUIRE_FALSE(parser.run());
                REQUIRE(parsed_msg_queue.full());

                // Очередь не сломана: после освобождения места разбор
                // продолжается.
                while(!parsed_msg_queue.empty())
                {
                    parsed_msg_queue.pop();
                }

                {
                    const auto msg =
                        serial_message_buffer.request(test_message);
                }

                {
                    decltype(auto) msg = queue_instance.front();
                    lwrb.write(msg.begin(), msg.end());
                    queue_instance.pop();
                }

                REQUIRE(parser.run());
                REQUIRE(parsed_msg_queue.size() == 1);

                {
                    auto parsed_msg = std::move(parsed_msg_queue.front());
                    parsed_msg.trim_head(
                        stv::stvlink_sender::message_header_size());
                    REQUIRE(memcmp(test_message.data(), parsed_msg.data(),
                                   test_message.size())
                            == 0);
                    parsed_msg_queue.pop();
                }
            }
        }
    }

    SECTION("Parsed message keeps message header")
    {
        // Парсер отрезает стартовый кадр и CRC, а заголовок сообщения
        // (dst_id, msg_id, pload_size) оставляет в начале распарсенного
        // сообщения: потребитель читает из него идентификатор сообщения.
        auto serial_message_buffer =
            make_serial_message_buffer(serial_msg_queue, stv::stvlink_sender{});

        using serial_parser_setup_type =
            stv::serial_parser_setup<lwrb_base_type, queue_type,
                                     stv::empty_mutex, stv::stvlink_parser>;

        serial_parser_setup_type setup;
        setup.lwrb = &lwrb;
        setup.set_queue<stv::stvlink_parser>(parsed_msg_queue);

        auto parser = stv::make_serial_parser<serial_parser_setup_type,
                                              stv::stvlink_parser>(setup);
        REQUIRE(parser);

        constexpr std::string_view test_message{"Hello world"};
        constexpr std::uint8_t     dst_id{42U};
        constexpr std::uint8_t     msg_id{24U};

        {
            const auto msg = serial_message_buffer.request(
                test_message, stv::stvlink_sender::setup_t{
                                  .dst_id = dst_id,
                                  .msg_id = msg_id,
                              });
            (void)msg;
        }

        decltype(auto) queue_instance = serial_message_buffer.queue_instance();
        {
            decltype(auto) msg = queue_instance.front();
            lwrb.write(msg.begin(), msg.end());
            queue_instance.pop();
        }

        REQUIRE(parser.run());
        REQUIRE(parsed_msg_queue.size() == 1U);

        {
            auto        msg = std::move(parsed_msg_queue.front());

            const auto *header =
                reinterpret_cast<const stv::stvlink_sender::message_header_t *>(
                    msg.data());
            REQUIRE(header->dst_id == dst_id);
            REQUIRE(header->msg_id == msg_id);
            REQUIRE(header->pload_size == test_message.size());

            msg.trim_head(stv::stvlink_sender::message_header_size());
            REQUIRE(memcmp(test_message.data(), msg.data(), test_message.size())
                    == 0);

            parsed_msg_queue.pop();
        }
    }

    SECTION("null queue of any decorator")
    {
        using serial_parser_setup_type =
            stv::serial_parser_setup<lwrb_base_type, queue_type,
                                     stv::empty_mutex, stv::stvlink_parser>;
        serial_parser_setup_type setup;
        setup.lwrb = &lwrb;
        setup.set_queue<stv::stvlink_parser>(
            static_cast<queue_type *>(nullptr));

        auto parser = stv::make_serial_parser<serial_parser_setup_type,
                                              stv::stvlink_parser>(setup);
        REQUIRE_FALSE(parser);
    }

    SECTION("single decorator still works")
    {
        using serial_parser_setup_type =
            stv::serial_parser_setup<lwrb_base_type, queue_type,
                                     stv::empty_mutex, stv::stvlink_parser>;
        serial_parser_setup_type setup;
        setup.lwrb = &lwrb;
        setup.set_queue<stv::stvlink_parser>(parsed_msg_queue);

        auto parser = stv::make_serial_parser<serial_parser_setup_type,
                                              stv::stvlink_parser>(setup);
        REQUIRE(parser);

        auto serial_message_buffer =
            make_serial_message_buffer(serial_msg_queue, stv::stvlink_sender{});
        constexpr std::string_view test_message{"A"};

        {
            const auto msg = serial_message_buffer.request(test_message);
        }

        decltype(auto) queue_instance = serial_message_buffer.queue_instance();
        auto           msg            = queue_instance.front();
        lwrb.write(msg.begin(), msg.end());
        queue_instance.pop();

        REQUIRE(parser.run());
        REQUIRE(parsed_msg_queue.size() == 1);

        auto parsed_msg = std::move(parsed_msg_queue.front());
        parsed_msg.trim_head(stv::stvlink_sender::message_header_size());
        REQUIRE(
            memcmp(test_message.data(), parsed_msg.data(), test_message.size())
            == 0);
        parsed_msg_queue.pop();
    }
}

// NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables)
// NOLINTEND(readability-function-cognitive-complexity)
// NOLINTEND(*-magic-numbers, google-build-using-namespace)
