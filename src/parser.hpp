#pragma once

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "packet.hpp"
#include "config.hpp"
#include "security.hpp"


namespace starDustNS::parser {
    enum class parserState : uint8_t {
        IDLE,
        READING_HEADER,
        READING_BODY,
        READING_CRC
    };

    enum class parseResult {
        OK,
        WAIT,
        READING,
        CRC_ERROR,
        SIGNATURE_ERROR,
        INVALID_FRAME,
        TIMEOUT,
        UNDEFINED_ERROR
    };

    inline constexpr bool isError(parseResult r) {
        return r == parseResult::CRC_ERROR
            || r == parseResult::SIGNATURE_ERROR
            || r == parseResult::INVALID_FRAME
            || r == parseResult::TIMEOUT
            || r == parseResult::UNDEFINED_ERROR;
    }

    struct parserCTX_t {
        parserState state = parserState::IDLE;
        uint8_t bytesRead = 0;
        uint32_t lastReadTS = 0;
        uint8_t rxBuffer[sizeof(packet::packet_t)] = {};
    };

    struct exportPack_t {
        packet::address_t sender{};
        packet::address_t receiver{};
        uint16_t functionCode = 0;
        uint8_t payload[config::PAYLOAD_LEN] = {};
        parseResult resultLog = parseResult::WAIT;
    };

    namespace detail {
        inline void resetParser(parserCTX_t& ctx){
            ctx.state = parserState::IDLE;
            ctx.bytesRead = 0;
            memset(ctx.rxBuffer, 0, sizeof(ctx.rxBuffer));
        }
    }

    inline parseResult parsePacket(uint8_t inComingByte, uint32_t millis, exportPack_t& outByte, parserCTX_t& ctx) {
        using starDustNS::security::calculateCRC16;
        using starDustNS::security::verifySignature;

        if (ctx.state != parserState::IDLE && (millis - ctx.lastReadTS) > config::TIMEOUT_MS) {
            detail::resetParser(ctx);
            ctx.lastReadTS = millis;
            outByte = exportPack_t{};
            outByte.resultLog = parseResult::TIMEOUT;

            if (inComingByte == config::FIRST_BYTE) {
                ctx.rxBuffer[0] = inComingByte;
                ctx.bytesRead = 1;
                ctx.state = parserState::READING_HEADER;
            }
            return parseResult::TIMEOUT;
        }
        ctx.lastReadTS = millis;

        switch (ctx.state) {
            case parserState::IDLE:
                if (inComingByte == config::FIRST_BYTE) {
                    ctx.rxBuffer[0] = inComingByte;
                    ctx.bytesRead = 1;
                    ctx.state = parserState::READING_HEADER;
                }
                return parseResult::READING;

            case parserState::READING_HEADER:
                if (ctx.bytesRead == 1 && inComingByte != config::SECOND_BYTE) {
                    detail::resetParser(ctx);
                    return parseResult::INVALID_FRAME;
                }
                ctx.rxBuffer[ctx.bytesRead++] = inComingByte;
                if (ctx.bytesRead == sizeof(packet::header_t)) {
                    ctx.state = parserState::READING_BODY;
                }
                return parseResult::READING;

            case parserState::READING_BODY:
                ctx.rxBuffer[ctx.bytesRead++] = inComingByte;
                if (ctx.bytesRead == sizeof(packet::header_t) + config::PAYLOAD_LEN + config::SIGNATURE_LEN) {
                    ctx.state = parserState::READING_CRC;
                }
                return parseResult::READING;

            case parserState::READING_CRC: {
                ctx.rxBuffer[ctx.bytesRead++] = inComingByte;
                if (ctx.bytesRead < sizeof(packet::packet_t)) {
                    return parseResult::READING;
                }

                auto& received = *reinterpret_cast<packet::packet_t*>(ctx.rxBuffer);

                uint16_t computedCRC = calculateCRC16(ctx.rxBuffer, sizeof(packet::packet_t) - sizeof(received.crc16));

                parseResult result;
                if (computedCRC != received.crc16) {
                    result = parseResult::CRC_ERROR;
                } else if (!verifySignature(config::SIGNATURE, received.signature, config::SIGNATURE_LEN)) {
                    result = parseResult::SIGNATURE_ERROR;
                } else {
                    starDustNS::security::crypto::decryptPayload(received.payload, config::PAYLOAD_LEN);
                    outByte.sender       = received.header.sender;
                    outByte.receiver     = received.header.receiver;
                    outByte.functionCode = received.header.functionCode;
                    memcpy(outByte.payload, received.payload, config::PAYLOAD_LEN);
                    result = parseResult::OK;
                }

                if (result == parseResult::CRC_ERROR || result == parseResult::SIGNATURE_ERROR) {
                    outByte = exportPack_t{};
                    outByte.sender   = received.header.sender;
                    outByte.receiver = received.header.receiver;
                } else if (result != parseResult::OK) {
                    outByte = exportPack_t{};
                }
                outByte.resultLog = result;
                detail::resetParser(ctx);
                return result;
            }
        }

        return parseResult::UNDEFINED_ERROR;
    }
}
