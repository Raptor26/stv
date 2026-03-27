/// @file serial_parser.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.

#ifndef SERIAL_PARSER_HPP
#define SERIAL_PARSER_HPP

#include "lwrb/lwrb.h"
#include "stv/communication//serial_decorators.hpp"
#include "stv/mutex_guard.hpp"
#include "stv/utils.hpp"
#include <cstddef>
#include <functional>
#include <type_traits>
#include <utility>

namespace stv {

/// @brief Параметры инициализации класса serial_parser.
///
/// @tparam TlwrbBase Тип кольцевого буфера который нужно распарсить и найти
/// сообщения.
/// @tparam TQueue Буфер, из которого можно запросить память под принятое
/// сообщение.
template<typename TlwrbBase, typename TQueue,
         typename TMutexOrPtr = stv::empty_mutex>
class serial_parser_setup
{
    // Определяем базовый тип мьютекса.
    using mutex_base_type = std::remove_pointer_t<TMutexOrPtr>;

    // Если передан указатель на мьютекс, то считаем что пользователь хочет
    // использовать внешний мьютекс.
    ///
    /// NOLINTNEXTLINE(bugprone-dynamic-static-initializers
    static constexpr bool is_external_mutex = std::is_pointer_v<TMutexOrPtr>;

    // Тип для хранения мьютекса. Либо указатель на мьютекс, либо пустой тип.
    using mutex_condition_type =
        std::conditional_t<is_external_mutex, mutex_base_type *,
                           std::monostate>;

  public:
    using lwrb_base_type = TlwrbBase;
    using queue_type     = TQueue;
    using mutex_type     = TMutexOrPtr;

    /// @brief Кольцевой буфер предоставляющий поток байт, из которого парсер
    /// должен выделить сообщение.
    lwrb_base_type *lwrb{nullptr};

    /// @brief Указатель на очередь, в которую помещаются выделенные из потока
    /// байт сообщения.
    queue_type *queue{nullptr};

    /// @brief МАксимальный размер одного сообщения, который парсер будет
    /// считать допустимым.
    std::size_t max_one_message_size{0};

    /// @brief Если указан внешний мьютекс, то mutex будет указателем на тип
    /// TMutex, в противном случае тип будет пустым.
    mutex_condition_type mutex{};
};

/// @brief Парсер сообщений. Анализирует поток байт в кольцевом буфере и находит
/// в нем сообщения. Каждое найденное сообщение копирует в очередь.
///
/// @tparam TSetup Тип структуры инициализации.
///     @see serial_parser_setup
/// @tparam Decorators Декораторы (возможно стоит удалить).
template<typename TSetup, typename... Decorators>
class serial_parser: virtual private stv::non_movable_non_copyable
{
  public:
    using setup_type = TSetup;

  private:
    using lwrb_base_type = typename setup_type::lwrb_base_type;
    using queue_type     = typename setup_type::queue_type;
    using container_type = typename queue_type::value_type;
    using mutex_type     = typename TSetup::mutex_type;

    /// @brief Определение типа данных для метода-состояния парсера.
    /// @warning Тип сообщения должен вернуть true если требуется продолжение
    /// обработки.
    using state_fnc_type =
        bool (stv::serial_parser<TSetup, Decorators...>::*)();

    enum class states {
        /// @brief Состояние поиска начала кадра, а также размера остального
        /// сообщения.
        start_frame_and_size,

        /// @brief Состояние ожидания появления в буфере всего сообщения (ведь в
        /// состоянии start_frame_and_size мы выяснили размер всего сообщения).
        wait_message_ready,

        max,
    };

    /// @brief Текущее состояние парсера.
    states state_{states::start_frame_and_size};

    /// @brief Тип хэш таблица, которая хранит указатели на метода каждого
    /// состояния.
    using hash_type =
        std::array<state_fnc_type, std::to_underlying(states::max)>;

    /// @brief Кольцевой буфер предоставляющий поток байт, из которого парсер
    /// должен выделить сообщение.
    lwrb_base_type *lwrb_;

    /// @brief Указатель на очередь, в которую помещаются выделенные из потока
    /// байт сообщения.
    queue_type *queue_;

    /// @brief МАксимальный размер одного сообщения, который парсер будет
    /// считать допустимым.
    const std::size_t max_one_message_size_;

    /// @brief Счетчик полученных сообщений.
    std::size_t parsed_cnt_{};

    /// @brief Размер сообщения, которое нужно распарсить в
    /// wait_message_ready_state.
    std::size_t next_message_size_{0};

    //
    static constexpr auto construct_states_hash()
    {
        hash_type hash{};
        hash[std::to_underlying(states::start_frame_and_size)] =
            &stv::serial_parser<TSetup,
                                Decorators...>::start_frame_and_size_state;

        hash[std::to_underlying(states::wait_message_ready)] =
            &stv::serial_parser<TSetup,
                                Decorators...>::wait_message_ready_state;

        return hash;
    }

