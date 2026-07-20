#pragma once

#include <stdint.h>
#include <stddef.h>

#define USE_DEFAULT_TUMEN_STARNET
// #define USE_ARDUINO_FRAMEWORK
// #define USE_ESPIDF_FRAMEWORK
#define USE_LINUX_FRAMEWORK


namespace starDustNS::config {
    inline constexpr uint8_t FIRST_BYTE = 0xAA;
    inline constexpr uint8_t SECOND_BYTE = 0xAA;
    inline constexpr size_t PAYLOAD_LEN = 16;
    inline constexpr size_t SIGNATURE_LEN = 6;
    inline constexpr uint8_t SIGNATURE[SIGNATURE_LEN] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05};
    inline constexpr uint8_t CRYPTO_KEY[16] = {};

    inline constexpr uint16_t CRC16_INIT = 0xFFFF;
    inline constexpr uint16_t CRC16_POLY = 0xA001;

    inline constexpr uint32_t TIMEOUT_MS = 50;
}
