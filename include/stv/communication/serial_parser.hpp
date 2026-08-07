/// @file serial_parser.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
///
/// NAME
///     stv::serial_parser -- парсер серийных сообщений.
///
/// DESCRIPTION
///     serial_parser выделяет сообщения из потока
///     байт кольцевого буфера и помещает готовые
///     пакеты в выходную очередь, соответствующую
///     формату кадра. Маршрутизация распарсенных
///     пакетов stvlink выполняется маршрутизатором
///     stv::stvlink_route (см. stvlink_route.hpp).
///
///     serial_parser_setup<TlwrbBase, TQueue,
///         TMutexOrPtr, Decorators...>
///         Параметры инициализации парсера.
///         Задают указатель на кольцевой буфер lwrb,
///         максимальный размер одного сообщения
///         max_one_message_size, мьютекс и набор
///         декораторов кадров. Каждому декоратору
///         через parsed_queue<Декоратор> сопоставляется
///         своя выходная очередь. Порядок указания
///         декораторов и очередей не важен.
///
///     serial_parser<TSetup, Decorators...>
///         Конечный автомат из двух состояний:
///         поиск начала кадра и поля размера,
///         ожидание полного кадра и проверка CRC.
///         Декоратор выбирается по стартовой
///         последовательности; далее все проверки
///         размера, CRC и обрезка служебных полей
///         выполняются через выбранный декоратор.
///         Метод run() выполняет один цикл парсинга
///         и возвращает true, если хотя бы одно
///         сообщение было успешно обработано.
///
///     make_serial_parser<TSetup, Decorators...>(
///         setup)
///         Фабричная функция для удобного создания
///         парсера.
///
/// EXAMPLE
///     Пример приема и маршрутизации сообщения:
///     ```cpp
///     #include "stv/communication/serial_parser.hpp"
///     #include "stv/communication/serial_sender.hpp"
///     #include "stv/communication/stvlink_frame.hpp"
///     #include "stv/communication/stvlink_route.hpp"
///     #include "stv/communication/stvlink_sender.hpp"
///     #include "stv/containers/lwrb.hpp"
///     #include "stv/containers/simbuff.hpp"
///     #include "etl/queue.h"
///     #include "etl/unordered_map.h"
///     #include <iostream>
///
///     int main() {
///         using namespace stv;
///
///         using sim_buffer_type = stv::sim_buff<stv::empty_mutex>;
///         using queue_type      = etl::queue<sim_buffer_type, 10>;
///         using lwrb_setup_type = stv::lwrb_setup<stv::empty_mutex>;
///         using lwrb_base_type  = stv::lwrb_base<lwrb_setup_type>;
///
///         queue_type parsed_msg_queue;
///         queue_type serial_msg_queue;
///         stv::lwrb<lwrb_base_type, 128> lwrb{lwrb_setup_type{}};
///
///         auto serial_message_buffer = make_serial_message_buffer(
///             serial_msg_queue,
///             stv::stvlink_frame_tx{});
///
///         constexpr std::string_view payload{"Hello world"};
///         {
///             auto msg = serial_message_buffer.request(payload);
///         }
///
///         auto &tx_queue = serial_message_buffer.queue_instance();
///         auto frame = tx_queue.front();
///         lwrb.write(frame.begin(), frame.end());
///         tx_queue.pop();
///
///         using parser_setup_type =
///             stv::serial_parser_setup<lwrb_base_type, queue_type,
///                                      stv::stvlink_frame>;
///         parser_setup_type parser_setup;
///         parser_setup.lwrb = &lwrb;
///         parser_setup.set_queue<stv::stvlink_frame>(
///             parsed_msg_queue);
///
///         auto parser = stv::make_serial_parser<
///             parser_setup_type, stv::stvlink_frame>(parser_setup);
///         if (!parser) {
///             std::cerr << "Invalid parser setup\n";
///             return 1;
///         }
///
///         if (parser.run()) {
///             std::cout << "Parsed messages: "
///                       << parsed_msg_queue.size() << "\n";
///         }
///
///         using hash_type = etl::iunordered_map<int, queue_type *>;
///         etl::unordered_map<int, queue_type *, 10> hash_table;
///         hash_table.insert({1, &parsed_msg_queue});
///
///         using route_setup_type =
///             stv::stvlink_route_setup<queue_type, hash_type>;
///         route_setup_type route_setup;
///         route_setup.queue_to_read = &parsed_msg_queue;
///         route_setup.hash_to_write = &hash_table;
///
///         stv::stvlink_route<route_setup_type> router(route_setup);
///         if (router.run()) {
///             std::cout << "Message routed to destination\n";
///         }
///
///         return 0;
///     }
///     ```
///
///     Подробные сценарии использования и проверки
///     корректности приведены в
///     test_serial_parser.cpp.
///
/// SEE ALSO
///     parsed_queue.hpp, serial_decorators.hpp,
///     serial_sender.hpp, stvlink_route.hpp,
///     test_serial_parser.cpp.

