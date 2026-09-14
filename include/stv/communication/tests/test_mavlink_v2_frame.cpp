/// @file test_mavlink_v2_frame.cpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// SPDX-License-Identifier: MIT.
/// See LICENSE file in the project root for full license information.

#include "stv/communication/crc16.hpp"
#include "stv/communication/mavlink_v2_frame.hpp"
#include "stv/communication/serial_decorators.hpp"
#include <array>
#include <catch2/catch_test_macros.hpp>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

// NOLINTBEGIN(*-magic-numbers)

// Catch2 SECTION-макросы резко завышают когнитивную сложность теста;
// разбивать единый тестовый сценарий ради метрики нецелесообразно.
// NOLINTBEGIN(readability-function-cognitive-complexity)

namespace {

/// @brief Тестовый провайдер CRC_EXTRA с маленькой таблицей на 2 msgid.
struct test_crc_extra_provider {
    static auto crc_extra_for(
        std::uint32_t msgid) -> std::optional<std::uint8_t>
    {
        switch(msgid)
        {
            case 0U:
                return std::uint8_t{50U};  // HEARTBEAT
            case 1U:
                return std::uint8_t{124U}; // SYS_STATUS
            default:
                return std::nullopt;
        }
    }
};

using frame_type = stv::mavlink_v2_frame<test_crc_extra_provider>;

/// @brief Собирает кадр MAVLink v2 с корректным CRC.
///
/// @param[in] incompat_flags Флаги несовместимости (бит 0 — подпись).
/// @param[in] sysid Идентификатор системы.
/// @param[in] compid Идентификатор компонента.
/// @param[in] msgid Идентификатор сообщения (24 бита).
/// @param[in] payload Полезная нагрузка.
/// @param[in] crc_extra Байт CRC_EXTRA для msgid.
/// @return Собранный кадр; при установленном бите подписи в конец
///     добавляются 13 нулевых байт подписи.
auto make_frame(
    std::uint8_t incompat_flags, std::uint8_t sysid, std::uint8_t compid,
    std::uint32_t msgid, const std::vector<std::byte> &payload,
    std::uint8_t crc_extra) -> std::vector<std::byte>
{
    std::vector<std::byte> frame{
        std::byte{0xFD},
        static_cast<std::byte>(payload.size()),
        std::byte{incompat_flags},
        std::byte{0x00}, // compat_flags
        std::byte{0x00}, // seq
        std::byte{sysid},
        std::byte{compid},
        static_cast<std::byte>(msgid & 0xFFU),
        static_cast<std::byte>((msgid >> 8U) & 0xFFU),
        static_cast<std::byte>((msgid >> 16U) & 0xFFU),
    };
    frame.insert(frame.end(), payload.begin(), payload.end());

    auto crc = stv::crc16_x25(frame.data() + 1U, frame.size() - 1U);
    crc      = stv::crc16_x25_accumulate(crc, std::byte{crc_extra});
    frame.push_back(static_cast<std::byte>(crc & 0xFFU));
    frame.push_back(static_cast<std::byte>(
        (static_cast<std::uint32_t>(crc) >> 8U) & 0xFFU));

    if((incompat_flags & 0x01U) != 0U)
    {
        frame.insert(frame.end(), 13U, std::byte{0x00});
    }

    return frame;
}

} // namespace

