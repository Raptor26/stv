/// @file legacy_bus.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.

#ifndef STV_DRIVERS_LEGACY_BUS_HPP
#define STV_DRIVERS_LEGACY_BUS_HPP

#include <cstdint>
#include <span>

namespace stv::drivers {

/// @brief Serial bus span type.
using serial_bus_span_t = std::span<std::uint8_t>;

enum class I2C_SlaveAddressType : std::uint8_t {
    kStart = 1,

    // Using for I2C.
    k7bit = kStart,

    // Using for I2C.
    k10bit = 2,

    kMaxNumber = 3
};

/// @brief Интерфейс для операций чтения/записи регистров датчика QMC5883.
class I2CBus7BitAddr
{
  public:
    /// @brief Виртуальный деструктор по умолчанию.
    virtual ~I2CBus7BitAddr() = default;

    I2CBus7BitAddr(const I2CBus7BitAddr &other) = delete;
    I2CBus7BitAddr(I2CBus7BitAddr &&other)      = delete;

    auto operator=(const I2CBus7BitAddr &other) -> I2CBus7BitAddr & = delete;
    auto operator=(I2CBus7BitAddr &&other) -> I2CBus7BitAddr &      = delete;

    /// @brief Записывает значение в указанный регистр устройства.
    ///
    /// @param[in] slave_addr: Адрес ведомого устройства.
    /// @param[in] reg_addr: Адрес регистра для записи.
    /// @param[in] write_reg_value: Значение для записи.
    /// @return true, если операция успешно завершена, false в противном
    ///         случае.
    virtual auto WriteReg(std::uint8_t slave_addr, std::uint8_t reg_addr,
                          std::uint8_t write_reg_value) -> bool = 0;

    /// @brief Считывает указанное количество регистров начиная с адреса.
    ///
    /// @param[in] slave_addr: Адрес ведомого устройства.
    /// @param[in] reg_addr: Адрес первого регистра для чтения.
    /// @param[out] dst: Указатель на буфер для записи данных.
    /// @param[in] len: Количество регистров для чтения.
    /// @return true, если операция успешно завершена, false в противном
    ///         случае.
    virtual auto ReadMultiReg(std::uint8_t slave_addr, uint8_t reg_addr,
                              std::uint8_t *dst, std::size_t len) -> bool = 0;

  protected:
    /// @brief Конструктор по умолчанию.
    I2CBus7BitAddr() = default;
};

class ISerialBus
{
  protected:
    ISerialBus() = default;

  public:
    virtual ~ISerialBus() = default;

    /// @brief Five rule.
    ISerialBus(ISerialBus &&other)                          = delete;
    auto operator=(ISerialBus &&other) -> ISerialBus &      = delete;
    auto operator=(const ISerialBus &other) -> ISerialBus & = delete;
    ISerialBus(const ISerialBus &other)                     = delete;

    /// @brief Метод выполняет считывание данных с указанного регистра.
    /// @param[in] address: Адрес регистра.
    /// @param[in] address_type:
    /// @param[in] dst_buff: Буфер, в который необходимо считать данные.
    /// @return Возвращает true в случае успешного чтения данных, иначе - false.
    [[nodiscard]] virtual auto ReadReg(serial_bus_span_t    address,
                                       I2C_SlaveAddressType address_type,
                                       serial_bus_span_t    dst_buff) const
        -> bool = 0;

    /// @brief Метод выполняет запись данных в указанный регистр.
    /// @param[in] address: Адрес регистра.
    /// @param[in] address_type:
    /// @param[in] src_buff: Буфер, из которого необходимо записать данные.
    /// @return Возвращает true в случае успешной отправки данных, иначе -
    /// false.
    [[nodiscard]] virtual auto WriteReg(serial_bus_span_t    address,
                                        I2C_SlaveAddressType address_type,
                                        serial_bus_span_t    src_buff) const
        -> bool = 0;

    /// @brief Метод реализует программную задержку продолжительностью 1 мс.
    virtual void Delay1ms() const = 0;
};

} // namespace stv::drivers

namespace kraslibs::drivers {

using serial_bus_span_t    = stv::drivers::serial_bus_span_t;
using I2C_SlaveAddressType = stv::drivers::I2C_SlaveAddressType;
using I2CBus7BitAddr       = stv::drivers::I2CBus7BitAddr;
using ISerialBus           = stv::drivers::ISerialBus;

} // namespace kraslibs::drivers

#endif /* STV_DRIVERS_LEGACY_BUS_HPP */
