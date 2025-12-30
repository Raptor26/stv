/// @file qma6100_regs.hpp
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

#ifndef QMA6100_REGS_HPP
#define QMA6100_REGS_HPP

#include "stv/i2c.hpp"
#include <cstddef>
#include <cstdint>

namespace stv {

/// @brief Тип регистра.
using qma6100_reg_type = std::byte;

class qma6100_bw_reg
{
    static constexpr int bw_offset{0U};
    static constexpr int nlpf_offset{5U};

  public:
    static constexpr qma6100_reg_type addr{0x10};

    qma6100_bw_reg(
        qma6100_reg_type value = qma6100_reg_type{0})
    {
        parse(value);
    }

    /// @brief Оператор преобразования в 8-битное значение регистра.
    ///
    /// @return 8-битное значение регистра, собранное из полей.
    explicit operator qma6100_reg_type() const
    {
        return static_cast<qma6100_reg_type>(
            (static_cast<std::uint8_t>(nlpf) << nlpf_offset)
            | (static_cast<std::uint8_t>(bw) << bw_offset));
    }

    bool operator==(
        const qma6100_bw_reg &other) const
    {
        return static_cast<qma6100_reg_type>(*this)
               == static_cast<qma6100_reg_type>(other);
    }
    // -------------------------------------------------------------------------

    enum struct nlpf_t : std::uint8_t {
        no_average = 0,
        average_2,
        average_4,
        average_16
    };

    nlpf_t nlpf;
    // -------------------------------------------------------------------------

    enum struct bw_t : std::uint8_t {
        mclk_512_977_hz,
        mclk_256_1953_hz,
        mclk_128_3906_hz,
        mclk_64_7813_hz,
        mclk_32_15625_hz,
        mclk_1024_488_hz,
        mclk_2048_244_hz,
        mclk_4096_122_hz,
    };

    bw_t bw;
    // -------------------------------------------------------------------------

  private:
    void parse(
        qma6100_reg_type reg)
    {
        constexpr qma6100_reg_type bw_mask{0x1F};
        bw = static_cast<decltype(bw)>((reg >> bw_offset) & bw_mask);

        constexpr qma6100_reg_type nlpf_mask{0x03};
        nlpf = static_cast<decltype(nlpf)>((reg >> nlpf_offset) & nlpf_mask);
    }
};

struct qma6100_fsr_reg {
    static constexpr qma6100_reg_type addr{0x0F};
    qma6100_reg_type                  reg{0x00};

    /// @brief Преобразует конфигурацию в 8-ми битное значение регистра.
    explicit operator std::uint8_t() const
    {
        return static_cast<std::uint8_t>(
            (static_cast<std::uint8_t>(range) << 0U));
    }
    // -------------------------------------------------------------------------

    enum struct range_t : std::uint8_t {
        g_2  = 1,
        g_4  = 2,
        g_8  = 4,
        g_16 = 8,
        g_32 = 15,
    };

    range_t range{range_t::g_2};
    // -------------------------------------------------------------------------
};

/// @brief Класс выполняет операции чтения/записи регистров датчика.
class qma6100_reg
{
    /// @brief Указатель на интерфейс шины I2C.
    stv::i2c_interface *i2c_;

    /// @brief Адрес устройства, установленный пользователем.
    qma6100_reg_type i2c_addr_;

  public:
    qma6100_reg(
        stv::i2c_interface *i2c, qma6100_reg_type i2c_addr):
        i2c_{i2c},
        i2c_addr_{i2c_addr}
    {
    }

    explicit operator bool() const { return i2c_ != nullptr; }

    //
    auto read(
        qma6100_reg_type reg_addr, void *dst, std::size_t len) const
    {
        return i2c_->read(i2c_addr_, reg_addr, dst, len);
    }

    auto read(
        qma6100_reg_type reg_addr) const
    {
        qma6100_reg_type value;
        read(reg_addr, reinterpret_cast<void *>(&value), sizeof(value));
        return value;
    }

    template<typename U>
    auto read()
    {
        return U{read(U::addr)};
    }

    auto write(
        qma6100_reg_type reg_addr, qma6100_reg_type value)
    {
        return i2c_->write(i2c_addr_, reg_addr, value);
    }

    auto write(
        const auto &reg)
    {
        return write(reg.addr, static_cast<qma6100_reg_type>(reg));
    }
};

struct qma6100_regs_setup {
    stv::qma6100_bw_reg bw_reg;
};

} // namespace stv

#endif /* QMA6100_REGS_HPP */
