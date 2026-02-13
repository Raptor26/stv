/// @file serial_sender.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
/// 
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.

#ifndef SERIAL_HPP
#define SERIAL_HPP

#include "etl/queue.h"
#include "serial_decorators.hpp"
#include "stv/containers/simbuff.hpp"
#include <cassert>
#include <concepts>
#include <cstddef>
#include <etl/utility.h>
#include <iterator>
#include <limits>
#include <memory>
#include <span>
#include <string_view>

namespace stv {

template<typename TQueueBase, typename... Decorators>
class composite_serial_message
{
    using queue_base_type = TQueueBase;
    using container_type =
        queue_base_type::value_type; ///< Тип объекта, который можно
                                     ///< поместить в queue_.

    /// @brief Декораторы.
    const std::tuple<Decorators...> decorators_;

    const std::uint16_t             payload_size_;
    const std::uint16_t             header_size_;
    const std::uint16_t             trailer_size_;
    const std::uint16_t             total_size_;

    queue_base_type                &queue_;
    container_type                  memory_;

  public:
    using span_type = std::span<const std::byte>;

    composite_serial_message(
        queue_base_type &queue, const pload_span &pload,
        Decorators &&...decorators):
        decorators_{std::forward<Decorators>(decorators)...},
        payload_size_{static_cast<decltype(payload_size_)>(pload.size_bytes())},
        header_size_{
            static_cast<decltype(header_size_)>(compute_header_size())},
        trailer_size_{
            static_cast<decltype(trailer_size_)>(compute_trailer_size())},
        total_size_{static_cast<decltype(total_size_)>(
            header_size_ + payload_size_ + trailer_size_)},
        queue_{queue},
        memory_{total_size_}
    {
        if(pload.data() && memory_.begin())
        {
            memcpy(memory_.begin() + header_size_, pload.data(),
                   pload.size_bytes());
        }
    }

    explicit composite_serial_message(
        std::nullptr_t, queue_base_type &queue):
        decorators_{},
        // value-initialization (требует default-constructible декораторов)

        payload_size_{0},
        header_size_{0},
        trailer_size_{0},
        total_size_{0},
        queue_{queue},
        memory_{0} // Пустой контейнер → memory_.data() == nullptr
    {
        // Декораторы не используются, так как в деструкторе есть проверка
        // if(memory_.begin())
    }

    explicit operator bool() const { return memory_.data() != nullptr; }

    virtual ~composite_serial_message()
    {
        if(memory_.begin())
        {
            setup_all_headers();

            setup_all_trailers(memory_.begin() + header_size_ + payload_size_,
                               memory_.begin(), total_size_);

            queue_.push(std::move(memory_));
        }
    }

    std::byte *pload() { return memory_.begin() + header_size_; }

    [[nodiscard]] const std::byte *pload() const
    {
        return memory_.begin() + header_size_;
    }

    [[nodiscard]] auto data() { return pload(); }

    [[nodiscard]] auto begin() { return memory_.begin(); }

    [[nodiscard]] auto begin() const { return memory_.begin(); }

    [[nodiscard]] auto end() { return memory_.end(); }

    [[nodiscard]] auto end() const { return memory_.end(); }

    [[nodiscard]] auto size() const { return memory_.size(); }

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

template<typename UserData, typename TQueueBaseType, typename... Decorators>
class serial_message
{
    using queue_base_type = TQueueBaseType;
    using user_type       = UserData;
    using composite_serial_message_type =
        stv::composite_serial_message<queue_base_type, Decorators...>;
    using span_type = typename composite_serial_message_type::span_type;

    composite_serial_message_type composite_message_;

  public:
    using value_type    = std::byte;
    using iterator_type = std::byte *;

    serial_message(
        queue_base_type &queue, const span_type &pload,
        Decorators... decorators):
        composite_message_{queue, pload,
                           std::forward<Decorators>(decorators)...}
    {
    }

    explicit serial_message(
        std::nullptr_t, queue_base_type &queue):
        composite_message_{nullptr, queue}
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

    [[nodiscard]] auto data() { return composite_message_.data(); }

    [[nodiscard]] auto begin() { return composite_message_.begin(); }

    [[nodiscard]] auto begin() const { return composite_message_.begin(); }

    [[nodiscard]] auto end() { return composite_message_.end(); }

    [[nodiscard]] auto end() const { return composite_message_.end(); }

