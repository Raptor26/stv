/// @file i2c_sensor_i2c.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.

#ifndef I2C_SENSOR_I2C_HPP
#define I2C_SENSOR_I2C_HPP

#include "stv/i2c.hpp"
#include "stv/utils.hpp"
#include <cstddef>
#include <cstdint>

namespace stv {

/// @brief Структура для инициализации класса i2c_sensor_i2c (настройки
/// подключения по I2C).
///
/// @details Содержит параметры, необходимые для установления связи с датчиком
/// по шине I2C. Используется конструктором класса i2c_sensor_i2c.
/// @tparam RegType Тип регистра (например, std::uint8_t).
/// @tparam DeviceI2CAddr 7-битный I2C адрес устройства.
template<typename RegType, RegType DeviceI2CAddr>
struct i2c_sensor_i2c_setup {
    /// @brief I2C адрес устройства (7-битный).
    static constexpr RegType i2c_addr{DeviceI2CAddr};

    /// @brief Указатель на объект интерфейса шины I2C (i2c_interface).
    stv::i2c_interface *i2c{nullptr};
};

/// @brief Класс для выполнения операций чтения и записи регистров датчика
/// по шине I2C.
///
/// @details Этот класс предоставляет высокоуровневый интерфейс для обмена
/// данными с датчиком через шину I2C. Он инкапсулирует логику работы с
/// низкоуровневым драйвером I2C (i2c_interface) и предоставляет удобные
/// типобезопасные методы для чтения и записи регистров.
///
/// @tparam RegType Тип регистра (например, std::uint8_t).
/// @tparam DeviceI2CAddr 7-битный I2C адрес устройства.
template<typename RegType, RegType DeviceI2CAddr>
class i2c_sensor_i2c: public stv::non_movable_non_copyable
{
    /// @brief Указатель на интерфейс шины I2C.
    const stv::i2c_interface *i2c_;

    /// @brief Адрес устройства на шине I2C.
    const RegType i2c_addr_;

  public:
    using reg_type = RegType;
    static constexpr RegType device_i2c_addr{DeviceI2CAddr};

    /// @brief Конструктор с инициализацией интерфейса I2C и адреса устройства.
    ///
    /// @param[in] setup Константная ссылка на структуру i2c_sensor_i2c_setup.
    explicit i2c_sensor_i2c(
        const i2c_sensor_i2c_setup<RegType, DeviceI2CAddr> &setup):
        i2c_{setup.i2c},
        i2c_addr_{setup.i2c_addr}
    {
    }

    virtual ~i2c_sensor_i2c() = default;

    /// @brief Проверка корректности инициализации объекта.
    ///
    /// @return true, если указатель на интерфейс I2C (i2c_) не равен nullptr.
    virtual explicit operator bool() const { return i2c_ != nullptr; }

    /// @brief Чтение последовательности байтов из регистров устройства.
    ///
    /// @param reg_addr Адрес начального регистра для чтения.
    /// @param dst Указатель на буфер в памяти пользователя.
    /// @param len Количество байтов для чтения.
    /// @return Результат операции чтения.
    auto read(
        RegType reg_addr, void *dst, std::size_t len) const
    {
        return i2c_->read(static_cast<stv::i2c_interface::byte_type>(i2c_addr_),
                          static_cast<stv::i2c_interface::byte_type>(reg_addr),
                          dst, len);
    }

    /// @brief Чтение одного байта из указанного регистра устройства.
    ///
    /// @param reg_addr Адрес регистра для чтения.
    /// @return Значение прочитанного регистра.
    [[nodiscard]] auto read(
        RegType reg_addr) const
    {
        RegType value{0}; // NOLINT(*-init-variables)
        read(reg_addr, reinterpret_cast<void *>(&value), sizeof(value));
        return value;
    }

    /// @brief Чтение регистра и автоматическое преобразование в структурный
    /// тип.
    ///
    /// @tparam U Тип регистра, который должен иметь статическое поле `addr` и
    /// конструктор, принимающий RegType.
    /// @return Экземпляр типа U, инициализированный значением из регистра.
    template<typename U>
    [[nodiscard]] auto read()
    {
        using read_type = std::remove_cvref_t<U>;
        return read_type{read(read_type::addr)};
    }

    /// @brief Запись одного байта в указанный регистр устройства.
    ///
    /// @param reg_addr Адрес регистра для записи.
    /// @param value Значение для записи в регистр.
    /// @return Результат операции записи.
    auto write(
        RegType reg_addr, RegType value)
    {
        return i2c_->write(
            static_cast<stv::i2c_interface::byte_type>(i2c_addr_),
            static_cast<stv::i2c_interface::byte_type>(reg_addr),
            static_cast<stv::i2c_interface::byte_type>(value));
    }

    /// @brief Запись структурного типа в соответствующий регистр устройства.
    ///
    /// @param reg Ссылка на объект, представляющий регистр. Должен иметь
    /// статическое поле `addr` и поддерживать преобразование в RegType.
    /// @return Результат операции записи.
    auto write(
        const auto &reg)
    { return write(reg.addr, static_cast<RegType>(reg)); }
};

} // namespace stv

#endif /* I2C_SENSOR_I2C_HPP */
