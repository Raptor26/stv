/// @file rm3100_regs.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.
///
/// @brief Описание регистров магнитометра PNI RM3100.
///
/// @details
///     Заголовок содержит типы, представляющие регистры RM3100: POLL, CMM,
///     CCx, TMRC, STATUS, BIST и REVID. Каждый класс регистра отвечает за
///     корректное упаковывание и распаковывание битового поля при обмене с
///     датчиком по I2C. Используются классом @ref stv::rm3100 и прикладным
///     кодом для настройки режимов измерения.
///
///     Карта регистров, описанных в этом файле:
///     - POLL (0x00) — запрос одиночного измерения, @ref rm3100_poll_reg;
///     - CMM (0x01) — непрерывный режим измерений, @ref rm3100_cmm_reg;
///     - CCX/CCY/CCZ (0x04–0x09) — cycle count по осям X, Y, Z:
///       @ref rm3100_ccx_reg, @ref rm3100_ccy_reg, @ref rm3100_ccz_reg;
///     - TMRC (0x0B) — частота обновления в режиме CMM,
///       @ref rm3100_tmrc_reg;
///     - BIST (0x33) — встроенный самотест, @ref rm3100_bist_reg;
///     - STATUS (0x34) — флаг готовности данных, @ref rm3100_status_reg;
///     - REVID (0x36) — идентификатор ревизии, @ref rm3100_revid_reg.
///
///     Однобайтовые регистры сериализуются через @c etl::bit_stream_writer
///     и @c etl::bit_stream_reader: первым передаётся старший бит (bit 7),
///     поэтому поля записываются и считываются от старшего бита к младшему.

#ifndef RM3100_REGS_HPP
#define RM3100_REGS_HPP

#include "etl/bit_stream.h"
#include "rm3100_types.hpp"
#include <array>
#include <cstdint>

namespace stv {

// NOLINTBEGIN(*-member*)

/// @brief Регистр запроса одиночного измерения (POLL), адрес 0x00.
///
/// @details
///     Запись в этот регистр запускает одиночное измерение по выбранным
///     осям: установка бита PMX/PMY/PMZ в «1» заказывает измерение по
///     соответствующей оси X/Y/Z. Когда все заказанные измерения
///     завершены, датчик поднимает линию DRDY, после чего результаты
///     доступны в регистрах измерений (0x24–0x2C).
///
///     Битовая раскладка: PMX[4], PMY[5], PMZ[6]; биты 0–3 и 7 должны
///     быть 0.
///
///     Запись в POLL игнорируется датчиком, пока активен непрерывный
///     режим (CMM), поэтому драйвер предварительно останавливает CMM в
///     методе @ref stv::rm3100::request_single().
///
/// @code
/// // Запрос одиночного измерения по всем трём осям через драйвер.
/// stv::rm3100_poll_reg poll{};
/// poll.pmx = true;
/// poll.pmy = true;
/// poll.pmz = true;
/// if(sensor.request_single(poll)) {
///     // измерение запущено; дождаться DRDY и прочитать результат
/// }
/// @endcode
class rm3100_poll_reg
{
  public:
    /// @brief Адрес регистра POLL.
    static constexpr rm3100_reg_type addr{0x00};

    /// @brief Конструктор по умолчанию.
    ///
    /// @details
    ///     Создаёт регистр со сброшенными флагами осей (сырое значение
    ///     0x00).
    rm3100_poll_reg() = default;

    /// @brief Конструктор из сырого значения регистра.
    ///
    /// @details
    ///     Распаковывает биты PMX/PMY/PMZ из переданного байта.
    ///
    /// @param[in] value Значение, считанное из регистра POLL.
    explicit rm3100_poll_reg(
        rm3100_reg_type value)
    { parse(value); }

