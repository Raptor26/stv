/// @file serial_parser.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
///
/// NAME
///     stv::serial_parser -- парсер серийных
///     сообщений и маршрутизатор пакетов.
///
/// DESCRIPTION
///     serial_parser выделяет сообщения из потока
///     байт кольцевого буфера и помещает готовые
///     пакеты в выходную очередь. serial_parser_route
///     читает очередь распарсенных сообщений,
///     извлекает из заголовка stv::head_route
///     идентификатор получателя dst_id и
///     перенаправляет пакет в целевую очередь.
///
///     serial_parser_setup<TlwrbBase, TQueue,
///         TMutexOrPtr>
///         Параметры инициализации парсера.
///         Задают указатель на кольцевой буфер lwrb,
///         очередь queue, максимальный размер одного
///         сообщения max_one_message_size и, при
///         необходимости, мьютекс. Если
///         max_one_message_size равен 0, используется
///         емкость кольцевого буфера.
///
///     serial_parser<TSetup, Decorators...>
///         Конечный автомат из двух состояний:
///         поиск начала кадра 0xAA 0xAA и поля
///         размера, ожидание полного кадра и проверка
///         CRC-16. Метод run() выполняет один цикл
///         парсинга и возвращает true, если хотя бы
///         одно сообщение было успешно обработано.
///         Метод queue_instance() возвращает ссылку на
///         выходную очередь. Оператор bool проверяет
///         корректность инициализации.
///
///     make_serial_parser<TSetup, Decorators...>(
///         setup)
///         Фабричная функция для удобного создания
///         парсера.
///
///     serial_route_setup<TQueue, THash,
///         TMutexOrPtr>
///         Параметры инициализации маршрутизатора.
///         Задают входную очередь queue_to_read и
///         хэш-таблицу hash_to_write, где ключом
///         служит dst_id, а значением -- указатель на
///         очередь получателя.
///
///     serial_parser_route<TSetup>
///         Маршрутизатор сообщений. Метод run()
///         обрабатывает все сообщения во входной
///         очереди. Если dst_id отсутствует в таблице
///         или сообщение пустое, пакет удаляется из
///         входной очереди без передачи получателю.
///         Возвращает количество успешно
///         маршрутизованных сообщений.
///
/// EXAMPLE
///     Пример приема и маршрутизации сообщения:
///     ```cpp
///     #include "stv/communication/serial_parser.hpp"
///     #include "stv/communication/serial_sender.hpp"
///     #include "stv/communication/serial_decorators.hpp"
///     #include "stv/containers/lwrb.hpp"
///     #include "stv/containers/simbuff.hpp"
///     #include "etl/queue.h"
///     #include "etl/unordered_map.h"
///     #include <iostream>
///
///     int main() {
///         using namespace stv;
///
///         // Кольцевой буфер и очереди для примера.
///         using sim_buffer_type = stv::sim_buff<stv::empty_mutex>;
///         using queue_type = etl::queue<sim_buffer_type, 10>;
///         using lwrb_setup_type = stv::lwrb_setup<stv::empty_mutex>;
///         using lwrb_base_type = stv::lwrb_base<lwrb_setup_type>;
///
///         queue_type parsed_msg_queue;
///         queue_type serial_msg_queue;
///         stv::lwrb<lwrb_base_type, 128> lwrb{lwrb_setup_type{}};
///
///         // Буфер для формирования кадра с CRC и
///         // маршрутизацией.
///         auto serial_message_buffer = make_serial_message_buffer(
///             serial_msg_queue,
///             stv::start_frame_and_crc_16{},
///             stv::head_route{});
///
///         // Формируем сообщение с полезной нагрузкой
///         // и dst_id.
///         constexpr std::string_view payload{"Hello world"};
///         stv::head_route::head_route_setup_t route{
///             .dst_id = 1, .pack_id = 0};
///         {
///             auto msg = serial_message_buffer.request(payload, route);
///         }
///
///         // Переносим готовый кадр в кольцевой буфер.
///         auto &tx_queue = serial_message_buffer.queue_instance();
///         auto frame = tx_queue.front();
///         lwrb.write(frame.begin(), frame.end());
///         tx_queue.pop();
///
///         // Создаем и запускаем парсер.
///         using parser_setup_type =
///             stv::serial_parser_setup<lwrb_base_type, queue_type>;
///         parser_setup_type parser_setup;
///         parser_setup.lwrb = &lwrb;
///         parser_setup.queue = &parsed_msg_queue;
///
///         auto parser = stv::make_serial_parser<parser_setup_type>(
///             parser_setup);
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
///         // Маршрутизация распарсенных сообщений по
///         // dst_id.
///         using hash_type = etl::iunordered_map<int, queue_type *>;
///         etl::unordered_map<int, queue_type *, 10> hash_table;
///         hash_table.insert({1, &parsed_msg_queue});
///
///         using route_setup_type =
///             stv::serial_route_setup<queue_type, hash_type>;
///         route_setup_type route_setup;
///         route_setup.queue_to_read = &parsed_msg_queue;
///         route_setup.hash_to_write = &hash_table;
///
///         stv::serial_parser_route<route_setup_type> router(route_setup);
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
///     serial_decorators.hpp, serial_sender.hpp,
///     test_serial_parser.cpp.

