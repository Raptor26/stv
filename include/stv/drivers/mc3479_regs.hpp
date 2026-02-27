/// @file mc3479_regs.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
/// @brief Описание регистров и битовых полей акселерометра MC3479.
/// @details
/// Файл содержит C++-представления регистров MC3479 в виде `struct` с
/// битовыми полями. Эти типы удобно использовать для:
/// - формирования байта конфигурации перед записью в устройство по I2C;
/// - разборки прочитанного байта статуса/настроек в именованные поля;
/// - хранения набора настроек (см. `mc3479_regs_setup`).
///
/// Важно:
/// - Все структуры рассчитаны на размер 1 байт (`static_assert`).
/// - Поля `addr` задают адрес регистра в карте MC3479.
/// - Поля `zero_*`/`reserve`/`reserved` должны оставаться равными 0.
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.

#ifndef MC3479_REGS_HPP
#define MC3479_REGS_HPP

#include "stv/drivers/mc3479_types.hpp"
#include <cstdint>

namespace stv {

/// @brief Состояние (режим) работы MC3479.
/// @details
/// Значение используется в регистрах статуса и режима (например 0x05/0x07).
/// Режим определяет, работает ли тактирование и выполняется ли выборка
/// данных по осям X/Y/Z.
enum class mc3479_state : std::uint8_t {
    /// Устройство спит: тактирование остановлено, выборка данных не ведётся.
    sleep = 0,
    /// Устройство активно: выборка X/Y/Z идёт с настроенной частотой.
    wake,
    /// Зарезервировано производителем. Не используйте.
    reserved,
    /// STANDBY: тактирование есть, но данные X/Y/Z не семплируются.
    standby,
};

/// @brief Регистр статуса устройства (device status).
/// @details
/// Представляет байт статуса MC3479. Обычно читается для определения:
/// - текущего режима `state` (sleep/wake/standby);
/// - наличия события watchdog по I2C (`i2c_wdt`);
/// - занятости OTP-памяти (`otp_busy`);
/// - режима разрешения данных (`res_mode`).
///
/// Поля `zero_*` всегда должны быть 0 и игнорируются при чтении.
struct mc3479_device_status_reg {
    static constexpr mc3479_reg_type addr{0x05};

    /// @brief Текущее состояние (режим) акселерометра.
    /// @details Значение кодируется в 2 бита.
    mc3479_state state : 2 {stv::mc3479_state::sleep};

    /// @brief Режим разрешения (Resolution mode) устройства.
    /// @details
    /// - 0: включено 16-bit (high) resolution.
    /// - 1: Reserved (не используйте).
    bool res_mode : 1 {false};

    /// @brief Зарезервированный бит, должен быть 0.
    const std::uint8_t zero_1 : 1 {0};

    /// @brief Флаг срабатывания I2C watchdog (timeout).
    /// @details
    /// Бит очищается при чтении регистра 0x05.
    /// - 0: watchdog не обнаружен.
    /// - 1: watchdog обнаружен; I2C slave FSM сброшен в idle.
    bool i2c_wdt : 1 {false};

    /// @brief Зарезервированные биты, должны быть 0.
    const std::uint8_t zero_2 : 2 {0};

    /// @brief Статус активности One-Time Programming (OTP).
    /// @details
    /// - 0: внутренняя память idle, устройство готово к работе.
    /// - 1: внутренняя память занята, устройство нельзя использовать.
    bool otp_busy : 1 {false};
};

static_assert(sizeof(mc3479_device_status_reg) == 1, "reg size must be 1");

// -----------------------------------------------------------------------------

/// @brief Регистр разрешения прерываний (interrupt enable), адрес 0x06.
/// @details
/// Управляет тем, какие события motion-блока будут "репортиться" в статусе и
/// вызывать прерывание. Многие биты работают совместно с настройками в
/// `mc3479_motion_control_reg` (0x09).
///
/// Типовой сценарий:
/// 1) включить нужные функции в 0x09 (tilt/flip/anym/shake/tilt-35);
/// 2) разрешить нужные прерывания в этом регистре (0x06);
/// 3) опрашивать `mc3479_status_reg` (0x13) или использовать вывод INT.
struct mc3479_interrupt_enable_reg {
    /// @brief Адрес регистра в карте MC3479.
    static constexpr mc3479_reg_type addr{0x06};

    /// @brief Разрешить репортинг tilt interrupt.
    /// @details Используется вместе с tilt/flip в 0x09 (bit 0).
    bool tilt_int_en : 1 {false};