#ifndef SERIAL_PARSER_HPP
#define SERIAL_PARSER_HPP

#include "lwrb/lwrb.h"
#include "stv/communication/parsed_queue.hpp"
#include "stv/communication/serial_decorators.hpp"
#include "stv/mutex_guard.hpp"
#include "stv/utils.hpp"
#include <algorithm>
#include <array>
#include <cstddef>
#include <functional>
#include <tuple>
#include <type_traits>
#include <utility>

namespace stv {

/// @brief Параметры инициализации парсера сообщений.
///
/// @details
/// Структура задает источник байт (кольцевой буфер), приемники готовых
/// сообщений (по одной очереди на каждый декоратор кадра), максимальный
/// размер одного сообщения и, при необходимости, мьютекс для защиты
/// внутреннего состояния парсера. Если @c TMutexOrPtr является указателем
/// на мьютекс, парсер использует внешний мьютекс; в противном случае
/// синхронизация отсутствует (заглушка @ref stv::empty_mutex).
///
/// @tparam TlwrbBase Тип кольцевого буфера, предоставляющего поток байт.
///     Должен быть совместим с @ref stv::lwrb_base.
/// @tparam TQueue Общий тип очередей для готовых сообщений. Её value_type
///     должен предоставлять непрерывный буфер байт.
/// @tparam TMutexOrPtr Тип мьютекса либо указатель на него.
///     По умолчанию используется @ref stv::empty_mutex.
/// @tparam Decorators Набор декораторов кадра. Каждому декоратору в
///     структуре соответствует одна очередь @ref parsed_queue.
template<typename TlwrbBase, typename TQueue, typename TMutexOrPtr,
         typename... Decorators>
class serial_parser_setup
{
    /// @brief Базовый тип мьютекса (без указателя).
    using mutex_base_type = std::remove_pointer_t<TMutexOrPtr>;

    /// @brief true, если пользователь передал указатель на внешний мьютекс.
    static constexpr bool is_external_mutex = std::is_pointer_v<TMutexOrPtr>;

    /// @brief Тип хранения мьютекса в поле @c mutex.
    using mutex_condition_type =
        std::conditional_t<is_external_mutex, mutex_base_type *,
                           std::monostate>;

    /// @brief Проверяет, что @c D входит в пакет декораторов.
    template<typename D>
    static constexpr bool is_decorator = (std::is_same_v<D, Decorators> || ...);

  public:
    /// @brief Тип кольцевого буфера.
    using lwrb_base_type = TlwrbBase;

    /// @brief Тип очередей для готовых сообщений.
    using queue_type = TQueue;

    /// @brief Тип мьютекса или указателя на него.
    using mutex_type = TMutexOrPtr;

    /// @brief Пакет декораторов кадра в виде кортежа типов.
    using decorator_types = std::tuple<Decorators...>;

    /// @brief Указатель на кольцевой буфер с входящим потоком байт.
    lwrb_base_type *lwrb{nullptr};

    /// @brief Максимальный допустимый размер одного сообщения в байтах.
    ///
    /// @details
    /// Задает верхнюю границу общего размера кадра (заголовок + полезная
    /// нагрузка + хвост). Если передать 0, конструктор @ref serial_parser
    /// заменит это значение на емкость кольцевого буфера.
    std::size_t max_one_message_size{0};

    /// @brief Внешний мьютекс либо пустой объект.
    ///
    /// @details
    /// При использовании внешнего мьютекса следует передать его адрес.
    /// При использовании @ref stv::empty_mutex поле остается пустым.
    mutex_condition_type mutex{};

    /// @brief Устанавливает очередь для декоратора @c Decorator.
    ///
    /// @tparam Decorator Декоратор кадра, для которого устанавливается
    ///     очередь.
    /// @param[in,out] queue Очередь, в которую будут помещаться распарсенные
    ///     кадры, соответствующие декоратору @c Decorator.
    template<typename Decorator>
    void set_queue(
        queue_type &queue)
    {
        static_assert(is_decorator<Decorator>,
                      "Decorator is not registered in this setup");
        std::get<parsed_queue<Decorator, queue_type>>(queues_) =
            parsed_queue<Decorator, queue_type>{queue};
    }