    /// @brief Преобразует регистр в сырое байтовое значение.
    ///
    /// @details
    ///     Упаковывает поля в байт через @c etl::bit_stream_writer;
    ///     первым записывается старший бит (bit 7).
    ///
    /// @return Значение POLL для записи в датчик.
    explicit operator rm3100_reg_type() const
    {
        std::array<rm3100_reg_type, 1U> reg{0};
        etl::bit_stream_writer          bit_stream{std::to_address(reg.begin()),
                                                   std::to_address(reg.end()),
                                                   etl::endian::little};
        // Первым записывается старший бит.
        bit_stream.write(false);
        bit_stream.write(pmz);
        bit_stream.write(pmy);
        bit_stream.write(pmx);
        bit_stream.write(false);
        bit_stream.write(false);
        bit_stream.write(false);
        bit_stream.write(false);

        return *reg.data();
    }

    /// @brief Присваивает регистру сырое значение.
    ///
    /// @param[in] value Значение, считанное из регистра POLL.
    /// @return Ссылка на текущий объект.
    auto operator=(
        rm3100_reg_type value) -> rm3100_poll_reg &
    {
        parse(value);
        return *this;
    }

    /// @brief Сравнивает два регистра POLL.
    ///
    /// @param[in] other Другой регистр POLL.
    /// @return `true`, если побитовые представления равны.
    auto operator==(
        const rm3100_poll_reg &other) const -> bool
    {
        return static_cast<rm3100_reg_type>(*this)
               == static_cast<rm3100_reg_type>(other);
    }

    /// @brief Запросить одиночное измерение по оси X (бит PMX, позиция 4).
    bool pmx{false};

    /// @brief Запросить одиночное измерение по оси Y (бит PMY, позиция 5).
    bool pmy{false};

    /// @brief Запросить одиночное измерение по оси Z (бит PMZ, позиция 6).
    bool pmz{false};

  private:
    /// @brief Распаковывает сырое значение регистра в битовые поля.
    ///
    /// @details
    ///     Считывает биты PMX/PMY/PMZ через @c etl::bit_stream_reader;
    ///     первым считывается старший бит (bit 7).
    ///
    /// @param[in] value Сырое значение регистра POLL.
    void parse(
        rm3100_reg_type value)
    {
        std::array<rm3100_reg_type, 1U> reg{value};
        etl::bit_stream_reader          bit_stream{std::to_address(reg.begin()),
                                                   std::to_address(reg.end()),
                                                   etl::endian::little};
        // Первым считывается старший бит.
        bit_stream.skip(1);
        pmz = bit_stream.read_unchecked<bool>();
        pmy = bit_stream.read_unchecked<bool>();
        pmx = bit_stream.read_unchecked<bool>();
        bit_stream.skip(4);
    }
};

/// @brief Условие подъёма линии DRDY в непрерывном режиме (поле DRDM
/// регистра CMM).
///
/// @details
///     Определяет, когда датчик переводит линию готовности данных DRDY в
///     высокий уровень: после завершения измерений по всем включённым
///     осям (CMX/CMY/CMZ) или после завершения измерения по любой из них.
enum class rm3100_cmm_drdm : std::uint8_t {
    after_all_axes =
        0, ///< DRDY поднимается после завершения всех включённых осей.
    after_any_axis = 1, ///< DRDY поднимается после завершения любой оси.
};

/// @brief Регистр непрерывного режима измерений (CMM), адрес 0x01.
///
/// @details
///     Управляет непрерывными измерениями: бит START запускает и
///     останавливает режим, биты CMX/CMY/CMZ выбирают измеряемые оси, а
///     поле DRDM определяет, когда поднимается линия готовности данных
///     DRDY (см. @ref rm3100_cmm_drdm).
///
///     Битовая раскладка: START[0], DRDM[2], CMX[4], CMY[5], CMZ[6];
///     биты 1, 3 и 7 зарезервированы и должны быть 0.
///
///     Частота измерений в непрерывном режиме задаётся регистром TMRC
///     (см. @ref rm3100_tmrc_reg). Обратите внимание: чтение регистра CMM
///     или запись в TMRC завершают непрерывный режим, после таких операций
///     режим необходимо запустить заново.
///
/// @code
/// // Настройка непрерывного режима по всем осям и его запуск.
/// stv::rm3100_regs_setup regs{};
/// regs.cmm.start = true;
/// regs.cmm.cmx   = true;
/// regs.cmm.cmy   = true;
/// regs.cmm.cmz   = true;
/// regs.cmm.drdm  = stv::rm3100_cmm_drdm::after_all_axes;
///
/// if(sensor.init(regs)) {
///     sensor.start_continuous();
/// }
/// @endcode
class rm3100_cmm_reg
{
  public:
    /// @brief Адрес регистра CMM.
    static constexpr rm3100_reg_type addr{0x01};

