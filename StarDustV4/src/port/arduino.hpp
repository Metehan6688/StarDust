#pragma once
#include "config.hpp"


#ifdef USE_ARDUINO_FRAMEWORK

#include "Arduino.h"
#include "port.hpp"


namespace starDustNS::port{
    class arduinoStreamPort : public internalPort{
        public:
            explicit arduinoStreamPort(Stream& stream) : stream_(stream) {}

            size_t writeByte(const uint8_t* data, size_t len) override {
                return stream_.write(data, len);
            }

            bool readByte(uint8_t& outByte) override {
                if (stream_.available() <= 0) return false;
                int b = stream_.read();
                if (b < 0) return false;
                outByte = static_cast<uint8_t>(b);
                return true;
        }

        private:
            Stream& stream_;
    };
}

#endif
