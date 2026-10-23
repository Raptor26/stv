/// @file demo_messages.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
///
/// NAME
///     stv demo messages -- внутренний учебный протокол сообщений stv.
///
/// DESCRIPTION
///     Демонстрационные типы сообщений для тестов stv: перечисление
///     идентификаторов demo_msg_id и четыре структуры с разными способами
///     описания полезной нагрузки:
///     - demo_telemetry -- тривиально копируемая структура (декодируется
///       побайтовым копированием);
///     - demo_command -- структура с битстрим-кодеком serialized_size_bytes()
///       и deserialize_from(begin, end) по контракту stv::try_deserialize;
///     - demo_status_text -- структура-тег со строкой-view, декодируется
///       только явной специализацией декодера;
///     - demo_registers -- тривиально копируемый блок регистров, читается
///       декодером zero-copy доступом view().
///
///     Заголовок также предоставляет общий для тестов список типов
///     протокола (demo_list) и сгенерированный из него каталог (demo_catalog).
///
///     Заголовок -- внутренний тестовый протокол stv, НЕ публичный API:
///     интегрирующие проекты описывают собственные типы сообщений.
///
/// SEE ALSO
///     message_traits.hpp (первичный шаблон и концепты),
///     stvlinkv2_kit.hpp (контракт stv::try_deserialize).

#ifndef DEMO_MESSAGES_HPP
#define DEMO_MESSAGES_HPP

#include "stv/communication/message_catalog.hpp"
#include "stv/communication/message_traits.hpp"
#include "stv/communication/payload_decoder.hpp"
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace stv {

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
///     среза большего размера игнорирует лишние хвостовые байты, из среза
///     меньшего размера оставляет недостающие хвостовые поля нулевыми.
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
///     sizeof(demo_registers) — min_pload_size_bytes в traits равен
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

#endif // DEMO_MESSAGES_HPP
