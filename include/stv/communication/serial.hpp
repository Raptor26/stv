/// @file serial.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// @copyright (c) 2025 Gagaring
///
/// MIT License:
///
/// Permission is hereby granted, free of charge, to any person obtaining a copy
/// of this software and associated documentation files (the 'Software'), to
/// deal in the Software without restriction, including without limitation the
/// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
/// sell copies of the Software, and to permit persons to whom the Software is
/// furnished to do so, subject to the following conditions:
///
/// The above copyright notice and this permission notice shall be included in
/// all copies or substantial portions of the Software.
///
/// THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
/// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
/// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
/// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
/// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
/// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
/// IN THE SOFTWARE.

#ifndef SERIAL_HPP
#define SERIAL_HPP

#include "stv/containers/simbuff.hpp"
#include <cassert>
#include <concepts>
#include <iterator>
#include <limits>
#include <memory>
#include <span>
#include <string_view>

namespace stv {

/// @brief Тип, указывающий на все сообщение в целом.
class total_message_span: public std::span<const std::byte>
{
    using std::span<const std::byte>::span;
};

/// @brief Псевдоним типа, указывающий на полезную нагрузку.
using pload_span = std::span<const std::byte>;

struct empty_serial_decorator {
    /// @brief Возвращает размер заголовка который будет добавлен перед полезной
    /// нагрузкой.
    ///
    /// @return Размер в байтах заголовка, который будет добавлен перед полезной
    /// нагрузкой.
    static constexpr size_t header_size() { return 0U; }

    /// @brief Возвращает размер хвоста, который будет добавлен после полезной
    /// нагрузки.
    ///
    /// @return Размер в байтах хвоста, который будет добавлен после полезной
    /// нагрузкой.
    static constexpr size_t trailer_size() { return 0U; }

    ///
    static void setup_header(
        std::byte *const dst, const total_message_span &total,
        const pload_span &pload)
    {
        (void)dst;
        (void)total;
        (void)pload;
    }

    static void setup_trailer(
        std::byte *dst, const total_message_span &total)
    {
        (void)dst;
        (void)total;
    }
};

/// @brief Декоратор начала сообщения и CRC.
///
/// @details Оборачивает полезную нагрузку символами Start Frame и контрольной
/// суммой.
class start_frame_and_crc_16
{
    using frame_size_type = std::uint8_t;
    using crc_type        = std::uint16_t;

#ifdef UNIT_TEST_ENABLE
  public:
#else
  private:
#endif

    STV_NO_PADDING_NO_OPTIMIZE_BEGIN

    struct start_frame_t {
        std::byte start_frame_first;
        std::byte start_frame_second;

        /// @brief Размер остальной части сообщения.
        frame_size_type frame_size;
    };

    STV_NO_PADDING_NO_OPTIMIZE_END

  public:
    static constexpr std::byte FIRST_BYTE{0xAA};
    static constexpr std::byte SECOND_BYTE{0xAA};

    /// @brief Возвращает размер заголовка который будет добавлен перед полезной
    /// нагрузкой.
    ///
    /// @return Размер в байтах заголовка, который будет добавлен перед полезной
    /// нагрузкой.
    static constexpr size_t header_size() { return sizeof(start_frame_t); }

    /// @brief Возвращает размер хвоста, который будет добавлен после полезной
    /// нагрузки.
    ///
    /// @return Размер в байтах хвоста, который будет добавлен после полезной
    /// нагрузкой.
    static constexpr size_t trailer_size() { return sizeof(crc_type); }

    /// @brief Заполняет заголовок.
    ///
    /// @param[out] dst Указатель на заголовок, который необходимо
    /// заполнить.
    /// @param[in] total Границы всего сообщения.
    /// @param[in] pload Границы полезной нагрузки.
    ///
    static void setup_header(
        std::byte *const dst, const total_message_span &total,
        const pload_span &pload)
    {
        (void)pload;
        auto *start_frame              = reinterpret_cast<start_frame_t *>(dst);
        start_frame->start_frame_first = FIRST_BYTE;
        start_frame->start_frame_second = SECOND_BYTE;
        start_frame->frame_size =
            static_cast<frame_size_type>(total.size_bytes() - header_size());
    }