    /// @brief Конструктор по умолчанию.
    ///
    /// @details
    ///     Создаёт регистр с сброшенным битом запуска и выключенными
    ///     осями (сырое значение 0x00).
    rm3100_cmm_reg() = default;

    /// @brief Конструктор из сырого значения регистра.
    ///
    /// @details
    ///     Распаковывает биты START, DRDM и CMX/CMY/CMZ из переданного
    ///     байта.
    ///
    /// @param[in] value Значение, считанное из регистра CMM.
    explicit rm3100_cmm_reg(
        rm3100_reg_type value)
    { parse(value); }

    /// @brief Преобразует регистр в сырое байтовое значение.
    ///
    /// @details
    ///     Упаковывает поля в байт через @c etl::bit_stream_writer;
    ///     первым записывается старший бит (bit 7).
    ///
    /// @return Значение CMM для записи в датчик.
    explicit operator rm3100_reg_type() const
    {
        std::array<rm3100_reg_type, 1U> reg{0};
        etl::bit_stream_writer          bit_stream{std::to_address(reg.begin()),
                                                   std::to_address(reg.end()),
                                                   etl::endian::little};
        // Первым записывается старший бит.
        bit_stream.write(false);
        bit_stream.write(cmz);
        bit_stream.write(cmy);
        bit_stream.write(cmx);
        bit_stream.write(false);
        bit_stream.write(static_cast<std::uint8_t>(drdm), 1U);
        bit_stream.write(false);
        bit_stream.write(start);

        return *reg.data();
    }

    /// @brief Присваивает регистру сырое значение.
    ///
    /// @param[in] value Значение, считанное из регистра CMM.
    /// @return Ссылка на текущий объект.
    auto operator=(
        rm3100_reg_type value) -> rm3100_cmm_reg &
    {
        parse(value);
        return *this;
    }

    /// @brief Сравнивает два регистра CMM.
    ///
    /// @param[in] other Другой регистр CMM.
    /// @return `true`, если побитовые представления равны.
    auto operator==(
        const rm3100_cmm_reg &other) const -> bool
    {
        return static_cast<rm3100_reg_type>(*this)
               == static_cast<rm3100_reg_type>(other);
    }

    /// @brief Запуск непрерывного режима измерений (бит START, позиция 0):
    /// `true` — режим запущен, `false` — остановлен.
    bool start{false};

    /// @brief Условие подъёма линии DRDY (поле DRDM, позиция 2).
    rm3100_cmm_drdm drdm{rm3100_cmm_drdm::after_all_axes};

    /// @brief Включить ось X в непрерывные измерения (бит CMX, позиция 4).
    bool cmx{false};

    /// @brief Включить ось Y в непрерывные измерения (бит CMY, позиция 5).
    bool cmy{false};

    /// @brief Включить ось Z в непрерывные измерения (бит CMZ, позиция 6).
    bool cmz{false};

