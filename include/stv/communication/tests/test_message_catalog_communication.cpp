/// @file test_message_catalog_communication.cpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// @brief Сквозной учебный пример: двухпоточный обмен сообщениями
/// демонстрационного протокола по stvlinkv2.
///
/// @details Образец для переиспользования в других проектах: показывает, как
/// собрать передающую и принимающую стороны обмена сообщениями из примитивов
/// stv без единой строчки доменного кода. Два std::thread разделяют общий
/// «провод» — кольцевой буфер stv::lwrb со встроенным std::mutex
/// (модель физического соединения). Поток передатчика собирает кадры
/// stvlinkv2 буфером сообщений stv::make_serial_message_buffer с
/// декоратором stv::stvlinkv2_sender; поток приёмника выделяет кадры из
/// потока байт парсером stv::serial_parser и диспетчеризует их модулем
/// stv::stvlink_module. Таблица обработчиков модуля собирается сборщиком
/// stv::make_message_dispatcher_table из каталога stv::make_message_catalog
/// (сгенерированного из специализаций stv::message_traits) и таблицы
/// обработчиков полезной нагрузки стороны; полнота приёмной логики
/// зафиксирована static_assert
/// stv::check_message_coverage рядом с таблицей обработчиков полезной
/// нагрузки. Принятые полезные
/// нагрузки обработчики декодируют декодером stv::payload_decoder.
/// Свойства протокола выведены метафункциями списка типов
/// (stv::type_list_*): размер буфера кадра -- fold-проходом
/// stv::type_list_for_each по минимальным размерам полезной нагрузки,
/// инвариант покрытия всех идентификаторов -- stv::enum_uniquely_covered
/// над каталогом.
///
/// Описание демонстрационного протокола (типы сообщений, специализации
/// traits, список типов и каталог) встроено в файл напрямую -- пример
/// самодостаточен; тот же код в виде переиспользуемого заголовка --
/// stv/communication/tests/demo_messages.hpp. Необязательные compile-time
/// проверки инвариантов протокола (stv::check_catalog_list_order,
/// stv::enum_uniquely_covered, stv::check_message_coverage) сгруппированы
/// под макросом DEMO_OPTIONAL_CHECKS: обмен работает и без них, но они
/// ловят расхождения на этапе компиляции.
///
/// Передатчик каждую итерацию передаёт телеметрию (тривиально копируемая
/// структура), команду (структура с битстрим-кодеком) и блок регистров
/// (тривиально копируемая структура, принимаемая zero-copy доступом view());
/// строка статуса в обмене не участвует и осознанно помечена игнорируемой.
/// Проверяется: приняты все кадры трёх сообщений, значения полей совпадают с
/// переданными, потерь и расхождений нет.
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.

#include "stv/communication/message_catalog.hpp"
#include "stv/communication/message_traits.hpp"
#include "stv/communication/payload_decoder.hpp"
#include "stv/communication/serial_parser.hpp"
#include "stv/communication/serial_sender.hpp"
#include "stv/communication/stvlink_dispatcher.hpp"
#include "stv/communication/stvlinkv2_kit.hpp"
#include "stv/communication/type_list.hpp"
#include "stv/containers/lwrb.hpp"
#include <algorithm>
#include <array>
#include <atomic>
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <etl/queue.h>
#include <mutex>
#include <optional>
#include <string_view>
#include <thread>
#include <utility>

// NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables) --
// состояние приёмника; обработчики сообщений — свободные функции без
// состояния, пишут только из потока приёмника (синхронизация с телом
// теста — через join обоих потоков до проверок).
// NOLINTBEGIN(llvm-prefer-static-over-anonymous-namespace) -- статические
// хелперы примера в одном безымянном namespace; проверка конфликтует с
// misc-use-anonymous-namespace.

// ---- Демонстрационный протокол сообщений ----
// Протокол встроен в пример напрямую, чтобы файл был самодостаточным.
// Тот же код в виде переиспользуемого заголовка:
// stv/communication/tests/demo_messages.hpp.

