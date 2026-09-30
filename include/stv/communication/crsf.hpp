/// @file crsf.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.

/// NAME
///     stv_crsf -- прием и разбор кадров протокола TBS CRSF (Crossfire)
///
/// SYNOPSIS
///     #include "stv/communication/crsf.hpp"
///
///     stv::crsf_parser_setup<TLwrb, TRuntime, THandlerMap>
///     stv::crsf_parser<TSetup>
///     stv::crsf_crc8(std::span<const std::byte>)
///
/// DESCRIPTION
///     Модуль stv_crsf реализует приемную часть протокола TBS CRSF: разбор
///     байтового потока от радиоприемника, поступающего через кольцевой
///     буфер stv::lwrb, в который данные пишутся DMA и/или обработчиком
///     прерывания UART. Спецификация протокола --
///     third_party/tbs-crsf-spec/crsf.md.
///
///     Кадр CRSF: sync (1 байт), frame length (1 байт, длина полей type +
///     payload + crc, допустимый диапазон 2..62), type (1 байт), payload
///     и crc-8/dvb-s2 (1 байт, poly 0xD5, init 0x00, по type и payload).
///     Максимальный размер кадра целиком -- 64 байта.
///
///     Константы протокола:
///     - crsf_frame_length_min / crsf_frame_length_max -- диапазон поля
///       frame length (2..62);
///     - crsf_frame_size_max -- максимальный размер кадра (64 байта);
///     - crsf_type_rc_channels_packed (0x16) -- кадр каналов управления;
///     - crsf_type_link_statistics (0x14) -- статистика канала связи;
///     - crsf_rc_channels_count (16) / crsf_rc_channels_payload_size (22)
///       -- 16 каналов по 11 бит, упаковка lsb-first;
///     - crsf_link_statistics_payload_size (10);
///     - crsf_channel_value_min / _mid / _max (172/992/1811) -- диапазон
///       значений канала в тиках (992 ~ 1500 мкс) для масштабирования
///       функцией stv::map().
///
///     Типы данных:
///     - crsf_rc_channels -- std::array<std::uint16_t, 16>, значения
///       каналов в тиках;
///     - crsf_link_statistics -- статистика канала связи: up_rssi_ant1,
///       up_rssi_ant2, up_link_quality, up_snr (знаковый), active_antenna,
///       rf_profile, up_rf_power, down_rssi, down_link_quality, down_snr
///       (знаковый);
///     - crsf_handler_type -- etl::delegate<bool(std::span<const
///       std::byte>)>, обработчик payload кадра.
///
///     Функция crsf_crc8() вычисляет crc-8/dvb-s2 по таблице, построенной
///     на этапе компиляции; контрольное значение для строки "123456789"
///     равно 0xBC.
///
///     crsf_parser_setup задает параметры парсера:
///     - lwrb -- кольцевой буфер входного потока (емкость не меньше
///       crsf_frame_size_max, рекомендуется от 128 байт);
///     - runtime -- источник времени для failsafe deadline таймера;
///     - handlers -- хэш-таблица обработчиков кадров, созданная
///       пользовательским кодом за пределами парсера (парсер таблицу не
///       создает и не владеет ею); типом setup может быть как конкретный
///       etl::unordered_map, так и интерфейс etl::iunordered_map;
///     - failsafe_deadline -- timeout приема кадров 0x16 до перехода в
///       failsafe (по умолчанию 1 с);
///     - max_bytes_per_run -- лимит пропуска мусора при ресинхронизации
///       за один вызов run() (по умолчанию 128); валидные кадры лимит не
///       расходуют и не ограничиваются.
///
///     crsf_parser -- конечный автомат (HFSM2) из состояний wait_head /
///     wait_frame / verify_crc / dispatch. Обработчики кадров 0x16
///     (каналы) и 0x14 (статистика) регистрируются автоматически;
///     пользовательские добавляются через register_handler(). Кадр
///     неизвестного типа с валидным crc пропускается и считается
///     разобранным.
///
///     Поведение при ошибках потока: при невалидном frame length или
///     несошедшемся crc из буфера пропускается ровно один байт, и поиск
///     заголовка повторяется (посимвольный resync, как в betaflight) --
///     валидные кадры, поглощенные искаженной длиной, не теряются.
///     Неполный кадр не потребляется: run() завершается и дожидается
///     дозаписи остатка. run() неблокирующий: всегда возвращает
///     управление, потребление мусора ограничено max_bytes_per_run.
///
///     Failsafe: deadline таймер перезапускается только валидными
///     кадрами 0x16. По истечении failsafe_deadline ближайший run()
///     взводит is_failsafe() и обнуляет буфер каналов (управление
///     сброшено); первый валидный кадр 0x16 снимает флаг. Кадры 0x14 и
///     прочие deadline не продлевают.
///
///     Основные методы crsf_parser:
///     - explicit operator bool() -- валидность setup (lwrb, runtime и
///       handlers не nullptr);
///     - run() -- один проход разбора; возвращает число изъятых кадров;
///     - is_failsafe() -- атомарный флаг failsafe;
///     - channels() / get_channel(n) -- значения 16 каналов в тиках;
///     - link_statistics() -- статистика из крайнего кадра 0x14;
///     - register_handler(type, handler) -- регистрация обработчика;
///       false, если таблица не задана, полна или тип занят (0x16 и 0x14
///       заняты).
///
///     Потокобезопасность: реализации не thread-safe. run() вызывается
///     из одной задачи; парсер -- единственный читатель кольцевого
///     буфера (писатель -- ISR/DMA). channels() и link_statistics()
///     возвращают ссылки на живые внутренние буферы: кросс-поточное
///     чтение одновременно с run() требует внешней синхронизации.
///     register_handler() вызывать до первого run().
///
/// EXAMPLE
///     Типовое использование (обвязка по мотивам test_crsf.cpp):
///     ```cpp
///     #include "stv/communication/crsf.hpp"
///     #include <etl/unordered_map.h>
///
///     using lwrb_setup = stv::lwrb_setup<stv::empty_mutex>;
///     using lwrb_base  = stv::lwrb_base<lwrb_setup>;
///     using lwrb       = stv::lwrb<lwrb_base, 128U>;
///     using runtime    = stv::runtime_interface<stv::runtime_counter_type>;
///     using handler_map =
///         etl::iunordered_map<std::uint8_t, stv::crsf_handler_type>;
///     using setup_type =
///         stv::crsf_parser_setup<lwrb_base, runtime, handler_map>;
///
///     lwrb ringbuff{lwrb_setup{}};
///     // Таблица обработчиков создается пользовательским кодом; парсер
///     // получает указатель на нее через setup и не владеет ею.
///     etl::unordered_map<std::uint8_t, stv::crsf_handler_type, 8U>
///         handler_map_impl{};
///     setup_type setup{.lwrb     = &ringbuff,
///                      .runtime  = &runtime_impl,
///                      .handlers = &handler_map_impl};
///     stv::crsf_parser<setup_type> parser{setup};
///     if(!parser) { /* setup невалиден */ }
///
///     // Пользовательский обработчик кадра типа 0x42.
///     parser.register_handler(
///         0x42U, stv::crsf_handler_type::create(
///                    [](std::span<const std::byte> payload) {
///                        // payload без sync, length, type и crc.
///                        return true;
///                    }));
///
///     // Периодическая задача разбора: ISR/DMA пишут байты в ringbuff.
///     if(parser.run() > 0 && !parser.is_failsafe())
///     {
///         const auto throttle = parser.get_channel(2);  // тики 172..1811
///         const auto &stats   = parser.link_statistics();
///         (void)throttle;
///         (void)stats.link_quality;
///     }
///     ```
///
///     Автоматическая проверка поведения -- test_crsf.cpp (разбор
///     валидных и испорченных кадров, resync, failsafe).