  private:
    /// @brief Распаковывает сырое значение регистра в битовые поля.
    ///
    /// @details
    ///     Считывает биты START, DRDM и CMX/CMY/CMZ через
    ///     @c etl::bit_stream_reader; первым считывается старший бит
    ///     (bit 7).
    ///
    /// @param[in] value Сырое значение регистра CMM.
    void parse(
        rm3100_reg_type value)
    {
        std::array<rm3100_reg_type, 1U> reg{value};
        etl::bit_stream_reader          bit_stream{std::to_address(reg.begin()),
                                                   std::to_address(reg.end()),
                                                   etl::endian::little};
        // Первым считывается старший бит.
        bit_stream.skip(1);
        cmz = bit_stream.read_unchecked<bool>();
        cmy = bit_stream.read_unchecked<bool>();
        cmx = bit_stream.read_unchecked<bool>();
        bit_stream.skip(1);
        drdm = static_cast<decltype(drdm)>(bit_stream.read_unchecked<bool>());
        bit_stream.skip(1);
        start = bit_stream.read_unchecked<bool>();
    }
};

/// @brief Регистр cycle count по оси X (CCX), адреса 0x04 (MSB) и
/// 0x05 (LSB).
///
/// @details
///     Cycle count задаёт число циклов генератора (накоплений), считаемых
///     при одном измерении по оси в прямом и обратном смещении. Это
///     16-битное значение: старший байт (MSB) записывается по адресу
///     @ref addr (0x04), младший (LSB) — по адресу 0x05. Увеличение cycle
///     count повышает усиление и разрешение измерения, уменьшение —
///     сокращает время измерения и потребление. Практический диапазон —
///     примерно от 30 (ограничение квантования) до 400 (шумовой предел).
///     Значение по умолчанию 200 (0x00C8) соответствует чувствительности
///     примерно 75 LSB/мкТл.
///
///     Регистры CCX–CCZ идут подряд (0x04–0x09), поэтому драйвер читает
///     все шесть байт одной транзакцией, а записывает cycle count каждой
///     оси двумя байтами (MSB, затем LSB) при инициализации.
///
/// @code
/// // Установка cycle count 200 по всем осям через настройки драйвера.
/// stv::rm3100_regs_setup regs{};
/// regs.ccx.cycle_count = 200U;
/// regs.ccy.cycle_count = 200U;
/// regs.ccz.cycle_count = 200U;
/// sensor.init(regs); // записывает CCX/CCY/CCZ с контрольным чтением
/// @endcode
struct rm3100_ccx_reg {
    /// @brief Адрес старшего байта (MSB) регистра CCX.
    static constexpr rm3100_reg_type addr{0x04};

    /// @brief Количество накоплений по оси X (по умолчанию 200).
    std::uint16_t cycle_count{200};
};

/// @brief Регистр cycle count по оси Y (CCY), адреса 0x06 (MSB) и
/// 0x07 (LSB).
///
/// @details
///     Аналог @ref rm3100_ccx_reg для оси Y: 16-битное значение cycle
///     count, определяющее усиление, разрешение и длительность измерения
///     по оси Y. Подробнее см. описание @ref rm3100_ccx_reg.
struct rm3100_ccy_reg {
    /// @brief Адрес старшего байта (MSB) регистра CCY.
    static constexpr rm3100_reg_type addr{0x06};

    /// @brief Количество накоплений по оси Y (по умолчанию 200).
    std::uint16_t cycle_count{200};
};

/// @brief Регистр cycle count по оси Z (CCZ), адреса 0x08 (MSB) и
/// 0x09 (LSB).
///
/// @details
///     Аналог @ref rm3100_ccx_reg для оси Z: 16-битное значение cycle
///     count, определяющее усиление, разрешение и длительность измерения
///     по оси Z. Подробнее см. описание @ref rm3100_ccx_reg.
struct rm3100_ccz_reg {
    /// @brief Адрес старшего байта (MSB) регистра CCZ.
    static constexpr rm3100_reg_type addr{0x08};

