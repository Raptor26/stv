/// @file rm3100.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
///
/// @brief Драйвер магнитометра PNI RM3100 по шине I2C.
///
/// @details
///     Заголовок предоставляет шаблонный класс @ref stv::rm3100 и
///     вспомогательные структуры @ref stv::rm3100_setup и
///     @ref stv::rm3100_regs_setup. Драйвер реализует инициализацию датчика,
///     запуск непрерывных и одиночных измерений, встроенный самотест (BIST),
///     чтение сырых 24-битных значений по трём осям и их нормализацию в
///     единицы Гаусса.
///
///     Класс наследуется от @ref stv::rm3100_i2c и @ref stv::imag, поэтому
///     может использоваться как источник магнитометрических данных в
///     прикладном коде.
///
/// @par Пример использования:
/// @code
/// // Тип измерения: 3-осевой вектор с меткой времени.
/// using mag_type = stv::mag<float, std::uint32_t>;
///
/// // Предполагается, что i2c — объект, реализующий stv::i2c_interface.
/// stv::rm3100_setup<mag_type> setup{{.i2c = &i2c}};
/// stv::rm3100<decltype(setup)> sensor{setup};
///
/// if(!sensor) {
///     // не удалось связаться с датчиком
/// }
///
/// if(!stv::rm3100<stv::rm3100_setup<mag_type>>::is_detected(&i2c)) {
///     // на шине нет RM3100
/// }
///
/// // Настройка непрерывного режима: cycle count 200, все оси, ~37 Гц.
/// stv::rm3100_regs_setup regs{};
/// regs.ccx.cycle_count = 200U;
/// regs.ccy.cycle_count = 200U;
/// regs.ccz.cycle_count = 200U;
/// regs.cmm.start       = true;
/// regs.cmm.cmx         = true;
/// regs.cmm.cmy         = true;
/// regs.cmm.cmz         = true;
/// regs.cmm.drdm        = stv::rm3100_cmm_drdm::after_all_axes;
///
/// if(sensor.init(regs) && sensor.start_continuous()) {
///     while(!sensor.is_data_ready()) {
///         // ожидание готовности
///     }
///
///     const auto field = sensor.read_normalized();
///     if(field) {
///         const float mx = field.give_x(); // Гауссы
///         const float my = field.give_y();
///         const float mz = field.give_z();
///     }
/// }
/// @endcode

#ifndef RM3100_HPP
#define RM3100_HPP

#include "rm3100_i2c.hpp"
#include "rm3100_regs.hpp"
#include "stv/gyraccmag_types.hpp"
#include "stv/utils.hpp"
#include <array>
#include <concepts>
#include <cstdint>
#include <type_traits>

namespace stv {

/// @brief Концепт проверяет, что переданный тип может использоваться как
/// функция задержки в миллисекундах.
///
/// @details
///     Тип должен поддерживать вызов с одним аргументом, представляющим
///     количество миллисекунд (@c std::uint16_t).
template<typename TDelayFnMs>
concept rm3100_delayable = requires(TDelayFnMs delay_fn, std::uint16_t ms) {
    { delay_fn(ms) };
};

/// @brief Параметры инициализации драйвера RM3100.
///
/// @details
///     Наследуется от @ref stv::rm3100_i2c_setup и добавляет тип измерения
///     магнитного поля @c mag_type. Используется как параметр шаблона класса
///     @ref stv::rm3100.
///
/// @tparam MagType Тип данных для нормализованных измерений магнитного поля.
///                 Обычно @c stv::mag<float, std::uint32_t>.
/// @tparam Addr 7-битный I2C-адрес устройства (по умолчанию 0x20).
template<typename MagType, rm3100_reg_type Addr = 0x20>
struct rm3100_setup: public rm3100_i2c_setup<Addr> {
    /// @brief Тип измерения магнитного поля.
    using mag_type = MagType;
};

/// @brief Набор регистров для инициализации RM3100.
///
/// @details
///     Содержит cycle count для каждой оси, регистр частоты обновления TMRC и
///     регистр непрерывного режима CMM. Передаётся в метод
///     @ref stv::rm3100::init().
struct rm3100_regs_setup {
    /// @brief Cycle count по оси X (по умолчанию 200).
    rm3100_ccx_reg ccx{200};

    /// @brief Cycle count по оси Y (по умолчанию 200).
    rm3100_ccy_reg ccy{200};

    /// @brief Cycle count по оси Z (по умолчанию 200).
    rm3100_ccz_reg ccz{200};

