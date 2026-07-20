#pragma once

#include <stdint.h>
#include <string.h>
#include "packet.hpp"
#include "config.hpp"
#include "security.hpp"
#include "port/port.hpp"


namespace starDustNS::sender{
    namespace detail {
        inline void preparePacket(packet::packet_t& pack,
                            packet::address_t myID, packet::address_t targetID,
                            packet::funcCode_t order, const uint8_t* payloadData){
            using starDustNS::security::calculateCRC16;
            using starDustNS::security::crypto::encryptPayload;

            pack.header.firstByte    = config::FIRST_BYTE;
            pack.header.secondByte   = config::SECOND_BYTE;
            pack.header.sender       = myID;
            pack.header.receiver     = targetID;
            pack.header.functionCode = order;

            memset(pack.payload, 0, config::PAYLOAD_LEN);
            memcpy(pack.payload, payloadData, config::PAYLOAD_LEN);
            encryptPayload(pack.payload, config::PAYLOAD_LEN);

            memcpy(pack.signature, config::SIGNATURE, config::SIGNATURE_LEN);

            pack.crc16 = calculateCRC16(
                reinterpret_cast<const uint8_t*>(&pack),
                sizeof(packet::packet_t) - sizeof(pack.crc16));
        }
    }

    inline bool sendPacket(port::internalPort& outPort,
                     packet::address_t myID, packet::address_t targetID,
                     packet::funcCode_t order, const uint8_t* payloadData){
        packet::packet_t pack{};
        detail::preparePacket(pack, myID, targetID, order, payloadData);

        size_t written = outPort.writeByte(reinterpret_cast<const uint8_t*>(&pack), sizeof(pack));
        return written == sizeof(pack);
    }
}