#ifndef CRSF_HPP
#define CRSF_HPP

#include "gsl/gsl"
#include "stv/containers/lwrb.hpp"
#include "stv/deadline_timer.hpp"
#include "stv/mutex_guard.hpp"
#include "stv/runtime.hpp"
#include "stv/utils.hpp"
#include <array>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <etl/delegate.h>
#include <hfsm2/machine.hpp>
#include <span>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

namespace stv {

/// @brief Минимальное значение поля frame length кадра crsf (type + crc).
/// @see Раздел «Frame Details» в crsf.md
inline constexpr std::uint8_t crsf_frame_length_min{2U};

/// @brief Максимальное значение поля frame length кадра crsf.
/// @see Раздел «Frame Details» в crsf.md
inline constexpr std::uint8_t crsf_frame_length_max{62U};

/// @brief Максимальный размер кадра crsf целиком, включая байты sync и crc.
/// @see Раздел «Structure» в crsf.md
inline constexpr std::size_t crsf_frame_size_max{64U};

/// @brief Тип кадра rc channels packed.
/// @see Раздел «0x16 RC Channels Packed Payload» в crsf.md
inline constexpr std::uint8_t crsf_type_rc_channels_packed{0x16U};

/// @brief Тип кадра link statistics.
/// @see Раздел «0x14 Link Statistics» в crsf.md
inline constexpr std::uint8_t crsf_type_link_statistics{0x14U};

/// @brief Размер payload кадра rc channels packed
/// (16 каналов по 11 бит каждый).
/// @see Раздел «0x16 RC Channels Packed Payload» в crsf.md
inline constexpr std::size_t crsf_rc_channels_payload_size{22U};

/// @brief Размер payload кадра link statistics.
/// @see Раздел «0x14 Link Statistics» в crsf.md
inline constexpr std::size_t crsf_link_statistics_payload_size{10U};

/// @brief Количество каналов управления в кадре rc channels packed.
/// @see Раздел «0x16 RC Channels Packed Payload» в crsf.md
inline constexpr std::size_t crsf_rc_channels_count{16U};

/// @brief Буфер значений каналов управления в тиках, распакованных из кадра
/// rc channels packed (0x16).
/// @see Раздел «0x16 RC Channels Packed Payload» в crsf.md
using crsf_rc_channels = std::array<std::uint16_t, crsf_rc_channels_count>;

/// @brief Статистика канала связи, разобранная из кадра link statistics
/// (0x14).
/// @see Раздел «0x14 Link Statistics» в crsf.md
struct crsf_link_statistics {
    /// @brief Uplink RSSI антенны 1, в -dbm.
    std::uint8_t up_rssi_ant1;

    /// @brief Uplink RSSI антенны 2, в -dbm.
    std::uint8_t up_rssi_ant2;

    /// @brief Uplink качество канала связи (lq), в процентах.
    std::uint8_t up_link_quality;

    /// @brief Uplink отношение сигнал/шум (snr), в db; может быть
    /// отрицательным.
    std::int8_t up_snr;

    /// @brief Номер активной антенны.
    std::uint8_t active_antenna;

    /// @brief RF-профиль (4fps = 0, 50fps = 1, 150fps = 2).
    std::uint8_t rf_profile;

    /// @brief Uplink RF-мощность (перечисление мощностей в mw).
    std::uint8_t up_rf_power;

    /// @brief Downlink RSSI, в -dbm.
    std::uint8_t down_rssi;

    /// @brief Downlink качество канала связи (lq), в процентах.
    std::uint8_t down_link_quality;