#ifndef SERIAL_PARSER_HPP
#define SERIAL_PARSER_HPP

#include "lwrb/lwrb.h"
#include "stv/communication/serial_decorators.hpp"
#include "stv/mutex_guard.hpp"
#include "stv/utils.hpp"
#include <array>
#include <cstddef>
#include <functional>
#include <type_traits>
#include <utility>

namespace stv {

/// @brief Параметры инициализации парсера сообщений.
///
/// @details
/// Структура задает источник байт (кольцевой буфер), приемник готовых
/// сообщений (очередь), максимальный размер одного сообщения и,
/// при необходимости, мьютекс для защиты внутреннего состояния парсера.
/// Если @c TMutexOrPtr является указателем на мьютекс, парсер
/// использует внешний мьютекс; в противном случае синхронизация
/// отсутствует (заглушка @ref stv::empty_mutex).
///
/// @tparam TlwrbBase Тип кольцевого буфера, предоставляющего поток байт.
///     Должен быть совместим с @ref stv::lwrb_base.
/// @tparam TQueue Тип очереди для готовых сообщений. Её value_type
///     должен предоставлять непрерывный буфер байт.
/// @tparam TMutexOrPtr Тип мьютекса либо указатель на него.
///     По умолчанию используется @ref stv::empty_mutex.
template<typename TlwrbBase, typename TQueue,
         typename TMutexOrPtr = stv::empty_mutex>
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

  public:
    /// @brief Тип кольцевого буфера.
    using lwrb_base_type = TlwrbBase;

    /// @brief Тип очереди для готовых сообщений.
    using queue_type = TQueue;

    /// @brief Тип мьютекса или указателя на него.
    using mutex_type = TMutexOrPtr;

    /// @brief Указатель на кольцевой буфер с входящим потоком байт.
    lwrb_base_type *lwrb{nullptr};

    /// @brief Указатель на очередь, в которую помещаются распарсенные
    /// сообщения.
    queue_type *queue{nullptr};

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
};