    /// @brief Количество накоплений по оси Z (по умолчанию 200).
    std::uint16_t cycle_count{200};
};

/// @brief Регистр частоты обновления в режиме CMM (TMRC), адрес 0x0B.
///
/// @details
///     Задаёт интервал между измерениями в непрерывном режиме. Старший
///     полубайт (биты 7–4) должен быть равен 0x9, младший полубайт
///     (TMRC3–TMRC0) кодирует частоту обновления: чем больше значение,
///     тем длиннее интервал между измерениями. Частоты приблизительны,
///     допуск — около 7% (одно стандартное отклонение).
///
///     Типовые значения: 0x92 — ~600 Гц, 0x93 — ~300 Гц, 0x94 — ~150 Гц,
///     0x95 — ~75 Гц, 0x96 — ~37 Гц (по умолчанию), 0x97 — ~18 Гц,
///     0x98 — ~9 Гц, 0x99 — ~4.5 Гц, 0x9A — ~2.3 Гц, 0x9B — ~1.2 Гц,
///     0x9C — ~0.6 Гц, 0x9D — ~0.3 Гц.
///
///     Реальная частота ограничена сверху длительностью измерения,
///     задаваемой cycle count: например, при cycle count 200 максимальная
///     частота обновления по трём осям — около 430 Гц, даже если TMRC
///     требует больше. Значение по умолчанию 0x96 соответствует примерно
///     37 Гц.
///
/// @code
/// // Частота обновления ~75 Гц в непрерывном режиме.
/// stv::rm3100_regs_setup regs{};
/// regs.tmrc.value = 0x95U;
/// sensor.init(regs); // записывает TMRC с контрольным чтением
/// @endcode
class rm3100_tmrc_reg
{
  public:
    /// @brief Адрес регистра TMRC.
    static constexpr rm3100_reg_type addr{0x0B};

    /// @brief Конструктор по умолчанию.
    ///
    /// @details
    ///     Создаёт регистр со значением по умолчанию 0x96 (~37 Гц).
    rm3100_tmrc_reg() = default;

    /// @brief Конструктор из сырого значения.
    ///
    /// @param[in] reg_value Код частоты обновления.
    explicit rm3100_tmrc_reg(
        rm3100_reg_type reg_value):
        value{reg_value}
    {
    }

    /// @brief Преобразует регистр в сырое значение.
    ///
    /// @return Значение TMRC.
    explicit operator rm3100_reg_type() const { return value; }

    /// @brief Присваивает регистру сырое значение.
    ///
    /// @param[in] new_value Новое значение TMRC.
    /// @return Ссылка на текущий объект.
    auto operator=(
        rm3100_reg_type new_value) -> rm3100_tmrc_reg &
    {
        value = new_value;
        return *this;
    }

    /// @brief Сравнивает два регистра TMRC.
    ///
    /// @param[in] other Другой регистр TMRC.
    /// @return `true`, если значения совпадают.
    auto operator==(
        const rm3100_tmrc_reg &other) const -> bool
    { return value == other.value; }

    /// @brief Сырое значение кода частоты обновления.
    ///
    /// @details
    ///     Старший полубайт должен быть 0x9, младший кодирует частоту
    ///     обновления. По умолчанию 0x96 (~37 Гц).
    rm3100_reg_type value{0x96};
};

/// @brief Регистр статуса (STATUS), адрес 0x34.
///
/// @details
///     Регистр только для чтения. Бит 7 (DRDY, Data Ready) показывает
///     готовность новых данных: «1» — измерения по всем заказанным осям
///     завершены и результаты можно читать из регистров измерений
///     (0x24–0x2C), «0» — данных нет. Биты 0–6 не определены и должны
///     игнорироваться.
///
///     Опрос DRDY — один из способов дождаться завершения измерения
///     наряду с контролем аппаратной линии DRDY. Драйвер использует его в
///     @ref stv::rm3100::is_data_ready() и при ожидании самотеста.
///
/// @code
/// // Ожидание готовности данных через драйвер.
/// while(!sensor.is_data_ready()) {
///     // опрашиваем STATUS, пока бит DRDY не поднимется
/// }
/// const auto field = sensor.read_normalized();
/// @endcode
class rm3100_status_reg
{
  public:
    /// @brief Адрес регистра STATUS.
    static constexpr rm3100_reg_type addr{0x34};

