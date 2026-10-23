/// @file test_two_protocols_communication.cpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// @brief Сквозной учебный пример: двухпоточный обмен сообщениями
/// демонстрационного протокола сразу по двум протоколам кадра — stvlinkv2
/// и RMP — по одному «проводу».
///
/// @details Образец для переиспользования в других проектах: показывает, как
/// собрать передающую и принимающую стороны обмена сообщениями из примитивов
/// stv без единой строчки доменного кода. Два std::thread разделяют общий
/// «провод» — кольцевой буфер stv::lwrb со встроенным std::mutex
/// (модель физического соединения). Оба протокола кадра идут по «проводу»
/// друг за другом: поток передатчика собирает кадры stvlinkv2 буфером
/// сообщений stv::make_serial_message_buffer с декоратором
/// stv::stvlinkv2_sender, а следом кадры RMP фиксированного размера вторым
/// буфером сообщений с декоратором stv::rmp_sender; поток приёмника
/// выделяет кадры обоих протоколов из потока байт одним парсером
/// stv::serial_parser: стартовые последовательности 0x55 0x55 (stvlinkv2)
/// и 0xAA 0x55 (RMP) различимы, поэтому одна пара «парсер + кольцевой
/// буфер» наполняет две очереди — по одной на протокол. stvlinkv2-очередь
/// диспетчеризуется модулем stv::stvlink_module: таблица обработчиков
/// собирается сборщиком stv::make_message_dispatcher_table из каталога
/// stv::make_message_catalog (сгенерированного из специализаций
/// stv::message_traits) и таблицы обработчиков полезной нагрузки стороны;
/// полнота приёмной логики зафиксирована static_assert
/// stv::check_message_coverage рядом с таблицей обработчиков полезной
/// нагрузки. RMP-очередь обрабатывается напрямую: полезная
/// нагрузка кадра отдаётся декодером stv::payload_decoder как есть
/// (идентификатора сообщения в кадре RMP нет).
///
/// Передатчик каждую итерацию передаёт телеметрию (тривиально копируемая
/// структура) кадром stvlinkv2 и команду (структура с битстрим-кодеком)
/// кадром RMP фиксированного размера; строка статуса и блок регистров в
/// обмене не участвуют и осознанно помечены игнорируемыми. Проверяется:
/// приняты все кадры обоих протоколов, значения полей совпадают с
/// переданными, потерь и расхождений нет.
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.

#include "stv/communication/message_catalog.hpp"
#include "stv/communication/message_traits.hpp"
#include "stv/communication/payload_decoder.hpp"
#include "stv/communication/rmp_parser.hpp"
#include "stv/communication/rmp_sender.hpp"
#include "stv/communication/serial_parser.hpp"
#include "stv/communication/serial_sender.hpp"
#include "stv/communication/stvlink_dispatcher.hpp"
#include "stv/communication/stvlinkv2_kit.hpp"
#include "stv/communication/tests/demo_messages.hpp"
#include "stv/containers/lwrb.hpp"
#include <array>
#include <atomic>
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <etl/queue.h>
#include <mutex>
#include <thread>
#include <utility>

// NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables) --
// состояние приёмника; обработчики сообщений — свободные функции без
// состояния, пишут только из потока приёмника (синхронизация с телом
// теста — через join обоих потоков до проверок).
// NOLINTBEGIN(llvm-prefer-static-over-anonymous-namespace) -- статические
// хелперы примера в одном безымянном namespace; проверка конфликтует с
// misc-use-anonymous-namespace.