namespace stv {

namespace {

/// @brief Идентификаторы сообщений демонстрационного протокола.
enum class demo_msg_id : std::uint16_t {
    telemetry,   ///< Телеметрия: счётчик кадров и температура.
    command,     ///< Команда из одного байта кода.
    status_text, ///< Строка статуса, декодируемая явной специализацией.
    registers,   ///< Блок регистров, читаемый zero-copy доступом view().

    max,         ///< Должен находиться в конце перечисляемого типа.
};

/// @brief Телеметрия: тривиально копируемая структура полезной нагрузки.
///
/// @details Принцип обратной совместимости по размеру: декодирование из
/// среза большего размера игнорирует лишние хвостовые байты, из среза
/// меньшего размера оставляет недостающие хвостовые поля нулевыми.
struct demo_telemetry {
    std::uint32_t counter{};     ///< Счётчик кадров.
    std::int16_t  temperature{}; ///< Температура.
};

/// @brief Команда: структура с битстрим-кодеком полезной нагрузки.
struct demo_command {
    std::uint8_t code{}; ///< Код команды.

    /// @brief Возвращает размер сериализованной полезной нагрузки в байтах.
    [[nodiscard]] static constexpr auto serialized_size_bytes() -> std::size_t
    { return 1U; }

    /// @brief Десериализует один байт кода команды.
    ///
    /// @param[in] begin Итератор на начало входного буфера.
    /// @param[in] end Итератор на конец входного буфера.
    /// @return Количество прочитанных байт (0 для пустого буфера).
    template<typename It>
    [[nodiscard]] auto deserialize_from(
        It begin, It end) -> std::size_t
    {
        if(begin == end)
        {
            return 0U;
        }

        code = std::to_integer<std::uint8_t>(*begin);
        return 1U;
    }
};

/// @brief Строка статуса: декодируется только явной специализацией.
struct demo_status_text {
    std::string_view text; ///< Строка-view на полезную нагрузку кадра.
};

/// @brief Блок регистров: тривиально копируемая структура для zero-copy
///     доступа view().
///
/// @details Декодируется без копирования: view() переинтерпретирует байты
///     кадра как const demo_registers*. Требует полного размера
///     sizeof(demo_registers) -- min_pload_size_bytes в traits равен
///     sizeof(demo_registers).
struct demo_registers {
    std::uint32_t                       reg0{}; ///< Регистр 0.
    std::uint32_t                       reg1{}; ///< Регистр 1.
    std::uint32_t                       reg2{}; ///< Регистр 2.
    std::uint32_t                       reg3{}; ///< Регистр 3.

    [[nodiscard]] friend constexpr auto operator==(const demo_registers &,
                                                   const demo_registers &)
        -> bool = default;
};

} // namespace

/// @brief Специализация traits телеметрии.
template<>
struct message_traits<demo_telemetry> {
    static constexpr demo_msg_id msg_id = demo_msg_id::telemetry;
    static constexpr std::size_t min_pload_size_bytes =
        sizeof(std::uint32_t) + sizeof(std::int16_t);
};

/// @brief Специализация traits команды.
template<>
struct message_traits<demo_command> {
    static constexpr demo_msg_id msg_id = demo_msg_id::command;
    static constexpr std::size_t min_pload_size_bytes =
        demo_command::serialized_size_bytes();
};

/// @brief Специализация traits строки статуса.
template<>
struct message_traits<demo_status_text> {
    static constexpr demo_msg_id msg_id = demo_msg_id::status_text;
    static constexpr std::size_t min_pload_size_bytes = 0U;
};

/// @brief Специализация traits блока регистров.
template<>
struct message_traits<demo_registers> {
    static constexpr demo_msg_id msg_id               = demo_msg_id::registers;
    static constexpr std::size_t min_pload_size_bytes = sizeof(demo_registers);
};

/// @brief Специализация декодера строки статуса.
///
/// @details Строка не десериализуется: decode возвращает тег со view на
///     полезную нагрузку кадра (валидно только на время обработки
///     сообщения).
template<>
struct payload_decoder<demo_status_text> {
    /// @brief Возвращает тег со строкой-view на полезную нагрузку.
    [[nodiscard]] static auto decode(
        std::span<std::byte> pload) -> std::optional<demo_status_text>
    {
        return demo_status_text{
            .text =
                std::string_view{reinterpret_cast<const char *>(pload.data()),
                                 pload.size_bytes()},
        };
    }
};

/// @brief Список типов демонстрационного протокола: порядок типов задаёт
///     порядок дескрипторов каталога.
using demo_list =
    type_list<demo_telemetry, demo_command, demo_status_text, demo_registers>;

/// @brief Каталог демонстрационного протокола, сгенерированный из traits.
constexpr auto demo_catalog = make_message_catalog<demo_list>();

} // namespace stv

