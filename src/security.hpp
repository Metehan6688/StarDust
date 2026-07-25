#pragma once

#include <stdint.h>
#include <array>
#include "config.hpp"


namespace starDustNS::security{
    namespace detail {
        inline constexpr std::array<uint16_t, 256> makeCRC16Table() {
            std::array<uint16_t, 256> table{};
            for (uint32_t i = 0; i < 256; ++i) {
                uint16_t crc = static_cast<uint16_t>(i);
                for (int bit = 0; bit < 8; ++bit) {
                    crc = (crc & 1) ? (crc >> 1) ^ config::CRC16_POLY : (crc >> 1);
                }
                table[i] = crc;
            }
            return table;
        }

        inline constexpr auto CRC16_TABLE = makeCRC16Table();
    }

    inline uint16_t calculateCRC16(const uint8_t* data, size_t len){
        uint16_t CRC = config::CRC16_INIT;
        for (size_t n = 0; n < len; ++n) {
            CRC = static_cast<uint16_t>((CRC >> 8) ^ detail::CRC16_TABLE[(CRC ^ data[n]) & 0xFF]);
        }
        return CRC;
    }

    inline bool verifySignature(const uint8_t* expected, const uint8_t* received, size_t len){
        uint8_t difference = 0;
        for (size_t n = 0; n < len; ++n) {
            difference |= static_cast<uint8_t>(expected[n] ^ received[n]);
        }
        return difference == 0;
    }

    namespace crypto{
        inline void encryptPayload(uint8_t* payload, size_t size) {
            for (size_t i = 0; i < size; ++i) {
                payload[i] ^= config::CRYPTO_KEY[i % 16];
                payload[i] = ((payload[i] << 3) | (payload[i] >> 5)) & 0xFF;
            }
        }   

        inline void decryptPayload(uint8_t* payload, size_t size) {
            for (size_t i = 0; i < size; ++i) {
                payload[i] = ((payload[i] >> 3) | (payload[i] << 5)) & 0xFF;
                payload[i] ^= config::CRYPTO_KEY[i % 16];
            }   
        }
    }
}