    /// @brief Конструктор по умолчанию.
    ///
    /// @details
    ///     Создаёт регистр со сброшенным флагом DRDY.
    rm3100_status_reg() = default;

    /// @brief Конструктор из сырого значения регистра.
    ///
    /// @details
    ///     Распаковывает бит DRDY из переданного байта.
    ///
    /// @param[in] value Значение, считанное из регистра STATUS.
    explicit rm3100_status_reg(
        rm3100_reg_type value)
    { parse(value); }

    /// @brief Преобразует регистр в сырое байтовое значение.
    ///
    /// @details
    ///     Упаковывает поля в байт через @c etl::bit_stream_writer;
    ///     первым записывается старший бит (bit 7).
    ///
    /// @return Значение STATUS.
    explicit operator rm3100_reg_type() const
    {
        std::array<rm3100_reg_type, 1U> reg{0};
        etl::bit_stream_writer          bit_stream{std::to_address(reg.begin()),
                                                   std::to_address(reg.end()),
                                                   etl::endian::little};
        // Первым записывается старший бит.
        bit_stream.write(drdy);
        bit_stream.write(false);
        bit_stream.write(false);
        bit_stream.write(false);
        bit_stream.write(false);
        bit_stream.write(false);
        bit_stream.write(false);
        bit_stream.write(false);

        return *reg.data();
    }

    /// @brief Присваивает регистру сырое значение.
    ///
    /// @param[in] value Значение, считанное из регистра STATUS.
    /// @return Ссылка на текущий объект.
    auto operator=(
        rm3100_reg_type value) -> rm3100_status_reg &
    {
        parse(value);
        return *this;
    }

    /// @brief Сравнивает два регистра STATUS.
    ///
    /// @param[in] other Другой регистр STATUS.
    /// @return `true`, если побитовые представления равны.
    auto operator==(
        const rm3100_status_reg &other) const -> bool
    {
        return static_cast<rm3100_reg_type>(*this)
               == static_cast<rm3100_reg_type>(other);
    }

    /// @brief Флаг готовности данных (Data Ready, бит 7): `true` — новые
    /// измерения готовы для чтения.
    bool drdy{false};

  private:
    /// @brief Распаковывает сырое значение регистра в битовые поля.
    ///
    /// @details
    ///     Считывает бит DRDY через @c etl::bit_stream_reader; первым
    ///     считывается старший бит (bit 7). Биты 0–6 пропускаются, так как
    ///     их значение не определено.
    ///
    /// @param[in] value Сырое значение регистра STATUS.
    void parse(
        rm3100_reg_type value)
    {
        std::array<rm3100_reg_type, 1U> reg{value};
        etl::bit_stream_reader          bit_stream{std::to_address(reg.begin()),
                                                   std::to_address(reg.end()),
                                                   etl::endian::little};
        // Первым считывается старший бит.
        drdy = bit_stream.read_unchecked<bool>();
        bit_stream.skip(7);
    }
};

/// @brief Регистр встроенного самотеста (BIST), адрес 0x33.
///
/// @details
///     Управляет встроенным самотестом LR-генераторов и хранит его
///     результат. Бит STE (Self Test Enable, бит 7) разрешает запуск
///     самотеста при следующей записи в регистр POLL; окончание самотеста
///     сигнализируется подъёмом DRDY. После завершения биты XOK/YOK/ZOK
///     (биты 4–6) показывают, корректно ли отработал LR-генератор
///     соответствующей оси; результат достоверен, только пока STE
///     остаётся в «1».
///
///     Поля настройки самотеста:
///     - BW (биты 3–2) — таймаут ожидания периодов LR-генератора:
///       0b01 — 1 sleep-цикл (~30 мкс), 0b10 — 2 (~60 мкс),
///       0b11 — 4 (~120 мкс);
///     - BP (биты 1–0) — число LR-периодов измерения: 0b01 — 1,
///       0b10 — 2, 0b11 — 4.
///
///     Полную последовательность самотеста (запись BIST, запуск через
///     POLL, ожидание DRDY, проверка XOK/YOK/ZOK, сброс STE) реализует
///     метод @ref stv::rm3100::run_self_test().
///
/// @code
/// // Так драйвер формирует BIST для самотеста по всем осям.
/// stv::rm3100_bist_reg bist{};
/// bist.ste = true; // разрешить самотест при записи POLL
/// bist.bw  = 0b11; // таймаут 4 sleep-цикла (~120 мкс)
/// bist.bp  = 0b11; // измерение по 4 LR-периодам
///
/// stv::rm3100_poll_reg poll{};
/// poll.pmx = true;
/// poll.pmy = true;
/// poll.pmz = true;
/// // далее: sensor.run_self_test(bist, poll, timeout_ms, delay_ms);
/// @endcode
class rm3100_bist_reg
{
  public:
    /// @brief Адрес регистра BIST.
    static constexpr rm3100_reg_type addr{0x33};

