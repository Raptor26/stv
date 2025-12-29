/// @file lwrb.hpp
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

#ifndef LWRB_HPP
#define LWRB_HPP

#include "lwrb/lwrb.h"
#include "stv/mutex_guard.hpp"
#include "stv/wrappers.hpp"
#include <cassert>
#include <concepts>
#include <cstddef>
#include <iterator>
#include <span>
#include <string_view>

namespace stv {

template<typename TMutex    = stv::empty_mutex,
         typename TMutexTag = stv::mutex_int_tag>
struct lwrb_setup {
    using value_type           = std::byte;
    using mutex_tag            = TMutexTag;
    using mutex_type           = TMutex;
    using mutex_condition_type = stv::mutex_type_setup_v<TMutex, TMutexTag>;

    /// @brief Если указан внешний мьютекс, то mutex будет указателем на тип
    /// TMutex, в противном случае тип будет пустым.
    mutex_condition_type mutex{};
};

using lwrb_setup_default = lwrb_setup<stv::empty_mutex, stv::mutex_int_tag>;

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

    using mutex_tag = typename TSetup::mutex_tag;
    using mutex_type =
        stv::mutex_type_v<typename TSetup::mutex_type, mutex_tag>;

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
            lwrb_skip(&lwrb_, container_type::size_bytes());
        }
    };

    lwrb_t lwrb_;

    /// @brief Используется для обеспечения атомарности обновления данных в
    /// многопоточном приложении.
    mutable mutex_type mutex_;

    auto get_mutex_ref() const -> std::remove_pointer_t<mutex_type> &
    {
        if constexpr(std::is_same_v<mutex_tag, stv::mutex_ext_tag>)
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
        const auto            flags{(write_all_or_nothing)
                                        ? LWRB_FLAG_WRITE_ALL
                                        : static_cast<std::uint16_t>(0)};

        lwrb_sz_t             write_bytes{0};
        const stv::lock_guard critical{get_mutex_ref(), is_isr};
        lwrb_write_ex(&lwrb_, src, size, &write_bytes, flags);

        return write_bytes;
    }

    auto read_helper(
        void *dst, std::size_t size, bool read_all_or_nothing,
        bool is_isr = false)
    {
        const auto flags{(read_all_or_nothing) ? LWRB_FLAG_READ_ALL
                                               : static_cast<std::uint16_t>(0)};
        lwrb_sz_t  read_bytes{0};
        (void)is_isr;
        const stv::lock_guard critical{get_mutex_ref(), is_isr};
        lwrb_read_ex(&lwrb_, dst, size, &read_bytes, flags);

        return read_bytes;
    }

  public:
    virtual ~lwrb_base() = default;

    /// @brief Запись данных в буфер.
    ///
    /// @tparam U Тип контейнера, из которого выполняется запись в буфер.
    ///
    /// @param[in] src Источник данных для записи в буфер.
    /// @param[in] write_all_or_nothing Если равен true, то данные будут
    /// записаны в буфер только в том случае, если все содержимое контейнера src
    /// помещается в буфер.
    /// @param[in] is_isr True если вызов выполнен из контекста прерывания,
    /// false - в противном случае.
    ///
    /// @return Возвращает количество записанных в буфер байт.
    template<typename U>
    auto write(
        const U &src, bool write_all_or_nothing = true, bool is_isr = false)
    {
        constexpr auto item_size =
            sizeof(typename std::remove_cvref_t<U>::value_type);

        return write_helper(
            src.data(),                    /// указатель на область памяти.
            src.size() * item_size,        /// Вычисление размера в байтах.
            write_all_or_nothing, is_isr); /// Записать в буфер все или ничего.
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
    template<typename U>
    auto read(
        U &dst, bool read_all_or_nothing = false, bool is_isr = false)
    {
        constexpr auto item_size =
            sizeof(typename std::remove_cvref_t<U>::value_type);

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
        return get_full(is_isr) == 0 ? true : false;
    }

    /// @brief Возвращает емкость буфера в байтах.
    ///
    /// @param[in] is_isr True если вызов выполнен из контекста прерывания,
    /// false - в противном случае.
    ///
    /// @return Возвращает количество байт, которое одновременно может
    /// хранить буфер.
    auto capacity(
        bool is_isr = false)
    {
        const stv::lock_guard critical{get_mutex_ref(), is_isr};
        return lwrb_.size - static_cast<decltype(lwrb_.size)>(1);
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
                        {static_cast<std::byte *>(
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
        return container_type{static_cast<std::byte *>(
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
        return lwrb_skip(&lwrb_, numb);
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
        auto to_skip, bool is_isr = false)
    {
        return skip(to_skip.size_bytes(), is_isr);
    }

  protected:
    // NOLINTBEGIN(*-member-init)
    lwrb_base(
        const setup_type &setup, container_type buffer_span)
    {
        lwrb_init(&lwrb_, buffer_span.data(), buffer_span.size_bytes());

        if constexpr(std::is_same_v<mutex_tag, stv::mutex_ext_tag>)
        {
            mutex_ = setup.mutex;
        }
    }

    // NOLINTEND(*-member-init)
};

template<typename TBase, std::size_t SIZE_IN_BYTES>
using lwrb = stv::container_size_wrapper<TBase, SIZE_IN_BYTES>;

} // namespace stv

#endif /* LWRB_HPP */
