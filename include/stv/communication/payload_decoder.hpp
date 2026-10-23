/// @file payload_decoder.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
///
/// NAME
///     stv::payload_decoder — концепт-диспетчеризуемый декодер
///     полезной нагрузки входящих сообщений.
///
/// DESCRIPTION
///     Единый шаблон payload_decoder<T>::decode(): путь декодирования
///     выбирается концептами (порядок важен):
///     - есть битстрим-кодек (serialized_size_bytes() и deserialize_from) —
///       stv::try_deserialize() с собственной проверкой размера (полезная
///       нагрузка меньше serialized_size_bytes() отклоняется);
///     - иначе тип тривиально копируемый — побайтовое копирование префикса
///       min(size, sizeof(T)) в T{} (принцип обратной совместимости по
///       спецификации CRSF: недостающие хвостовые поля остаются нулевыми,
///       лишние байты больших кадров игнорируются); минимальный размер НЕ
///       проверяется — на пути через диспетчер его гарантирует обёртка
///       guarded_handler (см. stv/communication/message_catalog.hpp);
///     - строка и прочие не тривиально копируемые типы — явная
///       специализация рядом со структурой сообщения.
///
///     Тривиально копируемый путь дополнительно предоставляет
///     payload_decoder<T>::view(): третий способ доступа к полезной
///     нагрузке — const-указатель на байты кадра без копирования
///     (zero-copy). Метод параметризован политикой выравнивания
///     (payload_alignment): по умолчанию (relaxed) runtime-проверка
///     выравнивания отключена — включение проверки (checked) даёт nullptr
///     при невыравненном адресе; проверка размера выполняется всегда.
///     Битстрим-путь view() удалён (= delete): сериализованные данные не
///     являются объектом T.
///
///     Первичный шаблон без определения: тип без пути декодирования ловится
///     static_assert'ом специализации-ловушки с понятным сообщением
///     (static_assert внутри тела decode() — срабатывает только при
///     инстанцировании вызова).
///
/// SEE ALSO
///     stv/communication/stvlinkv2_kit.hpp (stv::try_deserialize),
///     stv/communication/message_traits.hpp (min_pload_size_bytes),
///     stv/communication/message_catalog.hpp (guarded_handler, каталог).

#ifndef PAYLOAD_DECODER_HPP
#define PAYLOAD_DECODER_HPP

#include "stv/communication/stvlinkv2_kit.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <optional>
#include <span>
#include <type_traits>

namespace stv {

namespace decoder_detail {

/// @brief Концепт: тип имеет битстрим-кодек полезной нагрузки.
///
/// @details Первый путь выбора декодера (порядок важен): статический метод
///     serialized_size_bytes() и метод deserialize_from(begin, end) — контракт
///     stv::try_deserialize().
template<typename T>
concept has_bitstream_codec = requires {
    T::serialized_size_bytes();
} && requires(T value, std::span<std::byte> pload) {
    value.deserialize_from(pload.begin(), pload.end());
};

} // namespace decoder_detail

/// @brief Политика проверки выравнивания полезной нагрузки view().
enum class payload_alignment {
    relaxed, ///< Проверка выключена: доступ по невыравненному адресу разрешён.
    checked, ///< Проверка включена: невыравненный адрес даёт nullptr.
};

/// @brief Первичный шаблон декодера полезной нагрузки (без определения).
///
/// @details Каждый тип сообщения декодируется частичной специализацией по
///     концептам (ниже) или явной специализацией рядом со структурой.
template<typename T>
struct payload_decoder;

/// @brief Декодер полезной нагрузки по битстрим-кодеку сообщения.
///
/// @details Первый путь выбора (порядок важен): stv::try_deserialize() с
///     собственной проверкой размера — она сохранена, т.к. путь нужен прямому
///     приёмнику в обход диспетчера (guarded_handler на этом пути
///     отсутствует).
template<typename T>
    requires decoder_detail::has_bitstream_codec<T>
struct payload_decoder<T> {
    /// @brief Десериализует полезную нагрузку битстрим-кодеком сообщения.
    [[nodiscard]] static auto decode(
        std::span<std::byte> pload) -> std::optional<T>
    { return stv::try_deserialize<T>(pload); }

    /// @brief Не поддерживается: материализация обязательна.
    ///
    /// @details Битстрим-кодек сериализует данные, не являющиеся
    ///     представлением объекта T в памяти, поэтому zero-copy доступ к ним
    ///     как к const T* бессмыслен — используйте decode().
    static auto view(std::span<std::byte> pload) -> const T * = delete;
};

/// @brief Декодер полезной нагрузки побайтовым копированием префикса.
///
/// @details Второй путь выбора: тип тривиально копируемый и не имеет
///     битстрим-кодека. Принцип обратной совместимости из спецификации CRSF:
///     недостающие хвостовые поля остаются нулевыми (значение создаётся
///     вида T{}), лишние байты больших кадров игнорируются. Минимальный
///     размер НЕ проверяется: на пути через диспетчер его гарантирует
///     обёртка guarded_handler (см. stv/communication/message_catalog.hpp).
template<typename T>
    requires(!decoder_detail::has_bitstream_codec<T>)
            && std::is_trivially_copyable_v<T>
struct payload_decoder<T> {
    /// @brief Копирует префикс полезной нагрузки в нулевую структуру.
    [[nodiscard]] static auto decode(
        std::span<std::byte> pload) -> std::optional<T>
    {
        T value{};
        std::memcpy(&value, pload.data(),
                    std::min(pload.size_bytes(), sizeof(T)));
        return value;
    }