    /// @brief Устанавливает указатель на очередь для декоратора @c Decorator.
    ///
    /// @tparam Decorator Декоратор кадра, для которого устанавливается
    ///     очередь.
    /// @param[in] queue Указатель на очередь. Может быть @c nullptr.
    template<typename Decorator>
    void set_queue(
        queue_type *queue)
    {
        static_assert(is_decorator<Decorator>,
                      "Decorator is not registered in this setup");
        std::get<parsed_queue<Decorator, queue_type>>(queues_) =
            parsed_queue<Decorator, queue_type>{queue};
    }

    /// @brief Возвращает связку очереди с декоратором @c Decorator.
    ///
    /// @tparam Decorator Декоратор кадра.
    /// @return Ссылка на @ref parsed_queue<Decorator, queue_type>.
    template<typename Decorator>
    auto queue() -> parsed_queue<Decorator, queue_type> &
    {
        static_assert(is_decorator<Decorator>,
                      "Decorator is not registered in this setup");
        return std::get<parsed_queue<Decorator, queue_type>>(queues_);
    }

    /// @brief Возвращает кортеж связок очередей со всеми декораторами.
    ///
    /// @return Константная ссылка на кортеж @ref parsed_queue.
    [[nodiscard]] auto queues() const
        -> const std::tuple<parsed_queue<Decorators, queue_type>...> &
    { return queues_; }

  private:
    /// @brief Кортеж типизированных связок очередей с декораторами.
    std::tuple<parsed_queue<Decorators, queue_type>...> queues_{};
};

/// @brief Парсер сообщений из потока байт кольцевого буфера.
///
/// @details
/// Класс реализует конечный автомат из двух состояний:
///   - поиск стартового кадра и размера сообщения;
///   - ожидание появления в буфере полного кадра и проверка CRC.
///
/// Ожидаемый формат кадра определяется переданным пакетом декораторов
/// @c Decorators. Парсер выбирает декоратор по стартовой последовательности,
/// после чего использует его правила для проверки размера, CRC и обрезки
/// служебных полей. Готовая полезная нагрузка (вместе с внутренним
/// заголовком маршрутизации, если он есть) помещается в очередь,
/// сопоставленную выбранному декоратору.
///
/// Парсер должен вызываться из одного потока/контекста. Если передан
/// мьютекс, он защищает отдельные чтения и записи внутреннего состояния
/// конечного автомата от коротких ISR-style обращений, но не делает
/// метод run() полностью реентерабельным. Для извлечения нескольких
/// сообщений за один вызов метод run() обрабатывает состояния в цикле.
///
/// @tparam TSetup Тип параметров инициализации, например
///     @ref serial_parser_setup.
/// @tparam Decorators Пакет декораторов кадра. Должен совпадать с пакетом
///     декораторов, указанным в @c TSetup.
template<typename TSetup, typename... Decorators>
class serial_parser: virtual private stv::non_movable_non_copyable
{
  public:
    /// @brief Тип параметров инициализации.
    using setup_type = TSetup;

  private:
    using lwrb_base_type = typename setup_type::lwrb_base_type;
    using queue_type     = typename setup_type::queue_type;
    using container_type = typename queue_type::value_type;
    using mutex_type     = typename TSetup::mutex_type;
    using decorators_t   = std::tuple<Decorators...>;

    /// @brief true, если тип @c T входит в пакет @c Us.
    template<typename T, typename... Us>
    static constexpr bool is_one_of_v = (std::is_same_v<T, Us> || ...);

    /// @brief true, если два кортежа типов содержат один и тот же набор
    ///     типов (без учёта порядка и дубликатов).
    template<typename Tuple1, typename Tuple2>
    static constexpr bool is_same_set_v = false;

    /// @brief Специализация is_same_set_v для двух кортежей типов.
    template<typename... Ts1, typename... Ts2>
    static constexpr bool
        is_same_set_v<std::tuple<Ts1...>, std::tuple<Ts2...>> =
            (sizeof...(Ts1) == sizeof...(Ts2))
            && (is_one_of_v<Ts1, Ts2...> && ...)
            && (is_one_of_v<Ts2, Ts1...> && ...);

    static_assert(
        is_same_set_v<typename setup_type::decorator_types, decorators_t>,
        "Decorator tags in setup must match the parser template arguments");
    static_assert(sizeof...(Decorators) > 0,
                  "at least one frame decorator is required");

    /// @brief Число декораторов в пакете.
    static constexpr std::size_t decorators_count{sizeof...(Decorators)};