    /// @brief Конструктор по умолчанию.
    ///
    /// @details
    ///     Создаёт регистр со сброшенными STE, результатами и настройками
    ///     самотеста (сырое значение 0x00).
    rm3100_bist_reg() = default;

    /// @brief Конструктор из сырого значения регистра.
    ///
    /// @details
    ///     Распаковывает биты STE, XOK/YOK/ZOK и поля BW/BP из
    ///     переданного байта.
    ///
    /// @param[in] value Значение, считанное из регистра BIST.
    explicit rm3100_bist_reg(
        rm3100_reg_type value)
    { parse(value); }

    /// @brief Преобразует регистр в сырое байтовое значение.
    ///
    /// @details
    ///     Упаковывает поля в байт через @c etl::bit_stream_writer;
    ///     первым записывается старший бит (bit 7).
    ///
    /// @return Значение BIST для записи в датчик.
    explicit operator rm3100_reg_type() const
    {
        std::array<rm3100_reg_type, 1U> reg{0};
        etl::bit_stream_writer          bit_stream{std::to_address(reg.begin()),
                                                   std::to_address(reg.end()),
                                                   etl::endian::little};
        // Первым записывается старший бит.
        bit_stream.write(ste);
        bit_stream.write(zok);
        bit_stream.write(yok);
        bit_stream.write(xok);
        bit_stream.write(bw, 2U);
        bit_stream.write(bp, 2U);

        return *reg.data();
    }

    /// @brief Присваивает регистру сырое значение.
    ///
    /// @param[in] value Значение, считанное из регистра BIST.
    /// @return Ссылка на текущий объект.
    auto operator=(
        rm3100_reg_type value) -> rm3100_bist_reg &
    {
        parse(value);
        return *this;
    }

    /// @brief Сравнивает два регистра BIST.
    ///
    /// @param[in] other Другой регистр BIST.
    /// @return `true`, если побитовые представления равны.
    auto operator==(
        const rm3100_bist_reg &other) const -> bool
    {
        return static_cast<rm3100_reg_type>(*this)
               == static_cast<rm3100_reg_type>(other);
    }

    /// @brief Разрешение самотеста (Self Test Enable, бит 7).
    ///
    /// @details
    ///     «1» — выполнить самотест при следующей записи в регистр POLL.
    ///     Окончание самотеста сигнализируется подъёмом DRDY.
    bool ste{false};

    /// @brief Результат самотеста по оси Z (бит ZOK, позиция 6):
    /// `true` — генератор оси исправен.
    bool zok{false};

    /// @brief Результат самотеста по оси Y (бит YOK, позиция 5):
    /// `true` — генератор оси исправен.
    bool yok{false};

    /// @brief Результат самотеста по оси X (бит XOK, позиция 4):
    /// `true` — генератор оси исправен.
    bool xok{false};