namespace {

using namespace std::chrono_literals;

using stv::demo_catalog;
using stv::demo_command;
using stv::demo_msg_id;
using stv::demo_telemetry;

/// @brief Число передаваемых кадров каждого из сообщений.
constexpr std::size_t frames_per_message{5U};

/// @brief Максимальный размер одного кадра stvlinkv2 примера: стартовый
/// кадр и заголовок сообщения + самая большая полезная нагрузка (телеметрия)
/// + CRC-32.
constexpr std::size_t stvlinkv2_max_one_message_size =
    stv::stvlinkv2_sender::header_size() + sizeof(demo_telemetry)
    + stv::stvlinkv2_sender::trailer_size();

/// @brief Размер полезной нагрузки кадра RMP: сериализованная команда из
/// одного байта.
constexpr std::size_t rmp_pload_size = demo_command::serialized_size_bytes();

/// @brief Размер кадра RMP: полезная нагрузка команды + стартовая
/// последовательность (2 байта) + CRC-16 (2 байта), вычислен через API
/// передающего декоратора stv::rmp_sender.
constexpr std::size_t rmp_frame_size =
    stv::rmp_sender<rmp_pload_size + 4U>::header_size() + rmp_pload_size
    + stv::rmp_sender<rmp_pload_size + 4U>::trailer_size();

// Полезная нагрузка команды влезает в кадр RMP: кадр фиксированного
// размера, буфер сообщений отклонил бы большую нагрузку.
static_assert(demo_command::serialized_size_bytes()
                  <= stv::rmp_sender<rmp_frame_size>::max_pload_size(),
              "demo_command payload must fit into the fixed-size RMP frame");

/// @brief Температура телеметрии i-й посылки: одна и та же constexpr-функция
/// вычисляет образец на передающей и на принимающей стороне, стороны не
/// разделяют состояния.
[[nodiscard]] constexpr auto demo_temperature(
    const std::size_t iteration) -> std::int16_t
{ return static_cast<std::int16_t>(100 - (10 * iteration)); }

// ---- Типы инфраструктуры сторон ----

/// @brief Тип буфера сообщений очередей (без мьютекса: очереди каждого
/// потока трогает только он сам).
using sim_buffer_type = stv::sim_buff<stv::empty_mutex>;

/// @brief Тип очередей кадров и распарсенных сообщений.
using queue_type = etl::queue<sim_buffer_type, 16>;

/// @brief Тип «провода»: кольцевой буфер байт со встроенным мьютексом
/// (запись передатчиком и чтение парсером безопасны из разных потоков).
using lwrb_setup_type = stv::lwrb_setup<std::mutex>;
using lwrb_base_type  = stv::lwrb_base<lwrb_setup_type>;
using lwrb_type       = stv::lwrb<lwrb_base_type, 1024>;

/// @brief Декоратор кадра RMP и типы парсера, разбирающего оба протокола:
/// стартовые последовательности stvlinkv2 (0x55 0x55) и RMP (0xAA 0x55)
/// различимы, поэтому одна пара «парсер + кольцевой буфер» наполняет две
/// очереди — по одной на протокол.
using rmp_parser_type = stv::rmp_parser<rmp_frame_size>;
using parser_setup_type =
    stv::serial_parser_setup<lwrb_base_type, queue_type, stv::empty_mutex,
                             stv::stvlinkv2_parser, rmp_parser_type>;
using parser_type = stv::serial_parser<parser_setup_type, stv::stvlinkv2_parser,
                                       rmp_parser_type>;

/// @brief Тип хэш-таблицы обработчиков: ёмкость — число сообщений протокола.
constexpr auto handler_table_capacity = std::to_underlying(demo_msg_id::max);
using handler_table_type = stv::stvlink_handler_table<handler_table_capacity>;

/// @brief Тип настройки и тип модуля приёмника сообщений stvlinkv2.
using module_setup_type = stv::stvlink_setup<handler_table_type>;
using module_type =
    stv::stvlink_module<queue_type, module_setup_type, stv::stvlinkv2_parser>;

// ---- Состояние приёмника (пишут обработчики из потока приёмника) ----

/// @brief Число принятых телеметрических кадров (по stvlinkv2).
std::size_t telemetry_received{0};

/// @brief Число принятых команд (по RMP).
std::size_t commands_received{0};

/// @brief Число расхождений значений полей принятых кадров.
std::size_t payload_mismatches{0};

/// @brief Ожидаемый счётчик кадра следующей телеметрии.
std::uint32_t expected_telemetry_counter{0};

/// @brief Ожидаемый код следующей команды.
std::uint8_t expected_command_code{0};

/// @brief Флаг завершения передачи: выставляет поток передатчика после
/// отправки последнего кадра, читает поток приёмника.
std::atomic_bool transmission_done{false};

// ---- Обработчики полезной нагрузки приёмника ----

/// @brief Обработчик телеметрии: декодирует полезную нагрузку и сверяет
/// поля с переданным образцом.
///
/// @details Минимальный размер полезной нагрузки уже проверен обёрткой
/// таблицы обработчиков (сборщик make_message_dispatcher_table встраивает
/// его в обработчик на этапе компиляции); декодер тривиально копируемой
/// структуры игнорирует лишние хвостовые байты больших кадров (принцип
/// обратной совместимости CRSF).
bool handle_telemetry(
    stv::stvlink_message_span pload)
{
    if(const auto telemetry =
           stv::payload_decoder<demo_telemetry>::decode(pload))
    {
        if((telemetry->counter != expected_telemetry_counter)
           || (telemetry->temperature != demo_temperature(telemetry_received)))
        {
            ++payload_mismatches;
        }
        ++expected_telemetry_counter;
        ++telemetry_received;
    }
    else
    {
        ++payload_mismatches;
    }
    return true;
}

// ---- Таблица обработчиков полезной нагрузки и проверка покрытия каталога
// ----

/// @brief Обработчики полезной нагрузки приёмника stvlinkv2: обрабатывает
///     телеметрию; кадры
/// остальных сообщений протокола приёмник либо разбирает из RMP-очереди
/// напрямую (команда — у кадра RMP нет идентификатора сообщения), либо
/// осознанно игнорирует.
constexpr auto receiver_pload_handlers = std::array{
    std::pair{demo_msg_id::telemetry, &handle_telemetry},
};

/// @brief Осознанно игнорируемые входящие сообщения приёмника: команда
/// передаётся кадром RMP и обрабатывается напрямую из RMP-очереди
/// (stvlinkv2-обработчик не регистрируется), строка статуса и блок регистров
/// в обмене не участвуют.
constexpr auto receiver_ignored = std::array{
    demo_msg_id::command,
    demo_msg_id::status_text,
    demo_msg_id::registers,
};

// Каждый идентификатор каталога покрыт обработчиком полезной нагрузки или
// осознанным игнором: непокрытое входящее сообщение ловится здесь на этапе
// компиляции.
static_assert(stv::check_message_coverage(demo_catalog, receiver_pload_handlers,
                                          receiver_ignored));

/// @brief Таблица обработчиков: обёртка минимального размера полезной
/// нагрузки применяется сборщиком автоматически.
const auto &receiver_handler_table()
{
    const static auto table = stv::make_message_dispatcher_table<
        handler_table_type, receiver_pload_handlers, demo_catalog>();
    return table;
}

// ---- Потоки примера ----

/// @brief Перекладывает готовые кадры из очереди передатчика в кольцевой
/// буфер приёмника — модель физического соединения сторон.
void transfer(
    queue_type &tx_queue, lwrb_type &wire)
{
    while(!tx_queue.empty())
    {
        auto frame = tx_queue.front();
        wire.write(frame.begin(), frame.end());
        tx_queue.pop();
    }
}

/// @brief Поток передатчика: каждую итерацию передаёт телеметрию кадром
/// stvlinkv2 и команду кадром RMP фиксированного размера друг за другом,
/// затем выставляет флаг завершения передачи.
///
/// @param[in,out] wire Кольцевой буфер байт — модель соединения сторон.
void transmitter_thread(
    lwrb_type &wire)
{
    // Очереди и буферы сообщений — локальные: их трогает только этот
    // поток. Декораторы кадров собирают заголовки и CRC в деструкторе
    // объекта сообщения.
    queue_type tx_stvlink_queue{};
    queue_type tx_rmp_queue{};
    auto tx_stvlink = stv::make_serial_message_buffer(tx_stvlink_queue,
                                                      stv::stvlinkv2_sender{});
    auto tx_rmp     = stv::make_serial_message_buffer(
        tx_rmp_queue, stv::rmp_sender<rmp_frame_size>{});

    for(std::size_t i = 0; i < frames_per_message; ++i)
    {
        // Телеметрия кадром stvlinkv2: тривиально копируемая структура —
        // буфер сообщений сам копирует её байты в полезную нагрузку кадра.
        // Идентификатор сообщения берём из message_traits структуры.
        {
            auto msg = tx_stvlink.request<demo_telemetry>(stv::make_msg_setup(
                stv::message_traits<demo_telemetry>::msg_id));
            if(msg)
            {
                msg->counter     = static_cast<std::uint32_t>(i);
                msg->temperature = demo_temperature(i);
            }
        }

        // Команда кадром RMP фиксированного размера: голая полезная
        // нагрузка из одного байта кода, идентификатора сообщения в кадре
        // нет — приёмник знает, что в RMP-очереди лежат команды.
        {
            auto msg = tx_rmp.request(demo_command::serialized_size_bytes());
            if(msg)
            {
                msg.pload_data()[0U] = static_cast<std::byte>(i);
            }
        }

        // Кадры по проводу: байты готовых кадров обоих протоколов друг за
        // другом перекладываются в общий кольцевой буфер (в прошивке ту же
        // роль играет прерывание UART). Буфер со встроенным мьютексом:
        // запись потокобезопасна.
        transfer(tx_stvlink_queue, wire);
        transfer(tx_rmp_queue, wire);

        std::this_thread::sleep_for(5ms);
    }

    transmission_done.store(true, std::memory_order_release);
}

/// @brief Приёмная сторона: «провод», парсер кадров обоих протоколов,
/// диспетчер stvlinkv2 и разбор RMP-очереди.
///
/// @details «Провод» передаётся извне, потому что его байты пишет поток
/// передатчика.
struct receiver_side {
    lwrb_type &wire;
    queue_type stvlink_queue;
    queue_type rmp_queue;