/// @brief Необязательные compile-time проверки инвариантов протокола.
///
/// @details Значение 1 (по умолчанию): включены static_assert'ы, ловящие
///     расхождения списка типов, каталога и таблицы обработчиков полезной
///     нагрузки на этапе компиляции. Обмен сообщениями работает и без
///     них -- это страховка при изменении протокола. Значение 0:
///     проверки отключены. Макрос, а не constexpr-константа: значение
///     читается директивой препроцессора #if, constexpr её не заменяет.
#define DEMO_OPTIONAL_CHECKS 1 // NOLINT(cppcoreguidelines-macro-usage)

namespace {

using namespace std::chrono_literals;

using stv::demo_catalog;
using stv::demo_command;
using stv::demo_list;
using stv::demo_msg_id;
using stv::demo_registers;
using stv::demo_telemetry;

// ---- Свойства протокола, выведенные метафункциями списка типов ----

/// @brief Наибольший минимальный размер полезной нагрузки среди
/// сообщений протокола: fold-проход по списку типов. При добавлении
/// сообщения с большей полезной нагрузкой буфер кадра пересчитывается
/// автоматически -- ручные sizeof в размерах буферов не нужны.
constexpr std::size_t max_pload_size = [] {
    std::size_t result = 0U;
    stv::type_list_for_each<demo_list>([&result]<typename T> {
        result = std::max(result, stv::message_traits<T>::min_pload_size_bytes);
    });
    return result;
}();

#if DEMO_OPTIONAL_CHECKS
// ---- Необязательные проверки инвариантов протокола ----
// Обмен сообщениями работает и без них (см. DEMO_OPTIONAL_CHECKS): это
// страховка, ловящая расхождения на этапе компиляции.

// Список типов и каталог описывают одно и то же: размеры совпадают и
// порядок типов списка равен порядку дескрипторов каталога (проверка
// stv::check_catalog_list_order обходит все элементы списка, позиции
// вручную не перечисляются).
static_assert(stv::check_catalog_list_order<demo_list>(demo_catalog));

// Каждое значение demo_msg_id [0, max) имеет ровно один дескриптор в
// каталоге: ни одно сообщение протокола не потеряно и не продублировано
// (инвариант enum_uniquely_covered). Идентификаторы извлекает хелпер
// catalog_message_ids().
static_assert(stv::enum_uniquely_covered(stv::catalog_message_ids(demo_catalog),
                                         demo_msg_id::max));

// Выведенный размер буфера соответствует ожиданию: самая большая
// полезная нагрузка протокола -- блок регистров.
static_assert(max_pload_size == sizeof(demo_registers));
#endif

/// @brief Число передаваемых кадров каждого из сообщений.
constexpr std::size_t frames_per_message{5U};

/// @brief Максимальный размер одного кадра stvlinkv2 примера: стартовый
/// кадр и заголовок сообщения + самая большая полезная нагрузка
/// протокола + CRC-32.
constexpr std::size_t stvlinkv2_max_one_message_size =
    stv::stvlinkv2_sender::header_size() + max_pload_size
    + stv::stvlinkv2_sender::trailer_size();

/// @brief Температура телеметрии i-й посылки: одна и та же constexpr-функция
/// вычисляет образец на передающей и на принимающей стороне, стороны не
/// разделяют состояния.
[[nodiscard]] constexpr auto demo_temperature(
    const std::size_t iteration) -> std::int16_t
{ return static_cast<std::int16_t>(100 - (10 * iteration)); }

/// @brief Блок регистров i-й посылки: одна и та же constexpr-функция
/// вычисляет образец на передающей и на принимающей стороне, стороны не
/// разделяют состояния.
[[nodiscard]] constexpr auto demo_register_values(
    const std::size_t iteration) -> demo_registers
{
    return demo_registers{
        .reg0 = static_cast<std::uint32_t>(0x1000U + iteration),
        .reg1 = static_cast<std::uint32_t>(0x2000U + iteration),
        .reg2 = static_cast<std::uint32_t>(0x3000U + iteration),
        .reg3 = static_cast<std::uint32_t>(0x4000U + iteration),
    };
}

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

/// @brief Типы парсера кадров stvlinkv2: один декоратор — одна очередь.
using parser_setup_type =
    stv::serial_parser_setup<lwrb_base_type, queue_type, stv::empty_mutex,
                             stv::stvlinkv2_parser>;
using parser_type =
    stv::serial_parser<parser_setup_type, stv::stvlinkv2_parser>;

/// @brief Тип хэш-таблицы обработчиков: ёмкость — число сообщений протокола.
constexpr auto handler_table_capacity = std::to_underlying(demo_msg_id::max);
using handler_table_type = stv::stvlink_handler_table<handler_table_capacity>;

/// @brief Тип настройки и тип модуля приёмника сообщений stvlinkv2.
using module_setup_type = stv::stvlink_setup<handler_table_type>;
using module_type =
    stv::stvlink_module<queue_type, module_setup_type, stv::stvlinkv2_parser>;

// ---- Состояние приёмника (пишут обработчики из потока приёмника) ----

/// @brief Число принятых телеметрических кадров.
std::size_t telemetry_received{0};

/// @brief Число принятых команд.
std::size_t commands_received{0};

/// @brief Число принятых блоков регистров.
std::size_t registers_received{0};

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

/// @brief Обработчик команды: декодирует полезную нагрузку битстрим-кодеком
/// структуры и сверяет код команды с переданным образцом.
bool handle_command(
    stv::stvlink_message_span pload)
{
    if(const auto command = stv::payload_decoder<demo_command>::decode(pload))
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
    return true;
}

/// @brief Обработчик блока регистров: получает zero-copy доступ к полезной
///     нагрузке через view() и сверяет поля с constexpr-образцом.
///
/// @details Минимальный размер полезной нагрузки равен sizeof(demo_registers)
///     и уже проверен обёрткой таблицы обработчиков, поэтому view()
///     гарантированно возвращает не-null; указатель валиден только на время
///     обработки сообщения (буфер парсера переиспользуется между кадрами).
bool handle_registers(
    stv::stvlink_message_span pload)
{
    // Учебный демо-протокол на PC: дефолт relaxed. Для ядер без аппаратной
    // поддержки невыровненного доступа (Cortex-M0/M23) нужен
    // payload_alignment::checked.
    if(const auto *registers =
           stv::payload_decoder<demo_registers>::view(pload))
    {
        if(*registers != demo_register_values(registers_received))
        {
            ++payload_mismatches;
        }
        ++registers_received;
    }
    else
    {
        ++payload_mismatches;
    }
    return true;
}

// ---- Таблица обработчиков полезной нагрузки и проверка покрытия каталога
// ----

/// @brief Обработчики полезной нагрузки приёмника: обрабатывает все
///     передаваемые сообщения.
constexpr auto receiver_pload_handlers = std::array{
    std::pair{demo_msg_id::telemetry, &handle_telemetry},
    std::pair{demo_msg_id::command, &handle_command},
    std::pair{demo_msg_id::registers, &handle_registers},
};

/// @brief Осознанно игнорируемые входящие сообщения приёмника: строка
/// статуса в обмене не участвует. Используется необязательной проверкой
/// покрытия (см. DEMO_OPTIONAL_CHECKS) -- без неё список не нужен.
[[maybe_unused]] constexpr auto receiver_ignored = std::array{
    demo_msg_id::status_text,
};

#if DEMO_OPTIONAL_CHECKS
// Каждый идентификатор каталога покрыт обработчиком полезной нагрузки или
// осознанным игнором: непокрытое входящее сообщение ловится здесь на этапе
// компиляции. Необязательная проверка (см. DEMO_OPTIONAL_CHECKS): без неё
// обмен работает, но непокрытое сообщение будет молча принято
// обработчиком-игнором.
static_assert(stv::check_message_coverage(demo_catalog, receiver_pload_handlers,
                                          receiver_ignored));
#endif

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

/// @brief Поток передатчика: каждую итерацию передаёт телеметрию и команду
/// демонстрационного протокола, затем выставляет флаг завершения передачи.
///
/// @param[in,out] wire Кольцевой буфер байт — модель соединения сторон.
void transmitter_thread(
    lwrb_type &wire)
{
    // Очередь и буфер сообщений — локальные: их трогает только этот
    // поток. Декоратор кадра собирает заголовок и CRC в деструкторе
    // объекта сообщения.
    queue_type tx_queue{};
    auto       tx_buffer =
        stv::make_serial_message_buffer(tx_queue, stv::stvlinkv2_sender{});

    for(std::size_t i = 0; i < frames_per_message; ++i)
    {
        // Телеметрия: тривиально копируемая структура — буфер сообщений
        // сам копирует её байты в полезную нагрузку кадра. Идентификатор
        // сообщения берём из message_traits структуры.
        {
            auto msg = tx_buffer.request<demo_telemetry>(stv::make_msg_setup(
                stv::message_traits<demo_telemetry>::msg_id));
            if(msg)
            {
                msg->counter     = static_cast<std::uint32_t>(i);
                msg->temperature = demo_temperature(i);
            }
        }

        // Команда: структура с битстрим-кодеком — тот же путь передачи,
        // полезная нагрузка кадра равна сериализованному размеру.
        {
            auto msg = tx_buffer.request<demo_command>(
                stv::make_msg_setup(stv::message_traits<demo_command>::msg_id));
            if(msg)
            {
                msg->code = static_cast<std::uint8_t>(i);
            }
        }

        // Блок регистров: тривиально копируемая структура — приёмник
        // читает её zero-copy доступом view() без побайтового копирования.
        {
            auto msg = tx_buffer.request<demo_registers>(stv::make_msg_setup(
                stv::message_traits<demo_registers>::msg_id));
            if(msg)
            {
                const auto values = demo_register_values(i);
                msg->reg0         = values.reg0;
                msg->reg1         = values.reg1;
                msg->reg2         = values.reg2;
                msg->reg3         = values.reg3;
            }
        }

        // Кадры по проводу: байты готовых кадров перекладываются в общий
        // кольцевой буфер (в прошивке ту же роль играет прерывание UART).
        // Буфер со встроенным мьютексом: запись потокобезопасна.
        transfer(tx_queue, wire);

        std::this_thread::sleep_for(5ms);
    }

    transmission_done.store(true, std::memory_order_release);
}

/// @brief Приёмная сторона: «провод», парсер кадров stvlinkv2 и модуль
/// диспетчеризации.
///
/// @details «Провод» передаётся извне, потому что его байты пишет поток
/// передатчика.
struct receiver_side {
    lwrb_type &wire;
    queue_type rx_queue;

    explicit receiver_side(
        lwrb_type &wire_ref, const handler_table_type &handlers):
        wire{wire_ref},
        parser_{make_parser_setup()},
        module_{rx_queue, module_setup_type{.hash_table = &handlers}}
    {
    }

    /// @brief Разбирает накопленные байты и обрабатывает все принятые кадры.
    void poll()
    {
        std::ignore = parser_.run();
        module_.process_all();
    }

  private:
    parser_type parser_;
    module_type module_;

    auto        make_parser_setup() -> parser_setup_type
    {
        parser_setup_type setup{};
        setup.lwrb                 = &wire;
        setup.max_one_message_size = stvlinkv2_max_one_message_size;
        setup.set_queue<stv::stvlinkv2_parser>(rx_queue);
        return setup;
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
        std::this_thread::sleep_for(10ms);
    }
    receiver.poll();
}

} // namespace

// NOLINTEND(llvm-prefer-static-over-anonymous-namespace)
// NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables)

TEST_CASE(
    "пример: двухпоточный обмен по демо-протоколу")
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
    REQUIRE(registers_received == frames_per_message);
    REQUIRE(payload_mismatches == 0U);
}
