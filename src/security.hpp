#pragma once

#include <stdint.h>
#include <array>
#include "config.hpp"


namespace starDustNS::security{
    namespace detail {
        inline constexpr std::array<std::uint16_t, 256> makeCRC16Table() {
            std::array<std::uint16_t, 256> table{};
            for (std::uint32_t i = 0; i < 256; ++i) {
                std::uint16_t crc = static_cast<std::uint16_t>(i);
                for (int bit = 0; bit < 8; ++bit) {
                    crc = (crc & 1) ? (crc >> 1) ^ config::CRC16_POLY : (crc >> 1);
                }
                table[i] = crc;
            }
            return table;
        }

        inline constexpr auto CRC16_TABLE = makeCRC16Table();
    }

    inline std::uint16_t calculateCRC16(const uint8_t* data, size_t len){
        std::uint16_t CRC = config::CRC16_INIT;
        for (size_t n = 0; n < len; ++n) {
            CRC = static_cast<std::uint16_t>((CRC >> 8) ^ detail::CRC16_TABLE[(CRC ^ data[n]) & 0xFF]);
        }
        return CRC;
    }

    inline bool verifySignature(const std::uint8_t* expected, const std::uint8_t* received, size_t len){
        std::uint8_t difference = 0;
        for (std::size_t n = 0; n < len; ++n) {
            difference |= static_cast<std::uint8_t>(expected[n] ^ received[n]);
        }
        return difference == 0;
    }

    #if defined(ENABLE_CRYPTO_IN_PAYLOAD)
    namespace crypto{
        inline void encryptPayload(std::uint8_t* payload, std::size_t size) {
            for (size_t i = 0; i < size; ++i) {
                payload[i] ^= config::CRYPTO_KEY[i % 16];
                payload[i] = ((payload[i] << 3) | (payload[i] >> 5)) & 0xFF;
            }
        }   

        inline void decryptPayload(std::uint8_t* payload, std::size_t size) {
            for (std::size_t i = 0; i < size; ++i) {
                payload[i] = ((payload[i] >> 3) | (payload[i] << 5)) & 0xFF;
                payload[i] ^= config::CRYPTO_KEY[i % 16];
            }   
        }
    }
    #endif
}
