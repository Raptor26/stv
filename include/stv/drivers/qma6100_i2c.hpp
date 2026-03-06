/// @file qma6100_i2c.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.

#ifndef QMA6100_I2C_HPP
#define QMA6100_I2C_HPP

#include "qma6100_types.hpp"
#include "stv/i2c.hpp"
#include "stv/utils.hpp"

namespace stv {

/// @brief Структура для инициализации класса qma6100_i2c (настройки подключения
/// по I2C).
///
/// @details Содержит параметры, необходимые для установления связи с датчиком
/// QMA6100 по шине I2C. Используется конструктором класса qma6100_i2c.
struct qma6100_i2c_setup {
    /// @brief I2C адрес устройства, когда вывод AD0 подключен к земле (GND).
    static constexpr qma6100_reg_type i2c_addr_connect_to_gnd{0x12};

    /// @brief I2C адрес устройства, когда вывод AD0 подключен к питанию (VDD).
    static constexpr qma6100_reg_type i2c_addr_connect_to_vdd{0x13};

    /// @brief Указатель на объект интерфейса шины I2C (i2c_interface).
    stv::i2c_interface *i2c{nullptr};

    /// @brief Адрес устройства на шине I2C. По умолчанию используется адрес для
    /// AD0=GND.
    qma6100_reg_type i2c_addr{i2c_addr_connect_to_gnd};
};

/// @brief Класс для выполнения операций чтения и записи регистров датчика
/// QMA6100 по шине I2C.
///
/// @details Этот класс предоставляет высокоуровневый интерфейс для обмена
/// данными с датчиком QMA6100 через шину I2C. Он инкапсулирует логику работы с
/// низкоуровневым драйвером I2C (i2c_interface) и предоставляет удобные
/// типобезопасные методы для чтения и записи регистров, представленных в виде
/// классов (например, qma6100_bw_reg, qma6100_fsr_reg). Поддерживает чтение
/// сырых байтов, чтение с автоматическим парсингом в структуры, а также запись
/// как сырых значений, так и структур.
class qma6100_i2c:
    public virtual stv::non_copyable,
    public virtual stv::non_movable
{
    /// @brief Указатель на интерфейс шины I2C.
    const stv::i2c_interface *i2c_;

    /// @brief Адрес устройства на шине I2C.
    const qma6100_reg_type i2c_addr_;

  public:
    /// @brief Конструктор с инициализацией интерфейса I2C и адреса устройства.
    ///
    /// @param[in] setup Константная ссылка на структуру qma6100_i2c_setup,
    /// содержащую необходимые параметры для инициализации.
    explicit qma6100_i2c(
        const stv::qma6100_i2c_setup &setup):
        i2c_{setup.i2c},
        i2c_addr_{setup.i2c_addr}
    {
    }

    virtual ~qma6100_i2c() = default;

    /// @brief Проверка корректности инициализации объекта.
    ///
    /// @details Позволяет использовать объект в условиях (if/while).
    /// @return true, если указатель на интерфейс I2C (i2c_) не равен nullptr.
    virtual explicit operator bool() const { return i2c_ != nullptr; }

    /// @brief Чтение последовательности байтов из регистров устройства.
    ///
    /// @param reg_addr Адрес начального регистра для чтения.
    /// @param dst Указатель на буфер в памяти пользователя, куда будут записаны
    /// данные.
    /// @param len Количество байтов для чтения.
    /// @return Результат операции чтения (зависит от реализации i2c_interface).
    auto read(
        qma6100_reg_type reg_addr, void *dst, std::size_t len) const
    { return i2c_->read(i2c_addr_, reg_addr, dst, len); }

    /// @brief Чтение одного байта из указанного регистра устройства.
    ///
    /// @param reg_addr Адрес регистра для чтения.
    /// @return Значение прочитанного регистра (тип qma6100_reg_type).
    [[nodiscard]] auto read(
        qma6100_reg_type reg_addr) const
    {
        qma6100_reg_type value; // NOLINT(*-init-variables)
        read(reg_addr, reinterpret_cast<void *>(&value), sizeof(value));
        return value;
    }

    /// @brief Чтение регистра и автоматическое преобразование в структурный
    /// тип.
    ///
    /// @tparam U Тип регистра (например, qma6100_bw_reg), который должен иметь
    /// статическое поле `addr` и конструктор, принимающий qma6100_reg_type.
    /// @return Экземпляр типа U, инициализированный значением, прочитанным из
    /// регистра по адресу U::addr.
    template<typename U>
    [[nodiscard]] auto read()
    {
        using red_type = std::remove_cvref_t<U>;
        return red_type{read(red_type::addr)};
    }

    /// @brief Запись одного байта в указанный регистр устройства.
    ///
    /// @param reg_addr Адрес регистра для записи.
    /// @param value Значение для записи в регистр.
    /// @return Результат операции записи (зависит от реализации i2c_interface).
    auto write(
        qma6100_reg_type reg_addr, qma6100_reg_type value)
    { return i2c_->write(i2c_addr_, reg_addr, value); }

    /// @brief Запись структурного типа в соответствующий регистр устройства.
    ///
    /// @details Адрес регистра и значение для записи извлекаются из переданного
    /// объекта `reg` с помощью операторов преобразования.
    /// @param reg Ссылка на объект, представляющий регистр (например,
    /// qma6100_bw_reg). Объект должен иметь статическое поле `addr` и
    /// поддерживать преобразование в qma6100_reg_type.
    /// @return Результат операции записи (зависит от реализации i2c_interface).
    auto write(
        const auto &reg)
    { return write(reg.addr, static_cast<qma6100_reg_type>(reg)); }
};

} // namespace stv

#endif /* QMA6100_I2C_HPP */