    /// @brief Регистр частоты обновления в режиме CMM (по умолчанию 0x96).
    rm3100_tmrc_reg tmrc{0x96};

    /// @brief Регистр непрерывного режима измерений.
    rm3100_cmm_reg cmm;
};

/// @brief Класс для работы с магнитометром PNI RM3100.
///
/// @details
///     Драйвер предоставляет высокоуровневый интерфейс для управления
///     магнитометром PNI RM3100 по шине I2C. RM3100 — трёхосевой
///     магнито-индукционный датчик: результат измерения по каждой оси —
///     знаковое 24-битное число, а точность и шумовые характеристики
///     задаются параметром cycle count (числом накоплений, регистры
///     CCX/CCY/CCZ). Драйвер выполняет:
///     - проверку присутствия датчика на шине по регистру REVID;
///     - запись cycle count и запуск непрерывного (CMM) или одиночного
///       (POLL) режима измерений;
///     - встроенный самотест (BIST) по выбранным осям;
///     - чтение сырых 24-битных значений с осей X, Y, Z;
///     - нормализацию результатов в Гауссы с учётом текущего cycle count.
///
///     Класс наследуется от @ref stv::rm3100_i2c, через который выполняет
///     низкоуровневый обмен по I2C, и от @ref stv::imag<mag_type>, что
///     позволяет использовать его как источник магнитометрических данных
///     в прикладном коде.
///
///     Тип измерения и параметры подключения задаются через структуру
///     настроек @ref stv::rm3100_setup, а набор регистров инициализации —
///     через @ref stv::rm3100_regs_setup. Метка времени (@c packstamp)
///     увеличивается на единицу при каждом успешном чтении сырых данных,
///     что позволяет прикладному коду отличать свежие измерения от
///     повторно возвращённых.
///
/// @code
/// // Тип измерения: 3-осевой вектор float с меткой времени.
/// using mag_type = stv::mag<float, std::uint32_t>;
///
/// // i2c — объект, реализующий интерфейс stv::i2c_interface.
/// stv::rm3100_setup<mag_type> setup{{.i2c = &i2c}};
/// stv::rm3100<decltype(setup)> sensor{setup};
///
/// // Проверка присутствия датчика на шине.
/// if(!decltype(sensor)::is_detected(&i2c)) {
///     // RM3100 не найден: неверный адрес или обрыв линии.
/// }
///
/// // Встроенный самотест по всем осям. Функция задержки получает
/// // количество миллисекунд.
/// const auto delay_ms = [](std::uint16_t ms) { blocking_delay_ms(ms); };
/// if(!sensor.run_self_test(delay_ms)) {
///     // Самотест не пройден.
/// }
///
/// // Настройка: cycle count 200 по всем осям, частота обновления
/// // ~37 Гц (TMRC = 0x96), непрерывный режим по трём осям, DRDY
/// // поднимается после завершения измерения всех включённых осей.
/// stv::rm3100_regs_setup regs{};
/// regs.ccx.cycle_count = 200U;
/// regs.ccy.cycle_count = 200U;
/// regs.ccz.cycle_count = 200U;
/// regs.tmrc.value      = 0x96U;
/// regs.cmm.start       = true;
/// regs.cmm.cmx         = true;
/// regs.cmm.cmy         = true;
/// regs.cmm.cmz         = true;
/// regs.cmm.drdm        = stv::rm3100_cmm_drdm::after_all_axes;
///
/// if(sensor.init(regs) && sensor.start_continuous()) {
///     // Ожидание готовности данных (в реальном коде — по прерыванию
///     // от линии DRDY датчика).
///     while(!sensor.is_data_ready()) {
///     }
///
///     // Чтение и нормализация в Гауссы.
///     const auto field = sensor.read_normalized();
///     if(field) {
///         const float mx = field.give_x(); // Компонента X, Гс.
///         const float my = field.give_y(); // Компонента Y, Гс.
///         const float mz = field.give_z(); // Компонента Z, Гс.
///     }
/// }
/// @endcode
///
/// @tparam Setup Тип настроек, содержащий mag_type и параметры
///               I2C-подключения (см. @ref stv::rm3100_setup).
template<typename Setup>
class rm3100:
    public rm3100_i2c<Setup::i2c_addr>,
    public stv::imag<typename Setup::mag_type>
{
    using setup_type     = Setup;
    using mag_type       = typename setup_type::mag_type;
    using i2c_base       = rm3100_i2c<setup_type::i2c_addr>;
    using base_type      = stv::imag<mag_type>;
    using timestamp_type = typename mag_type::timestamp_type;
    using value_type     = typename mag_type::value_type;

  public:
    /// @brief Структура сырых 24-битных значений магнитного поля.
    ///
    /// @details
    ///     Хранит знаковые значения по осям X, Y и Z, полученные
    ///     непосредственно из регистров датчика. Оператор @c bool проверяет,
    ///     что значения не равны нулю одновременно и не выходят за границы
    ///     24-битного диапазона (исключая крайние значения @c -0x800000 и @c
    ///     0x7FFFFF).
    struct raw_t {
        /// @brief Сырое 24-битное значение по оси X.
        std::int32_t x{0};

        /// @brief Сырое 24-битное значение по оси Y.
        std::int32_t y{0};

        /// @brief Сырое 24-битное значение по оси Z.
        std::int32_t z{0};

        /// @brief Проверяет корректность сырых данных.
        ///
        /// @return
        ///     `true`, если все три оси ненулевые и лежат внутри допустимого
        ///     24-битного диапазона; `false` в противном случае.
        explicit operator bool() const
        {
            if((x == 0) && (y == 0) && (z == 0))
            {
                return false;
            }
            constexpr std::int32_t min24{-0x800000};
            constexpr std::int32_t max24{0x7FFFFF};
            return (x > min24) && (x < max24) && (y > min24) && (y < max24)
                   && (z > min24) && (z < max24);
        }
    };

  private:
    STV_NO_PADDING_NO_OPTIMIZE_BEGIN

    /// @brief Байтовое представление регистров результата измерения.
    ///
    /// @details
    ///     Отображает 9 байт, считываемых одной транзакцией I2C начиная с
    ///     адреса 0x24 (регистры MX2–MZ0). Для каждой оси байты идут от
    ///     старшего к младшему (big-endian): например, значение оси X
    ///     собирается из MX2 (биты 23–16), MX1 (биты 15–8) и MX0
    ///     (биты 7–0). Структура упакована (без выравнивания), поэтому её
    ///     размер в точности равен 9 байтам.
    struct raw_bytes_t {
        /// @brief Старший байт (биты 23–16) значения по оси X.
        std::uint8_t mx2;

        /// @brief Средний байт (биты 15–8) значения по оси X.
        std::uint8_t mx1;

        /// @brief Младший байт (биты 7–0) значения по оси X.
        std::uint8_t mx0;

        /// @brief Старший байт (биты 23–16) значения по оси Y.
        std::uint8_t my2;

        /// @brief Средний байт (биты 15–8) значения по оси Y.
        std::uint8_t my1;

        /// @brief Младший байт (биты 7–0) значения по оси Y.
        std::uint8_t my0;

        /// @brief Старший байт (биты 23–16) значения по оси Z.
        std::uint8_t mz2;

        /// @brief Средний байт (биты 15–8) значения по оси Z.
        std::uint8_t mz1;

        /// @brief Младший байт (биты 7–0) значения по оси Z.
        std::uint8_t mz0;
    };

    STV_NO_PADDING_NO_OPTIMIZE_END

    /// @brief Кэш последнего успешного нормализованного измерения.
    ///
    /// @details
    ///     Обновляется в @ref read_normalized() только при корректном
    ///     чтении сырых данных; в противном случае хранит предыдущее
    ///     значение.
    mag_type mag_{};

    /// @brief Счётчик успешных измерений.
    ///
    /// @details
    ///     Увеличивается на единицу при каждом корректном чтении сырых
    ///     данных и передаётся в измерение как метка @c packstamp.
    timestamp_type timestamp_{0};

    /// @brief Признак активного непрерывного режима (CMM).
    ///
    /// @details
    ///     Соответствует биту START регистра CMM. Обновляется методами
    ///     @ref init(), @ref start_continuous() и @ref stop_continuous().
    bool is_cmm_active_{false};

    /// @brief Копия регистров последней успешной инициализации.
    ///
    /// @details
    ///     Сохраняется в @ref init() и используется методами @ref reinit()
    ///     и @ref start_continuous() для восстановления конфигурации.
    rm3100_regs_setup regs_copy_{};

    /// @brief Масштаб LSB/мкТл, вычисляемый из cycle count оси X.
    ///
    /// @details
    ///     Драйвер предполагает, что cycle count одинаков для всех осей, и
    ///     использует значение по оси X в качестве масштаба для всех трёх
    ///     осей. По умолчанию 75.0F, что соответствует cycle count 200.
    value_type lsb_per_ut_{static_cast<value_type>(75.0F)};

    /// @brief Адрес первого регистра результата измерения (MX2).
    ///
    /// @details
    ///     Начиная с этого адреса, одной транзакцией считываются 9 байт
    ///     результата (MX2–MZ0), см. @ref raw_bytes_t.
    static constexpr rm3100_reg_type meas_result_base_addr{0x24};

    /// @brief Период ожидания LR-осциллятора во встроенном самотесте
    /// (4 sleep-цикла, ~120 мкс).
    static constexpr std::uint8_t default_bist_bw{0b11};

    /// @brief Количество LR-периодов измерения во встроенном самотесте
    /// (4 периода).
    static constexpr std::uint8_t default_bist_bp{0b11};

    /// @brief Таймаут ожидания DRDY во встроенном самотесте по умолчанию, мс.
    static constexpr std::uint16_t default_self_test_timeout_ms{100};

    /// @brief Выполняет знаковое расширение 24-битного значения до
    /// @c std::int32_t.
    ///
    /// @details
    ///     Регистры результата RM3100 хранят знаковые 24-битные числа в
    ///     дополнительном коде. Если старший бит (бит 23) установлен,
    ///     значение отрицательное, и старший байт 32-битного результата
    ///     заполняется единицами (0xFF000000).
    ///
    /// @param[in] value 24-битное значение без знака, собранное из трёх
    ///                  байт регистра результата.
    /// @return Знаковое 32-битное значение.
    [[nodiscard]] static auto sign_extend_24(
        std::uint32_t value) -> std::int32_t
    {
        constexpr std::uint32_t sign_mask{0x800000U};
        constexpr std::uint32_t extension{0xFF000000U};
        return static_cast<std::int32_t>(
            value | ((value & sign_mask) != 0U ? extension : 0U));
    }

    /// @brief Возвращает масштаб LSB/мкТл для заданного cycle count.
    ///
    /// @details
    ///     Драйвер предполагает, что cycle count одинаков для всех осей, и
    ///     использует значение по оси X в качестве масштаба для всех трёх осей.
    ///
    /// @param[in] cycle_count Количество накоплений (cycle count).
    /// @return Масштаб в LSB на микротесла.
    static auto get_lsb_per_ut(
        std::uint16_t cycle_count) -> value_type
    {
        switch(cycle_count)
        {
            case 200:
                return static_cast<value_type>(75.0F);
            case 100:
                return static_cast<value_type>(38.0F);
            case 50:
                return static_cast<value_type>(20.0F);
            default:
                return static_cast<value_type>(75.0F)
                       * static_cast<value_type>(cycle_count)
                       / static_cast<value_type>(200.0F);
        }
    }

    /// @brief Записывает 16-битный cycle count в пару регистров CCx.
    ///
    /// @details
    ///     Значение передаётся двумя байтами: сначала старший (MSB) по
    ///     адресу @p RegAddr, затем младший (LSB) по адресу @p RegAddr + 1.
    ///
    /// @tparam RegAddr Адрес первого (старшего) регистра пары CCx.
    /// @param[in] value Значение cycle count.
    /// @return `true`, если обе записи прошли успешно.
    template<rm3100_reg_type RegAddr>
    auto write_cycle_count(
        std::uint16_t value) -> bool
    {
        const auto msb = static_cast<rm3100_reg_type>(
            (static_cast<std::uint32_t>(value) >> 8U) & 0xFFU);
        const auto lsb = static_cast<rm3100_reg_type>(
            static_cast<std::uint32_t>(value) & 0xFFU);
        return stv::all_true(
            this->write(RegAddr, msb),
            this->write(static_cast<rm3100_reg_type>(RegAddr + 1), lsb));
    }

    /// @brief Считывает cycle count всех трёх осей.
    ///
    /// @details
    ///     Одной транзакцией читает 6 байт, начиная с адреса регистра CCX,
    ///     и разбирает их в три 16-битных значения (X, Y, Z). Используется
    ///     в @ref init() для контрольного чтения после записи.
    ///
    /// @param[out] out Структура, в которую записываются считанные
    ///                 значения cycle count.
    /// @return `true`, если чтение прошло успешно.
    auto read_cycle_counts(
        rm3100_regs_setup &out) const -> bool
    {
        std::array<std::uint8_t, 6> buf{};
        if(!this->read(rm3100_ccx_reg::addr, buf.data(), buf.size()))
        {
            return false;
        }
        out.ccx.cycle_count = static_cast<std::uint16_t>(
            (static_cast<std::uint32_t>(buf[0]) << 8U)
            | static_cast<std::uint32_t>(buf[1]));
        out.ccy.cycle_count = static_cast<std::uint16_t>(
            (static_cast<std::uint32_t>(buf[2]) << 8U)
            | static_cast<std::uint32_t>(buf[3]));
        out.ccz.cycle_count = static_cast<std::uint16_t>(
            (static_cast<std::uint32_t>(buf[4]) << 8U)
            | static_cast<std::uint32_t>(buf[5]));
        return true;
    }

    /// @brief Записывает регистр и проверяет запись контрольным чтением.
    ///
    /// @details
    ///     После записи регистр считывается обратно, и результат
    ///     сравнивается с записанным значением. Тип регистра должен
    ///     поддерживать запись/чтение через транспорт @ref stv::rm3100_i2c
    ///     и оператор сравнения.
    ///
    /// @param[in] reg Регистр для записи.
    /// @return `true`, если запись прошла успешно и считанное значение
    ///         совпало с записанным.
    auto write_reg_then_check(
        const auto &reg) -> bool
    {
        if(!this->write(reg))
        {
            return false;
        }
        using local_reg_type = std::remove_cvref_t<decltype(reg)>;
        const auto read_val  = this->template read<local_reg_type>();
        return read_val == reg;
    }

    /// @brief Ожидает подъёма бита DRDY в STATUS с заданным таймаутом.
    ///
    /// @tparam TDelayFnMs Тип функции задержки: аргумент — миллисекунды.
    ///
    /// @param[in] timeout_ms Таймаут ожидания, мс.
    /// @param[in] delay_ms Функция задержки в миллисекундах.
    /// @return true, если DRDY поднялся до истечения таймаута.
    template<rm3100_delayable TDelayFnMs>
    [[nodiscard]] auto wait_drdy(
        std::uint16_t timeout_ms, TDelayFnMs &delay_ms) -> bool
    {
        for(std::uint16_t elapsed = 0; elapsed <= timeout_ms; ++elapsed)
        {
            const auto status = this->template read<rm3100_status_reg>();
            if(status.drdy)
            {
                return true;
            }

            if(elapsed == timeout_ms)
            {
                break;
            }

            delay_ms(1);
        }

        return false;
    }

    /// @brief Проверяет флаги XOK/YOK/ZOK результата BIST для осей,
    /// участвовавших в самотесте.
    ///
    /// @param[in] result Считанный регистр BIST.
    /// @param[in] poll Настройки POLL: какие оси были протестированы.
    /// @return true, если все протестированные оси прошли проверку.
    [[nodiscard]] static auto are_bist_axes_ok(
        const rm3100_bist_reg &result, const rm3100_poll_reg &poll) -> bool
    {
        return stv::all_true(!poll.pmx || result.xok, !poll.pmy || result.yok,
                             !poll.pmz || result.zok);
    }

  public:
    /// @brief Конструктор из структуры настроек.
    ///
    /// @param[in] setup Настройки драйвера: I2C-интерфейс и адрес устройства.
    explicit rm3100(
        const Setup &setup):
        i2c_base{setup}
    {
    }

    /// @brief Виртуальный деструктор.
    ~rm3100() override = default;

    /// @brief Проверяет, что драйвер имеет валидный I2C-интерфейс.
    ///
    /// @return
    ///     `true`, если в конструктор был передан ненулевой указатель на
    ///     @ref stv::i2c_interface; `false` в противном случае.
    explicit operator bool() const override
    { return i2c_base::operator bool(); }

    /// @brief Проверяет наличие датчика RM3100 на шине I2C.
    ///
    /// @details
    ///     Считывает регистр REVID и сравнивает его с ожидаемым значением.
    ///     Метод не требует создания объекта @ref rm3100: достаточно передать
    ///     указатель на реализацию @ref stv::i2c_interface.
    ///
    /// @param[in] i2c Указатель на I2C-интерфейс.
    /// @param[in] expected_revid Ожидаемое значение REVID.
    /// @param[in] addr 7-битный I2C-адрес устройства.
    ///
    /// @return
    ///     `true`, если устройство обнаружено и REVID совпадает; `false` при
    ///     ошибке чтения или неверном идентификаторе.
    static auto is_detected(
        stv::i2c_interface *i2c,
        rm3100_revid_reg    expected_revid =
            rm3100_revid_reg{rm3100_revid_reg::expected_value},
        rm3100_reg_type addr = Setup::i2c_addr) -> bool
    {
        if(i2c == nullptr)
        {
            return false;
        }

        rm3100_reg_type revid{0};
        const auto      is_success = i2c->read(
            //
            static_cast<stv::i2c_interface::byte_type>(addr),

            // The REVID register provides revision identification of
            // the MagI2C. This is a single byte, read-only register.
            // To read the REVID register, send 0xB6
            static_cast<stv::i2c_interface::byte_type>(0xB6),

            //
            &revid,

            //
            sizeof(revid));

        return stv::all_true(is_success,
                             (rm3100_revid_reg{revid} == expected_revid));
    }

    /// @brief Инициализирует датчик заданной конфигурацией.
    ///
    /// @details
    ///     Последовательно выполняет:
    ///     - запись cycle count для всех осей;
    ///     - чтение cycle count обратно для проверки;
    ///     - запись регистра TMRC с контрольным чтением;
    ///     - запись регистра CMM (запуск непрерывного режима, если задан).
    ///
    /// @param[in] regs Набор регистров инициализации.
    ///
    /// @return
    ///     `true`, если все операции записи и проверки завершились успешно;
    ///     `false` при ошибке обмена по I2C.
    auto init(
        const rm3100_regs_setup &regs) -> bool
    {
        is_cmm_active_ = false;

        auto is_success = stv::all_true(
            write_cycle_count<rm3100_ccx_reg::addr>(regs.ccx.cycle_count),
            write_cycle_count<rm3100_ccy_reg::addr>(regs.ccy.cycle_count),
            write_cycle_count<rm3100_ccz_reg::addr>(regs.ccz.cycle_count));

        rm3100_regs_setup read_back{};
        if(is_success)
        {
            is_success = read_cycle_counts(read_back);
        }

        if(is_success)
        {
            is_success = (read_back.ccx.cycle_count == regs.ccx.cycle_count)
                         && (read_back.ccy.cycle_count == regs.ccy.cycle_count)
                         && (read_back.ccz.cycle_count == regs.ccz.cycle_count);
        }

        if(is_success)
        {
            is_success = write_reg_then_check(regs.tmrc);
        }

        if(is_success)
        {
            is_success = this->write(regs.cmm);
            if(is_success)
            {
                is_cmm_active_ = regs.cmm.start;
            }
        }

        if(is_success)
        {
            lsb_per_ut_ = get_lsb_per_ut(regs.ccx.cycle_count);
            regs_copy_  = regs;
        }

        return is_success;
    }

    /// @brief Переинициализирует датчик ранее сохранённой конфигурацией.
    ///
    /// @details
    ///     Повторно выполняет @ref init() с копией регистров
    ///     @ref regs_copy_, сохранённой при последней успешной
    ///     инициализации. Полезно для восстановления датчика после сбоя
    ///     обмена по шине I2C.
    ///
    /// @return `true`, если инициализация завершилась успешно.
    auto reinit() -> bool { return init(regs_copy_); }

    /// @brief Запускает непрерывный режим измерений.
    ///
    /// @details
    ///     Устанавливает бит START в регистре CMM, используя параметры,
    ///     заданные при последнем вызове @ref init().
    ///
    /// @return `true`, если режим запущен успешно.
    auto start_continuous() -> bool
    {
        rm3100_cmm_reg reg{regs_copy_.cmm};
        reg.start          = true;
        const auto success = this->write(reg);
        is_cmm_active_     = success;
        return success;
    }

    /// @brief Останавливает непрерывный режим измерений.
    ///
    /// @details
    ///     Записывает нулевой регистр CMM (бит START сброшен, все оси
    ///     выключены) и сбрасывает внутренний признак активности CMM.
    ///
    /// @return `true`, если режим остановлен успешно.
    auto stop_continuous() -> bool
    {
        rm3100_cmm_reg reg{};
        reg.start          = false;
        const auto success = this->write(reg);
        if(success)
        {
            is_cmm_active_ = false;
        }
        return success;
    }

    /// @brief Возвращает состояние непрерывного режима.
    ///
    /// @return
    ///     `true`, если CMM активен (бит START установлен); `false` в противном
    ///     случае.
    [[nodiscard]] auto is_continuous_active() const -> bool
    { return is_cmm_active_; }

    /// @brief Запускает одиночное измерение по выбранным осям.
    ///
    /// @details
    ///     Если непрерывный режим активен, он предварительно останавливается.
    ///     Затем в регистр POLL записываются флаги запрашиваемых осей.
    ///
    /// @param[in] poll Настройки одиночного измерения.
    ///
    /// @return `true`, если команда отправлена успешно.
    auto request_single(
        rm3100_poll_reg poll) -> bool
    {
        if(is_cmm_active_)
        {
            if(!stop_continuous())
            {
                return false;
            }
        }
        const auto success = this->write(poll);
        return success;
    }

    /// @brief Проверяет готовность новых данных.
    ///
    /// @return
    ///     `true`, если бит DRDY в регистре STATUS установлен.
    auto is_data_ready() -> bool
    {
        const auto status = this->template read<rm3100_status_reg>();
        return status.drdy;
    }

    /// @brief Считывает регистр STATUS.
    ///
    /// @return Текущее значение регистра STATUS.
    [[nodiscard]] auto read_status_reg() -> rm3100_status_reg
    { return this->template read<rm3100_status_reg>(); }

    /// @brief Считывает регистр REVID.
    ///
    /// @return Текущее значение регистра REVID.
    [[nodiscard]] auto read_revid_reg() -> rm3100_revid_reg
    { return this->template read<rm3100_revid_reg>(); }

    /// @brief Считывает регистр BIST.
    ///
    /// @return Текущее значение регистра BIST.
    [[nodiscard]] auto read_bist_reg() -> rm3100_bist_reg
    { return this->template read<rm3100_bist_reg>(); }

    /// @brief Запускает встроенный самотест (BIST) RM3100.
    ///
    /// @details
    ///     Последовательность согласно документации:
    ///     1. Останавливает непрерывный режим (CMM), если он активен.
    ///     2. Записывает BIST с STE = 1 (разрешение самотеста при записи POLL).
    ///     3. Записывает POLL для запуска одиночного измерения по выбранным
    ///     осям.
    ///     4. Ожидает подъёма DRDY, опрашивая STATUS с задержкой @p delay_ms.
    ///     5. Считывает BIST и проверяет флаги XOK/YOK/ZOK для протестированных
    ///        осей.
    ///     6. Сбрасывает STE в BIST для возврата в обычный режим работы.
    ///
    /// @tparam TDelayFnMs Тип функции задержки: вызывается с аргументом в
    /// миллисекундах.
    ///
    /// @param[in] bist Настройки BIST. Поле @c ste должно быть установлено.
    /// @param[in] poll Настройки одиночного измерения: какие оси тестировать.
    /// @param[in] timeout_ms Таймаут ожидания DRDY, мс.
    /// @param[in] delay_ms Функция задержки в миллисекундах.
    /// @return true, если все запрошенные оси успешно прошли самотест.
    template<rm3100_delayable TDelayFnMs>
    auto run_self_test(
        const rm3100_bist_reg &bist, const rm3100_poll_reg &poll,
        std::uint16_t timeout_ms, TDelayFnMs &delay_ms) -> bool
    {
        if(!bist.ste || (!poll.pmx && !poll.pmy && !poll.pmz))
        {
            return false;
        }

        if(is_cmm_active_ && !stop_continuous())
        {
            return false;
        }

        if(!this->write(bist))
        {
            return false;
        }

        if(!this->write(poll))
        {
            const rm3100_bist_reg clear_reg{};
            (void)this->write(clear_reg);
            return false;
        }

        const auto            drdy   = wait_drdy(timeout_ms, delay_ms);
        const auto            result = this->template read<rm3100_bist_reg>();
        const rm3100_bist_reg clear_reg{};
        (void)this->write(clear_reg);

        if(!drdy || !result.ste)
        {
            return false;
        }

        return are_bist_axes_ok(result, poll);
    }

    /// @brief Запускает встроенный самотест по всем осям с настройками по
    /// умолчанию.
    ///
    /// @details
    ///     Использует таймаут по умолчанию
    ///     @c default_self_test_timeout_ms (100 мс) и тестирует оси X, Y, Z.
    ///
    /// @tparam TDelayFnMs Тип функции задержки: вызывается с аргументом в
    /// миллисекундах.
    ///
    /// @param[in] delay_ms Функция задержки в миллисекундах.
    /// @return true, если X, Y и Z оси успешно прошли самотест.
    template<rm3100_delayable TDelayFnMs>
    [[nodiscard]] auto run_self_test(
        TDelayFnMs &delay_ms) -> bool
    {
        rm3100_bist_reg bist{};
        bist.ste = true;
        bist.bw  = default_bist_bw;
        bist.bp  = default_bist_bp;

        rm3100_poll_reg poll{};
        poll.pmx = true;
        poll.pmy = true;
        poll.pmz = true;

        return run_self_test(bist, poll, default_self_test_timeout_ms,
                             delay_ms);
    }

    /// @brief Запускает встроенный самотест по всем осям с заданным
    /// таймаутом.
    ///
    /// @tparam TDelayFnMs Тип функции задержки: вызывается с аргументом в
    /// миллисекундах.
    ///
    /// @param[in] timeout_ms Таймаут ожидания DRDY, мс.
    /// @param[in] delay_ms Функция задержки в миллисекундах.
    /// @return true, если X, Y и Z оси успешно прошли самотест.
    template<rm3100_delayable TDelayFnMs>
    [[nodiscard]] auto run_self_test(
        std::uint16_t timeout_ms, TDelayFnMs &delay_ms) -> bool
    {
        rm3100_bist_reg bist{};
        bist.ste = true;
        bist.bw  = default_bist_bw;
        bist.bp  = default_bist_bp;

        rm3100_poll_reg poll{};
        poll.pmx = true;
        poll.pmy = true;
        poll.pmz = true;

        return run_self_test(bist, poll, timeout_ms, delay_ms);
    }

    /// @brief Считывает сырое 24-битное значение магнитного поля.
    ///
    /// @details
    ///     Читает 9 байт из регистров измерений, начиная с адреса 0x24,
    ///     выполняет знаковое расширение 24-битных значений и проверяет их
    ///     корректность через @ref raw_t::operator bool(). При успешном чтении
    ///     увеличивается внутренняя метка времени.
    ///
    /// @return Структура @ref raw_t с компонентами X, Y, Z.
    auto read_meas_raw() -> raw_t
    {
        raw_bytes_t bytes{};
        if(!this->read(meas_result_base_addr, &bytes, sizeof(bytes)))
        {
            return raw_t{};
        }

        raw_t raw{};
        raw.x = sign_extend_24((static_cast<std::uint32_t>(bytes.mx2) << 16U)
                               | (static_cast<std::uint32_t>(bytes.mx1) << 8U)
                               | static_cast<std::uint32_t>(bytes.mx0));
        raw.y = sign_extend_24((static_cast<std::uint32_t>(bytes.my2) << 16U)
                               | (static_cast<std::uint32_t>(bytes.my1) << 8U)
                               | static_cast<std::uint32_t>(bytes.my0));
        raw.z = sign_extend_24((static_cast<std::uint32_t>(bytes.mz2) << 16U)
                               | (static_cast<std::uint32_t>(bytes.mz1) << 8U)
                               | static_cast<std::uint32_t>(bytes.mz0));

        if(raw)
        {
            ++timestamp_;
        }
        else
        {
            raw = raw_t{};
        }

        return raw;
    }

    /// @brief Преобразует сырые значения в нормализованные единицы Гаусса.
    ///
    /// @details
    ///     Каждая компонента вычисляется по формуле
    ///     `value_gs = raw / (lsb_per_ut * 100)`, где @ref lsb_per_ut_ —
    ///     масштаб в LSB/мкТл, соответствующий текущему cycle count, а
    ///     множитель 100 переводит микротеслы в Гауссы (1 Гс = 100 мкТл).
    ///     В результат также передаётся текущая метка времени.
    ///
    /// @param[in] raw Сырые 24-битные значения по осям.
    ///
    /// @return Структура @ref mag_type с нормализованными измерениями.
    auto normalize(
        const raw_t &raw) -> mag_type
    {
        const auto scale =
            static_cast<value_type>(1.0F) / (lsb_per_ut_ * 100.0F);
        return mag_type{static_cast<value_type>(raw.x) * scale,
                        static_cast<value_type>(raw.y) * scale,
                        static_cast<value_type>(raw.z) * scale, timestamp_};
    }

    /// @brief Считывает и нормализует текущее измерение магнитного поля.
    ///
    /// @details
    ///     Вызывает @ref read_meas_raw(). Если сырые данные корректны,
    ///     нормализует их и сохраняет внутреннее значение @ref mag_. Если
    ///     данные некорректны, возвращает последнее успешное измерение.
    ///
    /// @return
    ///     Структура @ref mag_type. Оператор `bool` результата показывает,
    ///     были ли данные успешно считаны.
    auto read_normalized() -> mag_type
    {
        const auto raw = read_meas_raw();
        if(raw)
        {
            mag_ = normalize(raw);
        }
        return mag_;
    }

    /// @brief Возвращает последнее сохранённое измерение.
    ///
    /// @details
    ///     Реализация интерфейса @ref stv::imag. Возвращает кэш
    ///     @ref mag_, который может содержать устаревшее значение, если
    ///     последнее чтение из датчика завершилось ошибкой.
    ///
    /// @return
    ///     Структура @ref mag_type с последним успешным измерением.
    [[nodiscard]] auto get_mag() const -> mag_type override { return mag_; }
};

} // namespace stv

#endif /* RM3100_HPP */