    /// @brief Downlink отношение сигнал/шум (snr), в db; может быть
    /// отрицательным.
    std::int8_t down_snr;
};

/// @brief Минимальное значение канала управления.
/// @details Нижняя граница диапазона исходных значений для последующего
/// масштабирования функцией stv::map().
/// @see Раздел «0x16 RC Channels Packed Payload» в crsf.md
inline constexpr std::uint16_t crsf_channel_value_min{172U};

/// @brief Среднее значение канала управления (соответствует 1500 мкс).
/// @see Раздел «0x16 RC Channels Packed Payload» в crsf.md
inline constexpr std::uint16_t crsf_channel_value_mid{992U};

/// @brief Максимальное значение канала управления.
/// @details Верхняя граница диапазона исходных значений для последующего
/// масштабирования функцией stv::map().
/// @see Раздел «0x16 RC Channels Packed Payload» в crsf.md
inline constexpr std::uint16_t crsf_channel_value_max{1811U};

namespace detail {

/// @brief Вычисляет таблицу crc-8/dvb-s2 (poly 0xD5, init 0x00) на этапе
/// компиляции.
/// @return Таблица из 256 значений crc.
/// @see Раздел «CRC» в crsf.md
[[nodiscard]] consteval auto make_crsf_crc8_table()
    -> std::array<std::uint8_t, 256U>
{
    return std::array<std::uint8_t, 256U>{
        0x00, 0xD5, 0x7F, 0xAA, 0xFE, 0x2B, 0x81, 0x54, 0x29, 0xFC, 0x56, 0x83,
        0xD7, 0x02, 0xA8, 0x7D, 0x52, 0x87, 0x2D, 0xF8, 0xAC, 0x79, 0xD3, 0x06,
        0x7B, 0xAE, 0x04, 0xD1, 0x85, 0x50, 0xFA, 0x2F, 0xA4, 0x71, 0xDB, 0x0E,
        0x5A, 0x8F, 0x25, 0xF0, 0x8D, 0x58, 0xF2, 0x27, 0x73, 0xA6, 0x0C, 0xD9,
        0xF6, 0x23, 0x89, 0x5C, 0x08, 0xDD, 0x77, 0xA2, 0xDF, 0x0A, 0xA0, 0x75,
        0x21, 0xF4, 0x5E, 0x8B, 0x9D, 0x48, 0xE2, 0x37, 0x63, 0xB6, 0x1C, 0xC9,
        0xB4, 0x61, 0xCB, 0x1E, 0x4A, 0x9F, 0x35, 0xE0, 0xCF, 0x1A, 0xB0, 0x65,
        0x31, 0xE4, 0x4E, 0x9B, 0xE6, 0x33, 0x99, 0x4C, 0x18, 0xCD, 0x67, 0xB2,
        0x39, 0xEC, 0x46, 0x93, 0xC7, 0x12, 0xB8, 0x6D, 0x10, 0xC5, 0x6F, 0xBA,
        0xEE, 0x3B, 0x91, 0x44, 0x6B, 0xBE, 0x14, 0xC1, 0x95, 0x40, 0xEA, 0x3F,
        0x42, 0x97, 0x3D, 0xE8, 0xBC, 0x69, 0xC3, 0x16, 0xEF, 0x3A, 0x90, 0x45,
        0x11, 0xC4, 0x6E, 0xBB, 0xC6, 0x13, 0xB9, 0x6C, 0x38, 0xED, 0x47, 0x92,
        0xBD, 0x68, 0xC2, 0x17, 0x43, 0x96, 0x3C, 0xE9, 0x94, 0x41, 0xEB, 0x3E,
        0x6A, 0xBF, 0x15, 0xC0, 0x4B, 0x9E, 0x34, 0xE1, 0xB5, 0x60, 0xCA, 0x1F,
        0x62, 0xB7, 0x1D, 0xC8, 0x9C, 0x49, 0xE3, 0x36, 0x19, 0xCC, 0x66, 0xB3,
        0xE7, 0x32, 0x98, 0x4D, 0x30, 0xE5, 0x4F, 0x9A, 0xCE, 0x1B, 0xB1, 0x64,
        0x72, 0xA7, 0x0D, 0xD8, 0x8C, 0x59, 0xF3, 0x26, 0x5B, 0x8E, 0x24, 0xF1,
        0xA5, 0x70, 0xDA, 0x0F, 0x20, 0xF5, 0x5F, 0x8A, 0xDE, 0x0B, 0xA1, 0x74,
        0x09, 0xDC, 0x76, 0xA3, 0xF7, 0x22, 0x88, 0x5D, 0xD6, 0x03, 0xA9, 0x7C,
        0x28, 0xFD, 0x57, 0x82, 0xFF, 0x2A, 0x80, 0x55, 0x01, 0xD4, 0x7E, 0xAB,
        0x84, 0x51, 0xFB, 0x2E, 0x7A, 0xAF, 0x05, 0xD0, 0xAD, 0x78, 0xD2, 0x07,
        0x53, 0x86, 0x2C, 0xF9,
    };
}

} // namespace detail

namespace detail {

/// @brief Проверяет на этапе компиляции вместимость таблицы обработчиков
/// кадров, если тип таблицы предоставляет статическую константу MAX_SIZE
/// (конкретные etl-контейнеры). Для интерфейсных типов
/// (etl::iunordered_map) проверка пропускается: вместимость контролируется
/// кодом, создающим таблицу.
/// @tparam THandlerMap Тип хэш-таблицы обработчиков кадров.
template<typename THandlerMap>
consteval void assert_handler_map_capacity()
{
    if constexpr(requires { THandlerMap::MAX_SIZE; })
    {
        // Парсер автоматически регистрирует два встроенных обработчика
        // кадров (0x16 и 0x14): таблица должна вмещать минимум два элемента.
        static_assert(THandlerMap::MAX_SIZE >= 2U,
                      "handler_map_type должен вмещать минимум 2 обработчика "
                      "(встроенные обработчики кадров 0x16 и 0x14)");
    }
}

} // namespace detail

/// @brief Вычисляет crc-8/dvb-s2 (poly 0xD5, init 0x00) кадра crsf.
/// @param data Байты type и payload кадра (без sync, frame length и crc).
/// @return Значение crc.
/// @see Раздел «CRC» в crsf.md
[[nodiscard]] inline auto crsf_crc8(
    std::span<const std::byte> data) -> std::uint8_t
{
    constexpr auto table = detail::make_crsf_crc8_table();

    std::uint8_t   crc{0U};
    for(const std::byte byte: data)
    {
        // Индекс — результат xor двух байт, всегда в диапазоне таблицы (256).
        crc = gsl::at(table, static_cast<gsl::index>(
                                 crc ^ std::to_integer<decltype(crc)>(byte)));
    }
    return crc;
}

/// @brief Обработчик кадра crsf.
///
/// @details Получает payload кадра без байт sync, frame length, type и crc.
/// Возвращает true, если кадр успешно обработан.
using crsf_handler_type = etl::delegate<bool(std::span<const std::byte>)>;

/// @brief Параметры инициализации модуля приема данных протокола crsf.
template<typename TLwrb, typename TRuntime, typename THandlerMap>
class crsf_parser_setup
{
  public:
    /// @brief Тип кольцевого буфера.
    using lwrb_type = TLwrb;

    /// @brief Тип runtime таймера. Нужен для перехода в failsafe по timeout.
    using runtime_type = TRuntime;