    /// @brief Тип указателя на метод-обработчик состояния парсера.
    ///
    /// @warning Метод состояния должен возвращать @c true, если нужно
    ///     продолжить обработку в текущем цикле run().
    using state_fnc_type =
        bool (stv::serial_parser<TSetup, Decorators...>::*)();

    /// @brief Состояния конечного автомата парсера.
    enum class states {
        /// @brief Поиск начала кадра и размера сообщения.
        start_frame_and_size,

        /// @brief Ожидание полного кадра в буфере.
        wait_message_ready,

        /// @brief Количество состояний (используется для размеров массивов).
        max,
    };

    /// @brief Текущее состояние парсера.
    states state_{states::start_frame_and_size};

    /// @brief Таблица указателей на методы-обработчики состояний.
    using hash_type =
        std::array<state_fnc_type, std::to_underlying(states::max)>;

    /// @brief Кольцевой буфер с входящим потоком байт.
    lwrb_base_type *lwrb_{nullptr};

    /// @brief Очереди для готовых сообщений (по одной на декоратор).
    std::array<queue_type *, decorators_count> queues_{};

    /// @brief Максимальный допустимый размер одного сообщения.
    const std::size_t max_one_message_size_{0};

    /// @brief Счетчик успешно распарсенных сообщений.
    std::size_t parsed_cnt_{0U};

    /// @brief Размер оставшейся части кадра (после заголовка), найденный в
    /// состоянии @c start_frame_and_size.
    std::size_t next_message_size_{0U};

    /// @brief Индекс декоратора, выбранного в состоянии
    /// @c start_frame_and_size. Значение @c decorators_count означает, что
    /// декоратор еще не выбран.
    std::size_t selected_decorator_idx_{decorators_count};

    /// @brief Счетчик подряд идущих опросов неполного кадра без новых
    /// байт в состоянии @c wait_message_ready.
    std::size_t wait_message_ready_polls_{0U};

    /// @brief Число байт в буфере на момент последнего опроса неполного
    /// кадра. Изменение значения означает приход новых байт и сбрасывает
    /// счетчик @c wait_message_ready_polls_.
    std::size_t wait_message_ready_bytes_seen_{0U};

    /// @brief Заполняет таблицу указателей на обработчики состояний.
    ///
    /// @return Массив с указателями на методы состояний.
    static consteval auto construct_states_hash()
    {
        hash_type hash{};
        // Индексы — значения enum states, ограничены размером таблицы.
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
        hash[std::to_underlying(states::start_frame_and_size)] =
            &stv::serial_parser<TSetup,
                                Decorators...>::start_frame_and_size_state;

        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
        hash[std::to_underlying(states::wait_message_ready)] =
            &stv::serial_parser<TSetup,
                                Decorators...>::wait_message_ready_state;

        return hash;
    }

    /// @brief Таблица указателей на обработчики состояний.
    ///
    /// NOLINTNEXTLINE(bugprone-dynamic-static-initializers)
    static constexpr hash_type state_fnc_hash{construct_states_hash()};

    /// @brief Мьютекс или указатель на него.
    mutable mutex_type mutex_;

    /// @brief Возвращает ссылку на реальный мьютекс.
    ///
    /// @details
    /// Если @c mutex_ является указателем, разыменовывает его; иначе
    /// возвращает сам объект. Используется для единообразной работы
    /// с @ref stv::lock_guard.
    ///
    /// @return Ссылка на мьютекс.
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

    /// @brief Описание выбранного кадра: индекс декоратора и размер
    ///     оставшейся части кадра.
    struct frame_selection {
        /// @brief Индекс выбранного декоратора.
        std::size_t decorator_idx;

        /// @brief Размер оставшейся части кадра.
        std::size_t remaining_size;
    };

    /// @brief Устанавливает выбранный декоратор и размер оставшегося кадра.
    ///
    /// @param[in] selection Структура с индексом декоратора и размером
    ///     оставшейся части кадра.
    void set_frame_selection(
        frame_selection selection)
    {
        const stv::lock_guard critical{get_mutex_ref()};
        selected_decorator_idx_ = selection.decorator_idx;
        next_message_size_      = selection.remaining_size;
    }

    /// @brief Сбрасывает таймаут ожидания неполного кадра.
    ///
    /// @details
    /// Вызывается при выходе из состояния @c wait_message_ready и при
    /// сборке кадра целиком, чтобы новый цикл ожидания начинал отсчет
    /// с нуля.
    void reset_wait_message_ready_timeout()
    {
        wait_message_ready_polls_      = 0U;
        wait_message_ready_bytes_seen_ = 0U;
    }

