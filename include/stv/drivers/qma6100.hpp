/// @file qma6100.hpp
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

#ifndef QMA6100_HPP
#define QMA6100_HPP

#include "qma6100_regs.hpp"
#include "stv/utils.hpp"
#include <cstddef>
#include <cstdint>

namespace stv {

/// @brief Структура для хранения параметров настройки QMA6100.
///
/// @details Эта структура содержит информацию, необходимую для инициализации и
/// работы с датчиком QMA6100.
struct qma6100_setup: public stv::qma6100_i2c_setup {
};

/// @brief Класс для работы с датчиком QMA6100.
///
/// @details Этот класс предоставляет интерфейс для чтения данных с датчика
/// QMA6100, инициализации и проверки его наличия на шине I2C.
/// Он наследуется от stv::qma6100_reg, который предоставляет базовые функции
/// для работы с регистрами датчика.
class qma6100:
    public stv::qma6100_i2c,
    public stv::non_copyable,
    public stv::non_movable
{
    STV_NO_PADDING_NO_OPTIMIZE_BEGIN

    /// @brief Структура для хранения необработанных данных датчика.
    struct raw_t {
        std::uint16_t x{0x00};
        std::uint16_t y{0x00};
        std::uint16_t z{0x00};

        /// @brief Сброс считанных данных в значения "по умолчанию".
        ///
        /// @details Функция обнуляет все поля структуры raw_t, приводя их к
        /// нулевым значениям.
        void reset()
        {
            x = 0;
            y = 0;
            z = 0;
        }
    };

    STV_NO_PADDING_NO_OPTIMIZE_END

    /// @brief Измерения датчика, полученные при крайнем вызове read_raw().
    raw_t raw_;

  public:
    /// @brief Конструктор класса qma6100.
    ///
    /// @param[in] setup Структура qma6100_setup, содержащая параметры настройки
    /// датчика.
    explicit qma6100(
        const qma6100_setup &setup):
        qma6100_i2c{setup}
    {
    }

    virtual ~qma6100() = default;

    /// @brief Оператор преобразования в bool.
    ///
    /// @return true, если датчик успешно инициализирован и доступен на шине
    /// I2C, false в противном случае.
    explicit operator bool() const
    {
        auto is_valid{true};
        if(!stv::qma6100_i2c::operator bool())
        {
            is_valid = false;
        }

        return is_valid;
    }

    /// @brief Проверяет наличие устройства на шине I2C.
    ///
    /// @param[in,out] i2c Интерфейс шины I2C.
    /// @param[in] slave_addr Адрес датчика на шине I2C.
    ///
    /// @return true, если датчик обнаружен на шине I2C.
    static auto is_detected(
        stv::i2c_interface *i2c, qma6100_reg_type slave_addr)
    {
        auto                              is_detected{false};
        qma6100_reg_type                  chip_id{0xFF};
        static constexpr qma6100_reg_type chip_id_expected{0xFA};
        static constexpr qma6100_reg_type chip_id_addr{0x00};
        const auto                        is_success =
            i2c->read(slave_addr, chip_id_addr, &chip_id, sizeof(chip_id));
        if(is_success && (chip_id == chip_id_expected))
        {
            is_detected = true;
        }

        return is_detected;
    }

    /// @brief Читает необработанные данные с датчика.
    ///
    /// @return Структура raw_t, содержащая необработанные данные с датчика.
    auto read_raw()
    {
        static constexpr qma6100_reg_type start_addr{0x01};
        const auto                        is_success =
            read(start_addr, reinterpret_cast<void *>(&raw_), sizeof(raw_));
        if(!is_success)
        {
            raw_.reset();
        }
        return raw_;
    }

    /// @brief Инициализирует датчик QMA6100.
    ///
    /// @param[in] setup Структура qma6100_regs_setup, содержащая параметры
    /// инициализации датчика.
    ///
    /// @return true, если инициализация прошла успешно, false в противном
    /// случае.
    auto init(
        const qma6100_regs_setup &setup)
    {
        auto is_init_success{true};

        write(setup.bw_reg);
        if(read<qma6100_bw_reg>() != setup.bw_reg)
        {
            is_init_success = false;
        }
        // ---------------------------------------------------------------------

        return is_init_success;
    }
};

} // namespace stv

#endif /* QMA6100_HPP */
