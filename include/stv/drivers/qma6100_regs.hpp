/// @file qma6100_regs.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.

#ifndef QMA6100_REGS_HPP
#define QMA6100_REGS_HPP

#include "qma6100_types.hpp"
#include "stv/register_field.hpp"
#include <cstddef>
#include <cstdint>

namespace stv {

// NOLINTBEGIN(*hicpp-signed-bitwise, *-member*)

// writing 0xB6, soft reset all of the registers.
// After soft-reset, user should write 0x00 back
static constexpr qma6100_reg_type qma6100_sfe_sr_addr{0x36};

/// @brief Класс для работы с регистром полосы пропускания и фильтрации QMA6100.
///
/// @details Этот класс инкапсулирует логику работы с регистром 0x10 (BANDWIDTH)
/// датчика QMA6100, который управляет настройками полосы пропускания
/// (bandwidth) и цифрового фильтра нижних частот (low-pass filter, LPF). Полоса
/// пропускания определяет частоту дискретизации акселерометра, а фильтр нижних
/// частот задаёт количество усредняемых выборок для уменьшения шума. Класс
/// предоставляет безопасный типобезопасный интерфейс для работы с этими
/// настройками через перечисления. Поддерживает преобразование в сырое значение
/// регистра и обратно.
class qma6100_bw_reg
{
    static constexpr int bw_offset{0U};
    static constexpr int nlpf_offset{5U};

  public:
    /// @brief Адрес регистра BANDWIDTH в памяти устройства.
    static constexpr qma6100_reg_type addr{0x10};

    /// @brief Конструктор с возможностью инициализации значением регистра.
    ///
    /// @details Создаёт объект класса, парсинг переданного сырого значения
    /// регистра и заполняет внутренние поля (bw и nlpf) соответствующими
    /// значениями.
    /// @param value Начальное сырое значение регистра (по умолчанию 0).
    explicit qma6100_bw_reg(
        qma6100_reg_type value = qma6100_reg_type{0})
    { parse(value); }

    /// @brief Оператор преобразования в сырое значение регистра.
    ///
    /// @details Собирает текущие настройки полей bw и nlpf в одно 8-битное
    /// значение, готовое для записи в регистр устройства.
    /// @return 8-битное значение регистра, собранное из полей.
    explicit operator qma6100_reg_type() const
    {
        return static_cast<qma6100_reg_type>(field_to_raw(nlpf, nlpf_offset)
                                             | field_to_raw(bw, bw_offset));
    }

    /// @brief Оператор сравнения двух экземпляров регистра на равенство.
    ///
    /// @param other Ссылка на другой экземпляр qma6100_bw_reg для сравнения.
    /// @return true, если значения регистров (после преобразования в
    /// qma6100_reg_type) равны.
    bool operator==(
        const qma6100_bw_reg &other) const
    {
        return static_cast<qma6100_reg_type>(*this)
               == static_cast<qma6100_reg_type>(other);
    }

    // -------------------------------------------------------------------------

    /// @brief Перечисление для настройки цифрового фильтра нижних частот (LPF).
    ///
    /// @details Определяет количество выборок акселерометра, которые
    /// усредняются перед выдачей результата. Более высокие значения уменьшают
    /// высокочастотный шум, но увеличивают задержку (латентность) выходных
    /// данных.
    enum struct nlpf_t : std::uint8_t {
        /// @brief Нет усреднения (используется 1 выборка).
        no_average = 0,
        /// @brief Усреднение по 2 выборкам.
        average_2,
        /// @brief Усреднение по 4 выборкам.
        average_4,
        /// @brief Усреднение по 16 выборкам.
        average_16,
    };

    /// @brief Текущая настройка цифрового фильтра нижних частот (LPF).
    nlpf_t nlpf;
    // -------------------------------------------------------------------------

    /// @brief Перечисление для настройки полосы пропускания (частоты
    /// дискретизации).
    ///
    /// @details Определяет частоту дискретизации (Output Data Rate, ODR)
    /// датчика. Значения соответствуют делению частоты внутреннего тактового
    /// генератора (MCLK). Более высокая частота дискретизации даёт более
    /// детальные данные о движении, но может быть более подвержена шуму и
    /// увеличивает потребление энергии.
    enum struct bw_t : std::uint8_t {
        mclk_512_977_hz,  ///< ODR ≈ 977 Гц (MCLK / 512)
        mclk_256_1953_hz, ///< ODR ≈ 1953 Гц (MCLK / 256)
        mclk_128_3906_hz, ///< ODR ≈ 3906 Гц (MCLK / 128)
        mclk_64_7813_hz,  ///< ODR ≈ 7813 Гц (MCLK / 64)
        mclk_32_15625_hz, ///< ODR ≈ 15625 Гц (MCLK / 32)
        mclk_1024_488_hz, ///< ODR ≈ 488 Гц (MCLK / 1024)
        mclk_2048_244_hz, ///< ODR ≈ 244 Гц (MCLK / 2048)
        mclk_4096_122_hz, ///< ODR ≈ 122 Гц (MCLK / 4096)
    };