    [[nodiscard]] auto size() const { return composite_message_.size(); }
};

template<typename TQueueBaseType, typename... Decorators>
class serial_message_buffer_base:
    private stv::non_copyable,
    private stv::non_movable
{
  public:
    using queue_base_type = TQueueBaseType;

  private:
    const std::tuple<Decorators...> decorators_;
    queue_base_type                &queue_;

  public:
    using span_type = std::span<const std::byte>;

    explicit serial_message_buffer_base(
        queue_base_type &queue, Decorators &&...decorators):
        decorators_{std::forward<Decorators>(decorators)...},
        queue_{queue}
    {
    }

    virtual ~serial_message_buffer_base() = default;

    template<typename UserData, typename... SetupParams>
    auto request_null(
        [[maybe_unused]] const SetupParams &...setup_params)
    {
        return serial_message<UserData, queue_base_type, Decorators...>(nullptr,
                                                                        queue_);
    }

    auto request_null()
    {
        return serial_message<std::byte, queue_base_type, Decorators...>(
            nullptr, queue_);
    }

    template<typename... SetupParams>
    auto request_null(
        [[maybe_unused]] std::size_t size,
        [[maybe_unused]] const SetupParams &...setup_params)
    {
        return serial_message<std::byte, queue_base_type, Decorators...>(
            nullptr, queue_);
    }

    template<stv::contiguous_trivial_container_concept Container>
    auto request_null(
        [[maybe_unused]] const Container &container)
    {
        using value_type = typename Container::value_type;
        // return request_impl<value_type>(std::as_bytes(std::span{container}));
        return serial_message<value_type, queue_base_type, Decorators...>(
            nullptr, queue_);
    }

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

    template<typename... SetupParams>
    auto request(
        std::size_t size, const SetupParams &...setup_params)
    {
        return request_impl<std::byte>(
            span_type(static_cast<const std::byte *>(nullptr), size),
            setup_params...);
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
        requires(!std::integral<UserData>)
    auto request(
        const UserData &user_data, const SetupParams &...setup_params)
    {
        return request_impl<UserData>(
            span_type(reinterpret_cast<const std::byte *>(&user_data),
                      sizeof(UserData)),
            setup_params...);
    }

    /// @brief Запрос области памяти под хранение структуры типа UserData.
    ///
    /// @tparam UserData Тип, под хранение которого запрашивается память.
    /// @tparam SetupParams Типы декораторов.
    ///
    /// @param[in] setup_params Декораторы для модификации запрошенной
    /// области памяти.
    ///
    /// @return Объект, который может содержать выделенную область памяти. После
    /// получения объекта, пользовательский код должен:
    /// - проверить через operator bool() что память успешно выделена.
    /// - в случае успешного выделения памяти, можно ее заполнить через оператор
    /// '->'.
    template<typename UserData, typename... SetupParams>
    auto request(
        const SetupParams &...setup_params)
    {
        return request_impl<UserData>(
            span_type(static_cast<const std::byte *>(nullptr),
                      sizeof(UserData)),
            setup_params...);
    }

    template<typename InputIt>
    auto request(
        InputIt cbegin, InputIt cend)
    {
        return request(std::as_bytes(std::span{cbegin, cend}));
    }

    auto queue_instance() -> decltype(queue_) & { return queue_; }

  private:
    /// @brief Выполняет запрос памяти из кучи под сообщение.
    ///
    /// @tparam UserData Тип данных, под который будет запрошена память. Обертка
    /// над request_impl должна гарантировать, что в случае типа который
    /// является не владеющим (std::string_view, std::span), pload.size_bytes()
    /// будет содержать актуальный размер.
    /// @tparam SetupParams Типы декораторов.
    ///
    /// @param[in] pload span, который может содержать указатель на область
    /// памяти и всегда должен содержать требуемый размер области памяти. Всегда
    /// будет запрошен размер памяти, указанный в pload.size_in_bytes(). Но,
    /// если pload.data() == nullptr, то пользователь должен самостоятельно
    /// заполнить область памяти, которую вернет request_impl(). Если
    /// pload.data() != nullptr, то в случае успешного выделения памяти данные
    /// автоматически будут скопированы из pload.data() в выделенную область
    /// памяти.
    /// @param[in] setup_params Модификаторы декораторов.
    ///
    /// @return Возвращает объект типа UserData. После получения объекта,
    /// пользовательский код должен:
    /// - проверить через operator bool() что память успешно выделена.
    /// - в случае успешного выделения памяти, можно ее заполнить через оператор
    /// '->'.
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
                return serial_message<UserData, queue_base_type, Decorators...>(
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

template<typename TSimbuff, std::size_t QUEUE_SIZE = 10, typename... Decorators>
class serial_message_buffer:
    public serial_message_buffer_base<etl::iqueue<TSimbuff>, Decorators...>
{
    using sim_buff_type   = TSimbuff;
    using queue_base_type = etl::iqueue<sim_buff_type>;
    using base_type =
        serial_message_buffer_base<queue_base_type, Decorators...>;

    etl::queue<sim_buff_type, 22> queue_;

  public:
    explicit serial_message_buffer(
        Decorators &&...decorators):
        base_type{queue_, std::forward<Decorators>(decorators)...}
    {
    }

    ~serial_message_buffer() override = default;
};

template<typename TSimBuff, std::size_t QUEUE_SIZE, typename... Decorators>
auto make_serial_message_buffer(
    Decorators &&...decorators)
{
    return serial_message_buffer<TSimBuff, QUEUE_SIZE, Decorators...>(
        std::forward<Decorators>(decorators)...);
}

} // namespace stv

#endif /* SERIAL_HPP */
