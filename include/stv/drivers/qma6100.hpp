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

#include "qma6100_i2c.hpp"
#include "qma6100_regs.hpp"
#include "stv/utils.hpp"
#include <cstddef>
#include <cstdint>

namespace stv {

/// @brief Структура для хранения параметров настройки датчика QMA6100.
///
/// @details Наследует `stv::qma6100_i2c_setup` и содержит конфигурационные
///          параметры, необходимые для инициализации соединения с датчиком
///          по шине I2C. Пользователь должен заполнить поля этой структуры
///          перед созданием объекта класса `qma6100`.
struct qma6100_setup: public stv::qma6100_i2c_setup {
};

/// @brief Класс для работы с акселерометром QMA6100.
///
/// @details Этот класс предоставляет высокоуровневый интерфейс для
/// взаимодействия
///          с датчиком QMA6100 по шине I2C. Он инкапсулирует логику чтения
///          необработанных данных, инициализации датчика с заданными
///          параметрами и проверки его наличия на шине. Класс наследует:
///          - `stv::qma6100_i2c` для низкоуровневого доступа к регистрам
///          датчика,
///          - `stv::non_copyable` и `stv::non_movable` для запрета копирования
///            и перемещения объекта.
///
///          Для использования необходимо создать объект класса, передав в
///          конструктор настроенную структуру `qma6100_setup`, затем вызвать
///          метод `init()` для настройки датчика. После успешной инициализации
///          можно периодически вызывать `read_raw()` для получения
///          акселерометрических данных.
class qma6100:
    public stv::qma6100_i2c,
    public stv::non_copyable,
    public stv::non_movable
{
    STV_NO_PADDING_NO_OPTIMIZE_BEGIN

    /// @brief Структура для хранения необработанных (raw) данных с
    /// акселерометра.
    ///
    /// @details Содержит три 16-битных поля для осей X, Y и Z. Данные хранятся
    ///          в формате, предоставляемом непосредственно датчиком, и требуют
    ///          последующего масштабирования в соответствии с выбранным
    ///          диапазоном измерений (FSR).
    struct raw_t {
        /// @brief Необработанное значение по оси X.
        std::uint16_t x{0x00};

        /// @brief Необработанное значение по оси Y.
        std::uint16_t y{0x00};

        /// @brief Необработанное значение по оси Z.
        std::uint16_t z{0x00};

        /// @brief Сбрасывает все значения структуры в ноль.
        ///
        /// @details Используется при ошибке чтения данных с датчика для
        /// возврата
        ///          структуры в предсказуемое состояние.
        void reset()
        {
            x = 0;
            y = 0;
            z = 0;
        }
    };

    STV_NO_PADDING_NO_OPTIMIZE_END

    /// @brief Внутренний буфер для хранения последних считанных необработанных
    /// данных.
    ///
    /// @details Обновляется при каждом успешном вызове метода `read_raw()`.
    ///          Если чтение прошло неудачно, поля структуры сбрасываются в
    ///          ноль.
    raw_t raw_;

  public:
    /// @brief Конструктор объекта для работы с датчиком QMA6100.
    ///
    /// @param[in] setup Конфигурационная структура с параметрами подключения
    ///                  по шине I2C (адрес, указатель на интерфейс и т.д.).
    ///
    /// @note Конструктор не выполняет инициализацию датчика. Для настройки
    ///       параметров работы (диапазон, полоса пропускания, прерывания)
    ///       необходимо отдельно вызвать метод `init()`.
    explicit qma6100(
        const qma6100_setup &setup):
        qma6100_i2c{setup}
    {
    }

    virtual ~qma6100() = default;

    /// @brief Проверяет корректность инициализации датчика.
    ///
    /// @return `true` - если датчик был успешно инициализирован и доступен
    ///         на шине I2C, `false` в противном случае.
    ///
    /// @note Оператор использует проверку состояния базового класса
    ///       `stv::qma6100_i2c`. Для полноценной проверки работоспособности
    ///       рекомендуется также вызывать `is_detected()`.
    explicit operator bool() const
    {
        auto is_valid{true};
        if(!stv::qma6100_i2c::operator bool())
        {
            is_valid = false;
        }

        return is_valid;
    }

    /// @brief Статический метод для проверки наличия датчика QMA6100 на шине
    /// I2C.
    ///
    /// @param[in,out] i2c Указатель на объект интерфейса I2C, через который
    ///                    осуществляется связь.
    /// @param[in] slave_addr Адрес датчика на шине I2C (обычно 0x12 или 0x13
    ///                       в зависимости от состояния вывода SDO).
    ///
    /// @return `true` - если по указанному адресу ответило устройство с
    ///         ожидаемым идентификатором (chip ID = 0xFA), `false` в противном
    ///         случае.
    ///
    /// @note Метод не требует предварительной инициализации датчика и может
    ///       быть использован для автоопределения устройства на шине.
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

    /// @brief Читает необработанные данные с акселерометра.
    ///
    /// @return Структура `raw_t`, содержащая три 16-битных значения для осей
    ///         X, Y и Z. Если в процессе чтения произошла ошибка, все поля
    ///         структуры будут равны нулю.
    ///
    /// @note Данные считываются начиная с регистра 0x01, который содержит
    ///       старший байт значения X. Чтение выполняется блоком из 6 байт
    ///       (X, Y, Z). Полученные значения требуют последующего преобразования
    ///       в физические единицы измерения (g) с учетом настроенного диапазона
    ///       (FSR).
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

    /// @brief Выполняет инициализацию датчика QMA6100 с заданными параметрами.
    ///
    /// @param[in] setup Структура `qma6100_regs_setup`, содержащая значения
    ///                  для ключевых регистров датчика: полоса пропускания
    ///                  (BW), диапазон измерений (FSR), настройка прерываний и
    ///                  т.д.
    ///
    /// @return `true` - если все регистры были успешно записаны и прочитаны
    ///         обратно с проверкой, `false` - если хотя бы одна операция
    ///         записи/чтения не удалась.
    ///
    /// @details Метод последовательно записывает переданные конфигурационные
    ///          значения в соответствующие регистры датчика и после каждой
    ///          записи выполняет чтение для верификации. Порядок настройки:
    ///          1. Полоса пропускания (bandwidth)
    ///          2. Диапазон измерений (full-scale range)
    ///          3. Включение прерываний (interrupt enable 1)
    ///          4. Привязка прерываний к выводам (interrupt map 1, 3)
    ///          5. Конфигурация выводов прерываний (interrupt pin
    ///          configuration)
    ///          6. Настройка параметров прерываний (interrupt configuration)
    auto init(
        const qma6100_regs_setup &setup)
    {
        auto is_init_success{true};

        write(setup.bw_reg);
        if(read<decltype(setup.bw_reg)>() != setup.bw_reg)
        {
            is_init_success = false;
        }
        // ---------------------------------------------------------------------

        write(setup.fsr_reg);
        if(read<decltype(setup.fsr_reg)>() != setup.fsr_reg)
        {
            is_init_success = false;
        }
        // ---------------------------------------------------------------------

        write(setup.int_en1_reg);
        if(read<decltype(setup.int_en1_reg)>() != setup.int_en1_reg)
        {
            is_init_success = false;
        }
        // ---------------------------------------------------------------------

        write(setup.int_map1_reg);
        if(read<decltype(setup.int_map1_reg)>() != setup.int_map1_reg)
        {
            is_init_success = false;
        }
        // ---------------------------------------------------------------------

        write(setup.int_map3_reg);
        if(read<decltype(setup.int_map3_reg)>() != setup.int_map3_reg)
        {
            is_init_success = false;
        }
        // ---------------------------------------------------------------------

        write(setup.intpin_conf_reg);
        if(read<decltype(setup.intpin_conf_reg)>() != setup.intpin_conf_reg)
        {
            is_init_success = false;
        }
        // ---------------------------------------------------------------------

        write(setup.int_cfg_reg);
        if(read<decltype(setup.int_cfg_reg)>() != setup.int_cfg_reg)
        {
            is_init_success = false;
        }
        // ---------------------------------------------------------------------

        return is_init_success;
    }
};

} // namespace stv

#endif /* QMA6100_HPP */
