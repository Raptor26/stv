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

// NOLINTBEGIN(*hicpp-signed-bitwise, *-member*)

/// @brief Тип регистра.
using qma6100_reg_type = std::byte;

/// @brief Класс для работы с регистром настройки полосы пропускания и
/// фильтрации QMA6100.
///
/// @details Этот класс инкапсулирует логику работы с регистром 0x10 датчика
/// QMA6100, который отвечает за настройку полосы пропускания (bandwidth) и
/// цифрового фильтра нижних частот (low-pass filter). Класс предоставляет
/// удобный интерфейс для работы с этими параметрами через перечислимые типы и
/// операторы преобразования.
class qma6100_bw_reg
{
    static constexpr int bw_offset{0U};
    static constexpr int nlpf_offset{5U};

  public:
    /// @brief Адрес регистра в устройстве.
    static constexpr qma6100_reg_type addr{0x10};

    /// @brief Конструктор с возможностью инициализации значением регистра.
    ///
    /// @param value Начальное значение регистра (по умолчанию 0).
    explicit qma6100_bw_reg(
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

    /// @brief Оператор сравнения двух экземпляров регистра.
    ///
    /// @param other Другой экземпляр для сравнения.
    /// @return true, если значения регистров равны.

    bool operator==(
        const qma6100_bw_reg &other) const
    {
        return static_cast<qma6100_reg_type>(*this)
               == static_cast<qma6100_reg_type>(other);
    }
    // -------------------------------------------------------------------------

    /// @brief Тип параметра цифрового фильтра нижних частот.
    ///
    /// @details Определяет количество образцов, используемых для усреднения
    /// перед выдачей результата. Более высокие значения уменьшают шум,
    /// но увеличивают задержку.
    enum struct nlpf_t : std::uint8_t {
        /// @brief Нет усреднения (1 образец).
        no_average = 0,
        /// @brief Усреднение по 2 образцам.
        average_2,
        /// @brief Усреднение по 4 образцам.
        average_4,
        /// @brief Усреднение по 16 образцам.
        average_16
    };

    /// @brief Текущая настройка цифрового фильтра нижних частот.
    nlpf_t nlpf;
    // -------------------------------------------------------------------------

    /// @brief Тип параметра полосы пропускания.
    ///
    /// @details Определяет частоту дискретизации датчика, которая влияет
    /// на полосу пропускания измерений. Более высокие частоты дают более
    /// детальные данные, но могут быть подвержены шуму.
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

    /// @brief Текущая настройка полосы пропускания.
    bw_t bw;
    // -------------------------------------------------------------------------

  private:
    /// @brief Парсинг значения регистра в поля класса.
    ///
    /// @param reg Значение регистра для парсинга.

    void parse(
        qma6100_reg_type reg)
    {
        constexpr qma6100_reg_type bw_mask{0x1F};
        bw = static_cast<decltype(bw)>((reg >> bw_offset) & bw_mask);

        constexpr qma6100_reg_type nlpf_mask{0x03};
        nlpf = static_cast<decltype(nlpf)>((reg >> nlpf_offset) & nlpf_mask);
    }
};

/// @brief Класс для работы с регистром настройки диапазона измерений QMA6100.
///
/// @details Этот класс инкапсулирует логику работы с регистром 0x0F датчика
/// QMA6100, который отвечает за настройку диапазона измерений (full scale
/// range). Класс предоставляет удобный интерфейс для работы с этим параметром
/// через перечислимый тип и операторы преобразования.
struct qma6100_fsr_reg {
    /// @brief Адрес регистра в устройстве.
    static constexpr qma6100_reg_type addr{0x0F};

    /// @brief Преобразует конфигурацию в 8-битное значение регистра.
    ///
    /// @return 8-битное значение регистра, собранное из полей.
    explicit operator std::uint8_t() const
    {
        return static_cast<std::uint8_t>(
            (static_cast<std::uint8_t>(range) << 0U));
    }
    // -------------------------------------------------------------------------

    /// @brief Тип параметра диапазона измерений.
    ///
    /// @details Определяет максимальное ускорение, которое может быть измерено
    /// датчиком без перегрузки. Более широкий диапазон позволяет измерять
    /// большие ускорения, но уменьшает разрешение измерений.
    enum struct range_t : std::uint8_t {
        g_2  = 1,
        g_4  = 2,
        g_8  = 4,
        g_16 = 8,
        g_32 = 15,
    };

    /// @brief Текущая настройка диапазона измерений.
    range_t range{range_t::g_2};
    // -------------------------------------------------------------------------
};

class qma6100_int_en1_reg
{
    static constexpr int bint_fwm_en_offset{6U};
    static constexpr int bint_ffull_en_offset{5U};
    static constexpr int bint_data_en_offset{4U};

  public:
    /// @brief Адрес регистра в устройстве.
    static constexpr qma6100_reg_type addr{0x17};

    /// @brief Оператор преобразования в 8-битное значение регистра.
    ///
    /// @return 8-битное значение регистра, собранное из полей.
    explicit operator qma6100_reg_type() const
    {
        return static_cast<qma6100_reg_type>(
            (static_cast<std::uint8_t>(int_fwm_en) << bint_fwm_en_offset)
            | (static_cast<std::uint8_t>(int_ffull_en) << bint_ffull_en_offset)
            | (static_cast<std::uint8_t>(int_data_en) << bint_data_en_offset));
    }

    enum struct switcher_t : std::uint8_t {
        disable = 0,
        enable  = 1,
    };

    /// @brief FIFO watermark interrupt.
    switcher_t int_fwm_en{switcher_t::disable};
    // -------------------------------------------------------------------------

