/// @file mc3479.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
///
/// @details
/// Заголовок содержит высокоуровневый драйвер акселерометра MC3479,
/// работающий поверх I2C-реализации (mc3479_i2c) и наборов регистров.
///
/// Драйвер предоставляет:
/// - обнаружение устройства на I2C (is_detected);
/// - reset-последовательность с внешней задержкой;
/// - инициализацию через набор регистров mc3479_regs_setup (init);
/// - чтение "сырых" данных (read_raw) и нормированных (read_norm);
/// - управление режимами standby/wake.
///
/// Нормализация выполняется с учётом диапазона (range) и масштабного
/// коэффициента LSB, извлекаемого из регистра range_and_scale_control.
///
/// Тип AccType задаёт формат выдаваемых нормированных данных. Обычно это
/// структура вида {x, y, z, timestamp}. Требования:
/// - AccType должен иметь вложенный тип timestamp_type;
/// - AccType должен быть конструируемым как:
///   AccType{float x, float y, float z, timestamp_type ts}.
///
/// Для доступа к последнему измерению предусмотрен интерфейс iacc<AccType>
/// (метод get_acc()).

#ifndef MC3479_HPP
#define MC3479_HPP

#include "stv/drivers/mc3479_i2c.hpp"
#include "stv/drivers/mc3479_regs.hpp"
#include "stv/drivers/mc3479_types.hpp"
#include "stv/gyraccmag_types.hpp"
#include "stv/utils.hpp"
#include <cstddef>
#include <cstring>
#include <utility>