    /// @brief Текущая настройка полосы пропускания (частоты дискретизации).
    bw_t bw;
    // -------------------------------------------------------------------------

  private:
    /// @brief Парсинг сырого значения регистра в поля класса.
    ///
    /// @details Извлекает битовые поля, соответствующие настройкам bw и nlpf,
    /// из переданного сырого значения регистра и сохраняет их в соответствующих
    /// полях объекта.
    /// @param reg Сырое значение регистра для парсинга.
    void parse(
        qma6100_reg_type reg)
    {
        constexpr qma6100_reg_type bw_mask{0x1F};
        bw = extract_field<decltype(bw)>(reg, bw_offset, bw_mask);

        constexpr qma6100_reg_type nlpf_mask{0x03};
        nlpf = extract_field<decltype(nlpf)>(reg, nlpf_offset, nlpf_mask);
    }
};

/// @brief Класс для работы с регистром диапазона измерений (Full Scale Range)
/// QMA6100.
///
/// @details Этот класс инкапсулирует логику работы с регистром 0x0F (RANGE)
/// датчика QMA6100, который отвечает за настройку максимального измеряемого
/// ускорения (Full Scale Range, FSR). Диапазон измерений определяет
/// максимальное ускорение, которое может быть измерено без перегрузки. Более
/// широкий диапазон позволяет измерять большие ускорения, но уменьшает
/// разрешение (чувствительность) измерений. Класс предоставляет простой
/// интерфейс для работы с этим параметром.
class qma6100_fsr_reg
{
    static constexpr int range_offset{0U};

  public:
    /// @brief Адрес регистра RANGE в памяти устройства.
    static constexpr qma6100_reg_type addr{0x0F};

    explicit qma6100_fsr_reg(
        qma6100_reg_type value = qma6100_reg_type{0})
    { parse(value); }

    /// @brief Преобразует конфигурацию в 8-битное значение регистра.
    ///
    /// @details Собирает текущую настройку диапазона в сырое значение,
    /// готовое для записи в регистр устройства.
    /// @return 8-битное значение регистра, содержащее настройку диапазона.
    explicit operator std::byte() const
    { return static_cast<std::byte>(field_to_raw(range, range_offset)); }

    // -------------------------------------------------------------------------

    /// @brief Оператор сравнения двух экземпляров регистра на равенство.
    ///
    /// @param other Ссылка на другой экземпляр qma6100_bw_reg для сравнения.
    /// @return true, если значения регистров (после преобразования в
    /// qma6100_reg_type) равны.
    bool operator==(
        const qma6100_fsr_reg &other) const
    {
        return static_cast<qma6100_reg_type>(*this)
               == static_cast<qma6100_reg_type>(other);
    }

    /// @brief Перечисление для настройки диапазона измерений (Full Scale
    /// Range).
    ///
    /// @details Определяет максимальное ускорение (в единицах g), которое может
    /// быть измерено датчиком. Выбор диапазона влияет на разрешение АЦП:
    /// меньший диапазон даёт более высокую чувствительность к малым ускорениям.
    enum struct range_t : std::uint8_t {
        g_2  = 0b0001, ///< Диапазон ±2g
        g_4  = 0b0010, ///< Диапазон ±4g
        g_8  = 0b0100, ///< Диапазон ±8g
        g_16 = 0b1000, ///< Диапазон ±16g
        g_32 = 0b1111, ///< Диапазон ±32g
    };

    /// @brief Текущая настройка диапазона измерений (Full Scale Range).
    range_t range{range_t::g_2};
    // -------------------------------------------------------------------------

  private:
    /// @brief Парсинг сырого значения регистра в поля класса.
    ///
    /// @param reg Сырое значение регистра для парсинга.
    void parse(
        qma6100_reg_type reg)
    {
        constexpr qma6100_reg_type range_mask{0x0F};
        const auto raw_value = field_raw(reg, range_offset, range_mask);

        // NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange)
        switch(static_cast<decltype(range)>(raw_value))
        {
                // Согласно документации, если значение не совпадает ни с одним
                // из указанных в range_t значений, то по умолчанию будет
                // применено значение range_t::g_2.

            case range_t::g_2:
                range = range_t::g_2;
                break;
            case range_t::g_4:
                range = range_t::g_4;
                break;
            case range_t::g_8:
                range = range_t::g_8;
                break;
            case range_t::g_16:
                range = range_t::g_16;
                break;
            case range_t::g_32:
                range = range_t::g_32;
                break;

            default:
                range = range_t::g_2;
                break;
        }
    }
};

/// @brief Класс для работы с регистром разрешения прерываний 1 (INT_EN1)
/// QMA6100.
///
/// @details Этот класс инкапсулирует логику работы с регистром 0x17 (INT_EN1),
/// который управляет разрешением (включением/выключением) различных источников
/// прерываний. Каждый бит этого регистра соответствует определённому типу
/// прерывания. Класс предоставляет типобезопасный доступ к этим настройкам.
class qma6100_int_en1_reg
{
    static constexpr int int_fwm_en_offset{6U};
    static constexpr int int_ffull_en_offset{5U};
    static constexpr int int_data_en_offset{4U};