/// @brief Парсер сообщений из потока байт кольцевого буфера.
///
/// @details
/// Класс реализует конечный автомат из двух состояний:
///   - поиск стартового кадра и размера сообщения;
///   - ожидание появления в буфере полного кадра и проверка CRC.
///
/// Ожидаемый формат кадра формируется декоратором
/// @ref stv::start_frame_and_crc_16: два байта 0xAA, 16-битное поле
/// размера оставшейся части кадра, полезная нагрузка, CRC-16.
/// После успешной проверки CRC заголовок и хвост отрезаются, а чистая
/// полезная нагрузка перемещается в выходную очередь.
///
/// Парсер безопасно вызывать из одного потока/контекста; если передан
/// мьютекс, внутреннее состояние защищено при переключении состояний.
/// Для извлечения нескольких сообщений за один вызов метод run()
/// обрабатывает состояния в цикле.
///
/// @tparam TSetup Тип параметров инициализации, например
///     @ref serial_parser_setup.
/// @tparam Decorators Зарезервированный пакет декораторов. В текущей
///     реализации не используется при парсинге, сохранен для совместимости
///     с API отправителя.
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

    /// @brief Очередь для готовых сообщений.
    queue_type *queue_{nullptr};

    /// @brief Максимальный допустимый размер одного сообщения.
    const std::size_t max_one_message_size_{0};

    /// @brief Счетчик успешно распарсенных сообщений.
    std::size_t parsed_cnt_{};

    /// @brief Размер следующего сообщения, найденного в состоянии
    /// @c start_frame_and_size.
    std::size_t next_message_size_{0};

    /// @brief Заполняет таблицу указателей на обработчики состояний.
    ///
    /// @return Массив с указателями на методы состояний.
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

    /// @brief Устанавливает размер следующего ожидаемого сообщения.
    ///
    /// @param[in] next_message_size Размер полезной нагрузки и хвоста
    ///     (без стартового кадра).
    void set_next_message_size(
        std::size_t next_message_size)
    {
        const stv::lock_guard critical{get_mutex_ref()};
        next_message_size_ = next_message_size;
    }

  public:
    /// @brief Конструирует парсер на основе параметров инициализации.
    ///
    /// @details
    /// Если @c setup.max_one_message_size равен 0, используется емкость
    /// кольцевого буфера. При использовании внешнего мьютекса сохраняется
    /// указатель на него из @c setup.mutex.
    ///
    /// @param[in] setup Структура с указателями на буфер, очередь,
    ///     максимальным размером сообщения и мьютексом.
    explicit serial_parser(
        const setup_type &setup):
        lwrb_{setup.lwrb},
        queue_{setup.queue},
        max_one_message_size_{(setup.max_one_message_size == 0)
                                  ? ((lwrb_ != nullptr) ? lwrb_->capacity() : 0)
                                  : (setup.max_one_message_size)}
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
    /// кольцевой буфер и очередь, максимальный размер сообщения больше 0,
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

        return stv::all_true(lwrb_, queue_, is_mutex_valid,
                             max_one_message_size_ > 0);
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

    /// @brief Возвращает ссылку на очередь готовых сообщений.
    ///
    /// @return Ссылка на очередь, в которую помещаются распарсенные
    ///     сообщения.
    auto queue_instance() -> std::remove_pointer_t<decltype(queue_)> &
    { return *queue_; }

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
    /// последовательность 0xAA 0xAA и следующее за ней 16-битное поле
    /// размера. При нахождении переключается в состояние
    /// @c wait_message_ready и сохраняет размер оставшейся части кадра.
    /// Если заголовок не найден, пропускает один байт и продолжает поиск.
    ///
    /// @return @c true, если нужно продолжить обработку в текущем цикле;
    ///     @c false в противном случае.
    auto start_frame_and_size_state()
    {
        bool                  is_need_continue{false};

        constexpr std::size_t need_bytes_available_befor_start{
            stv::start_frame_and_crc_16::header_size()};

        auto how_many_bytes_can_read_in_one_iteration{max_one_message_size_};

        while(lwrb_->get_full() >= need_bytes_available_befor_start)
        {
            stv::start_frame_and_crc_16::start_frame_t storage{};

            {
                lwrb_->peek(typename lwrb_base_type::container_type{
                    reinterpret_cast<std::byte *>(&storage), sizeof(storage)});
            }

            if((storage.start_frame_first
                == stv::start_frame_and_crc_16::first_byte)
               && (storage.start_frame_second
                   == stv::start_frame_and_crc_16::second_byte))
            {
                set_state(states::wait_message_ready);
                set_next_message_size(storage.frame_size);
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
    /// Ожидает, пока в буфере накопится полный кадр (заголовок + поле
    /// размера + полезная нагрузка + CRC). Проверяет CRC-16; при успехе
    /// удаляет кадр из буфера, отрезает служебные поля и помещает
    /// полезную нагрузку в выходную очередь. При ошибке CRC пропускает
    /// только стартовый кадр, чтобы продолжить поиск со следующего байта.
    /// Если заявленный размер кадра превышает @c max_one_message_size_,
    /// стартовый кадр отбрасывается и парсер возвращается к поиску.
    ///
    /// @return @c true, если в буфере осталось достаточно байт для поиска
    ///     следующего заголовка; @c false в противном случае.
    auto wait_message_ready_state()
    {
        bool       is_need_continue{false};
        const auto expect_total_message_size{
            stv::start_frame_and_crc_16::header_size() + next_message_size_};

        if(expect_total_message_size > max_one_message_size_)
        {
            // Размер кадра превышает допустимый: отбрасываем стартовый
            // кадр, чтобы не зациклиться на одном и том же месте.
            lwrb_->skip(sizeof(stv::start_frame_and_crc_16::start_frame_t));
            set_state(states::start_frame_and_size);
            return false;
        }

        container_type msg{expect_total_message_size};

        constexpr auto is_isr{false};
        const auto     peek_bytes = lwrb_->peek(
            typename lwrb_base_type::container_type{msg.data(),
                                                    msg.size_bytes()},
            0, is_isr);

        if(peek_bytes == expect_total_message_size)
        {
            if(stv::start_frame_and_crc_16::is_crc_valid(
                   stv::total_message_span{msg.begin(), msg.size()}))
            {
                lwrb_->skip(expect_total_message_size, is_isr);

                msg.trim_head(stv::start_frame_and_crc_16::header_size());
                msg.trim_tail(stv::start_frame_and_crc_16::trailer_size());

                queue_->push(std::move(msg));
                ++parsed_cnt_;
            }
            else
            {
                // CRC не сошлось: ложный заголовок. Пропускаем только
                // стартовый кадр, чтобы поиск продолжился со следующего
                // байта после ложного начала кадра.
                lwrb_->skip(sizeof(stv::start_frame_and_crc_16::start_frame_t),
                            is_isr);
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

/// @brief Создает объект @ref serial_parser.
///
/// @details
/// Фабричная функция, упрощающая создание парсера. Тип параметров
/// инициализации задается явно, остальные аргументы передаются
/// конструктору.
///
/// @tparam TSetup Тип параметров инициализации.
/// @tparam Decorators Типы аргументов конструктора (обычно один объект
///     @c TSetup).
/// @param[in] decorators Аргументы, передаваемые конструктору парсера.
/// @return Объект @ref serial_parser<TSetup, Decorators...>.
template<typename TSetup, typename... Decorators>
auto make_serial_parser(
    Decorators &&...decorators)
{
    return serial_parser<TSetup, Decorators...>(
        std::forward<Decorators>(decorators)...);
}

// -----------------------------------------------------------------------------

/// @brief Параметры инициализации маршрутизатора сообщений.
///
/// @details
/// Структура связывает очередь с уже распарсенными сообщениями и
/// хэш-таблицу, в которой каждому идентификатору получателя
/// соответствует указатель на целевую очередь. Идентификатор
/// получателя извлекается из заголовка @ref stv::head_route.
///
/// @tparam TQueue Тип очереди для чтения сообщений.
/// @tparam THash Тип хэш-таблицы: ключ — идентификатор получателя,
///     значение — указатель на очередь типа @c TQueue.
/// @tparam TMutexOrPtr Тип мьютекса или указатель на него.
///     По умолчанию используется @ref stv::empty_mutex.
template<typename TQueue, typename THash,
         typename TMutexOrPtr = stv::empty_mutex>
class serial_route_setup
{
    /// @brief Базовый тип мьютекса (без указателя).
    using mutex_base_type = std::remove_pointer_t<TMutexOrPtr>;

    /// @brief true, если пользователь передал указатель на внешний мьютекс.
    static constexpr bool is_external_mutex = std::is_pointer_v<TMutexOrPtr>;

    /// @brief Тип хранения мьютекса в поле @c mutex.
    using mutex_condition_type =
        std::conditional_t<is_external_mutex, mutex_base_type *,
                           std::monostate>;

  public:
    /// @brief Тип очереди для чтения сообщений.
    using queue_type = TQueue;

    /// @brief Тип хэш-таблицы маршрутов.
    using hash_type = THash;

    /// @brief Тип мьютекса или указателя на него.
    using mutex_type = TMutexOrPtr;

    /// @brief Указатель на очередь, из которой читаются сообщения.
    queue_type *queue_to_read{nullptr};

    /// @brief Указатель на хэш-таблицу очередей получателей.
    ///
    /// @details
    /// Ключом служит идентификатор получателя @c dst_id, значением —
    /// указатель на очередь, в которую нужно направить сообщение.
    hash_type *hash_to_write{nullptr};

    /// @brief Внешний мьютекс либо пустой объект.
    ///
    /// @details
    /// В текущей реализации маршрутизатор не использует мьютекс;
    /// поле сохранено для единообразия с @ref serial_parser_setup.
    mutex_type mutex{};
};

/// @brief Маршрутизатор распарсенных сообщений по очередям получателей.
///
/// @details
/// Класс читает сообщения из входной очереди, извлекает из заголовка
/// @ref stv::head_route идентификатор получателя @c dst_id и перемещает
/// сообщение в соответствующую очередь из хэш-таблицы. Если сообщение
/// не содержит корректного адреса или ключ отсутствует в таблице,
/// оно удаляется из входной очереди без передачи получателю, что
/// предотвращает её переполнение.
///
/// @tparam TSetup Тип параметров инициализации, например
///     @ref serial_route_setup.
template<typename TSetup>
class serial_parser_route: virtual public stv::non_movable_non_copyable
{
    using setup_type = TSetup;
    using queue_type = typename setup_type::queue_type;
    using hash_type  = typename setup_type::hash_type;

    /// @brief Очередь, из которой считываются входящие сообщения.
    queue_type *const queue_to_read_{nullptr};

    /// @brief Хэш-таблица очередей получателей.
    hash_type *const hash_to_write_{nullptr};

  public:
    /// @brief Конструирует маршрутизатор на основе параметров.
    ///
    /// @param[in] setup Структура с указателями на входную очередь и
    ///     таблицу маршрутов.
    explicit serial_parser_route(
        const setup_type &setup):
        queue_to_read_{setup.queue_to_read},
        hash_to_write_{setup.hash_to_write}
    { (void)setup; }

    /// @brief Деструктор по умолчанию.
    virtual ~serial_parser_route() = default;

    /// @brief Проверяет корректность инициализации маршрутизатора.
    ///
    /// @return @c true, если оба указателя (очередь и хэш-таблица)
    ///     установлены; иначе @c false.
    explicit operator bool() const
    { return stv::all_true(queue_to_read_, hash_to_write_); }

    /// @brief Выполняет маршрутизацию сообщений из входной очереди.
    ///
    /// @details
    /// Метод обрабатывает все сообщения, находящиеся во входной очереди
    /// на момент вызова. Для каждого сообщения извлекается @c dst_id
    /// из заголовка @ref stv::head_route::head_route_setup_with_pload_t.
    /// Если соответствующий ключ найден в хэш-таблице, сообщение
    /// перемещается в целевую очередь. В любом случае сообщение
    /// удаляется из входной очереди.
    ///
    /// @return Количество сообщений, успешно направленных получателям.
    auto run()
    {
        auto message_routed_cnt{0U};
        while(!queue_to_read_->empty())
        {
            decltype(auto) msg = queue_to_read_->front();

            const auto    *router_ptr = reinterpret_cast<
                const stv::head_route::head_route_setup_with_pload_t *>(
                msg.data());

            if(router_ptr)
            {
                // Используем итератор, чтобы избежать исключений.
                auto dst_buff_key_val_it = hash_to_write_->find(
                    static_cast<hash_type::key_type>(router_ptr->dst_id));
                if(dst_buff_key_val_it != hash_to_write_->end())
                {
                    dst_buff_key_val_it->second->push(std::move(msg));
                    ++message_routed_cnt;
                }
            }

            // Независимо от результата маршрутизации удаляем сообщение
            // из входной очереди, чтобы избежать её переполнения.
            queue_to_read_->pop();
        }

        return message_routed_cnt;
    }
};

} // namespace stv

#endif /* SERIAL_PARSER_HPP */