    /// @brief Возвращает const-указатель на полезную нагрузку без копирования.
    ///
    /// @details Третий способ доступа к полезной нагрузке (decode — побайтовая
    ///     копия; теги со view — явная специализация): reinterpret_cast байтов
    ///     кадра как const T*. Runtime-проверки: размер меньше sizeof(T) →
    ///     nullptr всегда; адрес невыравнен для T → nullptr только при
    ///     Alignment == payload_alignment::checked.
    ///
    /// @tparam Alignment Политика проверки выравнивания. По умолчанию
    ///     relaxed (проверка выключена): выравнивание — свойство контроллера,
    ///     на PC и на ядрах с аппаратной поддержкой невыровненного доступа
    ///     доступ по невыравненному адресу легален. На ядрах БЕЗ аппаратной
    ///     поддержки (например Cortex-M0/M23 — в том числе GD32E230
    ///     прошивки проекта) обязательно вызывайте с
    ///     payload_alignment::checked, иначе невыравненный доступ —
    ///     аппаратный отказ. Сохраняйте дефолт relaxed, только если знаете,
    ///     что устройство допускает невыравненный доступ.
    ///
    ///     Идиома фиксации политики на стороне устройства (wrapper):
    ///     @code
    ///     // Устройство без аппаратной поддержки невыровненного доступа:
    ///     template<typename T>
    ///     [[nodiscard]] auto checked_view(std::span<std::byte> pload) -> const
    ///     T* { return
    ///     stv::payload_decoder<T>::view<stv::payload_alignment::checked>(pload);
    ///     }
    ///     @endcode
    ///
    /// @note relaxed сохраняет тот же класс формальной UB, что и
    ///     reinterpret_cast ниже: разрешённость доступа — ответственность
    ///     пользователя, знающего свой контроллер.
    ///
    ///     Семантические отличия от decode():
    ///     - требуется полный размер sizeof(T): CRSF-совместимый нулевой
    ///       хвост decode() недоступен — поэтому min_pload_size_bytes в
    ///       traits view-обрабатываемого сообщения обязан быть sizeof(T);
    ///     - указатель валиден только на время обработки сообщения: буфер
    ///       парсера переиспользуется между кадрами;
    ///     - биты выравнивания-паддинга внутри T не определены.
    ///
    /// @note Реализация через reinterpret_cast: std::start_lifetime_as
    ///     отсутствует в используемых тулчейнах (xpack arm-none-eabi-g++
    ///     14.2.1, Apple clang), поэтому формально возможно UB (время жизни
    ///     объекта / strict aliasing). Путь будет заменён на
    ///     std::start_lifetime_as при появлении в тулчейнах.
    template<payload_alignment Alignment = payload_alignment::relaxed>
    [[nodiscard]] static auto view(
        std::span<std::byte> pload) -> const T *
    {
        if(pload.size_bytes() < sizeof(T))
        {
            return nullptr;
        }
        if constexpr(Alignment == payload_alignment::checked)
        {
            if(reinterpret_cast<std::uintptr_t>(pload.data()) % alignof(T)
               != 0U)
            {
                return nullptr;
            }
        }
        return reinterpret_cast<const T *>(pload.data());
    }
};

/// @brief Ловушка типа без пути декодирования полезной нагрузки.
///
/// @details Тип не имеет битстрим-кодека и не тривиально копируемый:
///     добавьте явную специализацию payload_decoder рядом со структурой
///     сообщения.
template<typename T>
    requires(!decoder_detail::has_bitstream_codec<T>)
            && (!std::is_trivially_copyable_v<T>)
struct payload_decoder<T> {
    /// @brief Не компилируется: у типа нет пути декодирования.
    [[nodiscard]] static auto decode(
        std::span<std::byte> pload) -> std::optional<T>
    {
        static_assert(
            decoder_detail::has_bitstream_codec<T>
                || std::is_trivially_copyable_v<T>,
            "payload_decoder<T>: the type has no payload "
            "decoding path -- "
            "no bitstream codec (serialized_size_bytes() + "
            "deserialize_from) and the type "
            "is not trivially copyable; add an explicit "
            "payload_decoder<T> specialization next to the message struct");
        (void)pload;
        return std::nullopt;
    }
};

} // namespace stv

#endif // PAYLOAD_DECODER_HPP