  public:
    /// @brief Адрес регистра INT_EN1 в памяти устройства.
    static constexpr qma6100_reg_type addr{0x17};

    explicit qma6100_int_en1_reg(
        qma6100_reg_type value = qma6100_reg_type{0})
    { parse(value); }

    /// @brief Оператор преобразования в сырое значение регистра.
    ///
    /// @details Собирает текущие настройки разрешений прерываний в одно
    /// 8-битное значение, готовое для записи в регистр устройства.
    /// @return 8-битное значение регистра, собранное из полей.
    explicit operator qma6100_reg_type() const
    {
        return static_cast<qma6100_reg_type>(
            field_to_raw(int_fwm_en, int_fwm_en_offset)
            | field_to_raw(int_ffull_en, int_ffull_en_offset)
            | field_to_raw(int_data_en, int_data_en_offset));
    }

    /// @brief Оператор сравнения двух экземпляров регистра на равенство.
    ///
    /// @param other Ссылка на другой экземпляр qma6100_bw_reg для сравнения.
    /// @return true, если значения регистров (после преобразования в
    /// qma6100_reg_type) равны.
    bool operator==(
        const qma6100_int_en1_reg &other) const
    {
        return static_cast<qma6100_reg_type>(*this)
               == static_cast<qma6100_reg_type>(other);
    }

    /// @brief Перечисление для управления включением/выключением функции.
    enum struct switcher_t : std::uint8_t {
        disable = 0, ///< Функция выключена.
        enable  = 1, ///< Функция включена.
    };

    /// @brief Разрешение прерывания по достижению уровня заполнения FIFO
    /// (watermark).
    switcher_t int_fwm_en{switcher_t::disable};
    // -------------------------------------------------------------------------

    /// @brief Разрешение прерывания по полному заполнению FIFO.
    switcher_t int_ffull_en{switcher_t::disable};
    // -------------------------------------------------------------------------

    /// @brief Разрешение прерывания по готовности новых данных (data ready).
    switcher_t int_data_en{switcher_t::disable};
    // -------------------------------------------------------------------------

  private:
    /// @brief Парсинг сырого значения регистра в поля класса.
    ///
    /// @param reg Сырое значение регистра для парсинга.
    void parse(
        qma6100_reg_type reg)
    {
        {
            constexpr qma6100_reg_type int_fwm_en_mask{0x01};
            int_fwm_en = extract_field<decltype(int_fwm_en)>(
                reg, int_fwm_en_offset, int_fwm_en_mask);
        }

        {
            constexpr qma6100_reg_type iint_ffull_en_mask{0x01};
            int_ffull_en = extract_field<decltype(int_ffull_en)>(
                reg, int_ffull_en_offset, iint_ffull_en_mask);
        }

        {
            constexpr qma6100_reg_type int_data_en_mask{0x01};
            int_data_en = extract_field<decltype(int_data_en)>(
                reg, int_data_en_offset, int_data_en_mask);
        }
    }
};

/// @brief Класс для работы с регистром маппинга прерываний на вывод INT1
/// (INT_MAP1) QMA6100.
/// @warning В классе определены не все биты регистра.
///
/// @details Этот класс инкапсулирует логику работы с регистром 0x1A (INT_MAP1),
/// который управляет маппингом (назначением) сигналов прерываний на физический
/// вывод INT1 датчика. Каждый бит определяет, будет ли определённый тип
/// прерывания выведен на этот пин.
class qma6100_int_map1_reg
{
    static constexpr int int1_no_mot_offset{7U};
    static constexpr int int1_any_mot_offset{0U};

  public:
    /// @brief Адрес регистра INT_MAP1 в памяти устройства.
    static constexpr qma6100_reg_type addr{0x1a};

    explicit qma6100_int_map1_reg(
        qma6100_reg_type value = qma6100_reg_type{0})
    { parse(value); }

    /// @brief Оператор преобразования в сырое значение регистра.
    ///
    /// @details Собирает текущие настройки маппинга прерываний для вывода INT1
    /// в одно 8-битное значение, готовое для записи в регистр устройства.
    /// @return 8-битное значение регистра, собранное из полей.
    explicit operator qma6100_reg_type() const
    {
        return static_cast<qma6100_reg_type>(
            field_to_raw(int1_no_mot, int1_no_mot_offset)
            | field_to_raw(int1_any_mot, int1_any_mot_offset));
    }

    /// @brief Оператор сравнения двух экземпляров регистра на равенство.
    ///
    /// @param other Ссылка на другой экземпляр qma6100_bw_reg для сравнения.
    /// @return true, если значения регистров (после преобразования в
    /// qma6100_reg_type) равны.
    bool operator==(
        const qma6100_int_map1_reg &other) const
    {
        return static_cast<qma6100_reg_type>(*this)
               == static_cast<qma6100_reg_type>(other);
    }

    /// @brief Перечисление для управления состоянием маппинга прерывания на
    /// пин.
    enum struct mapper_t : std::uint8_t {
        disable = 0, ///< Прерывание не выводится на соответствующий пин.
        enable  = 1, ///< Прерывание выводится на соответствующий пин.
    };

