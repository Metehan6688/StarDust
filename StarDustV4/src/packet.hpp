#pragma once

#include <stdint.h>
#include "config.hpp"


namespace starDustNS::packet{
    using funcCode_t = uint16_t;

    struct [[gnu::packed]] address_t {
        uint8_t squadID;
        uint8_t unitID;
    };

    struct [[gnu::packed]] header_t {
        uint8_t firstByte;
        uint8_t secondByte;
        address_t sender;
        address_t receiver;
        funcCode_t functionCode;
    };

    struct [[gnu::packed]] packet_t {
        header_t header;
        uint8_t payload[config::PAYLOAD_LEN];
        uint8_t signature[config::SIGNATURE_LEN];
        uint16_t crc16;
    };
}