    /// @brief Разрешить репортинг flip interrupt.
    /// @details Используется вместе с tilt/flip в 0x09 (bit 0).
    bool flip_int_en : 1 {false};

    /// @brief Разрешить репортинг AnyMotion interrupt.
    /// @details Используется вместе с AnyMotion в 0x09 (bit 2).
    bool anym_int_enable : 1 {false};

    /// @brief Разрешить репортинг shake interrupt.
    /// @details Требует включённых shake (0x09 bit 3) и AnyMotion (0x09 bit 2).
    bool shake_int_enable : 1 {false};

    /// @brief Разрешить репортинг tilt-35 interrupt.
    /// @details Требует tilt-35 (0x09 bit 4) и AnyMotion (0x09 bit 2).
    bool tilt_35_int_enable : 1 {false};

    /// @brief Зарезервировано, должно быть 0.
    const std::uint8_t reserved : 1 {0};

    /// @brief Автоочистка "pending" прерываний.
    /// @details
    /// Прерывания можно очищать автоматически или чтением регистра.
    /// Одновременное включение нескольких механизмов service/timeout может
    /// давать неожиданные эффекты.
    bool auto_clr_en : 1 {false};

    /// @brief Разрешить генерацию interrupts при acquisition.
    /// @details Конкретное поведение зависит от режима работы и прошивки ИС.
    bool acq_int_en : 1 {false};
};

static_assert(sizeof(mc3479_interrupt_enable_reg) == 1, "reg size must be 1");

// -----------------------------------------------------------------------------

/// @brief Регистр режима работы (mode), адрес 0x07.
/// @details
/// Основной регистр управления состоянием акселерометра. Поле `state`
/// переводит устройство в SLEEP/WAKE/STANDBY. Также присутствуют биты
/// сторожевого таймера I2C (watchdog) для "stall" по SCL.
struct mc3479_mode_reg {
    /// @brief Адрес регистра в карте MC3479.
    static constexpr mc3479_reg_type addr{0x07};

    /// @brief Операционное состояние акселерометра.
    /// @details
    /// - 00: SLEEP. Тактирование остановлено, X/Y/Z не семплируются.
    /// - 01: WAKE. Тактирование есть, X/Y/Z семплируются с sample rate.
    /// - 10: Reserved (не используйте).
    /// - 11: STANDBY. Тактирование есть, но X/Y/Z не семплируются.
    mc3479_state state : 2 {stv::mc3479_state::wake};

    /// @brief Зарезервированные биты, должны быть 0.
    const std::uint8_t zero_1 : 2 {0};

    /// @brief Watchdog для отрицательных stalls SCL.
    /// @details
    /// - 0: watchdog отключён (default).
    /// - 1: watchdog включён.
    bool wtd_neg : 1 {false};

    /// @brief Watchdog для положительных stalls SCL.
    /// @details
    /// - 0: watchdog отключён (default).
    /// - 1: watchdog включён.
    bool wtd_pos : 1 {false};

    /// @brief Зарезервированные биты, должны быть 0.
    const std::uint8_t zero_2 : 2 {0};
};

static_assert(sizeof(mc3479_mode_reg) == 1, "reg size must be 1");

// -----------------------------------------------------------------------------

/// @brief Перечень кодов частоты дискретизации (Output Data Rate).
/// @details
/// Значения соответствуют кодам, записываемым в поле `rate` регистра 0x08.
/// Названия `hz_*` указывают целевую частоту выдачи измерений.
enum class mc3479_sample_rate : uint8_t {
    hz_50   = 0x08, ///< 50 Гц
    hz_100  = 0x09, ///< 100 Гц
    hz_125  = 0x0A, ///< 125 Гц
    hz_200  = 0x0B, ///< 200 Гц
    hz_250  = 0x0C, ///< 250 Гц
    hz_500  = 0x0D, ///< 500 Гц
    hz_1000 = 0x0E, ///< 1000 Гц (значение по умолчанию)
    hz_2000 = 0x0F, ///< 2000 Гц
};

/// @brief Регистр частоты семплирования (sample rate), адрес 0x08.
/// @details
/// Используется для выбора ODR устройства. Поле `reserve` должно быть 0.
/// Обычно этот регистр настраивается в режиме STANDBY, а затем устройство
/// переводится в WAKE.
struct mc3479_sample_rate_reg {
    /// @brief Адрес регистра в карте MC3479.
    static constexpr mc3479_reg_type addr{0x08};

    /// @brief Код частоты семплирования (ODR).
    mc3479_sample_rate rate : 4 {stv::mc3479_sample_rate::hz_1000};

