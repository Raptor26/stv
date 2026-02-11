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
#include "stv/gyraccmag_types.hpp"
#include "stv/utils.hpp"
#include <array>
#include <bit>
#include <cstdint>
#include <cstring>
#include <type_traits>

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
///          - `stv::non_copyable` и `stv::non_movable` для запрета
///          копирования и перемещения объекта.
///
///          Для использования необходимо создать объект класса, передав в
///          конструктор настроенную структуру `qma6100_setup`, затем
///          вызвать метод `init()` для настройки датчика. После успешной
///          инициализации можно периодически вызывать `read_raw()` для
///          получения акселерометрических данных.
template<typename AccType>
class qma6100:
    public stv::qma6100_i2c,
    public stv::iacc<AccType>,
    virtual public stv::non_movable_non_copyable
{
    using acc_type       = AccType;
    using base_type      = stv::iacc<acc_type>;
    using timestamp_type = typename acc_type::timestamp_type;

    STV_NO_PADDING_NO_OPTIMIZE_BEGIN

    /// @brief Структура для хранения необработанных (raw) данных с
    /// акселерометра.
    ///
    /// @details Содержит три 16-битных поля для осей X, Y и Z. Данные
    /// хранятся в формате, предоставляемом непосредственно датчиком, и требуют
    /// последующего масштабирования в соответствии с выбранным диапазоном
    /// измерений (FSR).
    struct raw_t {
        /// @brief Необработанное значение по оси X.
        std::int16_t x{0x00};

        /// @brief Необработанное значение по оси Y.
        std::int16_t y{0x00};

        /// @brief Необработанное значение по оси Z.
        std::int16_t z{0x00};

        /// @brief Сбрасывает все значения структуры в ноль.
        ///
        /// @details Используется при ошибке чтения данных с датчика для
        /// возврата структуры в предсказуемое состояние.
        void reset()
        {
            x = 0;
            y = 0;
            z = 0;
        }
    };

    STV_NO_PADDING_NO_OPTIMIZE_END

    timestamp_type timestamp_{};

    /// @brief Внутренний буфер для хранения последних считанных
    /// необработанных данных.
    ///
    /// @details Обновляется при каждом успешном вызове метода `read_raw()`.
    /// Если чтение прошло неудачно, поля структуры сбрасываются в ноль.
    raw_t    raw_{};

    acc_type acc_;

    /// @brief Значение LSB (Least Significant Bit) для текущего диапазона
    /// измерений.
    ///
    /// @details Используется для преобразования необработанных значений в
    /// физические единицы (g). Значение рассчитывается при инициализации
    /// датчика init().
    float lsb_{};

    /// @brief Преобразует сырые байты оси в знаковое 16-битное значение.
    ///
    /// @details Метод объединяет два байта (старший и младший) в 16-битное
    ///          значение и выполняет сдвиг вправо на 2 бита для получения
    ///          фактического значения оси из 14-битного представления.
    ///
    /// @param[in] axis Указатель на массив из двух байтов, где [0] -
    /// младший байт, [1] - старший байт.
    ///
    /// @return Преобразованное 16-битное знаковое значение оси.
    static auto convert_axis(
        const std::uint8_t *axis)
    {
        // NOLINTNEXTLINE(cppcoreguidelines-init-variables)
        std::int16_t raw_value;
        std::memcpy(&raw_value, axis, sizeof(raw_value));

        // При необходимости, выполним преобразование из little endian в big
        // endian.
        if constexpr(std::endian::native == std::endian::big)
        {
            raw_value = std::byteswap(raw_value);
        }

        // NOLINTNEXTLINE(hicpp-signed-bitwise)
        return static_cast<std::int16_t>(raw_value >> 2U);
    }

    /// @brief Проверяет, обновлены ли данные оси.
    ///
    /// @details Проверяет младший бит первого байта, который указывает на
    /// обновление данных для соответствующей оси.
    ///
    /// @param[in] axis Байт данных, первый байт из группы данных оси.
    ///
    /// @return `true`, если данные оси были обновлены, `false` в противном
    /// случае.
    static auto is_axis_updated(
        std::uint8_t axis)
    {
        return axis
               & static_cast<
                   std::remove_const_t<std::remove_pointer_t<decltype(axis)>>>(
                   0x01);
    }

    /// @brief Вычисляет значение масштабного коэффициента на основе
    /// записанного в память датчика значения.
    ///
    /// @details Метод читает текущий диапазон измерений (FSR) из регистра
    /// датчика и возвращает соответствующее значение LSB (Least Significant
    /// Bit), которое используется для преобразования необработанных данных в
    /// физические единицы (g).
    ///
    /// @return Масштабный коэффициент (LSB) в единицах g/bit.
    auto compute_lsb()
    {
        constexpr auto lsb_2g  = .000244;
        constexpr auto lsb_4g  = lsb_2g * 2;
        constexpr auto lsb_8g  = lsb_4g * 2;
        constexpr auto lsb_16g = lsb_8g * 2;
        constexpr auto lsb_32g = lsb_16g * 2;

        const auto     reg = read<stv::qma6100_fsr_reg>();

        switch(reg.range)
        {
            using lsb_type = float;

            case stv::qma6100_fsr_reg::range_t::g_2:
                return static_cast<lsb_type>(lsb_2g);

            case stv::qma6100_fsr_reg::range_t::g_4:
                return static_cast<lsb_type>(lsb_4g);

            case stv::qma6100_fsr_reg::range_t::g_8:
                return static_cast<lsb_type>(lsb_8g);

            case stv::qma6100_fsr_reg::range_t::g_16:
                return static_cast<lsb_type>(lsb_16g);

            case stv::qma6100_fsr_reg::range_t::g_32:
                return static_cast<lsb_type>(lsb_32g);

            default:
                return static_cast<lsb_type>(lsb_2g);
        }
    }

    /// @brief Читает необработанные данные с акселерометра.
    ///
    /// @details Метод выполняет чтение 6 байт данных, начиная с регистра
    /// 0x01, которые содержат необработанные значения по осям X, Y и Z. После
    /// чтения проверяется флаг обновления для каждой оси, и если данные
    /// обновлены, они преобразуются и сохраняются во внутреннем буфере `raw_`.
    /// В случае ошибки чтения, буфер сбрасывается в ноль.
    ///
    /// @return Структура `raw_t`, содержащая три 16-битных значения для
    /// осей X, Y и Z. Если в процессе чтения произошла ошибка, все поля
    /// структуры будут равны нулю.
    ///
    /// @note Данные считываются начиная с регистра 0x01, который содержит
    /// старший байт значения X. Чтение выполняется блоком из 6 байт (X, Y, Z).
    /// Полученные значения требуют последующего преобразования в физические
    /// единицы измерения (g) с учетом настроенного диапазона (FSR).
    auto read_raw()
    {
        static constexpr qma6100_reg_type start_addr{0x01};

        /// @brief В переменную ниже будут записаны считанные измерения
        /// датчика.
        std::array<std::uint8_t, 6> storage{};

        if(read(start_addr, reinterpret_cast<void *>(storage.data()),
                sizeof(storage)))
        {
            if(is_axis_updated(static_cast<std::uint8_t>(storage.at(0))))
            {
                raw_.x = convert_axis(storage.data());
            }

            if(is_axis_updated(static_cast<std::uint8_t>(storage.at(2))))
            {
                raw_.y = convert_axis(storage.data() + 2);
            }

            if(is_axis_updated(static_cast<std::uint8_t>(storage.at(4))))
            {
                raw_.z = convert_axis(storage.data() + 4);
            }

            ++timestamp_;
        }
        else
        {
            raw_.reset();
        }

        return raw_;
    }

    /// @brief Преобразует необработанные данные в нормализованные (в g).
    ///
    /// @details Метод принимает необработанные данные и умножает каждую
    /// компоненту на текущий коэффициент LSB для получения значений в g.
    ///
    /// @tparam T Тип структуры с необработанными данными (должна иметь поля
    /// x, y, z).
    ///
    /// @param[in] raw Структура с необработанными данными.
    ///
    /// @return Структура `normalize_t` с нормализованными значениями осей.
    auto normalize(
        const auto &raw)
    {
        return acc_type{raw.x * lsb_, raw.y * lsb_, raw.z * lsb_, timestamp_};
    }

    /// @brief Записывает регистр в память датчика, а затем считывает его и
    /// сравнивает с тем что планировалось записать.
    ///
    /// @param[in] reg регистр, значение которого необходимо записать.
    ///
    /// @return true если записанное и, затем считанное значение совпали.
    auto write_reg_then_check(
        const auto &reg)
    {
        volatile auto is_reg_written_success{false};
        write(reg);
        if(read<std::remove_cvref_t<decltype(reg)>>() == reg)
        {
            is_reg_written_success = true;
        }

        return is_reg_written_success;
    }

  public:
    /// @brief Конструктор объекта для работы с датчиком QMA6100.
    ///
    /// @details Создает экземпляр класса `qma6100`, инициализируя базовый
    /// класс `qma6100_i2c` с помощью переданной структуры настроек. Не
    /// производит инициализацию самого датчика.
    ///
    /// @param[in] setup Конфигурационная структура с параметрами
    /// подключения по шине I2C (адрес, указатель на интерфейс и т.д.).
    ///
    /// @note Конструктор не выполняет инициализацию датчика. Для настройки
    /// параметров работы (диапазон, полоса пропускания, прерывания) необходимо
    /// отдельно вызвать метод `init()`.
    explicit qma6100(
        const qma6100_setup &setup):
        qma6100_i2c{setup}
    {
    }

    /// @brief Виртуальный деструктор класса.
    ///
    /// @details Позволяет корректно уничтожать производные классы через
    /// указатель на базовый класс. Реализация по умолчанию.
    ~qma6100() override = default;

    /// @brief Проверяет корректность инициализации датчика.
    ///
    /// @details Является оператором приведения к bool. Проверяет состояние
    /// базового класса `stv::qma6100_i2c`, чтобы убедиться, то датчик был
    /// правильно инициализирован и доступен.
    ///
    /// @return `true` - если датчик был успешно инициализирован и доступен на
    /// шине I2C, `false` в противном случае.
    ///
    /// @note Оператор использует проверку состояния базового класса
    /// `stv::qma6100_i2c`. Для полноценной проверки работоспособности
    /// рекомендуется также вызывать `is_detected()`.
    explicit operator bool() const
    {
        auto is_valid{true};
        if(!stv::qma6100_i2c::operator bool())
        {
            is_valid = false;
        }

        return is_valid;
    }

    /// @brief Статический метод для проверки наличия датчика QMA6100 на
    /// шине I2C.
    ///
    /// @details Метод читает идентификационный регистр датчика (0x00) и
    /// сравнивает его значение с ожидаемым (0x90). Это позволяет определить,
    /// подключен ли датчик QMA6100 к шине I2C по указанному адресу.
    ///
    /// @param[in] i2c Указатель на объект интерфейса I2C, через который
    /// осуществляется связь.
    /// @param[in] slave_addr Адрес датчика на шине I2C (обычно 0x12 или 0x13 в
    /// зависимости от состояния вывода SDO).
    ///
    /// @return `true` - если по указанному адресу ответило устройство с
    /// ожидаемым идентификатором (chip ID = 0x90), `false` в противном случае.
    ///
    /// @note Метод не требует предварительной инициализации датчика и может
    /// быть использован для автоопределения устройства на шине.
    static auto is_detected(
        stv::i2c_interface *i2c, qma6100_reg_type slave_addr)
    {
        auto                              is_detected{false};
        static constexpr qma6100_reg_type chip_id_valid{0x90};
        static constexpr qma6100_reg_type chip_id_addr{0x00};
        qma6100_reg_type                  chip_id{0xFF};
        const auto                        is_success =
            i2c->read(slave_addr, chip_id_addr, &chip_id, sizeof(chip_id));
        if(is_success && (chip_id == chip_id_valid))
        {
            is_detected = true;
        }

        return is_detected;
    }

    /// @brief Читает нормализованные (в g) данные с акселерометра.
    ///
    /// @details Метод сначала вызывает `read_raw()` для получения
    /// необработанных данных, а затем преобразует их в физические единицы (g) с
    /// помощью внутреннего коэффициента LSB и возвращает результат.
    ///
    /// @return Структура `normalize_t`, содержащая нормализованные значения
    /// ускорения по осям X, Y и Z в g.
    auto read_normalized()
    {
        acc_ = normalize(read_raw());
        return acc_;
    }

    [[nodiscard]] auto get_acc() const -> acc_type override { return acc_; }

    /// @brief Выполняет инициализацию датчика QMA6100 с заданными
    /// параметрами.
    ///
    /// @details Метод последовательно записывает переданные
    /// конфигурационные значения в соответствующие регистры датчика и после
    /// каждой записи выполняет чтение для верификации. Также вычисляет значение
    /// LSB на основе установленного диапазона измерений.
    ///
    /// @param[in] setup Структура `qma6100_regs_setup`, содержащая значения для
    /// ключевых регистров датчика: полоса пропускания (BW), диапазон измерений
    /// (FSR), настройка прерываний и т.д.
    ///
    /// @return `true` - если все регистры были успешно записаны и прочитаны
    /// обратно с проверкой, `false` - если хотя бы одна операция записи/чтения
    /// не удалась.
    auto init(
        const qma6100_regs_setup &setup)
    {
        volatile const auto is_init_success =
            stv::all_true(write_reg_then_check(setup.bw_reg),
                          write_reg_then_check(setup.fsr_reg),
                          write_reg_then_check(setup.int_en1_reg),
                          write_reg_then_check(setup.int_map1_reg),
                          write_reg_then_check(setup.int_map3_reg),
                          write_reg_then_check(setup.intpin_conf_reg),
                          write_reg_then_check(setup.int_cfg_reg),
                          write_reg_then_check(setup.pm_reg));

        // Вычисление масштабного коэффициента необходимо выполнить после записи
        // регистра "fsr_reg".
        lsb_ = compute_lsb();

        return is_init_success;
    }
};

} // namespace stv

#endif /* QMA6100_HPP */