    explicit receiver_side(
        lwrb_type &wire_ref, const handler_table_type &handlers):
        wire{wire_ref},
        parser_{make_parser_setup()},
        module_{stvlink_queue, module_setup_type{.hash_table = &handlers}}
    {
    }

    /// @brief Разбирает накопленные байты и обрабатывает все принятые кадры.
    void poll()
    {
        std::ignore = parser_.run();
        module_.process_all();
        process_rmp_queue();
    }

  private:
    parser_type parser_;
    module_type module_;

    auto        make_parser_setup() -> parser_setup_type
    {
        parser_setup_type setup{};
        setup.lwrb                 = &wire;
        setup.max_one_message_size = stvlinkv2_max_one_message_size;
        setup.set_queue<stv::stvlinkv2_parser>(stvlink_queue);
        setup.set_queue<rmp_parser_type>(rmp_queue);
        return setup;
    }

    /// @brief Разбирает очередь кадров RMP: парсер отрезал стартовую
    /// последовательность и CRC, поэтому в очереди — голая полезная
    /// нагрузка кадра (идентификатора сообщения у RMP нет): декодируем её
    /// как команду и сверяем код с переданным образцом.
    void process_rmp_queue()
    {
        while(!rmp_queue.empty())
        {
            auto       rmp_payload = rmp_queue.front();
            const auto pload = stv::stvlink_message_span{rmp_payload.data(),
                                                         rmp_payload.size()};

            if(const auto command =
                   stv::payload_decoder<demo_command>::decode(pload))
            {
                if(command->code != expected_command_code)
                {
                    ++payload_mismatches;
                }
                ++expected_command_code;
                ++commands_received;
            }
            else
            {
                ++payload_mismatches;
            }

            rmp_queue.pop();
        }
    }
};

/// @brief Поток приёмника: опрашивает «провод», пока передача не завершена
/// и в буфере остаются байты, затем делает последний проход.
void receiver_thread(
    lwrb_type &wire)
{
    receiver_side receiver{wire, receiver_handler_table()};

    while(!transmission_done.load(std::memory_order_acquire)
          || wire.get_full() > 0U)
    {
        receiver.poll();
        std::this_thread::sleep_for(1ms);
    }
    receiver.poll();
}

} // namespace

// NOLINTEND(llvm-prefer-static-over-anonymous-namespace)
// NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables)

TEST_CASE(
    "пример: двухпротокольный обмен stvlinkv2 и RMP по демо-протоколу")
{
    lwrb_type wire{lwrb_setup_type{}};

    // Потоки разделяют только «провод»; join обоих потоков — до любых
    // проверок, поэтому гонок на состоянии приёмника нет.
    std::thread transmitter{transmitter_thread, std::ref(wire)};
    std::thread receiver{receiver_thread, std::ref(wire)};

    transmitter.join();
    receiver.join();

    REQUIRE(telemetry_received == frames_per_message);
    REQUIRE(commands_received == frames_per_message);
    REQUIRE(payload_mismatches == 0U);
}