    /// @brief Маппинг прерывания "no motion" (отсутствие движения) на вывод
    /// INT1.
    mapper_t int1_no_mot{mapper_t::disable};
    // -------------------------------------------------------------------------

    /// @brief Маппинг прерывания "any motion" (любое движение) на вывод INT1.
    mapper_t int1_any_mot{mapper_t::disable};

  private:
    /// @brief Парсинг сырого значения регистра в поля класса.
    ///
    /// @param reg Сырое значение регистра для парсинга.
    void parse(
        qma6100_reg_type reg)
    {
        {
            constexpr qma6100_reg_type int1_no_mot_mask{0x01};
            int1_no_mot = extract_field<decltype(int1_no_mot)>(
                reg, int1_no_mot_offset, int1_no_mot_mask);
        }

        {
            constexpr qma6100_reg_type int1_any_mot_mask{0x01};
            int1_any_mot = extract_field<decltype(int1_any_mot)>(
                reg, int1_any_mot_offset, int1_any_mot_mask);
        }
    }
};

/// @brief Класс для работы с регистром маппинга прерываний на вывод INT2
/// (INT_MAP3) QMA6100.
/// @warning В классе определены не все биты регистра.
///
/// @details Этот класс инкапсулирует логику работы с регистром 0x1C (INT_MAP3),
/// который управляет маппингом (назначением) сигналов прерываний на физический
/// вывод INT2 датчика. Каждый бит определяет, будет ли определённый тип
/// прерывания выведен на этот пин.
class qma6100_int_map3_reg
{
    static constexpr int int2_no_mot_offset{7U};
    static constexpr int int2_any_mot_offset{0U};

  public:
    /// @brief Адрес регистра INT_MAP3 в памяти устройства.
    static constexpr qma6100_reg_type addr{0x1c};

    explicit qma6100_int_map3_reg(
        qma6100_reg_type value = qma6100_reg_type{0})
    { parse(value); }

    /// @brief Оператор сравнения двух экземпляров регистра на равенство.
    ///
    /// @param other Ссылка на другой экземпляр qma6100_bw_reg для сравнения.
    /// @return true, если значения регистров (после преобразования в
    /// qma6100_reg_type) равны.
    bool operator==(
        const qma6100_int_map3_reg &other) const
    {
        return static_cast<qma6100_reg_type>(*this)
               == static_cast<qma6100_reg_type>(other);
    }

    /// @brief Оператор преобразования в сырое значение регистра.
    ///
    /// @details Собирает текущие настройки маппинга прерываний для вывода INT2
    /// в одно 8-битное значение, готовое для записи в регистр устройства.
    /// @return 8-битное значение регистра, собранное из полей.
    explicit operator qma6100_reg_type() const
    {
        return static_cast<qma6100_reg_type>(
            field_to_raw(int2_no_mot, int2_no_mot_offset)
            | field_to_raw(int2_any_mot, int2_any_mot_offset));
    }

    /// @brief Перечисление для управления состоянием маппинга прерывания на
    /// пин.
    enum struct mapper_t : std::uint8_t {
        disable = 0, ///< Прерывание не выводится на соответствующий пин.
        enable  = 1, ///< Прерывание выводится на соответствующий пин.
    };

    /// @brief Маппинг прерывания "no motion" (отсутствие движения) на вывод
    /// INT2.
    mapper_t int2_no_mot{mapper_t::disable};
    // -------------------------------------------------------------------------

    /// @brief Маппинг прерывания "any motion" (любое движение) на вывод INT2.
    mapper_t int2_any_mot{mapper_t::disable};
    // -------------------------------------------------------------------------

  private:
    /// @brief Парсинг сырого значения регистра в поля класса.
    ///
    /// @param reg Сырое значение регистра для парсинга.
    void parse(
        qma6100_reg_type reg)
    {
        {
            constexpr qma6100_reg_type int2_no_mot_mask{0x01};
            int2_no_mot = extract_field<decltype(int2_no_mot)>(
                reg, int2_no_mot_offset, int2_no_mot_mask);
        }

        {
            constexpr qma6100_reg_type int2_any_mot_mask{0x01};
            int2_any_mot = extract_field<decltype(int2_any_mot)>(
                reg, int2_any_mot_offset, int2_any_mot_mask);
        }
    }
};

/// @brief Класс для работы с регистром конфигурации выводов прерываний
/// (INTPIN_CONF) QMA6100.
///
/// @details Этот класс инкапсулирует логику работы с регистром 0x20
/// (INTPIN_CONF), который управляет настройками физических характеристик
/// выводов прерываний (INT1, INT2), а также некоторыми общими настройками
/// интерфейса (например, подтяжками и режимом SPI). Позволяет настраивать такие
/// параметры, как активный уровень, тип выхода (push-pull/open-drain) и другие.
class qma6100_intpin_conf_reg
{
    static constexpr int dis_pu_senb_offset{7U};
    static constexpr int dis_ie_ad0_offset{6U};
    static constexpr int en_spi3w_offset{5U};
    static constexpr int int2_od_offset{3U};
    static constexpr int int2_lvl_offset{2U};
    static constexpr int int1_od_offset{1U};
    static constexpr int int1_lvl_offset{0U};