    /// @brief Тип хэш-таблицы обработчиков кадров: ключ — тип кадра,
    /// значение — обработчик кадра.
    /// @details Таблица создается пользовательским кодом за пределами парсера
    /// и передается указателем через поле @ref handlers (парсер таблицу не
    /// создает и не владеет ею). Типом может быть как конкретный
    /// etl::unordered_map, так и интерфейс etl::iunordered_map — тогда
    /// конкретная реализация и ее емкость выбираются в месте создания
    /// таблицы. Ёмкость таблицы должна быть не меньше 2 (встроенные
    /// обработчики кадров 0x16 и 0x14, регистрируемые парсером) плюс число
    /// пользовательских обработчиков; для таблиц с проверяемой на этапе
    /// компиляции ёмкостью (конкретные etl-контейнеры с константой MAX_SIZE)
    /// это требование проверяется static_assert, для интерфейсных типов
    /// (etl::iunordered_map) контроль емкости — на создающем таблицу коде.
    using handler_map_type = THandlerMap;

    using deadline_counter_type = runtime_type::counter_type;

    /// @brief Указатель на интерфейс кольцевого буфера.
    lwrb_type *lwrb{nullptr};

    /// @brief Указатель на интерфейс runtime таймера. Используется для создания
    /// deadline таймера переход в filesafe.
    runtime_type *runtime{nullptr};

    /// @brief Указатель на хэш-таблицу обработчиков кадров. Таблица создается
    /// пользовательским кодом за пределами парсера; парсер не владеет ею и
    /// использует только по указателю, поэтому время жизни таблицы должно
    /// покрывать время жизни парсера.
    handler_map_type *handlers{nullptr};

    /// @brief Время с момента получения крайнего валидного кадра rc channels
    /// packed (0x16) перед переходом в failsafe.
    /// @note Значение 0 приводит к перманентному failsafe (таймер не
    /// запускается, is_elapsed() всегда true) — такая конфигурация не имеет
    /// смысла, задавать значение больше 0.
    deadline_counter_type failsafe_deadline{std::chrono::seconds{1}};

    /// @brief Максимальное количество байт мусора, пропускаемых при
    /// ресинхронизации за один вызов crsf_parser::run().
    /// @details Лимит касается только resync-потребления: пропуска байтов с
    /// невалидным полем frame length в состоянии поиска заголовка и пропуска
    /// байта sync кадра с несошедшимся crc. Байты валидных кадров лимит не
    /// расходуют и не ограничиваются: обнаружение валидного заголовка
    /// завершает resync, и все полные валидные кадры буфера разбираются за
    /// тот же вызов run(), даже если лимит к этому моменту исчерпан. По
    /// достижении лимита на продолжающемся мусоре run() неблокирующе
    /// возвращает управление, остаток мусора добирается следующими
    /// вызовами run(). Значение 0 запрещает resync-потребление (разбор
    /// остановится на первом же мусорном байте) — осмысленный минимум 1.
    std::size_t max_bytes_per_run{128U};
};

/// @brief Парсер протокола crsf.
///
/// @details Разбор потока байт кольцевого буфера выполняется конечным
/// автоматом (HFSM2) из четырёх состояний, отражающих фазы разбора кадра:
///
/// @verbatim
///                    (неполный кадр: run() завершается,
///                     байты НЕ потребляются)
///                            |
///   wait_head_state -> wait_frame_state -> verify_crc_state -> dispatch_state
///         ^  |                                                    |
///         |  | (невалидная длина кадра:                           |
///         |  |  resync, пропуск ровно 1 байта)                    |
///         |  v                                                    v
///         +--------------------------------------------------------+
///         (verify_crc_state при невалидном crc пропускает ровно
///          1 байт (sync) и тоже возвращается в wait_head_state)
/// @endverbatim
///
/// Поведение resync. Кадр просматривается через lwrb::peek() без потребления
/// байт, потребление выполняется через lwrb::skip(). Если поле frame length
/// невалидно (< @ref crsf_frame_length_min или > @ref crsf_frame_length_max),
/// из буфера пропускается ровно один байт и поиск заголовка повторяется со
/// следующего байта (посимвольный resync) — так парсер восстанавливает
/// синхронизацию после мусора или потери байт в потоке. Если crc не сошлось,
/// из буфера также пропускается ровно один байт (sync) и поиск заголовка
/// повторяется: поле frame length к этому моменту проверено только на
/// диапазон, поэтому искажённый байт длины мог поглотить в «кадр»
/// последующие валидные кадры, и посимвольный resync находит их (как в
/// betaflight). Неполный кадр не потребляется:
/// run() завершается, а разбор продолжится после дозаписи остатка кадра.
/// Кадр с валидным crc, но с типом, для которого не зарегистрирован
/// обработчик, пропускается и считается разобранным.
///
/// Неблокирующая гарантия. run() всегда возвращает управление: каждая
/// итерация конечного автомата либо потребляет байты (разобранный кадр или
/// пропущенный мусор), либо завершает вызов. Потребление мусора при
/// ресинхронизации ограничено setup.max_bytes_per_run за один вызов run()
/// (см. документацию поля): на буфере, забитом мусором, run() не крутится
/// дольше лимита, а «зависший» буфер (байты есть, валидный кадр не
/// собирается) опустошается за несколько вызовов либо ожидает дозаписи
/// неполного заголовка/кадра, не потребляя его.
///
/// Нарушение контракта буфера. Контракт «парсер — единственный читатель
/// буфера» считается нарушенным, если peek() вернул меньше байт, чем показал
/// get_full(), либо если буфер оказался пуст, хотя автомат ждёт дозаписи
/// кадра (буфер сброшен или опустошён извне). При нарушении контракта
/// автомат переходит (или остаётся) в wait_head_state со сброшенным
/// frame_size: текущий run() завершается, а следующий начинает чистую
/// ресинхронизацию вместо ожидания по устаревшему состоянию.
///
/// Требования к окружению. Ёмкость кольцевого буфера должна быть не меньше
/// @ref crsf_frame_size_max (64 байта), иначе кадр максимального размера
/// никогда не накопится целиком и разбор навсегда заблокируется на нём;
/// рекомендуется ёмкость не меньше 2 x @ref crsf_frame_size_max для запаса
/// на wrap-around.
///
/// Обработчики кадров 0x16 (rc channels packed) и 0x14 (link statistics)
/// регистрируются парсером автоматически при создании; пользовательские
/// обработчики добавляются через register_handler().
///
/// Failsafe по timeout. За приемом команд управления следит deadline таймер,
/// стартующий при создании парсера с задержкой setup.failsafe_deadline
/// (по умолчанию 1 с). Перезапускают deadline только валидные кадры rc
/// channels packed (0x16) — новая команда управления; кадры 0x14 (link
/// statistics) и кадры прочих типов deadline не продлевают. Проверка
/// deadline выполняется в run(): при истечении устанавливается is_failsafe()
/// и буфер каналов channels() сбрасывается в нулевые значения (управление
/// сброшено). Первый валидный кадр 0x16 после failsafe снимает флаг и
/// обновляет каналы значениями нового кадра.
///
/// @note Между вызовами run() конечный автомат сохраняет frame_size
/// ожидаемого кадра, пока кадр не накопится целиком. Внешний сброс или
/// опустошение кольцевого буфера между вызовами run() — нарушение контракта
/// «парсер — единственный читатель буфера»: оно обнаруживается ближайшим
/// run() (см. «Нарушение контракта буфера» выше), но штатной работой не
/// является и по-прежнему запрещено.
template<typename TSetup>
class crsf_parser
{
    using setup_type            = TSetup;
    using lwrb_type             = typename setup_type::lwrb_type;
    using runtime_type          = typename setup_type::runtime_type;
    using handler_map_type      = typename setup_type::handler_map_type;
    using deadline_counter_type = runtime_type::counter_type;
    using deadline_setup_type   = stv::deadline_timer_setup<runtime_type>;
    using deadline_type         = stv::deadline_timer<deadline_setup_type>;