    /// @brief Хэщ таблица указателей на метода каждого поддерживаемого
    /// состояния парсера.
    ///
    /// NOLINTNEXTLINE(bugprone-dynamic-static-initializers
    static constexpr hash_type state_fnc_hash{construct_states_hash()};
    /// @brief Мьютекс.
    mutable mutex_type mutex_;

    auto get_mutex_ref() const -> std::remove_pointer_t<decltype(mutex_)> &
    {
        if constexpr(std::is_pointer_v<decltype(mutex_)>)
        {
            assert(mutex_ != nullptr);
            return *mutex_;
        }
        else
        {
            return mutex_;
        }
    }

    void set_next_message_size(
        std::size_t next_message_size)
    {
        const stv::lock_guard critical{get_mutex_ref()};
        next_message_size_ = next_message_size;
    }

  public:
    explicit serial_parser(
        const setup_type &setup):
        lwrb_{setup.lwrb},
        queue_{setup.queue},
        max_one_message_size_{
            (setup.max_one_message_size
             == 0) ///< значение по умолчанию
                   // NOLINTNEXTLINE(*-avoid-nested-conditional-operator)
                ? ((lwrb_ != nullptr) ? lwrb_->capacity() : 0)
                : (setup.max_one_message_size)}
    {
        if constexpr(std::is_pointer_v<decltype(mutex_)>)
        {
            mutex_ = setup.mutex;
        }
    }

    virtual ~serial_parser() = default;

    explicit operator bool() const
    {
        auto is_mutex_valid{true};

        if constexpr(std::is_pointer_v<decltype(mutex_)>)
        {
            if(!mutex_)
            {
                is_mutex_valid = false;
            }
        }

        return stv::all_true(lwrb_, queue_, is_mutex_valid,
                             max_one_message_size_ > 0);
    }

    /// @brief Метод анализирует поток байт. Если найдено сообщение, то вернет
    /// true.
    ///
    /// @return true если найдено сообщение и помещено в очередь, false в
    /// противном случае.
    auto run()
    {
        const auto parsed_cnt = parsed_cnt_;

        while(std::invoke(
            state_fnc_hash.at(static_cast<std::size_t>(get_state())), this))
        {
        }
        return parsed_cnt != parsed_cnt_;
    }

    /// @brief Возвращает ссылку на очередь, в которую помещаются распарсенные
    /// сообщения.
    ///
    /// @return Ссылка на очередь, которая может содержать распарсенные
    /// сообщения.
    auto queue_instance() -> std::remove_pointer_t<decltype(queue_)> &
    { return *queue_; }

  private:
    auto set_state(
        states new_state)
    {
        auto is_state_set{false};

        if(new_state < states::max)
        {
            is_state_set = true;
            const stv::lock_guard critical{get_mutex_ref()};
            state_ = new_state;
        }

        return is_state_set;
    }

    auto get_state()
    {
        const stv::lock_guard critical{get_mutex_ref()};
        return state_;
    }

    /// @brief Состояние поиска начала кадра и размера всего сообщения.
    ///
    /// @note Размер всего сообщения (кроме старка кадра) следует сразу за
    /// символами начала кадра.
    ///
    /// @return true если нужно продолжить обработку в текущем цикле, false в
    /// противном случае.
    auto start_frame_and_size_state()
    {
        bool                  is_need_continue{false};

        constexpr std::size_t need_bytes_available_befor_start{
            stv::start_frame_and_crc_16::header_size()};

        while(lwrb_->get_full() >= need_bytes_available_befor_start)
        {
            stv::start_frame_and_crc_16::start_frame_t storage{};

            {
                lwrb_->peek(typename lwrb_base_type::container_type{
                    reinterpret_cast<std::byte *>(&storage), sizeof(storage)});
            }

            std::size_t skip_cnt{1};
            if((storage.start_frame_first
                == stv::start_frame_and_crc_16::first_byte)
               && (storage.start_frame_second
                   == stv::start_frame_and_crc_16::second_byte))
            {
                set_state(states::wait_message_ready);
                set_next_message_size(storage.frame_size);
                is_need_continue = true;
                skip_cnt         = 0;
            }

            // Нужно пометить считанные байты как прочитанные. Если
            // заголовок успешно считан, то весь заголовок будет помечен как
            // считанный, в противном случае помечается только 1 байт чтобы
            // продолжить чтение внутри while() со следующего байта.
            lwrb_->skip(skip_cnt);

            if(is_need_continue)
            {
                break;
            }
        }

        return is_need_continue;
    }