    /// @brief Ведет таймаут ожидания неполного кадра.
    ///
    /// @details
    /// Вызывается из состояния @c wait_message_ready, когда кадр еще не
    /// собран целиком. Приход новых байт сбрасывает отсчет: медленный,
    /// но живой отправитель таймаут не вызывает. Если за
    /// @c max_wait_message_ready_polls подряд идущих опросов новых байт
    /// не появилось, заголовок отбрасывается и парсер возвращается к
    /// поиску начала кадра, как и при ошибке CRC.
    ///
    /// @param[in] header_size Размер заголовка выбранного декоратора.
    void track_incomplete_frame_timeout(
        std::size_t header_size)
    {
        const auto bytes_available = lwrb_->get_full();

        if(bytes_available != wait_message_ready_bytes_seen_)
        {
            // Пришли новые байты: отправитель живой, отсчет таймаута
            // начинается заново.
            wait_message_ready_bytes_seen_ = bytes_available;
            wait_message_ready_polls_      = 0U;
            return;
        }

        if(++wait_message_ready_polls_ < max_wait_message_ready_polls)
        {
            return;
        }

        // Отправитель оборвал передачу посреди кадра (либо заголовок
        // ложный).
        lwrb_->skip(header_size);
        set_state(states::start_frame_and_size);
        set_frame_selection(frame_selection{decorators_count, 0});
        reset_wait_message_ready_timeout();
    }

    /// @brief Вычисляет максимальный размер одного сообщения.
    ///
    /// @details
    /// Если @c setup.max_one_message_size равен 0, используется емкость
    /// кольцевого буфера @c setup.lwrb. В противном случае используется
    /// значение из @c setup.
    ///
    /// @param[in] setup Структура с указателями на буфер и максимальным
    ///     размером сообщения.
    ///
    /// @return Максимальный допустимый размер одного сообщения.
    static std::size_t calculate_max_one_message_size(
        const setup_type &setup)
    {
        if(setup.max_one_message_size != 0)
        {
            return setup.max_one_message_size;
        }

        if(setup.lwrb != nullptr)
        {
            return setup.lwrb->capacity();
        }

        return 0;
    }

    /// @brief Возвращает максимальный размер заголовка среди всех
    /// декораторов.
    ///
    /// @return Максимальный размер заголовка в байтах.
    static constexpr std::size_t max_header_size()
    { return std::max({Decorators::header_size()...}); }

    /// @brief Возвращает индекс декоратора @c D в пакете.
    ///
    /// @tparam D Искомый декоратор.
    /// @return Индекс декоратора или @c decorators_count, если декоратор
    ///     не найден.
    template<typename D>
    static constexpr auto index_of_decorator()
    {
        constexpr std::array<bool, decorators_count> matches{
            std::is_same_v<D, Decorators>...};

        std::size_t index{0};
        for(const bool is_match: matches)
        {
            if(is_match)
            {
                return index;
            }
            ++index;
        }

        return decorators_count;
    }

    /// @brief Диспетчеризует вызов по сохраненному индексу декоратора.
    ///
    /// @details
    /// Вызывает переданный функциональный объект ровно один раз, передавая
    /// ему @c std::integral_constant<std::size_t, I>, где @c I совпадает с
    /// @c idx. Если @c idx вне диапазона, функциональный объект не
    /// вызывается.
    ///
    /// @tparam Func Тип функционального объекта.
    /// @param[in] idx Индекс декоратора.
    /// @param[in] func Функциональный объект, принимающий
    ///     @c std::integral_constant<std::size_t, I>.
    template<typename Func>
    static void dispatch_by_index(
        std::size_t idx, Func &&func)
    {
        dispatch_by_index(idx, std::forward<Func>(func),
                          std::make_index_sequence<decorators_count>{});
    }

    /// @brief Реализация dispatch_by_index с раскрытием индексов.
    ///
    /// @tparam Func Тип функционального объекта.
    /// @tparam Is Последовательность индексов декораторов.
    /// @param[in] idx Индекс декоратора.
    /// @param[in] func Функциональный объект.
    template<typename Func, std::size_t... Is>
    static void dispatch_by_index(
        std::size_t idx, Func &&func, std::index_sequence<Is...> /*indexes*/)
    {
        bool invoked{false};
        auto invoke_once = [&](auto index_constant) {
            if(!invoked)
            {
                std::invoke(std::forward<Func>(func), index_constant);
                invoked = true;
            }
        };

        (void)(((idx == Is)
                && (invoke_once(std::integral_constant<std::size_t, Is>{}),
                    true))
               || ...);
    }