    /// @brief Зарезервировано, должно быть 0.
    std::uint8_t reserve : 4 {0};
};

static_assert(sizeof(mc3479_sample_rate_reg) == 1, "reg size must be 1");

// -----------------------------------------------------------------------------

/// @brief Регистр управления motion-функциями, адрес 0x09.
/// @details
/// Включает/отключает алгоритмы определения движения (tilt/flip/AnyMotion,
/// shake, tilt-35), задаёт ориентацию Z и режим обработки флагов.
///
/// Практика:
/// - Сначала включите нужные функции (`tf_enable`, `anym_en`, ...).
/// - Затем разрешите репортинг соответствующих прерываний в 0x06.
/// - Считывайте `mc3479_status_reg` (0x13) для получения флагов.
struct mc3479_motion_control_reg {
    /// @brief Адрес регистра в карте MC3479.
    static constexpr mc3479_reg_type addr{0x09};

    /// @brief Включение функции наклона/переворота (tilt/flip).
    /// @details Используется совместно с регистрами 0x13, 0x14 и 0x06.
    /// - 0: Функция Tilt/Flip отключена (значение по умолчанию).
    /// - 1: Функция Tilt/Flip включена.
    bool tf_enable : 1 {false};

    /// @brief Режим фиксации выходов блока движения.
    /// @details Если используются прерывания движения, этот бит обычно
    /// не используется.
    /// - 0: Блок движения не фиксирует выходы.
    /// - 1: Блок движения фиксирует выходы до сброса.
    bool motion_latch : 1 {false};

    /// @brief Включить/выключить AnyMotion feature.
    /// @details
    /// Используется совместно с AnyMotion в 0x13/0x14/0x06, а также с shake и
    /// tilt-35 (0x14/0x06).
    /// - 0: выключено (default).
    /// - 1: включено.
    bool anym_en : 1 {false};

    /// @brief Включение функции обнаружения встряхивания (shake).
    /// @details Используется совместно с регистрами 0x13, 0x14 и 0x06.
    /// Требует включения ANYM_EN.
    /// - 0: Функция Shake отключена (значение по умолчанию).
    /// - 1: Функция Shake включена.
    bool shake_en : 1 {false};

    /// @brief Включить/выключить tilt-35 feature.
    /// @details
    /// Используется совместно с настройками tilt-35 в 0x13/0x14/0x06.
    /// - 0: выключено (default).
    /// - 1: включено. Требует `anym_en == 1`.
    bool tilt35_en : 1 {false};

    /// @brief Ориентация оси Z акселерометра.
    /// @details Определяет направление положительного значения ускорения
    /// по оси Z относительно корпуса устройства.
    /// - 0: Положительное направление через верх корпуса (по умолчанию).
    /// - 1: Положительное направление через низ корпуса.
    bool z_axis_ort : 1 {false};

    /// @brief Включение/отключение фильтрации данных движения.
    /// @details Управляет обработкой флагов движения.
    /// - 0: Флаги фильтруются через debounce и другие настройки (по умолчанию).
    /// - 1: Флаги передаются как необработанные данные в реальном времени.
    bool raw_proc_stat : 1 {false};

    /// @brief Сброс motion-блока (не очищается автоматически).
    /// @details
    /// - 0: сброс не активен (default).
    /// - 1: motion-блок удерживается в reset. ПО должно снять reset, записав
    ///   соответствующее значение в этот бит.
    bool motion_reset : 1 {false};
};

static_assert(sizeof(mc3479_motion_control_reg) == 1, "reg size must be 1");

// -----------------------------------------------------------------------------

/// @brief Регистр флагов событий (status), адрес 0x13.
/// @details
/// Содержит флаги детектированных событий движения и служебные флаги.
/// Эти биты обычно читаются после возникновения прерывания или при опросе.
///
/// Очистка флагов зависит от настроек auto clear (см. 0x06 `auto_clr_en`)
/// и механизма, описанного в документации MC3479.
struct mc3479_status_reg {
    /// @brief Адрес регистра в карте MC3479.
    static constexpr mc3479_reg_type addr{0x13};

    /// @brief Флаг события tilt.
    bool tilt_flag : 1 {false};

    /// @brief Флаг события flip.
    bool flip_flag : 1 {false};

    /// @brief Флаг события AnyMotion.
    bool anym_flag : 1 {false};

    /// @brief Флаг события shake.
    bool shake_flag : 1 {false};

    /// @brief Флаг события tilt-35.
    bool tilt35_flag : 1 {false};

    /// @brief Флаг FIFO (например, заполнение/порог), если поддерживается.
    bool fifo_flag : 1 {false};

