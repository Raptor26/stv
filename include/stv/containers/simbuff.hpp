/// @file simbuff.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.

#ifndef SIMBUFF_HPP
#define SIMBUFF_HPP

#include "stv/mutex_guard.hpp"
#include <cstddef>
#include <memory>
#include <type_traits>

namespace stv {

/// @brief SimBuff container. Manage memory, requested from ALLOCATOR.
/// @tparam ALLOCATOR - Allocator for request memory.
template<typename TMutex    = stv::empty_mutex,
         typename ALLOCATOR = std::allocator<std::byte>>
class sim_buff
{
    ALLOCATOR allocator_;
    using alloc_traits = std::allocator_traits<ALLOCATOR>;

  public:
    using value_type        = std::byte;
    using pointer           = value_type *;
    using reference         = value_type &;
    using iterator          = pointer;
    using const_iterator    = const value_type *;
    using iterator_category = std::random_access_iterator_tag;

    static_assert(sizeof(typename std::allocator_traits<ALLOCATOR>::value_type)
                      == sizeof(value_type),
                  "ALLOCATOR must allocate memory per byte");

    // -------------------------------------------------------------------------

    /// @brief Запрашивает из кучи размер памяти, указанный в size_in_bytes
    /// @param[in] size_in_bytes Размер области памяти в байтах, который
    /// необходимо выделить из аллокатора памяти.
    explicit sim_buff(
        const std::size_t size_in_bytes):
        size_in_bytes_{size_in_bytes},
        data_ptr_{safe_allocate(size_in_bytes_)},
        offset_head_{0},
        offset_tail_{0}
    {
    }

    // -------------------------------------------------------------------------

    sim_buff():
        size_in_bytes_{0},
        data_ptr_{nullptr},
        offset_head_{0},
        offset_tail_{0}
    {
    }

    // -------------------------------------------------------------------------

    virtual ~sim_buff() { safe_deallocate(); }

    // -------------------------------------------------------------------------

    sim_buff(
        const sim_buff &other):
        size_in_bytes_{other.size_in_bytes_},
        data_ptr_{safe_allocate(size_in_bytes_)},
        offset_head_{other.offset_head_},
        offset_tail_{other.offset_tail_}
    {
        if(data_ptr_ != nullptr)
        {
            // After memory allocated, need copy bytes in allocated memory area
            // from other memory area.
            memcpy(data_ptr_, other.data_ptr_, size_in_bytes_);
        }
    }

    sim_buff(
        sim_buff &&other) noexcept:
        size_in_bytes_{other.size_in_bytes_},
        data_ptr_{other.data_ptr_},
        offset_head_{other.offset_head_},
        offset_tail_{other.offset_tail_}
    { other.data_ptr_ = nullptr; }

    auto operator=(const sim_buff &other) -> sim_buff & = delete;

    auto operator=(
        sim_buff &&other) noexcept -> sim_buff &
    {
        if(&other != this)
        {
            this->safe_deallocate();

            this->data_ptr_ = other.data_ptr_;
            other.data_ptr_ = nullptr;

            this->size_in_bytes_ = other.size_in_bytes_;
        }

        return *this;
    }

    // -------------------------------------------------------------------------
    [[nodiscard]] auto begin() noexcept
    { return reinterpret_cast<iterator>(data()); }

    [[nodiscard]] auto begin() const noexcept
    { return reinterpret_cast<const_iterator>(data()); }

    [[nodiscard]] auto cbegin() const noexcept { return begin(); }

    [[nodiscard]] auto end() noexcept
    { return reinterpret_cast<iterator>(begin() + size()); }

    [[nodiscard]] auto end() const noexcept
    { return reinterpret_cast<const_iterator>(begin() + size()); }

    [[nodiscard]] auto cend() const noexcept { return end(); }

    // -------------------------------------------------------------------------

    explicit operator bool() const noexcept
    {
        bool is_ready{false};

        if(data_ptr_)
        {
            is_ready = true;
        }

        return is_ready;
    }

    // -------------------------------------------------------------------------

    /// @brief Возвращает адрес выделенной области памяти (не-const версия).
    template<typename USER_DATA_TYPE = value_type>
    [[nodiscard]] auto data() noexcept
    {
        using return_type =
            std::remove_pointer_t<std::remove_reference_t<USER_DATA_TYPE>> *;
        return reinterpret_cast<return_type>(data_ptr_ + offset_head_);
    }

    // Исправление 3: Перегрузка data() для const объектов с возвратом const
    // указателя
    /// @brief Возвращает адрес выделенной области памяти (const версия).
    template<typename USER_DATA_TYPE = value_type>
    [[nodiscard]] auto data() const noexcept
    {
        using return_type =
            const std::remove_pointer_t<std::remove_reference_t<USER_DATA_TYPE>>
                *;
        return reinterpret_cast<return_type>(data_ptr_ + offset_head_);
    }

    // -------------------------------------------------------------------------

    /// @brief Возвращает размер выделенной области памяти в байтах с учетом
    /// вызова методов trim*().
    [[nodiscard]] auto size() const noexcept
    { return size_in_bytes_ - offset_head_ - offset_tail_; }

    /// @brief Возвращает размер выделенной области памяти в байтах с учетом
    /// вызова методов trim*().
    [[nodiscard]] auto size_bytes() const noexcept { return size(); }

    // -------------------------------------------------------------------------

    /// @brief Принудительно освобождает область памяти, выделенную под
    /// сообщение. После вызова данного метода, объект становиться не валидным.
    void free() noexcept { safe_deallocate(); }

    // -------------------------------------------------------------------------

    /// @brief Задает смещение в байтах относительно начала выделенной области
    /// памяти. Метод data() возвращает адрес начала выделенной в аллокаторе
    /// области памяти + смещение.
    ///
    /// @note Значение вызова size() уменьшается на количество байт, указанное
    /// при вызове trim_head() и trim_tail().
    ///
    /// @param offset Смещение в байтах относительно начала области памяти.
    void trim_head(
        std::size_t offset) noexcept
    {
        if(offset < size_in_bytes_)
        {
            offset_head_ += offset;
        }
    }

    /// @brief Задает смещение в байтах относительно конца выделенной области
    /// памяти.
    ///
    /// @note Значение вызова size() уменьшается на количество байт, указанное
    /// при вызове trim_head() и trim_tail().
    ///
    /// @param offset Смещение в байтах относительно конца области памяти.
    void trim_tail(
        std::size_t offset) noexcept
    {
        if(offset < size_in_bytes_)
        {
            offset_tail_ += offset;
        }
    }

  private:
    [[nodiscard]] auto safe_allocate(
        std::size_t size_in_bytes) -> pointer
    {
        if(size_in_bytes > 0U)
        {
            return alloc_traits::allocate(allocator_, size_in_bytes);
        }

        return nullptr;
    }

    // -------------------------------------------------------------------------

    void safe_deallocate() noexcept
    {
        if(data_ptr_ != nullptr)
        {
            alloc_traits::deallocate(allocator_, data_ptr_, size_in_bytes_);
            data_ptr_ = nullptr;
        }
    }

    /// @brief Размер выделенной области памяти в байтах.
    std::size_t size_in_bytes_;

    /// @brief Указатель на выделенную область памяти под хранение сообщения.
    pointer     data_ptr_;

    std::size_t offset_head_;

    std::size_t offset_tail_;
};

} // namespace stv

#endif /* SIMBUFF_HPP */