    /// @brief FIFO full interrupt.
    switcher_t int_ffull_en{switcher_t::disable};
    // -------------------------------------------------------------------------

    /// @brief data ready interrupt.
    switcher_t int_data_en{switcher_t::disable};
    // -------------------------------------------------------------------------
};

/// @brief Класс работы с регистром настройки mapping вывода прерывания датчика.
/// @warning В классе определены не все биты регистра.
class qma6100_int_map1_reg
{
    static constexpr int int1_no_mot_offset{7U};
    static constexpr int int1_any_mot_offset{0U};

  public:
    /// @brief Адрес регистра в устройстве.
    static constexpr qma6100_reg_type addr{0x1a};

    /// @brief Оператор преобразования в 8-битное значение регистра.
    ///
    /// @return 8-битное значение регистра, собранное из полей.
    explicit operator qma6100_reg_type() const
    {
        return static_cast<qma6100_reg_type>(
            (static_cast<std::uint8_t>(int1_no_mot) << int1_no_mot_offset)
            | (static_cast<std::uint8_t>(int1_any_mot) << int1_any_mot_offset));
    }

    /// @brief Отвечает за состояние mapping.
    enum struct mapper_t : std::uint8_t {
        disable = 0,
        enable  = 1,
    };

    /// @brief map no_motion interrupt to INT1 pin.
    mapper_t int1_no_mot{mapper_t::disable};
    // -------------------------------------------------------------------------

    mapper_t int1_any_mot{mapper_t::disable};
};

/// @brief Структура инициализации класса работы с шиной i2c.
struct qma6100_i2c_setup {
    /// @brief Адрес устройства при подключении вывода AD0 к земле.
    static constexpr qma6100_reg_type i2c_addr_connect_to_gnd{0x12};

    /// @brief Адрес устройства при подключении вывода AD0 к питанию.
    static constexpr qma6100_reg_type i2c_addr_connect_to_vdd{0x13};

    /// @brief Указатель на интерфейс шины I2C.
    stv::i2c_interface *i2c;

    /// @brief Адрес устройства, установленный пользователем.
    qma6100_reg_type i2c_addr{i2c_addr_connect_to_gnd};
};

/// @brief Класс выполняет операции чтения/записи регистров датчика.
///
/// @details Этот класс предоставляет интерфейс для работы с регистрами датчика
/// QMA6100 через интерфейс I2C. Он инкапсулирует логику обмена данными с
/// устройством и предоставляет удобные методы для чтения и записи регистров как
/// отдельных байтов, так и структурных типов.
class qma6100_i2c
{
    /// @brief Указатель на интерфейс шины I2C.
    stv::i2c_interface *i2c_;

    /// @brief Адрес устройства, установленный пользователем.
    qma6100_reg_type i2c_addr_;

  public:
    /// @brief Конструктор с инициализацией интерфейса и адреса устройства.
    ///
    /// @param[in] setup Ссылка на структуру инициализации.
    explicit qma6100_i2c(
        const stv::qma6100_i2c_setup &setup):
        i2c_{setup.i2c},
        i2c_addr_{setup.i2c_addr}
    {
    }

    /// @brief Проверка инициализации интерфейса.
    ///
    /// @return true, если интерфейс I2C инициализирован.
    explicit operator bool() const { return i2c_ != nullptr; }

    /// @brief Чтение данных из регистра устройства.
    ///
    /// @param reg_addr Адрес регистра для чтения.
    /// @param dst Указатель на буфер для хранения данных.
    /// @param len Количество байтов для чтения.
    /// @return Результат операции (успех/ошибка).
    auto read(
        qma6100_reg_type reg_addr, void *dst, std::size_t len) const
    {
        return i2c_->read(i2c_addr_, reg_addr, dst, len);
    }

    /// @brief Чтение одного байта из регистра устройства.
    ///
    /// @param reg_addr Адрес регистра для чтения.
    /// @return Значение регистра.
    [[nodiscard]] auto read(
        qma6100_reg_type reg_addr) const
    {
        qma6100_reg_type value; // NOLINT(*-init-variables)
        read(reg_addr, reinterpret_cast<void *>(&value), sizeof(value));
        return value;
    }

    /// @brief Чтение регистра и преобразование в структурный тип.
    ///
    /// @tparam U Тип регистра (например, qma6100_bw_reg).
    /// @return Экземпляр структурного типа, инициализированный значением
    /// регистра.
    template<typename U>
    [[nodiscard]] auto read()
    {
        return U{read(U::addr)};
    }

    /// @brief Запись значения в регистр устройства.
    ///
    /// @param reg_addr Адрес регистра для записи.
    /// @param value Значение для записи.
    /// @return Результат операции (успех/ошибка).
    auto write(
        qma6100_reg_type reg_addr, qma6100_reg_type value)
    {
        return i2c_->write(i2c_addr_, reg_addr, value);
    }

    /// @brief Запись структурного типа в регистр устройства.
    ///
    /// @param reg Структура, содержащая адрес и значение регистра.
    /// @return Результат операции (успех/ошибка).
    auto write(
        const auto &reg)
    {
        return write(reg.addr, static_cast<qma6100_reg_type>(reg));
    }
};

// NOLINTEND(*hicpp-signed-bitwise, *-member*)

/// @brief Структура для настройки регистров датчика QMA6100.
///
/// @details Эта структура используется для хранения конфигурации регистров
/// датчика перед их записью в устройство. Она может быть расширена для
/// поддержки дополнительных регистров в будущем.
struct qma6100_regs_setup {
    /// @brief Настройка регистра полосы пропускания и фильтрации.
    stv::qma6100_bw_reg bw_reg;
};

} // namespace stv

#endif /* QMA6100_REGS_HPP */