    /// @brief Заполняет хвост.
    ///
    /// @param[out] dst Указатель на хвост сообщения.
    /// @param[in] total Память, выделенная под все сообщение.
    static void setup_trailer(
        std::byte *dst, const total_message_span &total)
    {
        auto *crc = reinterpret_cast<crc_type *>(&dst[0]);
        *crc = calculate_crc(total.data(), total.size_bytes() - trailer_size());
    }

  private:
    static uint16_t calculate_crc(
        const std::byte *data, size_t length)
    {
        // NOLINTBEGIN(hicpp-signed-bitwise)
        // Простая реализация Crc для примера
        uint16_t crc = 0xFFFF;
        for(size_t i = 0; i < length; ++i)
        {
            crc ^= static_cast<std::uint8_t>(data[i]);
            for(int j = 0; j < 8; ++j)
            {
                if(crc & 0x0001)
                {
                    crc = (crc >> 1) ^ 0xA001;
                }
                else
                {
                    crc >>= 1;
                }
            }
        }
        // NOLINTEND(hicpp-signed-bitwise)
        return crc;
    }
};

struct head_route {
    /// ########################################################################
    /// Арбитраж сообщения.
    /// ########################################################################
    STV_NO_PADDING_NO_OPTIMIZE_BEGIN

    struct head_route_setup_t {
        uint8_t dst_id{std::numeric_limits<decltype(dst_id)>::min()};
        uint8_t pack_id{std::numeric_limits<decltype(pack_id)>::min()};
    };

    STV_NO_PADDING_NO_OPTIMIZE_END

    /// ------------------------------------------------------------------------

    explicit head_route(
        const head_route_setup_t &setup = head_route_setup_t{.dst_id  = 0,
                                                             .pack_id = 0}):
        setup_{setup}
    {
    }

    static constexpr size_t header_size()
    {
        return sizeof(head_route_setup_with_pload_t);
    }

    static constexpr size_t trailer_size() { return 0; }

    /// @brief Заполняет заголовок сообщения.
    void setup_header(
        std::byte *const dst, const total_message_span &total,
        const pload_span &pload) const
    {
        (void)total;
        auto *header = reinterpret_cast<head_route_setup_with_pload_t *>(dst);
        *header = static_cast<std::remove_pointer_t<decltype(header)>>(setup_);
        header->pload_size =
            static_cast<decltype(header->pload_size)>(pload.size_bytes());
        assert(pload.size_bytes()
               <= std::numeric_limits<decltype(header->pload_size)>::max());
    }

    static void setup_trailer(
        std::byte *dst, const total_message_span &total, size_t total_size)
    {
        (void)dst;
        (void)total;
        (void)total_size;
    }

#ifdef UNIT_TEST_ENABLE
  public:
#else
  private:
#endif
    STV_NO_PADDING_NO_OPTIMIZE_BEGIN

    struct head_route_setup_with_pload_t: public head_route_setup_t {
        uint8_t pload_size{std::numeric_limits<decltype(pload_size)>::min()};
    };

    STV_NO_PADDING_NO_OPTIMIZE_END

  private:
    head_route_setup_t setup_;
};

template<typename TQueue, typename... Decorators>
class composite_serial_message
{
    using queue_type = TQueue;
    using container_type =
        queue_type::value_type; ///< Тип объекта, который можно
                                ///< поместить в queue_.

    /// @brief Декораторы.
    const std::tuple<Decorators...> decorators_;

    const std::uint16_t             payload_size_;
    const std::uint16_t             header_size_;
    const std::uint16_t             trailer_size_;
    const std::uint16_t             total_size_;

    queue_type                     &queue_;
    container_type                  memory_;