  private:
    /// @brief Формирует массив указателей на очереди из кортежа
    ///     типизированных связок @ref parsed_queue.
    ///
    /// @tparam Is Последовательность индексов декораторов.
    /// @param[in] queues Кортеж связок очередей с декораторами.
    /// @return Массив указателей на очереди в том же порядке, что и
    ///     декораторы в пакете.
    template<typename TQueueTuple, std::size_t... Is>
    static auto make_queues_pointers(
        const TQueueTuple &queues, std::index_sequence<Is...> /*indexes*/)
        -> std::array<queue_type *, decorators_count>
    {
        return std::array<queue_type *,
                          decorators_count>{static_cast<queue_type *>(
            std::get<parsed_queue<Decorators, queue_type>>(queues).queue())...};
    }

  public:
    /// @brief Максимальное число подряд идущих опросов неполного кадра,
    /// после которого стартовый кадр отбрасывается.
    ///
    /// @details
    /// Счетчик опросов ведется только пока в буфере не появляются новые
    /// байты: приход даже одного байта сбрасывает отсчет, поэтому таймаут
    /// означает именно обрыв передачи, а не медленного, но живого
    /// отправителя. Значение 64 выбрано как запас к типичному периоду
    /// опроса парсера из задачи приема: при опросе каждую 1 мс таймаут
    /// соответствует ~64 мс тишины в линии, что заведомо больше
    /// межбайтового интервала на поддерживаемых скоростях UART
    /// (на 9600 бод байт приходит каждые ~1 мс).
    static constexpr std::size_t max_wait_message_ready_polls{64U};

    /// @brief Конструирует парсер на основе параметров инициализации.
    ///
    /// @details
    /// Если @c setup.max_one_message_size равен 0, используется емкость
    /// кольцевого буфера. При использовании внешнего мьютекса сохраняется
    /// указатель на него из @c setup.mutex. Указатели на очереди
    /// извлекаются из типизированных обёрток @ref parsed_queue.
    ///
    /// @param[in] setup Структура с указателями на буфер, очередями,
    ///     максимальным размером сообщения и мьютексом.
    explicit serial_parser(
        const setup_type &setup):
        lwrb_{setup.lwrb},
        queues_{make_queues_pointers(
            setup.queues(), std::make_index_sequence<decorators_count>{})},
        max_one_message_size_{calculate_max_one_message_size(setup)}
    {
        if constexpr(std::is_pointer_v<decltype(mutex_)>)
        {
            mutex_ = setup.mutex;
        }
    }

    /// @brief Деструктор по умолчанию.
    virtual ~serial_parser() = default;

    /// @brief Проверяет корректность инициализации парсера.
    ///
    /// @details
    /// Возвращает @c true, если установлены корректные указатели на
    /// кольцевой буфер и все очереди, максимальный размер сообщения больше 0,
    /// а при использовании внешнего мьютекса указатель на него не равен
    /// @c nullptr.
    ///
    /// @return @c true при валидной конфигурации, иначе @c false.
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

        const auto are_queues_valid =
            [&]<std::size_t... Is>(std::index_sequence<Is...>) {
                return ((queues_[Is] != nullptr) && ...);
            }(std::make_index_sequence<decorators_count>{});

        return stv::all_true(lwrb_, max_one_message_size_ > 0, is_mutex_valid,
                             are_queues_valid);
    }

    /// @brief Выполняет один цикл парсинга потока байт.
    ///
    /// @details
    /// Метод запускает конечный автомат до тех пор, пока текущее состояние
    /// не вернет @c false. За один вызов может быть распарсено несколько
    /// сообщений, если они есть в буфере.
    ///
    /// @return @c true, если за вызов было найдено и помещено в очередь
    ///     хотя бы одно сообщение; @c false в противном случае.
    auto run()
    {
        const auto parsed_cnt = parsed_cnt_;

        while(std::invoke(
            state_fnc_hash.at(static_cast<std::size_t>(get_state())), this))
        {
        }
        return parsed_cnt != parsed_cnt_;
    }

    /// @brief Возвращает ссылку на очередь готовых сообщений декоратора.
    ///
    /// @tparam Decorator Декоратор кадра, очередь которого запрашивается.
    /// @return Ссылка на очередь, в которую помещаются распарсенные
    ///     сообщения, соответствующие декоратору.
    template<typename Decorator>
    auto queue_instance() -> queue_type &
    {
        constexpr auto idx{index_of_decorator<Decorator>()};
        static_assert(idx < decorators_count,
                      "Decorator is not registered in this parser");
        assert(queues_[idx] != nullptr);
        return *queues_[idx];
    }