  public:
    /// @brief Адрес регистра INTPIN_CONF в памяти устройства.
    static constexpr qma6100_reg_type addr{0x20};

    explicit qma6100_intpin_conf_reg(
        qma6100_reg_type value = qma6100_reg_type{0})
    { parse(value); }

    /// @brief Оператор сравнения двух экземпляров регистра на равенство.
    ///
    /// @param other Ссылка на другой экземпляр qma6100_bw_reg для сравнения.
    /// @return true, если значения регистров (после преобразования в
    /// qma6100_reg_type) равны.
    bool operator==(
        const qma6100_intpin_conf_reg &other) const
    {
        return static_cast<qma6100_reg_type>(*this)
               == static_cast<qma6100_reg_type>(other);
    }

    /// @brief Оператор преобразования в сырое значение регистра.
    ///
    /// @details Собирает все текущие настройки конфигурации выводов в одно
    /// 8-битное значение, готовое для записи в регистр устройства.
    /// @return 8-битное значение регистра, собранное из полей.
    explicit operator qma6100_reg_type() const
    {
        return static_cast<qma6100_reg_type>(
            field_to_raw(dis_pu_senb, dis_pu_senb_offset)
            | field_to_raw(dis_ie_ad0, dis_ie_ad0_offset)
            | field_to_raw(en_spi3w, en_spi3w_offset)
            | field_to_raw(int2_od, int2_od_offset)
            | field_to_raw(int2_lvl, int2_lvl_offset)
            | field_to_raw(int1_od, int1_od_offset)
            | field_to_raw(int1_lvl, int1_lvl_offset));
    }

    /// @brief Перечисление для управления внутренней подтяжкой вывода PIN_SENB.
    enum struct dis_pu_senb_t : std::uint8_t {
        enable =
            0, ///< Включить подтягивающий резистор вывода PIN_SENB к VDDIO.
        disable = 1, ///< Отключить подтягивающий резистор вывода PIN_SENB.
    };

    /// @brief Конфигурация внутренней подтяжки вывода PIN_SENB.
    dis_pu_senb_t dis_pu_senb{dis_pu_senb_t::enable};
    // -------------------------------------------------------------------------

    /// @brief Перечисление для управления внутренней подтяжкой вывода AD0
    /// (адресной линии I2C).
    enum struct dis_ie_ad0_t : std::uint8_t {
        enable  = 0, ///< Включить внутреннюю подтяжку вывода AD0 к VDDIO.
        disable = 1, ///< Отключить внутреннюю подтяжку вывода AD0.
    };

    /// @brief Конфигурация внутренней подтяжки вывода AD0 (используется для
    /// выбора адреса I2C).
    dis_ie_ad0_t dis_ie_ad0{dis_ie_ad0_t::enable};
    // -------------------------------------------------------------------------

    /// @brief Перечисление для управления режимом интерфейса SPI.
    enum struct en_spi3w_t : std::uint8_t {
        disable = 0, ///< Использовать стандартный 4-проводной SPI интерфейс.
        enable  = 1, ///< Использовать 3-проводной (3-wire) SPI интерфейс.
    };

    /// @brief Включение режима 3-проводного SPI (SI и SO объединены на одной
    /// линии).
    en_spi3w_t en_spi3w{en_spi3w_t::disable};
    // -------------------------------------------------------------------------

    /// @brief Перечисление для настройки типа выхода (output type) вывода INT2.
    enum struct int2_od_t : std::uint8_t {
        push_pull  = 0, ///< Режим push-pull (двухтактный выход).
        open_drain = 1, ///< Режим open-drain (открытый сток).
    };

    /// @brief Настройка типа выхода для вывода INT2.
    int2_od_t int2_od{int2_od_t::push_pull};
    // -------------------------------------------------------------------------

    /// @brief Перечисление для настройки активного уровня на выводе INT2.
    enum struct int2_lvl_t : std::uint8_t {
        logic_low_as_active  = 0, ///< Активный уровень - низкий (0).
        logic_high_as_active = 1, ///< Активный уровень - высокий (1).
    };

    /// @brief Настройка активного уровня для вывода INT2.
    int2_lvl_t int2_lvl{int2_lvl_t::logic_high_as_active};
    // -------------------------------------------------------------------------

    /// @brief Перечисление для настройки типа выхода (output type) вывода INT1.
    enum struct int1_od_t : std::uint8_t {
        push_pull  = 0, ///< Режим push-pull (двухтактный выход).
        open_drain = 1, ///< Режим open-drain (открытый сток).
    };

    /// @brief Настройка типа выхода для вывода INT1.
    int1_od_t int1_od{int1_od_t::push_pull};
    // -------------------------------------------------------------------------

    /// @brief Перечисление для настройки активного уровня на выводе INT1.
    enum struct int1_lvl_t : std::uint8_t {
        logic_low_as_active  = 0, ///< Активный уровень - низкий (0).
        logic_high_as_active = 1, ///< Активный уровень - высокий (1).
    };

    /// @brief Настройка активного уровня для вывода INT1.
    int1_lvl_t int1_lvl{int1_lvl_t::logic_high_as_active};
    // -------------------------------------------------------------------------