  public:
    using span_type = std::span<const std::byte>;

    composite_serial_message(
        queue_type &queue, const pload_span &pload, Decorators &&...decorators):
        decorators_{std::forward<Decorators>(decorators)...},
        payload_size_{static_cast<decltype(payload_size_)>(pload.size_bytes())},
        header_size_{
            static_cast<decltype(header_size_)>(compute_header_size())},
        trailer_size_{
            static_cast<decltype(trailer_size_)>(compute_trailer_size())},
        total_size_{static_cast<decltype(total_size_)>(
            header_size_ + payload_size_ + trailer_size_)},
        queue_{queue},
        memory_(total_size_)
    {
        if(pload.data())
        {
            memcpy(memory_.begin() + header_size_, pload.data(),
                   pload.size_bytes());
        }
    }

    explicit operator bool() const { return memory_.data() != nullptr; }

    virtual ~composite_serial_message()
    {
        setup_all_headers();

        setup_all_trailers(memory_.begin() + header_size_ + payload_size_,
                           memory_.begin(), total_size_);

        queue_.push(std::move(memory_));
    }

    std::byte *pload() { return memory_.begin() + header_size_; }

    [[nodiscard]] const std::byte *pload() const
    {
        return memory_.begin() + header_size_;
    }

  private:
    constexpr size_t compute_header_size()
    {
        return std::apply(
            [](auto &...decorators) {
                return (decorators.header_size() + ...);
            },
            decorators_);
    }

    constexpr size_t compute_trailer_size()
    {
        return std::apply(
            [](auto &...decorators) {
                return (decorators.trailer_size() + ...);
            },
            decorators_);
    }

    void setup_all_headers()
    {
        assert(memory_.begin());
        size_t offset{0};
        std::apply(
            [&](auto &...decorators) {
                (setup_single_header(decorators, offset), ...);
            },
            decorators_);
    }

    template<typename Decorator>
    void setup_single_header(
        Decorator &decorator, size_t &offset) const
    {
        if constexpr(Decorator::header_size() > 0)
        {
            decorator.setup_header(
                memory_.begin() + offset,
                total_message_span(memory_.begin(), memory_.size()),
                pload_span(pload(), payload_size_));
            offset += Decorator::header_size();
        }
    }

    /// @brief Вызывает все декораторы, заполняющие хвосты сообщений.
    void setup_all_trailers(
        std::byte *dst, const std::byte *data, size_t data_size)
    {
        size_t offset{0};
        std::apply(
            [&](auto &...decorators) {
                (setup_single_trailer(decorators, dst, offset, data, data_size),
                 ...);
            },
            decorators_);
    }

    template<typename Decorator>
    void setup_single_trailer(
        Decorator &decorator, std::byte *dst, size_t &offset,
        const std::byte *full_message, size_t full_message_size)
    {
        if constexpr(Decorator::trailer_size() > 0)
        {
            decorator.setup_trailer(
                dst + offset,
                total_message_span(full_message, full_message_size));
            offset += Decorator::trailer_size();
        }
    }
};

template<typename UserData, typename TQueue, typename... Decorators>
class serial_message
{
    using queue_type = TQueue;
    using user_type  = UserData;
    using composite_serial_message_type =
        stv::composite_serial_message<queue_type, Decorators...>;
    using span_type = typename composite_serial_message_type::span_type;

    composite_serial_message_type composite_message_;

  public:
    serial_message(
        queue_type &queue, const span_type &pload, Decorators... decorators):
        composite_message_{queue, pload,
                           std::forward<Decorators>(decorators)...}
    {
    }

    virtual ~serial_message() = default;

    /// @brief Возвращает true если успешно выделена память под сообщение.
    explicit operator bool() const
    {
        return static_cast<bool>(composite_message_);
    }