  private:
    /// @brief Переключает парсер в новое состояние.
    ///
    /// @param[in] new_state Целевое состояние.
    /// @return @c true, если состояние валидно и было установлено.
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

    /// @brief Возвращает текущее состояние парсера.
    ///
    /// @return Текущее состояние конечного автомата.
    auto get_state()
    {
        const stv::lock_guard critical{get_mutex_ref()};
        return state_;
    }

    /// @brief Состояние поиска начала кадра и размера сообщения.
    ///
    /// @details
    /// Просматривает буфер байт за байтом, пока не найдет стартовую
    /// последовательность, совпадающую с одним из декораторов. При
    /// совпадении переключается в состояние @c wait_message_ready,
    /// запоминает индекс выбранного декоратора и сохраняет размер
    /// оставшейся части кадра. Если заголовок не найден, пропускает один
    /// байт и продолжает поиск.
    ///
    /// @return @c true, если нужно продолжить обработку в текущем цикле;
    ///     @c false в противном случае.
    auto start_frame_and_size_state()
    {
        bool                  is_need_continue{false};

        constexpr std::size_t need_bytes_available_befor_start{
            max_header_size()};

        // matches() вызывается с двумя первыми байтами заголовка: буфер
        // заголовка обязан вмещать минимум 2 байта.
        static_assert(max_header_size() >= 2U,
                      "max_header_size() must be at least 2: matches() reads"
                      " two header bytes");

        auto how_many_bytes_can_read_in_one_iteration{max_one_message_size_};

        while(lwrb_->get_full() >= need_bytes_available_befor_start)
        {
            std::array<std::byte, max_header_size()> header_storage{};

            {
                lwrb_->peek(typename lwrb_base_type::container_type{
                    header_storage.data(), header_storage.size()});
            }

            auto        is_matched{false};
            std::size_t matched_idx{decorators_count};
            std::size_t remaining_size{0};

            auto try_match = [&]<std::size_t Idx>(
                                 std::integral_constant<std::size_t, Idx>) {
                using decorator_type = std::tuple_element_t<Idx, decorators_t>;

                if(!is_matched
                   && decorator_type::matches(header_storage[0],
                                              header_storage[1]))
                {
                    is_matched  = true;
                    matched_idx = Idx;
                    remaining_size =
                        decorator_type::total_frame_size(
                            total_message_span{header_storage.data(),
                                               decorator_type::header_size()})
                        - decorator_type::header_size();
                }
            };

            [&]<std::size_t... Is>(std::index_sequence<Is...>) {
                (try_match(std::integral_constant<std::size_t, Is>{}), ...);
            }(std::make_index_sequence<decorators_count>{});

            if(is_matched)
            {
                set_state(states::wait_message_ready);
                set_frame_selection(
                    frame_selection{matched_idx, remaining_size});
                is_need_continue = true;
            }
            else
            {
                // Если заголовок не найден, пропускаем один байт, чтобы
                // на следующей итерации вновь попробовать найти заголовок.
                lwrb_->skip(1U);
            }

            if(is_need_continue)
            {
                break;
            }

            if(how_many_bytes_can_read_in_one_iteration
               == static_cast<
                   decltype(how_many_bytes_can_read_in_one_iteration)>(0))
            {
                break;
            }

            --how_many_bytes_can_read_in_one_iteration;
        }

        return is_need_continue;
    }