TEST_CASE(
    "mavlink_v2_frame", "[stv][communication]")
{
    SECTION("first byte and matches")
    {
        REQUIRE(frame_type::first_byte == std::byte{0xFD});

        // Второй байт (len) может быть любым.
        REQUIRE(frame_type::matches(std::byte{0xFD}, std::byte{0x00}));
        REQUIRE(frame_type::matches(std::byte{0xFD}, std::byte{0xFF}));
        REQUIRE(frame_type::matches(std::byte{0xFD}, std::byte{0x55}));

        REQUIRE_FALSE(frame_type::matches(std::byte{0xFE}, std::byte{0x00}));
        REQUIRE_FALSE(frame_type::matches(std::byte{0x55}, std::byte{0xFD}));
    }

    SECTION("header and trailer sizes")
    {
        REQUIRE(frame_type::header_size() == 3U);
        REQUIRE(frame_type::trailer_size() == 2U);
    }

    SECTION("total frame size")
    {
        // len = 9: 10 (заголовок) + 9 (payload) + 2 (CRC) = 21.
        const std::array<std::byte, 3> header{
            std::byte{0xFD},
            std::byte{0x09},
            std::byte{0x00},
        };
        const stv::total_message_span header_span{header.data(), header.size()};
        REQUIRE(frame_type::total_frame_size(header_span) == 21U);
    }

    SECTION("total frame size with signature flag")
    {
        // Установлен бит подписи (incompat_flags & 0x01): +13 байт.
        const std::array<std::byte, 3> header{
            std::byte{0xFD},
            std::byte{0x09},
            std::byte{0x01},
        };
        const stv::total_message_span header_span{header.data(), header.size()};
        REQUIRE(frame_type::total_frame_size(header_span) == 34U);
    }

    SECTION("reference heartbeat frame")
    {
        // Эталонный кадр HEARTBEAT (msgid = 0, CRC_EXTRA = 50), len = 0.
        // CRC-16/X.25 по байтам len..msgid + 0x32 = 0x8179.
        const std::array<std::byte, 12> frame{
            std::byte{0xFD},                                   // magic
            std::byte{0x00},                                   // len
            std::byte{0x00},                                   // incompat_flags
            std::byte{0x00},                                   // compat_flags
            std::byte{0x00},                                   // seq
            std::byte{0x01},                                   // sysid
            std::byte{0x01},                                   // compid
            std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, // msgid = 0 (LE)
            std::byte{0x79}, std::byte{0x81},                  // CRC
        };

        const stv::total_message_span total{frame.data(), frame.size()};
        REQUIRE(frame_type::is_crc_valid(total));
    }

    SECTION("valid frame with payload")
    {
        const auto frame = make_frame(0x00U, 0x01U, 0x01U, 0U,
                                      {std::byte{0x48}, std::byte{0x69}}, 50U);

        const stv::total_message_span total{frame.data(), frame.size()};
        REQUIRE(frame_type::is_crc_valid(total));
    }

    SECTION("unknown msgid is invalid")
    {
        // msgid = 2 отсутствует в таблице провайдера.
        const auto frame = make_frame(0x00U, 0x01U, 0x01U, 2U,
                                      {std::byte{0x48}, std::byte{0x69}}, 0U);

        const stv::total_message_span total{frame.data(), frame.size()};
        REQUIRE_FALSE(frame_type::is_crc_valid(total));
    }

    SECTION("corrupted crc is invalid")
    {
        auto frame = make_frame(0x00U, 0x01U, 0x01U, 0U,
                                {std::byte{0x48}, std::byte{0x69}}, 50U);
        frame[frame.size() - 1U] = ~frame[frame.size() - 1U];

        const stv::total_message_span total{frame.data(), frame.size()};
        REQUIRE_FALSE(frame_type::is_crc_valid(total));
    }

    SECTION("signed frame is rejected")
    {
        // Верификация подписи не реализована: подписанный кадр отклоняется
        // даже с корректным CRC. Полный размер кадра по заголовку при этом
        // по-прежнему учитывает 13 байт подписи.
        const auto frame = make_frame(0x01U, 0x01U, 0x01U, 0U,
                                      {std::byte{0x48}, std::byte{0x69}}, 50U);

        const stv::total_message_span total{frame.data(), frame.size()};
        REQUIRE_FALSE(frame_type::is_crc_valid(total));

        const stv::total_message_span header_span{frame.data(),
                                                  frame_type::header_size()};
        REQUIRE(frame_type::total_frame_size(header_span) == frame.size());
    }

    SECTION("frame shorter than header plus trailer is invalid")
    {
        // Кадр короче header + trailer не должен приводить к underflow.
        const std::array<std::byte, 3> short_frame{
            std::byte{0xFD},
            std::byte{0x00},
            std::byte{0x00},
        };
        const stv::total_message_span total{short_frame.data(),
                                            short_frame.size()};
        REQUIRE_FALSE(frame_type::is_crc_valid(total));
    }

    SECTION("frame between size guards is invalid")
    {
        // Первый guard (header + trailer = 5) пройден, второй (полный
        // заголовок + CRC = 12) - нет: кадры размером 5..11 байт без
        // подписи отвергаются без underflow size_t.
        for(std::size_t size{5U}; size <= 11U; ++size)
        {
            std::vector<std::byte> frame(size, std::byte{0x00});
            frame[0U] = std::byte{0xFD};

            const stv::total_message_span total{frame.data(), frame.size()};
            REQUIRE_FALSE(frame_type::is_crc_valid(total));
        }

        // Подписанный кадр (бит 0x01 в incompat_flags) отклоняется до
        // проверки размера и CRC: кадры размером 12..24 байта с флагом
        // подписи отвергаются, как и любые кадры с ненулевыми
        // incompat_flags.
        for(std::size_t size{12U}; size <= 24U; ++size)
        {
            std::vector<std::byte> frame(size, std::byte{0x00});
            frame[0U] = std::byte{0xFD};
            frame[2U] = std::byte{0x01}; // флаг подписи

            const stv::total_message_span total{frame.data(), frame.size()};
            REQUIRE_FALSE(frame_type::is_crc_valid(total));
        }
    }

    SECTION("max payload frame")
    {
        // len = 255 (максимум): 10 (заголовок) + 255 (payload) + 2 (CRC).
        const std::vector<std::byte> payload(255U, std::byte{0x41});
        const auto frame = make_frame(0x00U, 0x01U, 0x01U, 0U, payload, 50U);

        REQUIRE(frame.size() == 267U);

        const stv::total_message_span total{frame.data(), frame.size()};
        REQUIRE(frame_type::is_crc_valid(total));

        const stv::total_message_span header_span{frame.data(),
                                                  frame_type::header_size()};
        REQUIRE(frame_type::total_frame_size(header_span) == frame.size());
    }

    SECTION("total frame size boundaries")
    {
        // len = 0: 10 + 0 + 2 = 12; len = 255: 10 + 255 + 2 = 267;
        // подписанный максимум: 267 + 13 = 280.
        const std::array<std::byte, 3> header_empty{
            std::byte{0xFD},
            std::byte{0x00},
            std::byte{0x00},
        };
        const stv::total_message_span empty_span{header_empty.data(),
                                                 header_empty.size()};
        REQUIRE(frame_type::total_frame_size(empty_span) == 12U);

        const std::array<std::byte, 3> header_max{
            std::byte{0xFD},
            std::byte{0xFF},
            std::byte{0x00},
        };
        const stv::total_message_span max_span{header_max.data(),
                                               header_max.size()};
        REQUIRE(frame_type::total_frame_size(max_span) == 267U);

        const std::array<std::byte, 3> header_max_signed{
            std::byte{0xFD},
            std::byte{0xFF},
            std::byte{0x01},
        };
        const stv::total_message_span max_signed_span{header_max_signed.data(),
                                                      header_max_signed.size()};
        REQUIRE(frame_type::total_frame_size(max_signed_span) == 280U);
    }

    SECTION("signed frame with corrupted crc is invalid")
    {
        auto frame = make_frame(0x01U, 0x01U, 0x01U, 0U,
                                {std::byte{0x48}, std::byte{0x69}}, 50U);

        // Портим младший байт CRC, расположенный перед 13 байтами подписи.
        // Кадр отклоняется: подписанные кадры не принимаются вне
        // зависимости от корректности CRC.
        frame[frame.size() - 13U - 2U] ^= std::byte{0xFF};

        const stv::total_message_span total{frame.data(), frame.size()};
        REQUIRE_FALSE(frame_type::is_crc_valid(total));
    }

    SECTION("frame with unknown incompat flags is rejected")
    {
        // Кадр с неизвестным битом 0x02 в incompat_flags и корректным CRC
        // отбрасывается: спецификация требует отбрасывать кадры с
        // неопознанными битами incompat_flags.
        const auto frame = make_frame(0x02U, 0x01U, 0x01U, 0U,
                                      {std::byte{0x48}, std::byte{0x69}}, 50U);

        const stv::total_message_span total{frame.data(), frame.size()};
        REQUIRE_FALSE(frame_type::is_crc_valid(total));
    }

    SECTION("msgid is assembled little-endian")
    {
        // msgid = 1 (SYS_STATUS, CRC_EXTRA = 124): байты {0x01, 0x00, 0x00}.
        // При обратном порядке сборки msgid был бы неизвестен провайдеру.
        const auto frame = make_frame(0x00U, 0x01U, 0x01U, 1U,
                                      {std::byte{0x48}, std::byte{0x69}}, 124U);

        REQUIRE(frame[7U] == std::byte{0x01});
        REQUIRE(frame[8U] == std::byte{0x00});
        REQUIRE(frame[9U] == std::byte{0x00});

        const stv::total_message_span total{frame.data(), frame.size()};
        REQUIRE(frame_type::is_crc_valid(total));
    }
}

// NOLINTEND(readability-function-cognitive-complexity)

// NOLINTEND(*-magic-numbers)
