#pragma once

#include <stdint.h>
#include "config.hpp"

namespace starDustNS::security{
    inline uint16_t calculateCRC16(const uint8_t* data, size_t len){
        uint16_t CRC = config::CRC16_INIT;
        for (size_t n = 0; n < len; ++n) {
            CRC ^= data[n];
            for (int m = 0; m < 8; ++m) {
                if (CRC & 1) CRC = (CRC >> 1) ^ config::CRC16_POLY;
                else CRC >>= 1;
            }
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