    /// @brief Размер поля sync кадра crsf в байтах.
    static constexpr std::size_t sync_size{1U};

    /// @brief Размер поля frame length кадра crsf.
    static constexpr std::size_t frame_length_size{1U};

    /// @brief Размер заголовка кадра crsf (sync + frame length).
    static constexpr std::size_t head_size{sync_size + frame_length_size};

    /// @brief Размер поля type кадра crsf.
    static constexpr std::size_t frame_type_size{1U};

    /// @brief Размер поля crc кадра crsf.
    static constexpr std::size_t crc_size{1U};

    /// @brief Индекс байта frame length в кадре.
    static constexpr std::size_t frame_length_index{sync_size};

    /// @brief Индекс байта type в кадре.
    static constexpr std::size_t frame_type_index{head_size};

    /// @brief Все вызовы lwrb из парсера выполняются не из контекста
    /// прерывания: парсер — единственный читатель, писатель — ISR/DMA.
    static constexpr bool is_isr{false};

    // Парсер автоматически регистрирует два встроенных обработчика кадров
    // (0x16 и 0x14): таблица обработчиков должна вмещать минимум два
    // элемента (см. документацию crsf_parser_setup::handler_map_type).
    static_assert((detail::assert_handler_map_capacity<handler_map_type>(),
                   true));

    /// @brief Контекст конечного автомата разбора кадров.
    struct parser_context {
        /// @brief Кольцевой буфер с входящим потоком байт.
        lwrb_type *lwrb{nullptr};

        /// @brief Хэш-таблица обработчиков кадров по типу кадра.
        handler_map_type *handlers{nullptr};

        /// @brief Локальный буфер текущего кадра. Читается через peek, весь
        /// кольцевой буфер не копируется.
        std::array<std::byte, crsf_frame_size_max> frame{};

        /// @brief Полный размер текущего кадра (sync + frame length + type +
        /// payload + crc).
        std::size_t frame_size{0U};

        /// @brief Счетчик валидных кадров, изъятых за текущий вызов run().
        std::size_t parsed_count{0U};

        /// @brief Счетчик байт мусора, пропущенных при ресинхронизации за
        /// текущий вызов run() (пропуск байта с невалидным frame length и
        /// пропуск байта sync кадра с несошедшимся crc).
        std::size_t skipped_count{0U};

        /// @brief Лимит resync-потребления за один вызов run()
        /// (setup.max_bytes_per_run); копия параметра setup.
        std::size_t max_bytes_per_run{0U};

        /// @brief Признак завершения текущего вызова run().
        bool is_need_stop{false};
    };

    using machine_base_type =
        hfsm2::MachineT<hfsm2::Config::ContextT<parser_context>>;

    struct wait_head_state;
    struct wait_frame_state;
    struct verify_crc_state;
    struct dispatch_state;

    /// @brief Конечный автомат разбора кадров crsf.
    using machine_type = typename machine_base_type::template PeerRoot<
        wait_head_state, wait_frame_state, verify_crc_state, dispatch_state>;

    /// @brief Состояние ожидания начала кадра: читает заголовок и проверяет
    /// поле frame length.
    struct wait_head_state: machine_type::State {
        void update(
            typename machine_type::FullControl &control)
        {
            auto &context = control.context();

            if(context.lwrb->get_full(is_isr) < head_size)
            {
                // В буфере нет даже заголовка: завершаем run().
                context.is_need_stop = true;
                return;
            }

            std::array<std::byte, head_size> head{};
            if(context.lwrb->peek(
                   typename lwrb_type::container_type{head.data(), head.size()},
                   0U, is_isr)
               != head.size())
            {
                // Прочитано меньше байт, чем показал get_full(): контракт
                // «парсер — единственный читатель буфера» нарушен, разбор
                // прерывается во избежание разбора мусора. Автомат остаётся
                // в wait_head_state со сброшенным frame_size: следующий
                // run() начнёт чистую ресинхронизацию.
                context.frame_size   = 0U;
                context.is_need_stop = true;
                return;
            }

            const auto frame_length =
                // frame_length_index — константный индекс в пределах заголовка.
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
                std::to_integer<std::uint8_t>(head[frame_length_index]);

            if(frame_length < crsf_frame_length_min
               || frame_length > crsf_frame_length_max)
            {
                // Лимит resync-потребления исчерпан: мусорный байт не
                // пропускается, run() неблокирующе возвращает управление;
                // resync продолжится со следующего вызова run().
                if(context.skipped_count >= context.max_bytes_per_run)
                {
                    context.is_need_stop = true;
                    return;
                }

                // Resync: пропускаем ровно один байт и повторяем поиск
                // заголовка со следующего байта.
                std::ignore = context.lwrb->skip(1U, is_isr);
                ++context.skipped_count;
                return;
            }

            // Полный размер кадра: заголовок + type + payload + crc.
            context.frame_size =
                static_cast<std::size_t>(frame_length) + head_size;
            control.template changeTo<wait_frame_state>();
        }
    };

