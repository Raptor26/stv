/// @file i2c.hpp
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

#ifndef I2C_HPP
#define I2C_HPP

#include "stv/utils.hpp"
#include <cstddef>

namespace stv {

class i2c_interface: public stv::non_copyable, public stv::non_movable
{
  public:
    using byte_type = std::byte;

    virtual ~i2c_interface() = default;

    /// @brief Записывает значение в указанный регистр устройства.
    ///
    /// @param[in] slave_addr Адрес ведомого устройства.
    /// @param[in] reg_addr Адрес регистра для записи.
    /// @param[in] write_reg_value Значение для записи.
    ///
    /// @return true, если операция успешно завершена, false в противном
    ///         случае.
    [[nodiscard("Check i2c write operation status")]] virtual auto
    write(byte_type slave_addr, byte_type reg_addr,
          byte_type write_reg_value) const -> bool = 0;

    /// @brief Считывает указанное количество регистров начиная с адреса.
    ///
    /// @param[in] slave_addr Адрес ведомого устройства.
    /// @param[in] reg_addr Адрес первого регистра для чтения.
    /// @param[out] dst Указатель на буфер для записи данных.
    /// @param[in] len Количество регистров для чтения.
    ///
    /// @return true, если операция успешно завершена, false в противном
    ///         случае.
    [[nodiscard("Check i2c read operation status")]] virtual auto
    read(byte_type slave_addr, byte_type reg_addr, void *dst,
         std::size_t len) const -> bool = 0;

  protected:
    i2c_interface() = default;
};

} // namespace stv

#endif /* I2C_HPP */