namespace stv {

/// @brief Настройки драйвера MC3479, расширяющие I2C-настройки.
/// @details
/// Структура наследует mc3479_i2c_setup и предназначена для передачи
/// параметров I2C-слоя (интерфейс, адрес, таймауты и т.п.) в конструктор
/// mc3479.
///
/// Дополнительных полей не добавляет, но позволяет явно различать
/// "настройки датчика" и "настройки транспорта" на уровне типов.
struct mc3479_setup: public stv::mc3479_i2c_setup {
};

/// @brief Драйвер акселерометра MC3479 с нормализацией данных.
/// @tparam AccType Тип нормированных измерений акселерометра.
/// @details
/// Класс инкапсулирует работу с MC3479:
/// - использует stv::mc3479_i2c для чтения/записи регистров по I2C;
/// - реализует интерфейс stv::iacc<AccType>, чтобы отдавать последнее
///   нормированное измерение через get_acc();
/// - хранит кэш последнего измерения (acc_) и монотонный timestamp_.
///
/// Типичный сценарий использования:
/// 1) Создайте объект, передав mc3479_setup с настройками I2C.
/// 2) (Опционально) Выполните is_detected() для проверки наличия чипа.
/// 3) Выполните reset(delay) при необходимости аппаратного сброса.
/// 4) Вызовите init(regs_setup), чтобы записать конфигурацию регистров.
/// 5) Периодически вызывайте read_norm() или read_raw().
///
/// Нормализация:
/// - read_raw() читает три 16-битных значения (X,Y,Z) из блока данных;
/// - драйвер конвертирует сырые значения в float через lsb_;
/// - lsb_ вычисляется из текущего диапазона (range) в регистре
///   range_and_scale_control.
///
/// Примечания:
/// - Класс не копируем и не перемещаем (non_movable_non_copyable).
/// - read_raw() увеличивает timestamp_ на каждое чтение.
/// - Для big-endian платформ выполняется byteswap сырых значений.
///
/// Важно: init() переводит датчик в standby, пишет настройки, обновляет
/// lsb_ и затем переводит датчик в wake.
template<typename AccType>
class mc3479:
    public stv::mc3479_i2c,
    public stv::iacc<AccType>,
    virtual public stv::non_movable_non_copyable
{
    using acc_type       = AccType;
    using timestamp_type = typename acc_type::timestamp_type;

    STV_NO_PADDING_NO_OPTIMIZE_BEGIN

    /// @brief Пакет сырых данных акселерометра, читаемый единым блоком.
    /// @details
    /// Структура соответствует формату данных MC3479 для осей X/Y/Z.
    /// Данные читаются из регистра addr последовательным чтением.
    ///
    /// Поля x/y/z содержат значения в "counts" (LSB) и требуют масштабирования
    /// коэффициентом lsb_ для получения значений в g (если диапазон задан
    /// стандартно).
    struct acc_t {
        /// @brief Адрес первого регистра блока сырых данных (X LSB).
        static constexpr mc3479_reg_type addr{0x0D};

        /// @brief Сырое значение по оси X (signed 16-bit).
        std::int16_t x;
        /// @brief Сырое значение по оси Y (signed 16-bit).
        std::int16_t y;
        /// @brief Сырое значение по оси Z (signed 16-bit).
        std::int16_t z;
    };

    STV_NO_PADDING_NO_OPTIMIZE_END

    /// @brief Коэффициент пересчёта из raw counts в нормированное значение.
    /// @details
    /// lsb_ хранит величину "g на 1 count": lsb_ = 1 / counts_per_g.
    /// Обновляется в init() на основании текущего диапазона range.
    float lsb_{1.0F / 16384.0F};

    /// @brief Последнее нормированное измерение акселерометра.
    /// @details
    /// Обновляется при вызове read_norm(). Возвращается пользователю через
    /// get_acc() и read_norm().
    acc_type acc_;

    /// @brief Монотонная метка времени/счётчик выборок.
    /// @details
    /// Увеличивается при каждом read_raw(). Используется при формировании
    /// acc_type в normalize().
    timestamp_type timestamp_{};

    /// @brief Прочитать регистр статуса устройства (device status).
    /// @details
    /// Внутренний метод для получения состояния устройства из регистра 0x05.
    /// Может использоваться для диагностики, если расширять драйвер.
    ///
    /// @return Значение mc3479_device_status_reg, прочитанное из датчика.
    auto read_dev_status_red()
    {
        return mc3479_i2c::read(mc3479_device_status_reg{});
    }

    /// @brief Записать регистр и проверить, что чтение совпало с записью.
    /// @details
    /// Метод записывает переданную структуру регистра, затем читает её обратно
    /// и сравнивает побайтно с исходным значением.
    ///
    /// Подходит для регистров, поддерживающих read-back и не имеющих
    /// "самоизменяющихся" битов.
    ///
    /// @param[in] reg Объект регистра (структура с addr и полями).
    /// @return true если read-back совпал с записанным значением.
    auto write_reg_then_check(
        const auto &reg)
    {
        write(reg);

        const auto read_val = read(reg);

        const auto is_equal =
            std::memcmp(reinterpret_cast<const void *>(&reg),
                        reinterpret_cast<const void *>(&read_val),
                        sizeof(read_val))
            == 0;

        return is_equal;
    }

    /// @brief Записать регистр без проверки read-back.
    /// @details
    /// Используется для регистров, где read-back может быть недоступен или
    /// неинформативен, либо проверка не требуется по скорости.
    ///
    /// @param[in,out] reg Объект регистра для записи.
    /// @return Всегда true (контракт: "запрос на запись принят").
    auto write_reg_without_check(
        auto reg)
    {
        write(reg);
        return true;
    }

    /// @brief Установить режим работы датчика (standby/sleep/wake и т.п.).
    /// @details
    /// Метод читает mc3479_mode_reg, заменяет младшие 4 бита значением mode,
    /// сохраняет старшие биты и записывает результат обратно.
    ///
    /// @param[in] mode Требуемое состояние mc3479_state.
    void set_mode(
        stv::mc3479_state mode)
    {
        auto val = static_cast<std::uint8_t>(read(mc3479_mode_reg::addr));

        val &= 0b11110000U;
        val |= std::to_underlying(mode);

        write(mc3479_mode_reg::addr, std::byte{val});
    }

    /// @brief Получить коэффициент LSB для текущего диапазона измерений.
    /// @details
    /// Читает stv::mc3479_range_and_scale_control_reg и по полю range выбирает
    /// counts_per_g. Возвращает величину 1 / counts_per_g (g на 1 count).
    ///
    /// @return Коэффициент lsb_ для преобразования raw -> нормированные данные.
    auto get_lsb()
    {
        float      lsb{};
        const auto reg = read(stv::mc3479_range_and_scale_control_reg{});
        switch(reg.range)
        {
            case mc3479_range::g2:
                lsb = 16384.0F;
                break;
            case mc3479_range::g4:
                lsb = 8192.0F;
                break;
            case mc3479_range::g8:
                lsb = 4096.0F;
                break;
            case mc3479_range::g16:
                lsb = 2048.0F;
                break;
            case mc3479_range::g12:
                lsb = 2730.0F;
                break;

            default:
                break;
        }
        return 1.0F / lsb;
    }

    /// @brief Преобразовать сырые данные в нормированный формат AccType.
    /// @details
    /// Перемножает raw.x/y/z на lsb_ и добавляет текущий timestamp_.
    /// Ожидается, что AccType имеет конструктор:
    /// AccType{float x, float y, float z, timestamp_type ts}.
    ///
    /// @param[in] raw Сырые значения осей.
    /// @return Нормированные данные в формате AccType.
    auto normalize(
        const auto &raw)
    {
        return acc_type{raw.x * lsb_, raw.y * lsb_, raw.z * lsb_, timestamp_};
    }

  public:
    /// @brief Создать экземпляр драйвера MC3479.
    /// @details
    /// Конструктор инициализирует базовый I2C-драйвер mc3479_i2c настройками
    /// транспорта. Настройка регистров датчика выполняется отдельно через
    /// init().
    ///
    /// @param[in] setup Настройки I2C (интерфейс, адрес, параметры обмена).
    explicit mc3479(
        const mc3479_setup &setup):
        stv::mc3479_i2c{setup}
    {
        (void)setup;
    }

    /// @brief Проверка готовности драйвера/транспорта.
    /// @details
    /// Возвращает true, если базовый I2C-слой корректно инициализирован и
    /// готов к операциям чтения/записи.
    explicit operator bool() const
    {
        return stv::all_true(stv::mc3479_i2c::operator bool());
    }

    /// @brief Проверить наличие MC3479 на шине I2C по Chip ID.
    /// @details
    /// Метод читает байт Chip ID из регистра 0x18 и сравнивает с ожидаемым
    /// значением 0xA4.
    ///
    /// Использование:
    /// @code
    /// if (!mc3479<AccT>::is_detected(i2c, addr)) { ... }
    /// @endcode
    ///
    /// @param[in] i2c Указатель на I2C-интерфейс.
    /// @param[in] slave_addr 7-bit адрес устройства на I2C.
    /// @return true если чтение успешно и Chip ID совпал.
    static auto is_detected(
        stv::i2c_interface *i2c, mc3479_reg_type slave_addr)
    {
        static constexpr mc3479_reg_type chip_id_valid{0xA4};
        static constexpr mc3479_reg_type chip_id_addr{0x18};
        mc3479_reg_type                  chip_id{0xFF};
        const auto                       is_success =
            i2c->read(slave_addr, chip_id_addr, &chip_id, sizeof(chip_id));

        return stv::all_true(is_success, (chip_id == chip_id_valid));
    }

    /// @brief Выполнить reset-последовательность датчика.
    /// @details
    /// Метод переводит датчик в sleep, выполняет программный power-on-reset
    /// (запись 0x40 в регистр 0x1C), отключает прерывания, задаёт Analog Gain
    /// 1.00x и отключает DCM.
    ///
    /// Между операциями выполняются задержки через пользовательскую функцию
    /// delay(), что позволяет применять драйвер в bare-metal/RTOS средах.
    ///
    /// Ожидаемый интерфейс DelayFn:
    /// - вызывается как delay(ms), где ms --- целое число миллисекунд.
    ///
    /// @tparam DelayFn Тип функции задержки.
    /// @param[in] delay Функция задержки в миллисекундах.
    template<typename DelayFn>
    void reset(
        DelayFn &delay)
    {
        // Stand by mode
        {
            stv::mc3479_mode_reg reg;
            reg.state = mc3479_state::sleep;
            write(reg);
        }

        delay(10);

        // power-on-reset
        write(std::byte(0x1c), std::byte(0x40));

        delay(50);

        // Disable interrupt
        write(std::byte(0x06), std::byte(0x00));
        delay(10);
        // 1.00x Aanalog Gain
        write(std::byte(0x2B), std::byte(0x00));
        delay(10);

        // DCM disable
        write(std::byte(0x15), std::byte(0x00));

        delay(50);
    }

    /// @brief Прочитать status-регистр датчика.
    /// @details
    /// Читает mc3479_status_reg по его адресу (mc3479_status_reg::addr).
    /// Поля регистра зависят от определения в mc3479_regs.hpp.
    ///
    /// @return Прочитанное значение mc3479_status_reg.
    auto read_status_reg()
    {
        mc3479_status_reg val{};
        mc3479_i2c::read(mc3479_status_reg::addr,
                         reinterpret_cast<void *>(&val), sizeof(val));
        return val;
    }

    /// @brief Прочитать сырые значения осей X/Y/Z.
    /// @details
    /// Читает блок данных acc_t из регистра acc_t::addr. После чтения
    /// увеличивает timestamp_. На big-endian платформах выполняется byteswap
    /// для каждого 16-битного значения.
    ///
    /// @return Структура acc_t с полями x/y/z в raw counts.
    auto read_raw()
    {
        acc_t raw;
        read(acc_t::addr, reinterpret_cast<void *>(&raw), sizeof(raw));
        ++timestamp_;

        if constexpr(std::endian::native == std::endian::big)
        {
            raw.x = std::byteswap(raw.x);
            raw.y = std::byteswap(raw.y);
            raw.z = std::byteswap(raw.z);
        }

        return raw;
    }

    /// @brief Прочитать и сохранить нормированные значения акселерометра.
    /// @details
    /// Выполняет read_raw(), затем преобразует данные в AccType через
    /// normalize() и сохраняет результат в acc_. Возвращает обновлённое
    /// значение.
    ///
    /// Важно: корректность масштаба зависит от lsb_, который обновляется в
    /// init().
    ///
    /// @return Нормированные измерения в формате AccType.
    auto read_norm()
    {
        acc_ = normalize(read_raw());

        return acc_;
    }

    /// @brief Перевести датчик в standby (остановить измерения).
    /// @details
    /// Устанавливает режим mc3479_state::standby. Используйте перед записью
    /// конфигурационных регистров или для экономии энергии.
    void stop() { set_mode(stv::mc3479_state::standby); }

    /// @brief Перевести датчик в режим измерений (wake).
    /// @details
    /// Устанавливает режим mc3479_state::wake. Обычно вызывается после init().
    void wake() { set_mode(stv::mc3479_state::wake); }

    /// @brief Получить последнее нормированное измерение.
    /// @details
    /// Возвращает значение, сохранённое в acc_. Чтобы обновить acc_, вызовите
    /// read_norm(). Метод не выполняет I2C-операций.
    ///
    /// @return Последнее измерение в формате AccType.
    [[nodiscard]] auto get_acc() const -> acc_type override { return acc_; }

    /// @brief Инициализация драйвера.
    ///
    /// @details
    /// Метод выполняет конфигурирование датчика набором регистров
    /// stv::mc3479_regs_setup:
    /// - переводит датчик в standby (stop());
    /// - записывает регистры из setup (часть с проверкой read-back);
    /// - вычисляет и сохраняет коэффициент lsb_ по текущему диапазону;
    /// - переводит датчик в wake (wake()).
    ///
    /// Выбор write_reg_then_check / write_reg_without_check сделан исходя из
    /// предположений о возможности надёжного read-back для конкретных
    /// регистров.
    ///
    /// @param[in] setup Настройки, которыми нужно инициализировать датчик.
    ///
    /// @return true если инициализация успешно завершена, false в противном
    /// случае.
    auto init(
        const stv::mc3479_regs_setup &setup)
    {
        stop();
        const auto is_init_success = stv::all_true(
            write_reg_then_check(setup.interrupt_enable),
            write_reg_without_check(setup.sample_rate),
            write_reg_then_check(setup.motion_control),
            write_reg_without_check(setup.range_and_scale_control),
            write_reg_then_check(setup.mode));

        lsb_ = get_lsb();

        wake();
        return is_init_success;
    }
};

} // namespace stv

#endif /* MC3479_HPP */
