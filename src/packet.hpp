#pragma once

#include <cstdint>
#include "config.hpp"


namespace starDustNS::packet{
    using funcCode_t = std::uint16_t;

    struct [[gnu::packed]] address_t {
        std::uint8_t squadID;
        std::uint8_t unitID;
    };

    struct [[gnu::packed]] header_t {
        std::uint8_t firstByte;
        std::uint8_t secondByte;
        address_t sender;
        address_t receiver;
        funcCode_t functionCode;
    };

    struct [[gnu::packed]] packet_t {
        header_t header;
        std::uint8_t payload[config::PAYLOAD_LEN];
        std::uint8_t signature[config::SIGNATURE_LEN];
        std::uint16_t crc16;
    };
}