    /// @brief Состояние ожидания полного кадра: неполный кадр не потребляет,
    /// полный читает в локальный буфер контекста.
    struct wait_frame_state: machine_type::State {
        void update(
            typename machine_type::FullControl &control)
        {
            auto      &context = control.context();

            const auto buffered = context.lwrb->get_full(is_isr);

            if(buffered == 0U)
            {
                // Автомат ждёт кадр, а буфер пуст: буфер сброшен или
                // опустошён извне, контракт «парсер — единственный читатель
                // буфера» нарушен. Возврат в wait_head_state со сброшенным
                // frame_size: следующий run() начнёт чистую ресинхронизацию
                // вместо ожидания по устаревшему состоянию (как в
                // serial_parser).
                context.frame_size   = 0U;
                context.is_need_stop = true;
                control.template changeTo<wait_head_state>();
                return;
            }

            if(buffered < context.frame_size)
            {
                // Неполный кадр не потребляется: разбор продолжится при
                // следующем вызове run(), после дозаписи остатка кадра.
                context.is_need_stop = true;
                return;
            }

            if(context.lwrb->peek(
                   typename lwrb_type::container_type{
                       context.frame.data(),
                       context.frame_size,
                   },
                   0U, is_isr)
               != context.frame_size)
            {
                // Прочитано меньше байт, чем показал get_full(): контракт
                // «парсер — единственный читатель буфера» нарушен, разбор
                // прерывается во избежание разбора мусора. Возврат в
                // wait_head_state со сброшенным frame_size: следующий run()
                // начнёт чистую ресинхронизацию.
                context.frame_size   = 0U;
                context.is_need_stop = true;
                control.template changeTo<wait_head_state>();
                return;
            }
            control.template changeTo<verify_crc_state>();
        }
    };

    /// @brief Состояние проверки crc кадра.
    struct verify_crc_state: machine_type::State {
        void update(
            typename machine_type::FullControl &control)
        {
            auto &context = control.context();

            // crc вычисляется по байтам type и payload (без sync, frame
            // length и crc).
            const auto crc_actual = std::byte{
                crsf_crc8(std::span{context.frame}.subspan(
                    frame_type_index,
                    context.frame_size - head_size - crc_size)),
            };

            // frame_size проверен выше: crc всегда в пределах кадра.
            const auto crc_index =
                static_cast<gsl::index>(context.frame_size - crc_size);
            if(crc_actual != gsl::at(context.frame, crc_index))
            {
                // Лимит resync-потребления исчерпан: кадр не потребляется,
                // run() неблокирующе возвращает управление; resync
                // продолжится со следующего вызова run() с поиска заголовка.
                // Возврат в wait_head_state со сброшенным frame_size:
                // следующий run() начнёт чистую ресинхронизацию.
                if(context.skipped_count >= context.max_bytes_per_run)
                {
                    context.frame_size   = 0U;
                    context.is_need_stop = true;
                    control.template changeTo<wait_head_state>();
                    return;
                }

                // Resync: пропускается ровно один байт (sync), поиск
                // заголовка повторяется со следующего байта. Поле frame
                // length проверено только на диапазон, поэтому искажённая
                // длина могла поглотить в «кадр» последующие валидные
                // кадры — посимвольный resync находит их (как в betaflight).
                std::ignore = context.lwrb->skip(1U, is_isr);
                ++context.skipped_count;
                control.template changeTo<wait_head_state>();
                return;
            }

            control.template changeTo<dispatch_state>();
        }
    };

    /// @brief Состояние диспетчеризации: вызывает обработчик по типу кадра и
    /// изымает кадр из буфера.
    struct dispatch_state: machine_type::State {
        void update(
            typename machine_type::FullControl &control)
        {
            auto      &context = control.context();

            const auto frame_type =
                // frame_type_index — константный индекс в пределах кадра.
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
                std::to_integer<std::uint8_t>(context.frame[frame_type_index]);

            const auto handler_it = context.handlers->find(frame_type);
            if(handler_it != context.handlers->end())
            {
                // Копия делегата защищает от инвалидации итератора, если
                // обработчик изменит таблицу во время вызова.
                const auto handler = handler_it->second;
                // payload — без sync, frame length, type и crc.
                const std::span<const std::byte> payload{
                    std::span{context.frame}.subspan(
                        head_size + frame_type_size,
                        context.frame_size - head_size - frame_type_size
                            - crc_size),
                };
                std::ignore = handler(payload);
            }

            // Кадр неизвестного типа с валидным crc тоже пропускается и
            // считается разобранным.
            std::ignore = context.lwrb->skip(context.frame_size, is_isr);
            ++context.parsed_count;
            control.template changeTo<wait_head_state>();
        }
    };

    /// @brief Указатель на кольцевой буфер с входящим потоком байт.
    lwrb_type *lwrb_;

    /// @brief Timeout приема кадров управления для перехода в failsafe.
    deadline_counter_type failsafe_deadline_{};

    /// @brief Таймер перехода в failsafe. Перезапускается приемом каждого
    /// валидного кадра rc channels packed (0x16); кадры прочих типов (включая
    /// 0x14 link statistics) таймер не продлевают. Истечение таймера
    /// проверяется в run(): устанавливается is_failsafe_ и буфер каналов
    /// сбрасывается в нулевые значения.
    /// @details Таймер собран с stv::empty_mutex (без внутренней
    /// синхронизации); все обращения к нему выполняются из конструктора и
    /// run(), поэтому требование единственного контекста покрывается
    /// контрактом run() (см. @note в документации run()).
    deadline_type deadline_;

    /// @brief True, если превышен timeout на прием пакета, false если пакеты
    /// приходят регулярно.
    std::atomic<bool> is_failsafe_{false};