    /// @brief Состояние ожидания и обработки полного кадра.
    ///
    /// @details
    /// Если кадр (заголовок + поле размера + полезная нагрузка + CRC) еще
    /// не принят целиком, состояние завершает текущий цикл run() без
    /// выделения памяти и без блокировки вызывающего потока; разбор
    /// продолжится на следующем вызове run(). Если отправитель оборвал
    /// передачу посреди кадра, после @c max_wait_message_ready_polls
    /// подряд идущих вызовов без новых байт стартовый кадр отбрасывается
    /// и парсер возвращается к поиску заголовка; приход новых байт
    /// сбрасывает отсчет, поэтому медленный, но живой отправитель
    /// таймаут не вызывает. Проверяет CRC-16; при успехе
    /// удаляет кадр из буфера, отрезает служебные поля и помещает
    /// сообщение в очередь выбранного декоратора. Если выходная очередь
    /// переполнена, сообщение отбрасывается: push в полную очередь ETL
    /// перезаписывает живой элемент без вызова деструктора, что приводит
    /// к утечке памяти и порче счетчиков очереди. При ошибке CRC
    /// пропускает только заголовок, чтобы продолжить поиск со
    /// следующего байта. Если заявленный размер кадра превышает
    /// @c max_one_message_size_, заголовок отбрасывается и парсер
    /// возвращается к поиску.
    ///
    /// @return @c true, если в буфере осталось достаточно байт для поиска
    ///     следующего заголовка; @c false в противном случае.
    auto wait_message_ready_state()
    {
        bool        is_need_continue{false};
        std::size_t header_size{0};
        std::size_t trailer_size{0};
        std::size_t expect_total_message_size{0};

        if(selected_decorator_idx_ >= decorators_count)
        {
            set_state(states::start_frame_and_size);
            return false;
        }

        dispatch_by_index(selected_decorator_idx_, [&](auto index_constant) {
            constexpr auto idx   = decltype(index_constant)::value;
            using decorator_type = std::tuple_element_t<idx, decorators_t>;

            header_size               = decorator_type::header_size();
            trailer_size              = decorator_type::trailer_size();
            expect_total_message_size = header_size + next_message_size_;
        });

        if(expect_total_message_size > max_one_message_size_)
        {
            // Размер кадра превышает допустимый: отбрасываем заголовок,
            // чтобы не зациклиться на одном и том же месте.
            lwrb_->skip(header_size);
            set_state(states::start_frame_and_size);
            set_frame_selection(frame_selection{decorators_count, 0});
            reset_wait_message_ready_timeout();
            return false;
        }

        // Означает, что буфер был сброшен за пределами парсера, значит нужно
        // перейти в режим поиска начала кадра.
        if(lwrb_->get_full() == 0)
        {
            set_state(states::start_frame_and_size);
            set_frame_selection(frame_selection{decorators_count, 0});
            reset_wait_message_ready_timeout();
            return false;
        }

        // В буфер еще не записано сообщение целиком.
        if(lwrb_->get_full() < expect_total_message_size)
        {
            track_incomplete_frame_timeout(header_size);
            return false;
        }

        // Кадр собран целиком: таймаут ожидания более неактуален.
        reset_wait_message_ready_timeout();

        // Динамическое выделение памяти с идиомой RAII.
        container_type msg{expect_total_message_size};

        constexpr auto is_isr{false};
        const auto     peek_bytes = lwrb_->peek(
            typename lwrb_base_type::container_type{msg.data(),
                                                    msg.size_bytes()},
            0, is_isr);

        if(peek_bytes == expect_total_message_size)
        {
            auto is_crc_valid{false};

            dispatch_by_index(
                selected_decorator_idx_, [&](auto index_constant) {
                    constexpr auto idx = decltype(index_constant)::value;
                    using decorator_type =
                        std::tuple_element_t<idx, decorators_t>;

                    is_crc_valid = decorator_type::is_crc_valid(
                        total_message_span{msg.begin(), msg.size()});
                });

            if(is_crc_valid)
            {
                lwrb_->skip(expect_total_message_size, is_isr);

                msg.trim_head(header_size);
                msg.trim_tail(trailer_size);

                auto *const target_queue = queues_.at(selected_decorator_idx_);
                if((target_queue != nullptr) && !target_queue->full())
                {
                    target_queue->push(std::move(msg));
                    ++parsed_cnt_;
                }
                // Переполненная очередь: сообщение отброшено, его буфер
                // освобождается деструктором msg.
            }
            else
            {
                // CRC не сошлось: ложный заголовок. Пропускаем только
                // заголовок, чтобы поиск продолжился со следующего
                // байта после ложного начала кадра.
                lwrb_->skip(header_size, is_isr);
            }

            set_state(states::start_frame_and_size);
            set_frame_selection(frame_selection{decorators_count, 0});
        }

        if(lwrb_->get_full() >= header_size)
        {
            is_need_continue = true;
        }

        return is_need_continue;
    }
};

/// @brief Создает объект @ref serial_parser.
///
/// @details
/// Фабричная функция, упрощающая создание парсера. Тип параметров
/// инициализации задается явно, декораторы кадра передаются пакетом
/// шаблонных параметров.
///
/// @tparam TSetup Тип параметров инициализации.
/// @tparam DecoratorTypes Декораторы кадра, которые будет разбирать парсер.
/// @param[in] setup Параметры инициализации парсера.
/// @return Объект @ref serial_parser<TSetup, DecoratorTypes...>.
template<typename TSetup, typename... DecoratorTypes>
auto make_serial_parser(
    const TSetup &setup)
{ return serial_parser<TSetup, DecoratorTypes...>{setup}; }

} // namespace stv

#endif /* SERIAL_PARSER_HPP */