    auto wait_message_ready_state()
    {
        bool       is_need_continue{false};
        const auto expect_total_message_size{
            stv::start_frame_and_crc_16::header_size() + next_message_size_};

        // Если ожидается сообщение больше чем емкость буфера, то что-то пошло
        // не так, начнем поиск сообщения снова.
        if(expect_total_message_size > max_one_message_size_)
        {
            // если start_frame_and_size_state() нашел начало фрейма, то он не
            // удаляет эти байты из буфера. Поэтому, если обнаружена ошибка, то
            // принудительно удалим из буфера начало фрейма чтобы на следующей
            // итерации парсера не попасть в туже ловушку.
            lwrb_->skip(sizeof(stv::start_frame_and_crc_16::start_frame_t));
            set_state(states::start_frame_and_size);
            return false;
        }

        container_type msg{expect_total_message_size};

        constexpr auto read_all_or_nothing{true};
        constexpr auto is_isr{false};
        const auto read_bytes = lwrb_->read(msg, read_all_or_nothing, is_isr);

        if(read_bytes == expect_total_message_size)
        {
            if(stv::start_frame_and_crc_16::is_crc_valid(
                   stv::total_message_span{msg.begin(), msg.size()}))
            {
                msg.trim_head(stv::start_frame_and_crc_16::header_size());
                msg.trim_tail(stv::start_frame_and_crc_16::trailer_size());

                queue_->push(std::move(msg));
                ++parsed_cnt_;
            }

            set_state(states::start_frame_and_size);
            set_next_message_size(next_message_size_);
        }

        if(lwrb_->get_full()
           >= sizeof(stv::start_frame_and_crc_16::start_frame_t))
        {
            is_need_continue = true;
        }

        return is_need_continue;
    }
};

template<typename TSetup, typename... Decorators>
auto make_serial_parser(
    Decorators &&...decorators)
{
    return serial_parser<TSetup, Decorators...>(
        std::forward<Decorators>(decorators)...);
}

// -----------------------------------------------------------------------------

template<typename TQueue, typename THash,
         typename TMutexOrPtr = stv::empty_mutex>
class serial_route_setup
{
    // Определяем базовый тип мьютекса.
    using mutex_base_type = std::remove_pointer_t<TMutexOrPtr>;

    // Если передан указатель на мьютекс, то считаем что пользователь хочет
    // использовать внешний мьютекс.
    ///
    /// NOLINTNEXTLINE(bugprone-dynamic-static-initializers
    static constexpr bool is_external_mutex = std::is_pointer_v<TMutexOrPtr>;

    // Тип для хранения мьютекса. Либо указатель на мьютекс, либо пустой тип.
    using mutex_condition_type =
        std::conditional_t<is_external_mutex, mutex_base_type *,
                           std::monostate>;

  public:
    using queue_type = TQueue;
    using hash_type  = THash;
    using mutex_type = TMutexOrPtr;

    /// @brief Указатель на очередь, из которой выполняется чтение сообщений для
    /// их дальнейшей маршрутизации.
    queue_type *queue_to_read{nullptr};

    /// @brief Указатель на хэш-таблицу, которая содержит очереди для записи
    /// сообщения согласно указанному маршруту.
    hash_type *hash_to_write{nullptr};

    mutex_type mutex{};
};

template<typename TSetup>
class serial_parser_route: virtual public stv::non_movable_non_copyable
{
    using setup_type = TSetup;
    using queue_type = typename setup_type::queue_type;
    using hash_type  = typename setup_type::hash_type;

    /// @brief Из данной очереди считываются сообщения и перемещаются к
    /// получателю, которые указаны в hash_to_write_.
    queue_type *const queue_to_read_{nullptr};

    /// @brief Указатель на хэш-таблицу, которая содержит очереди для записи
    /// сообщения согласно указанному маршруту.
    hash_type *const hash_to_write_{nullptr};

  public:
    explicit serial_parser_route(
        const setup_type &setup):
        queue_to_read_{setup.queue_to_read},
        hash_to_write_{setup.hash_to_write}
    { (void)setup; }

    virtual ~serial_parser_route() = default;

    explicit operator bool() const
    { return stv::all_true(queue_to_read_, hash_to_write_); }

    ///
    auto run()
    {
        auto message_routed_cnt{0U};
        while(!queue_to_read_->empty())
        {
            decltype(auto) msg = queue_to_read_->front();

            const auto    *router_ptr = reinterpret_cast<
                const stv::head_route::head_route_setup_with_pload_t *>(
                msg.data());

            // Используем итератор чтобы избежать выброса исключений.
            auto dst_buff_key_val_it = hash_to_write_->find(
                static_cast<hash_type::key_type>(router_ptr->dst_id));
            if(dst_buff_key_val_it != hash_to_write_->end())
            {
                dst_buff_key_val_it->second->push(std::move(msg));
                ++message_routed_cnt;
            }

            // Независимо от того удалось переместить сообщение в очередь
            // получателя или нет, нужно удалить сообщение из текущей очереди
            // чтобы избежать ее переполнения.
            queue_to_read_->pop();
        }

        return message_routed_cnt;
    }
};

} // namespace stv

#endif /* SERIAL_PARSER_HPP */