    /// @brief Указатель на хэш-таблицу обработчиков кадров по типу кадра.
    /// Таблица создается пользовательским кодом за пределами парсера и
    /// передается через crsf_parser_setup::handlers; парсер не владеет ею.
    handler_map_type *handlers_;

    /// @brief Буфер значений каналов управления в тиках. Пишется только
    /// обработчиком кадра rc channels packed (0x16) из run().
    crsf_rc_channels channels_{};

    /// @brief Статистика канала связи из крайнего принятого кадра link
    /// statistics (0x14). Пишется только обработчиком кадра 0x14 из run().
    crsf_link_statistics link_statistics_{};

    /// @brief Экземпляр конечного автомата разбора кадров. Владеет контекстом
    /// разбора (доступ через context()).
    typename machine_type::Instance machine_;

    const bool                      is_object_valid_;

  public:
    explicit crsf_parser(
        const setup_type &setup):
        lwrb_{setup.lwrb},
        failsafe_deadline_{setup.failsafe_deadline},
        // При невалидном setup таймер не запускается (delay == 0), чтобы не
        // разыменовывать нулевой указатель на runtime.
        deadline_{
            deadline_setup_type{
                .runtime = setup.runtime,
                .delay =
                    stv::all_true(setup.runtime, setup.lwrb, setup.handlers)
                        ? setup.failsafe_deadline
                        : deadline_counter_type{0},
            },
        },
        handlers_{setup.handlers},
        machine_{
            parser_context{
                .lwrb              = lwrb_,
                .handlers          = handlers_,
                .max_bytes_per_run = setup.max_bytes_per_run,
            },
        },
        is_object_valid_{
            stv::all_true(setup.runtime, setup.lwrb, setup.handlers),
        }
    {
        // Встроенные обработчики кадров rc channels packed (0x16) и link
        // statistics (0x14).
        std::ignore = register_handler(
            crsf_type_rc_channels_packed,
            crsf_handler_type::create<crsf_parser,
                                      &crsf_parser::handle_rc_channels>(*this));
        std::ignore = register_handler(
            crsf_type_link_statistics,
            crsf_handler_type::create<crsf_parser,
                                      &crsf_parser::handle_link_statistics>(
                *this));
    }

    explicit operator bool() const { return is_object_valid_; }

    /// @brief Возвращает признак failsafe: true, если с момента приема
    /// крайнего валидного кадра rc channels packed (0x16) истек
    /// setup.failsafe_deadline.
    /// @details Флаг атомарен, чтение допустимо из другого потока. В failsafe
    /// буфер channels() сброшен в нулевые значения; первый валидный кадр 0x16
    /// после failsafe снимает флаг и обновляет каналы.
    auto is_failsafe() const -> bool { return is_failsafe_.load(); }

    /// @brief Возвращает буферизированные значения каналов управления в
    /// тиках, распакованные из крайнего принятого кадра rc channels packed
    /// (0x16).
    /// @details До первого принятого кадра 0x16 все значения равны нулю.
    /// @note Потокобезопасность: буфер пишется только из run(); читать
    /// channels() допустимо после завершения run() в том же потоке. При
    /// чтении из другого потока одновременно с run() требуется внешняя
    /// синхронизация. Возвращаемая ссылка указывает на внутренний буфер
    /// (живые данные, а не снимок): содержимое изменяется каждым run().
    /// Пара is_failsafe() + channels() не является атомарным снимком:
    /// кросс-поточному читателю следует синхронизироваться с задачей
    /// разбора либо копировать данные под внешней блокировкой.
    auto channels() const -> const crsf_rc_channels & { return channels_; }

    /// @brief Возвращает значение указанного в ch_numb калана.
    ///
    /// @param[in] ch_numb Номер канала, значение которого необходимо вернуть.
    ///
    /// @return Значение указанного канала.
    auto get_channel(
        std::uint8_t ch_numb) const
    {
        // Номер канала по контракту меньше crsf_rc_channels_count.
        return gsl::at(channels_, ch_numb);
    }

    /// @brief Возвращает статистику канала связи, разобранную из крайнего
    /// принятого кадра link statistics (0x14).
    /// @details До первого принятого кадра 0x14 все поля равны нулю.
    /// @note Потокобезопасность: статистика пишется только из run(); читать
    /// link_statistics() допустимо после завершения run() в том же потоке.
    /// При чтении из другого потока одновременно с run() требуется внешняя
    /// синхронизация. Возвращаемая ссылка указывает на внутренний буфер
    /// (живые данные, а не снимок): содержимое изменяется каждым run().
    auto link_statistics() const -> const crsf_link_statistics &
    { return link_statistics_; }

    /// @brief Регистрирует пользовательский обработчик кадра заданного типа.
    /// @param frame_type Тип кадра crsf.
    /// @param handler Обработчик кадра; получает payload без sync, frame
    /// length, type и crc.
    /// @return true, если обработчик зарегистрирован; false, если таблица
    /// обработчиков не задана (nullptr), полна или тип кадра уже занят.
    /// @note Потокобезопасность: вызывать до первого run() либо при
    /// остановленном разборе; конкурентный с run() вызов — гонка данных по
    /// таблице обработчиков и требует внешней синхронизации.
    auto register_handler(
        std::uint8_t frame_type, crsf_handler_type handler) -> bool
    {
        if(handlers_ == nullptr || handlers_->full())
        {
            return false;
        }

        if(handlers_->find(frame_type) != handlers_->end())
        {
            return false;
        }

        handlers_->insert({frame_type, handler});
        return true;
    }

