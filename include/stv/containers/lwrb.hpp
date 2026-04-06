/// @file lwrb.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.

#ifndef LWRB_HPP
#define LWRB_HPP

#include "lwrb/lwrb.h"
#include "stv/mutex_guard.hpp"
#include "stv/wrappers.hpp"
#include <cassert>
#include <concepts>
#include <cstddef>
#include <cstring>
#include <iterator>
#include <span>
#include <string_view>
#include <type_traits>

namespace stv {

template<typename TMutexOrPtr = stv::empty_mutex>
class lwrb_setup
{
    // Определяем базовый тип мьютекса.
    using mutex_base_type = std::remove_pointer_t<TMutexOrPtr>;

    // Если передан указатель на мьютекс, то считаем что пользователь хочет
    // использовать внешний мьютекс.
    static constexpr bool is_external_mutex = std::is_pointer_v<TMutexOrPtr>;

    // Тип для хранения мьютекса. Либо указатель на мьютекс, либо пустой тип.
    using mutex_condition_type =
        std::conditional_t<is_external_mutex, mutex_base_type *,
                           std::monostate>;

  public:
    using value_type = std::byte;
    using mutex_type = TMutexOrPtr;

    /// @brief Если указан внешний мьютекс, то mutex будет указателем на тип
    /// TMutex, в противном случае тип будет пустым.
    mutex_condition_type mutex{};
};

using lwrb_setup_default = lwrb_setup<stv::empty_mutex>;

/// @brief API управления кольцевым буфером.
///
/// @tparam TSetup Тип lwrb_setup
template<typename TSetup>
class lwrb_base:
    virtual public stv::non_copyable,
    virtual public stv::non_movable
{
  public:
    using value_type           = typename TSetup::value_type;
    using container_type       = std::span<value_type>;
    using container_const_type = std::span<const value_type>;
    using setup_type           = TSetup;
    using mutex_type           = typename TSetup::mutex_type;

  private:
    /// @brief Класс, деструктор которого автоматически освобождает прочитанную
    /// память в буфере.
    class skipable final: public container_type
    {
        using mutex_type_private =
            std::remove_pointer_t<std::remove_cvref_t<mutex_type>>;
        mutex_type_private &mutex_;
        lwrb_t             &lwrb_;

      public:
        skipable(
            lwrb_t &lwrb, mutex_type_private &mutex, container_type data):
            container_type{data},
            mutex_{mutex},
            lwrb_{lwrb}
        {
        }

        ~skipable()
        {
            const stv::lock_guard critical{mutex_};
            lwrb_skip(&lwrb_,
                      static_cast<lwrb_sz_t>(container_type::size_bytes()));
        }
    };

    container_type storage_;
    lwrb_t         lwrb_;

    /// @brief Используется для обеспечения атомарности обновления данных в
    /// многопоточном приложении.
    mutable mutex_type mutex_;

    auto get_mutex_ref() const -> std::remove_pointer_t<mutex_type> &
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

    auto write_helper(
        const void *src, std::size_t size, bool write_all_or_nothing,
        bool is_isr = false)
    {
        const auto flags{write_all_or_nothing ? LWRB_FLAG_WRITE_ALL
                                              : static_cast<std::uint16_t>(0)};

        lwrb_sz_t  write_bytes{0};
        const stv::lock_guard critical{get_mutex_ref(), is_isr};
        lwrb_write_ex(&lwrb_, src, static_cast<lwrb_sz_t>(size), &write_bytes,
                      flags);

        return write_bytes;
    }

    auto read_helper(
        void *dst, std::size_t size, bool read_all_or_nothing,
        bool is_isr = false)
    {
        const auto flags{read_all_or_nothing ? LWRB_FLAG_READ_ALL
                                             : static_cast<std::uint16_t>(0)};
        lwrb_sz_t  read_bytes{0};
        (void)is_isr;
        const stv::lock_guard critical{get_mutex_ref(), is_isr};
        lwrb_read_ex(&lwrb_, dst, static_cast<lwrb_sz_t>(size), &read_bytes,
                     flags);

        return read_bytes;
    }

  public:
    virtual ~lwrb_base() = default;

    auto reset(
        bool is_isr = false) -> void
    {
        const stv::lock_guard critical{get_mutex_ref(), is_isr};
        lwrb_reset(&lwrb_);
    }

    auto write(
        const void *src, std::size_t len, bool write_all_or_nothing = true,
        bool is_isr = false)
    { return write_helper(src, len, write_all_or_nothing, is_isr); }

    template<typename TIter>
        requires std::input_or_output_iterator<TIter>
    auto write(
        const TIter iter_begin, const TIter iter_end,
        bool write_all_or_nothing = true, bool is_isr = false)
    {
        constexpr auto item_size =
            sizeof(typename std::iterator_traits<TIter>::value_type);
        return write_helper(
            iter_begin,
            static_cast<std::size_t>(std::distance(iter_begin, iter_end))
                * item_size,
            write_all_or_nothing, is_isr);
    }

    /// @brief Запись данных в буфер.
    ///
    /// @param[in] src Источник данных для записи в буфер.
    /// @param[in] write_all_or_nothing Если равен true, то данные будут
    /// записаны в буфер только в том случае, если все содержимое контейнера src
    /// помещается в буфер.
    /// @param[in] is_isr True если вызов выполнен из контекста прерывания,
    /// false - в противном случае.
    ///
    /// @return Возвращает количество записанных в буфер байт.
    auto write(
        const std::ranges::contiguous_range auto &src,
        bool write_all_or_nothing = true, bool is_isr = false)
    {
        constexpr auto item_size =
            sizeof(typename std::remove_cvref_t<decltype(src)>::value_type);

        return write_helper(
            src.data(),                    /// указатель на область памяти.
            src.size() * item_size,        /// Вычисление размера в байтах.
            write_all_or_nothing, is_isr); /// Записать в буфер все или ничего.
    }

    /// @brief Запись C-style строки в буфер.
    ///
    /// @param[in] str Указатель на C-style строку в буфер.
    /// @param[in] write_all_or_nothing Если равен true, то данные будут
    /// записаны в буфер только в том случае, если все содержимое контейнера src
    /// помещается в буфер.
    /// @param[in] is_isr True если вызов выполнен из контекста прерывания,
    /// false - в противном случае.
    ///
    /// @return Возвращает количество записанных в буфер байт.
    auto write(
        const char *str, bool write_all_or_nothing = true, bool is_isr = false)
    {
        return write_helper(str, std::strlen(str), write_all_or_nothing,
                            is_isr);
    }

    /// @brief Чтение данных из буфера и запись в dst.
    ///
    /// @note После записи в dst, считанные данные удаляются из буфера.
    ///
    /// @param[in] dst Контейнер, в который будут записаны данные из буфера.
    /// @param[in] read_all_or_nothing Если равен true, то dst будет
    /// заполнен только в том случае, если буфер содержит равное или большее
    /// количество байт, чем указано в dst.
    ///
    /// @return Фактическое количество считанных байт.
    auto read(
        std::ranges::contiguous_range auto &dst,
        bool read_all_or_nothing = false, bool is_isr = false)
    {
        constexpr auto item_size =
            sizeof(typename std::remove_cvref_t<decltype(dst)>::value_type);

        return read_helper(
            dst.data(),                   /// Указатель на область памяти.
            dst.size() * item_size,       /// Вычисление размера в байтах
            read_all_or_nothing, is_isr); /// Считать из буфера все или ничего
    }

    /// @brief Чтение данных из буфера и запись в dst.
    ///
    /// @note После записи в dst, считанные данные удаляются из буфера.
    ///
    /// @param[in] dst Является типом std::span<std::byte> - в него будут
    /// записаны данные из буфера.
    /// @param[in] read_all_or_nothing Если равен true, то dst будет
    /// заполнен только в том случае, если буфер содержит равное или большее
    /// количество байт, чем указано в dst.
    ///
    /// @return Фактическое количество считанных байт.
    auto read(
        container_type dst, bool read_all_or_nothing = false,
        bool is_isr = false)
    {
        return read_helper(dst.data(), dst.size_bytes(), read_all_or_nothing,
                           is_isr);
    }

    /// @brief Возвращает количество свободного места в буфере.
    ///
    /// @param[in] is_isr True если вызов выполнен из контекста прерывания,
    /// false - в противном случае.
    ///
    /// @return Количество байт, которое доступно для записи.
    auto get_free(
        bool is_isr = false) const
    {
        const stv::lock_guard critical{get_mutex_ref(), is_isr};
        return lwrb_get_free(&lwrb_);
    }

    /// @brief Возвращает количество байт, доступное для чтения.
    ///
    /// @param[in] is_isr True если вызов выполнен из контекста прерывания,
    /// false - в противном случае.
    ///
    /// @return Количество байт, доступное для чтения.
    auto get_full(
        bool is_isr = false) const
    {
        const stv::lock_guard critical{get_mutex_ref(), is_isr};
        return lwrb_get_full(&lwrb_);
    }

    /// @brief Проверяет, пуст ли буфер.
    ///
    /// @param[in] is_isr True если вызов выполнен из контекста прерывания,
    /// false - в противном случае.
    ///
    /// @return Возвращает true если буфер пуст.
    auto is_empty(
        bool is_isr = false) const
    {
        const stv::lock_guard critical{get_mutex_ref(), is_isr};
        return get_full(is_isr) == 0;
    }

    /// @brief Возвращает емкость буфера в байтах.
    ///
    /// @param[in] is_isr True если вызов выполнен из контекста прерывания,
    /// false - в противном случае.
    ///
    /// @return Возвращает емкость буфера..
    /// хранить буфер.
    auto capacity(
        bool is_isr = false)
    {
        const stv::lock_guard critical{get_mutex_ref(), is_isr};
        return lwrb_.size;
    }

    auto reset_buff(
        bool is_isr = false) -> void
    {
        const stv::lock_guard critical{get_mutex_ref(), is_isr};
        lwrb_reset(&lwrb_);
    }

    /// @brief Возвращает std::span, который указывает на линейный участок
    /// памяти, доступный для чтения.
    ///
    /// @param[in] is_isr True если вызов выполнен из контекста прерывания,
    /// false - в противном случае.
    ///
    /// @note Деструктор возвращаемого объекта автоматически освободит память в
    /// буфере.
    ///
    /// @return std::span, указывающий на линейный участок памяти с данными.
    auto read_linear_addr(
        bool is_isr = false)
    {
        const stv::lock_guard critical{get_mutex_ref(), is_isr};
        return skipable{lwrb_,
                        get_mutex_ref(),
                        {static_cast<value_type *>(
                             lwrb_get_linear_block_read_address(&lwrb_)),
                         lwrb_get_linear_block_read_length(&lwrb_)}};
    }

    /// @brief Возвращает std::span, содержащий линейный участок памяти.
    ///
    /// @note Вызов метода не удаляет данные из буфера, поэтому, чтобы
    /// освободить память, необходимо вызвать skip().
    ///
    /// @param[in] is_isr True если вызов выполнен из контекста прерывания,
    /// false - в противном случае.
    ///
    /// @return std::span, содержащий линейный участок памяти с доступными
    /// байтами.
    auto get_linear_addr(
        bool is_isr = false)
    {
        const stv::lock_guard critical{get_mutex_ref(), is_isr};
        return container_type{static_cast<value_type *>(
                                  lwrb_get_linear_block_read_address(&lwrb_)),
                              lwrb_get_linear_block_read_length(&lwrb_)};
    }

    /// @brief Помечает указанное в numb количество байт как прочитанные, т.е.
    /// перемещает указатель чтения кольцевого буфера на указанное количество
    /// байт.
    ///
    /// @param numb Количество байт, на которое нужно переместить указатель
    /// чтения.
    ///
    /// @return Фактическое количество байт, которое помечено как прочитанное.
    auto skip(
        std::size_t numb, bool is_isr = false)
    {
        const stv::lock_guard critical{get_mutex_ref(), is_isr};
        return lwrb_skip(&lwrb_, static_cast<lwrb_sz_t>(numb));
    }

    /// @brief Помечает указанное в to_skip количество байт как прочитанные,
    /// т.е. перемещает указатель чтения кольцевого буфера на указанное
    /// количество байт.
    ///
    /// @param to_skip std::span, указывающий на область памяти в кольцевом
    /// буфере, которую необходимо пометить как прочитанную.
    ///
    /// @return Фактическое количество байт, которое помечено как прочитанное.
    auto skip(
        const container_type &to_skip, bool is_isr = false)
    { return skip(to_skip.size_bytes(), is_isr); }

    /// @brief Подсматривает указанное количество байт без удаления из
    /// кольцевого буфера.
    ///
    /// @param[in] dst Область памяти, в которую нужно записать читанные из
    /// кольцевого буфера байты.
    /// @param[in] skip_count количество байт, которое нужно пропустить перед
    /// чтением данных из буфера.
    ///
    /// @return Количество подсмотренных байт.
    auto peek(
        container_type dst, std::size_t skip_count = 0, bool is_isr = false)
    {
        const stv::lock_guard critical{get_mutex_ref(), is_isr};
        return lwrb_peek(&lwrb_, static_cast<lwrb_sz_t>(skip_count), dst.data(),
                         static_cast<lwrb_sz_t>(dst.size_bytes()));
    }

    auto advance(
        std::size_t len, bool is_isr = false)
    {
        const stv::lock_guard critical{get_mutex_ref(), is_isr};
        return lwrb_advance(&lwrb_, len);
    }

    decltype(auto) get_instance() { return &lwrb_; }

  protected:
    // NOLINTBEGIN(*-member-init)
    lwrb_base(
        const setup_type &setup, container_type buffer_span):
        storage_{buffer_span}
    {
        lwrb_init(&lwrb_, storage_.data(),
                  static_cast<lwrb_sz_t>(storage_.size_bytes()));

        if constexpr(std::is_pointer_v<decltype(mutex_)>)
        {
            mutex_ = setup.mutex;
        }
    }

    // NOLINTEND(*-member-init)
};

template<typename TBase, std::size_t SIZE_IN_BYTES>
using lwrb = stv::container_size_wrapper<TBase, SIZE_IN_BYTES + 1>;

} // namespace stv

#endif /* LWRB_HPP */