    /// @brief Таймаут ожидания периодов LR-генератора в самотесте
    /// (поле BW, биты 3–2).
    ///
    /// @details
    ///     0b01 — 1 sleep-цикл (~30 мкс), 0b10 — 2 sleep-цикла (~60 мкс),
    ///     0b11 — 4 sleep-цикла (~120 мкс). Значение 0b00 не используется.
    std::uint8_t bw{0};

    /// @brief Количество LR-периодов измерения в самотесте (поле BP,
    /// биты 1–0).
    ///
    /// @details
    ///     0b01 — 1 период, 0b10 — 2 периода, 0b11 — 4 периода.
    ///     Значение 0b00 не используется.
    std::uint8_t bp{0};

  private:
    /// @brief Распаковывает сырое значение регистра в битовые поля.
    ///
    /// @details
    ///     Считывает биты STE, XOK/YOK/ZOK и поля BW/BP через
    ///     @c etl::bit_stream_reader; первым считывается старший бит
    ///     (bit 7).
    ///
    /// @param[in] value Сырое значение регистра BIST.
    void parse(
        rm3100_reg_type value)
    {
        std::array<rm3100_reg_type, 1U> reg{value};
        etl::bit_stream_reader          bit_stream{std::to_address(reg.begin()),
                                                   std::to_address(reg.end()),
                                                   etl::endian::little};
        // Первым считывается старший бит.
        ste = bit_stream.read_unchecked<bool>();
        zok = bit_stream.read_unchecked<bool>();
        yok = bit_stream.read_unchecked<bool>();
        xok = bit_stream.read_unchecked<bool>();
        bw  = bit_stream.read_unchecked<std::uint8_t>(2U);
        bp  = bit_stream.read_unchecked<std::uint8_t>(2U);
    }
};

/// @brief Регистр идентификатора ревизии (REVID), адрес 0x36.
///
/// @details
///     Однобайтовый регистр только для чтения, содержащий идентификатор
///     ревизии MagI2C. Позволяет убедиться, что на шине присутствует
///     именно RM3100: драйвер считывает регистр и сравнивает значение с
///     @ref expected_value в методе @ref stv::rm3100::is_detected().
///
/// @code
/// // Проверка присутствия датчика на шине I2C без создания объекта.
/// if(!stv::rm3100<stv::rm3100_setup<mag_type>>::is_detected(&i2c)) {
///     // REVID не совпал с ожидаемым — на шине нет RM3100
/// }
/// @endcode
class rm3100_revid_reg
{
  public:
    /// @brief Адрес регистра REVID.
    static constexpr rm3100_reg_type addr{0x36};

    /// @brief Ожидаемое значение REVID.
    ///
    /// @details
    ///     Эталон, с которым драйвер сравнивает считанный идентификатор
    ///     при проверке присутствия датчика. Значение зависит от ревизии
    ///     MagI2C — уточнить по конкретной ревизии чипа.
    static constexpr rm3100_reg_type expected_value{0x00};

    /// @brief Конструктор по умолчанию.
    ///
    /// @details
    ///     Создаёт регистр с нулевым значением идентификатора.
    rm3100_revid_reg() = default;

    /// @brief Конструктор из сырого значения ревизии.
    ///
    /// @param[in] value Значение, считанное из регистра REVID.
    explicit rm3100_revid_reg(
        rm3100_reg_type value):
        revid{value}
    {
    }

    /// @brief Сравнивает два регистра REVID.
    ///
    /// @param[in] other Другой регистр REVID.
    /// @return `true`, если значения ревизий совпадают.
    auto operator==(
        const rm3100_revid_reg &other) const -> bool
    { return revid == other.revid; }

    /// @brief Считанное значение идентификатора ревизии.
    rm3100_reg_type revid{0};
};

// NOLINTEND(*-member*)

} // namespace stv

#endif /* RM3100_REGS_HPP */