  private:
    /// @brief Парсинг сырого значения регистра в поля класса.
    ///
    /// @param reg Сырое значение регистра для парсинга.
    void parse(
        qma6100_reg_type reg)
    {
        {
            constexpr qma6100_reg_type idis_pu_senb_mask{0x01};
            dis_pu_senb = extract_field<decltype(dis_pu_senb)>(
                reg, dis_pu_senb_offset, idis_pu_senb_mask);
        }

        {
            constexpr qma6100_reg_type dis_ie_ad0_mask{0x01};
            dis_ie_ad0 = extract_field<decltype(dis_ie_ad0)>(
                reg, dis_ie_ad0_offset, dis_ie_ad0_mask);
        }

        {
            constexpr qma6100_reg_type en_spi3w_mask{0x01};
            en_spi3w = extract_field<decltype(en_spi3w)>(reg, en_spi3w_offset,
                                                         en_spi3w_mask);
        }

        {
            constexpr qma6100_reg_type int2_od_mask{0x01};
            int2_od = extract_field<decltype(int2_od)>(reg, int2_od_offset,
                                                       int2_od_mask);
        }

        {
            constexpr qma6100_reg_type int2_lvl_mask{0x01};
            int2_lvl = extract_field<decltype(int2_lvl)>(reg, int2_lvl_offset,
                                                         int2_lvl_mask);
        }

        {
            constexpr qma6100_reg_type int1_od_mask{0x01};
            int1_od = extract_field<decltype(int1_od)>(reg, int1_od_offset,
                                                       int1_od_mask);
        }

        {
            constexpr qma6100_reg_type int1_lvl_mask{0x01};
            int1_lvl = extract_field<decltype(int1_lvl)>(reg, int1_lvl_offset,
                                                         int1_lvl_mask);
        }
    }
};

/// @brief Класс для работы с регистром конфигурации прерываний (INT_CONF)
/// QMA6100.
///
/// @details Этот класс инкапсулирует логику работы с регистром 0x21 (INT_CONF),
/// который управляет общими настройками системы прерываний, такими как режим
/// защёлкивания (latch), политика очистки флагов прерываний, настройка теневого
/// регистра для данных и управление интерфейсом I2C.
class qma6100_int_cfg_reg
{
    static constexpr int int_rd_clr_offset{7U};
    static constexpr int shadow_dis_offset{6U};
    static constexpr int dis_i2c_offset{5U};
    static constexpr int latch_int_step_offset{1U};
    static constexpr int latch_int_offset{0U};

  public:
    /// @brief Адрес регистра INT_CONF в памяти устройства.
    static constexpr qma6100_reg_type addr{0x21};

    explicit qma6100_int_cfg_reg(
        qma6100_reg_type value = qma6100_reg_type{0})
    { parse(value); }

    /// @brief Оператор сравнения двух экземпляров регистра на равенство.
    ///
    /// @param other Ссылка на другой экземпляр qma6100_bw_reg для сравнения.
    /// @return true, если значения регистров (после преобразования в
    /// qma6100_reg_type) равны.
    bool operator==(
        const qma6100_int_cfg_reg &other) const
    {
        return static_cast<qma6100_reg_type>(*this)
               == static_cast<qma6100_reg_type>(other);
    }

    /// @brief Оператор преобразования в сырое значение регистра.
    ///
    /// @details Собирает все текущие настройки конфигурации прерываний в одно
    /// 8-битное значение, готовое для записи в регистр устройства.
    /// @return 8-битное значение регистра, собранное из полей.
    explicit operator qma6100_reg_type() const
    {
        return static_cast<qma6100_reg_type>(
            field_to_raw(int_rd_clr, int_rd_clr_offset)
            | field_to_raw(shadow_dis, shadow_dis_offset)
            | field_to_raw(dis_i2c, dis_i2c_offset)
            | field_to_raw(latch_int_step, latch_int_step_offset)
            | field_to_raw(latch_int, latch_int_offset));
    }

    /// @brief Перечисление для политики очистки флагов прерываний при чтении.
    enum struct int_rd_clr_t : std::uint8_t {
        /// @brief Очищать соответствующий флаг прерывания только при чтении
        /// его статусного регистра (INT_ST, адреса 0x09–0x0D).
        clear_related = 0,

        /// @brief Очищать все флаги прерываний, находящихся в режиме
        /// защёлкивания (latched-mode), при ЛЮБОЙ операции чтения любого
        /// регистра в диапазоне 0x09–0x0D. Для прерываний в режиме без
        /// защёлкивания (non-latched-mode) поведение не меняется.
        clear_all = 1,
    };

    /// @brief Настройка политики очистки флагов прерываний при чтении.
    int_rd_clr_t int_rd_clr{int_rd_clr_t::clear_related};
    // -------------------------------------------------------------------------

    /// @brief Перечисление для управления функцией теневого регистра
    /// (shadowing).
    enum struct shadow_dis_t : std::uint8_t {
        /// @brief Включить функцию теневого режима (shadowing) для данных
        /// акселерометра.
        enable = 0,

