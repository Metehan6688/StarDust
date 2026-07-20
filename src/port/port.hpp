#pragma once

#include <stdint.h>
#include <stddef.h>


namespace starDustNS::port{
    class internalPort {
        public:
            virtual ~internalPort() = default;
            virtual size_t writeByte(const uint8_t* data, size_t len) = 0;
            virtual bool readByte(uint8_t& outByte) = 0;
    };
}