    /// @brief Выполняет один проход разбора: изымает из кольцевого буфера все
    /// полные валидные кадры.
    /// @details Поведение resync описано в документации класса. Перед разбором
    /// проверяется failsafe deadline: если с момента приема крайнего валидного
    /// кадра rc channels packed (0x16) истек setup.failsafe_deadline,
    /// устанавливается is_failsafe() и буфер каналов сбрасывается в нулевые
    /// значения.
    /// @details Неблокирующая гарантия: run() всегда возвращает управление —
    /// каждая итерация конечного автомата либо потребляет байты, либо
    /// завершает вызов; resync-потребление мусора ограничено
    /// setup.max_bytes_per_run за вызов. При нарушении контракта буфера
    /// (внешний сброс/опустошение, peek() вернул меньше байт, чем get_full())
    /// автомат переводится в wait_head_state, и следующий run() начинает
    /// чистую ресинхронизацию.
    /// @return Количество валидных кадров, изъятых из буфера за вызов
    /// (включая кадры неизвестных типов). На невалидном объекте
    /// (operator bool() == false) возвращает 0, не обращаясь к буферу.
    /// @note Потокобезопасность: run() должен вызываться только из одной
    /// задачи (парсер — единственный читатель кольцевого буфера);
    /// одновременный вызов run() из нескольких задач — гонка данных по
    /// состоянию конечного автомата и запрещён.
    auto processing() -> std::size_t
    {
        if(!is_object_valid_)
        {
            return 0U;
        }

        // Проверка failsafe: deadline перезапускается только кадрами 0x16.
        if(deadline_.is_elapsed())
        {
            // Сначала обнуляется буфер каналов, затем взводится флаг
            // failsafe: читатель из другого потока, увидевший флаг,
            // гарантированно видит и обнулённые каналы (happens-before через
            // seq_cst store).
            channels_.fill(std::uint16_t{0U});
            is_failsafe_ = true;
        }

        auto &context         = machine_.context();
        context.parsed_count  = 0U;
        context.skipped_count = 0U;
        context.is_need_stop  = false;

        while(!context.is_need_stop)
        {
            machine_.update();
        }

        return context.parsed_count;
    }

    /// @brief Обертка для периодического вызова в контексте
    /// stv::callback_timer_context{}.
    void run() { processing(); }

  private:
    /// @brief Обработчик кадра rc channels packed (0x16).
    /// @details Распаковывает 16 каналов по 11 бит lsb-first из первых
    /// @ref crsf_rc_channels_payload_size байт payload в буфер каналов; байты
    /// сверх этого размера (расширенный payload) игнорируются. Packed
    /// bitfield-структуры не используются из-за платформозависимости:
    /// распаковка выполняется побитово — канал i занимает биты
    /// [i*11, i*11+11) потока payload. После распаковки вызывается
    /// on_any_rc_frame().
    /// @param payload Payload кадра без sync, frame length, type и crc.
    /// @return true, если payload достаточен для распаковки всех каналов;
    /// false, если payload короче @ref crsf_rc_channels_payload_size (буфер
    /// каналов при этом не изменяется).
    auto handle_rc_channels(
        std::span<const std::byte> payload) -> bool
    {
        if(payload.size() < crsf_rc_channels_payload_size)
        {
            return false;
        }

        constexpr std::size_t bits_per_channel{11U};
        constexpr std::size_t bits_per_byte{8U};

        for(std::size_t channel{0U}; channel < crsf_rc_channels_count;
            ++channel)
        {
            std::uint16_t value{0U};
            for(std::size_t bit{0U}; bit < bits_per_channel; ++bit)
            {
                const std::size_t bit_index{(channel * bits_per_channel) + bit};
                const auto        byte = static_cast<
                    unsigned>(std::to_integer<std::uint8_t>(
                    // bit_index < 176, поэтому индекс байта всегда меньше
                    // crsf_rc_channels_payload_size (22), проверенного
                    // выше.
                    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
                    payload[bit_index / bits_per_byte]));
                value |= static_cast<std::uint16_t>(
                    ((byte >> (bit_index % bits_per_byte)) & 1U) << bit);
            }
            // Номер канала ограничен циклом значением crsf_rc_channels_count.
            gsl::at(channels_, static_cast<gsl::index>(channel)) = value;
        }

        on_any_rc_frame();
        return true;
    }

    /// @brief Точка перезапуска failsafe deadline: вызывается обработчиком
    /// каждого валидного кадра rc channels packed (0x16).
    /// @details Только кадры 0x16 (новая команда управления) перезапускают
    /// deadline: кадры 0x14 (link statistics) и кадры прочих типов deadline
    /// не продлевают. Прием кадра 0x16 также снимает флаг failsafe.
    void on_any_rc_frame()
    {
        std::ignore  = deadline_.set_delay(failsafe_deadline_);
        is_failsafe_ = false;
    }

    /// @brief Обработчик кадра link statistics (0x14).
    /// @details Разбирает первые @ref crsf_link_statistics_payload_size
    /// байт payload в структуру статистики канала связи. Поля up_snr и
    /// down_snr знаковые (db, могут быть отрицательными) — байты payload
    /// интерпретируются как std::int8_t.
    /// @param payload Payload кадра без sync, frame length, type и crc.
    /// @return true, если payload достаточен для разбора всех полей; false,
    /// если payload короче @ref crsf_link_statistics_payload_size
    /// (статистика при этом не изменяется).
    auto handle_link_statistics(
        std::span<const std::byte> payload) -> bool
    {
        if(payload.size() < crsf_link_statistics_payload_size)
        {
            return false;
        }

        // Размер payload проверен выше: не меньше
        // crsf_link_statistics_payload_size (10), поэтому обращения по
        // константным индексам 0..9 безопасны.
        // NOLINTBEGIN(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
        link_statistics_.up_rssi_ant1 =
            std::to_integer<std::uint8_t>(payload[0U]);
        link_statistics_.up_rssi_ant2 =
            std::to_integer<std::uint8_t>(payload[1U]);
        link_statistics_.up_link_quality =
            std::to_integer<std::uint8_t>(payload[2U]);
        link_statistics_.up_snr = static_cast<std::int8_t>(
            std::to_integer<std::uint8_t>(payload[3U]));
        link_statistics_.active_antenna =
            std::to_integer<std::uint8_t>(payload[4U]);
        link_statistics_.rf_profile =
            std::to_integer<std::uint8_t>(payload[5U]);
        link_statistics_.up_rf_power =
            std::to_integer<std::uint8_t>(payload[6U]);
        link_statistics_.down_rssi = std::to_integer<std::uint8_t>(payload[7U]);
        link_statistics_.down_link_quality =
            std::to_integer<std::uint8_t>(payload[8U]);
        link_statistics_.down_snr = static_cast<std::int8_t>(
            std::to_integer<std::uint8_t>(payload[9U]));
        // NOLINTEND(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
        return true;
    }
};

} // namespace stv

#endif /* CRSF_HPP */