        /// @brief Отключить функцию теневого режима (shadowing) для данных
        /// акселерометра.
        disable = 1,
    };

    /// @brief Настройка функции теневого регистра (shadowing) для данных
    /// акселерометра.
    ///
    /// @details Когда теневой режим включён (enable), старший байт (MSB) данных
    /// акселерометра блокируется (фиксируется) в момент чтения соответствующего
    /// младшего байта (LSB) этих данных. Это гарантирует целостность данных
    /// акселерометра во время чтения (предотвращает считывание байтов из разных
    /// выборок). Старший байт будет разблокирован, только когда он сам будет
    /// прочитан. При отключении (disable) такого блокирования не происходит.
    shadow_dis_t shadow_dis{shadow_dis_t::enable};
    // -------------------------------------------------------------------------

    /// @brief Перечисление для управления интерфейсом I2C.
    enum struct dis_i2c_t : std::uint8_t {
        /// @brief Включить интерфейс I2C (используется по умолчанию).
        enable = 0,

        /// @brief Отключить интерфейс I2C. Установка этого бита в 1
        /// рекомендуется при работе в режиме SPI для исключения конфликтов на
        /// шине.
        disable = 1,
    };

    /// @brief Настройка отключения интерфейса I2C.
    dis_i2c_t dis_i2c{dis_i2c_t::enable};
    // -------------------------------------------------------------------------

    /// @brief Перечисление для настройки режима защёлкивания прерываний,
    /// связанных с подсчётом шагов.
    enum struct latch_int_step_t : std::uint8_t {
        /// @brief Прерывания, связанные с подсчётом шагов, работают в режиме
        /// без защёлкивания (non-latched). Флаг сбрасывается автоматически,
        /// когда условие прерывания перестаёт выполняться.
        non_latch_mode = 0,

        /// @brief Прерывания, связанные с подсчётом шагов, работают в режиме
        /// защёлкивания (latched). Флаг остаётся установленным до тех пор, пока
        /// не будет очищен чтением статусного регистра или согласно настройке
        /// int_rd_clr.
        latch_mode = 1,
    };

    /// @brief Настройка режима защёлкивания для прерываний, связанных с
    /// подсчётом шагов.
    latch_int_step_t latch_int_step{latch_int_step_t::non_latch_mode};
    // -------------------------------------------------------------------------

    /// @brief Перечисление для настройки режима защёлкивания для остальных
    /// прерываний.
    enum struct latch_int_t : std::uint8_t {
        /// @brief Прерывания (кроме связанных с шагами) работают в режиме без
        /// защёлкивания (non-latched). Флаг сбрасывается автоматически, когда
        /// условие прерывания перестаёт выполняться.
        non_latch_mode = 0,

        /// @brief Прерывания (кроме связанных с шагами) работают в режиме
        /// защёлкивания (latched). Флаг остаётся установленным до тех пор, пока
        /// не будет очищен чтением статусного регистра или согласно настройке
        /// int_rd_clr.
        latch_mode = 1,
    };

    /// @brief Настройка режима защёлкивания для прерываний (кроме связанных с
    /// подсчётом шагов).
    latch_int_t latch_int{latch_int_t::non_latch_mode};
    // -------------------------------------------------------------------------

  private:
    /// @brief Парсинг сырого значения регистра в поля класса.
    ///
    /// @param reg Сырое значение регистра для парсинга.
    void parse(
        qma6100_reg_type reg)
    {
        {
            constexpr qma6100_reg_type int_rd_clr_mask{0x01};
            int_rd_clr = extract_field<decltype(int_rd_clr)>(
                reg, int_rd_clr_offset, int_rd_clr_mask);
        }

        {
            constexpr qma6100_reg_type shadow_dis_mask{0x01};
            shadow_dis = extract_field<decltype(shadow_dis)>(
                reg, shadow_dis_offset, shadow_dis_mask);
        }

        {
            constexpr qma6100_reg_type dis_i2c_mask{0x01};
            dis_i2c = extract_field<decltype(dis_i2c)>(reg, dis_i2c_offset,
                                                       dis_i2c_mask);
        }

        {
            constexpr qma6100_reg_type latch_int_step_mask{0x01};
            latch_int_step = extract_field<decltype(latch_int_step)>(
                reg, latch_int_step_offset, latch_int_step_mask);
        }

        {
            constexpr qma6100_reg_type latch_int_mask{0x01};
            latch_int = extract_field<decltype(latch_int)>(
                reg, latch_int_offset, latch_int_mask);
        }
    }
};

class qma6100_pm_reg
{
    static constexpr int mode_bit_offset{7};
    static constexpr int t_rstb_sinc_sel_offset{4};
    static constexpr int mclk_sel_offset{0};

  public:
    /// @brief Адрес регистра BANDWIDTH в памяти устройства.
    static constexpr qma6100_reg_type addr{0x11};

    //
    explicit operator qma6100_reg_type() const
    {
        return static_cast<qma6100_reg_type>(
            field_to_raw(mode_bit, mode_bit_offset)
            | field_to_raw(t_rstb_sinc_sel, t_rstb_sinc_sel_offset)
            | field_to_raw(mclk_sel, mclk_sel_offset));
    }