    /// @brief Возвращает указатель на полезную нагрузку сообщения.
    auto operator->()
    {
        return reinterpret_cast<user_type *>(composite_message_.pload());
    }
};

template<typename TQueue, typename... Decorators>
class serial_message_buffer: private stv::non_copyable, private stv::non_movable
{
    using queue_type = TQueue;

    const std::tuple<Decorators...> decorators_;
    queue_type                      queue_;

  public:
    using span_type = std::span<const std::byte>;

    explicit serial_message_buffer(
        Decorators &&...decorators):
        decorators_{std::forward<Decorators>(decorators)...}
    {
    }

    virtual ~serial_message_buffer() = default;

    auto request(
        std::string_view str)
    {
        return request_impl<decltype(str)::value_type>(
            std::as_bytes(std::span(str.data(), str.size())));
    }

    template<typename... SetupParams>
    auto request(
        std::string_view str, SetupParams &&...setup_params)
    {
        return request_impl<decltype(str)::value_type>(
            span_type(reinterpret_cast<const std::byte *>(str.data()),
                      str.size()),
            std::forward<SetupParams>(setup_params)...);
    }

    auto request(
        const char *str)
    {
        return request(std::string_view{str});
    }

    template<typename... SetupParams>
    auto request(
        const char *str, SetupParams &&...setup_params)
    {
        return request(std::string_view{str},
                       std::forward<SetupParams>(setup_params)...);
    }

    // Для std::vector и других контейнеров
    template<stv::contiguous_trivial_container_concept Container>
    auto request(
        const Container &container)
    {
        using value_type = typename Container::value_type;
        return request_impl<value_type>(std::as_bytes(std::span{container}));
    }

    template<typename UserData, typename... SetupParams>
    auto request(
        const UserData &user_data, SetupParams &&...setup_params)
    {
        return request_impl<UserData>(
            span_type(reinterpret_cast<const std::byte *>(&user_data),
                      sizeof(UserData)),
            std::forward<SetupParams>(setup_params)...);
    }

    template<typename UserData, typename... SetupParams>
    auto request(
        SetupParams &&...setup_params)
    {
        return request_impl<UserData>(
            span_type(static_cast<const std::byte *>(nullptr),
                      sizeof(UserData)),
            std::forward<SetupParams>(setup_params)...);
    }

    template<typename InputIt>
    auto request(
        InputIt cbegin, InputIt cend)
    {
        return request(std::as_bytes(std::span{cbegin, cend}));
    }

  private:
    template<typename UserData, typename... SetupParams>
    auto request_impl(
        const span_type &pload, SetupParams &&...setup_params)
    {
        auto decorators_copy = decorators_;

        // Сопоставление каждого аргумента из пачки параметров setup_params с
        // каждым декоратором.
        (apply_param_to_decorators(decorators_copy,
                                   std::forward<SetupParams>(setup_params)),
         ...);

        // Конструирование объекта serial_message из updated_decorators.
        return std::apply(
            [this, &pload](auto &&...decs) {
                return serial_message<UserData, TQueue, Decorators...>(
                    queue_, pload, std::forward<decltype(decs)>(decs)...);
            },
            decorators_copy);
    }

    template<typename Param>
    void apply_param_to_decorators(
        auto &tuple_of_decorators, Param &param)
    {
        std::apply(
            [&param, this](auto &...decorators) {
                (void)this;
                (apply_to_single_decorator(decorators, param), ...);
            },
            tuple_of_decorators);
    }

    // Универсальная проверка - пытаемся сконструировать декоратор из параметра
    template<typename Decorator, typename Param>
    void apply_to_single_decorator(
        Decorator &decorator, const Param &param)
    {
        if constexpr(std::is_constructible_v<Decorator, Param>)
        {
            decorator = Decorator(param);
        }
    }
};

template<typename TQueue, typename... Decorators>
auto make_serial_message_buffer(
    Decorators &&...decorators)
{
    return serial_message_buffer<TQueue, Decorators...>(
        std::forward<Decorators>(decorators)...);
}

} // namespace stv

#endif /* SERIAL_HPP */
