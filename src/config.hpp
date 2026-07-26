#pragma once

#include <stdint.h>
#include <stddef.h>
#include <array>


#define USE_DEFAULT_TUMEN_STARNET
// #define USE_ARDUINO_FRAMEWORK
// #define USE_ESPIDF_FRAMEWORK
#define USE_LINUX_FRAMEWORK

#define ENABLE_BROADCAST_IN_PARSER
#define ENABLE_MULTICAST_IN_PARSER


namespace starDustNS::config {
    inline std::array<uint8_t, 2> myAddress = {0x00, 0x00};
    
    inline constexpr uint8_t FIRST_BYTE = 0xAA;
    inline constexpr uint8_t SECOND_BYTE = 0xAA;
    inline constexpr size_t PAYLOAD_LEN = 16;
    inline constexpr size_t SIGNATURE_LEN = 6;
    inline constexpr uint8_t SIGNATURE[SIGNATURE_LEN] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05};
    inline std::array<uint8_t, 16> CRYPTO_KEY = {};

    #if defined(ENABLE_BROADCAST_IN_PARSER) || defined(ENABLE_MULTICAST_IN_PARSER)
    inline constexpr uint8_t WILDCARD_BYTE = 0xFF;
    #endif

    inline constexpr uint16_t CRC16_INIT = 0xFFFF;
    inline constexpr uint16_t CRC16_POLY = 0xA001;

    inline constexpr uint32_t TIMEOUT_MS = 50;
}