    explicit qma6100_pm_reg(
        qma6100_reg_type value = qma6100_reg_type{0})
    { parse(value); }

    bool operator==(
        const qma6100_pm_reg &other) const
    {
        return static_cast<qma6100_reg_type>(*this)
               == static_cast<qma6100_reg_type>(other);
    }

    enum struct mode_bit_t : std::uint8_t {
        active  = 1,
        standby = 0,
    };

    mode_bit_t mode_bit{mode_bit_t::active};
    // -------------------------------------------------------------------------

    enum struct t_rstb_sinc_sel_t : std::uint8_t {
        k_3_mult_mckl = 0,
        k_4_mult_mckl = 1,
        k_6_mult_mckl = 2,
        k_8_mult_mckl = 3,
    };

    t_rstb_sinc_sel_t t_rstb_sinc_sel{t_rstb_sinc_sel_t::k_3_mult_mckl};
    // -------------------------------------------------------------------------

    enum struct mclk_sel_t : std::uint8_t {
        freq_500k = 0,
        freq_333k = 1,
        freq_200k = 2,
        freq_100k = 3,
        freq_50k  = 4,
        freq_20k  = 5,
        freq_10k  = 6,
        freq_5k   = 7,
    };

    mclk_sel_t mclk_sel{mclk_sel_t::freq_500k};
    // -------------------------------------------------------------------------

  private:
    /// @brief Парсинг сырого значения регистра в поля класса.
    ///
    /// @details Извлекает битовые поля, соответствующие настройкам bw и nlpf,
    /// из переданного сырого значения регистра и сохраняет их в
    /// соответствующих полях объекта.
    /// @param reg Сырое значение регистра для парсинга.
    void parse(
        qma6100_reg_type reg)
    {
        {
            constexpr qma6100_reg_type mode_bit_mask{0x01};
            mode_bit = extract_field<decltype(mode_bit)>(reg, mode_bit_offset,
                                                         mode_bit_mask);
        }

        {
            constexpr qma6100_reg_type t_rstb_sinc_mask{0x03};
            t_rstb_sinc_sel = extract_field<decltype(t_rstb_sinc_sel)>(
                reg, t_rstb_sinc_sel_offset, t_rstb_sinc_mask);
        }

        {
            constexpr qma6100_reg_type mclk_sel_mask{0x0F};
            mclk_sel = extract_field<decltype(mclk_sel)>(reg, mclk_sel_offset,
                                                         mclk_sel_mask);
        }
    }
};

class qma6100_st_reg
{
    static constexpr int step_by_axix_offset{0};
    static constexpr int selftest_sing_offset{2};
    static constexpr int selftest_bit_offset{7};

  public:
    static constexpr stv::qma6100_reg_type addr{0x32};

    enum struct selftest_bit_t : std::uint8_t {
        normal  = 0,
        enabled = 1,
    };

    enum struct selftest_sign_t : std::uint8_t {
        negative = 0,
        positive = 1,
    };

    selftest_bit_t  selftest_bit{selftest_bit_t::normal};
    selftest_sign_t selftest_sign{selftest_sign_t::positive};

    explicit qma6100_st_reg(
        stv::qma6100_reg_type value = stv::qma6100_reg_type{0})
    { parse(value); }

    explicit operator stv::qma6100_reg_type() const
    {
        return static_cast<stv::qma6100_reg_type>(
            field_to_raw(selftest_bit, selftest_bit_offset)
            | field_to_raw(selftest_sign, selftest_sing_offset));
    }

  private:
    void parse(
        stv::qma6100_reg_type reg)
    {
        {
            constexpr qma6100_reg_type selftest_mask{0x80};
            selftest_bit = static_cast<selftest_bit_t>(
                (reg >> selftest_bit_offset) & selftest_mask);
        }
        {
            constexpr qma6100_reg_type selftest_sign_mask{0x04};
            selftest_sign = static_cast<selftest_sign_t>(
                (reg >> selftest_sing_offset) & selftest_sign_mask);
        }
    }
};

// NOLINTEND(*hicpp-signed-bitwise, *-member*)

/// @brief Структура для хранения настройки регистров датчика QMA6100.
///
/// @details Эта структура используется как контейнер для хранения конфигураций
/// различных регистров датчика перед их записью через класс qma6100_i2c.
/// Позволяет сгруппировать все настройки в одном месте. Может быть расширена
/// для включения конфигураций других регистров в будущем.
struct qma6100_regs_setup {
    /// @brief Настройка регистра полосы пропускания и фильтра (qma6100_bw_reg).
    stv::qma6100_bw_reg          bw_reg;

    stv::qma6100_fsr_reg         fsr_reg;

    stv::qma6100_int_en1_reg     int_en1_reg;

    stv::qma6100_int_map1_reg    int_map1_reg;

    stv::qma6100_int_map3_reg    int_map3_reg;

    stv::qma6100_intpin_conf_reg intpin_conf_reg;

    stv::qma6100_int_cfg_reg     int_cfg_reg;

    stv::qma6100_pm_reg          pm_reg;
};

} // namespace stv

#endif /* QMA6100_REGS_HPP */
