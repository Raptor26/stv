/// @file gnss_dialect.hpp
/// @author vgtutf081
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.

#ifndef GNSS_DIALECT_HPP
#define GNSS_DIALECT_HPP

#include <cstdint>

namespace stv {

/// @brief Диалект обмена данными бортового GNSS-приемника.
///
/// @details Нейтральный формат буферизации последних полученных от приемника
/// данных. Заполняется модулем, разбирающим входящий поток (например NMEA
/// 0183), и потребляется приложениями без привязки к конкретному приёмнику.
/// Не зависит от аппаратуры и операционной системы.
struct gnss_dialect {
    /// @brief Метка времени, увеличивается при получении каждого сообщения.
    std::uint32_t timestamp{0};

    /// @brief Оценка скорости, м/с (северная составляющая).
    std::int32_t vel_n{0};
    /// @brief Оценка скорости, м/с (восточная составляющая).
    std::int32_t vel_e{0};
    /// @brief Оценка скорости, м/с (вертикальная составляющая, вниз).
    std::int32_t vel_d{0};

    /// @brief Оценка широты, градусы * 1e-7.
    std::int32_t lat{0};
    /// @brief Оценка долготы, градусы * 1e-7.
    std::int32_t lon{0};
    /// @brief Высота над эллипсоидом, м.
    float height{0.0F};

    /// @brief Тип достоверности навигационного решения.
    enum class fix_t : std::uint8_t {
        no_fix,         ///< 0: решение отсутствует
        dead_reckoning, ///< 1: только счисление (dead reckoning)
        fix_2d,         ///< 2: 2D fix
        fix_3d,         ///< 3: 3D fix
        fix_gnss_dr,    ///< 4: GNSS + dead reckoning
        time_only,      ///< 5: только время (time only fix)
    };

    /// @brief Тип достоверности данных. Интерпретируется как валидный фикс
    /// только при gnss_fix_ok == true.
    fix_t fix{fix_t::no_fix};

    /// @brief Число использованных спутников.
    std::uint8_t num_sv{0};

    /// @brief true, если навигационное решение достоверно.
    bool gnss_fix_ok{false};

    /// @brief Оценка приемником погрешности позиции по горизонтали, мм.
    std::uint32_t h_acc{0};
    /// @brief Оценка приемником погрешности позиции по вертикали, мм.
    std::uint32_t v_acc{0};

    /// @brief Геометрический фактор точности (PDOP).
    float pdop{0.0F};
};

} // namespace stv

#endif /* GNSS_DIALECT_HPP */