    /// @brief Зарезервировано, должно быть 0.
    std::uint8_t reserved : 1 {0};

    /// @brief Признак "есть новые данные".
    /// @details Обычно сигнализирует, что выборка X/Y/Z обновилась.
    bool new_data : 1 {false};
};

static_assert(sizeof(mc3479_status_reg) == 1, "reg size must be 1");

// -----------------------------------------------------------------------------

/// @brief Настройка LPF (Low-Pass Filter) для канала ускорений.
/// @details
/// Значения соответствуют кодам в регистре диапазона/масштаба (0x20).
/// Имена оставлены как в даташите (например, `IDRdev6`) для связности.
enum class mc3479_lpf : uint8_t {
    /// Зарезервировано. Используйте только при явной необходимости.
    reserve = 0,
    /// LPF = IDR/dev4.255 (см. даташит MC3479).
    IDRdev4255 = 1,
    /// LPF = IDR/dev6 (см. даташит MC3479).
    IDRdev6 = 2,
    /// LPF = IDR/dev12 (см. даташит MC3479).
    IDRdev12 = 3,
    /// LPF = IDR/dev16 (см. даташит MC3479).
    IDRdev16 = 5,
};

/// @brief Диапазон измерения ускорения (full-scale range).
/// @details
/// Используется в регистре 0x20. Выбор диапазона влияет на масштаб
/// преобразования "сырых" значений в g.
enum class mc3479_range : uint8_t {
    /// Диапазон ±2g.
    g2 = 0,
    /// Диапазон ±4g.
    g4 = 1,
    /// Диапазон ±8g.
    g8 = 2,
    /// Диапазон ±16g.
    g16 = 3,
    /// Диапазон ±12g (если поддерживается конкретной ревизией).
    g12 = 4,
};

/// @brief Регистр диапазона и масштаба, адрес 0x20.
/// @details
/// Управляет:
/// - параметрами LPF (`lpf`, `lpf_enable`);
/// - диапазоном измерения (`range`).
///
/// Поле `reserve` должно быть 0.
///
/// Рекомендация:
/// - Выбирайте диапазон минимально возможный под ожидаемые ускорения, чтобы
///   повысить разрешение.
/// - LPF включайте при необходимости подавления шума/вибраций.
struct mc3479_range_and_scale_control_reg {
    /// @brief Адрес регистра в карте MC3479.
    static constexpr mc3479_reg_type addr{0x20};

    /// @brief Выбор коэффициента LPF.
    mc3479_lpf lpf : 3 {mc3479_lpf::reserve};

    /// @brief Включение LPF.
    /// @details
    /// - 0: LPF выключен.
    /// - 1: LPF включён, применяется значение `lpf`.
    bool lpf_enable : 1 {false};

    /// @brief Выбор full-scale диапазона (±2g/±4g/±8g/±16g/...).
    mc3479_range range : 3 {mc3479_range::g2};

    /// @brief Зарезервировано, должно быть 0.
    std::uint8_t reserve : 1 {0};
};

static_assert(sizeof(mc3479_range_and_scale_control_reg) == 1,
              "reg size must be 1");

// -----------------------------------------------------------------------------

/// @brief Набор настроек регистров MC3479 для первичной конфигурации.
/// @details
/// Структура агрегирует наиболее часто настраиваемые регистры в одном месте.
/// Её удобно использовать как "профиль" конфигурации:
/// - заполнить поля нужными значениями;
/// - последовательно записать каждый регистр по адресу `addr`;
/// - при необходимости хранить несколько профилей (например, low-power и
///   high-performance).
///
/// Порядок записи обычно выбирается проектом драйвера. Часто удобно:
/// 1) перевести устройство в STANDBY (mode.state);
/// 2) настроить sample rate, range, motion, interrupts;
/// 3) перевести в WAKE.
struct mc3479_regs_setup {
    /// @brief Регистр режима (0x07): state и I2C watchdog.
    stv::mc3479_mode_reg mode;

    /// @brief Регистр разрешения прерываний (0x06).
    stv::mc3479_interrupt_enable_reg interrupt_enable;

    /// @brief Регистр частоты семплирования (0x08).
    stv::mc3479_sample_rate_reg sample_rate;

    /// @brief Регистр управления motion-функциями (0x09).
    stv::mc3479_motion_control_reg motion_control;

    /// @brief Регистр диапазона и LPF (0x20).
    stv::mc3479_range_and_scale_control_reg range_and_scale_control;
};

} // namespace stv

#endif /* MC3479_REGS_HPP */
