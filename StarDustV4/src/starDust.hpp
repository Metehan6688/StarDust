#pragma once

#include "config.hpp"
#include "commandset.hpp"
#include "packet.hpp"
#include "security.hpp"
#include "parser.hpp"
#include "sender.hpp"
#include "port/port.hpp"


#if defined(USE_LINUX_FRAMEWORK)
    #include "port/linux.hpp"
#elif defined(USE_ARDUINO_FRAMEWORK)
    #include "port/arduino.hpp"
#else
    // #error
#endif


class StarDust{
    public:
        using Packet = starDustNS::parser::exportPack_t;
        using Result = starDustNS::parser::parseResult;

    #ifdef USE_DEFAULT_TUMEN_STARNET
        using Command = starDustNS::useDefaultTumenStarNet::starNetCodes;
    #endif

    #if defined(USE_LINUX_FRAMEWORK)
        explicit StarDust(const char* devicePath, uint32_t baudRate = 115200): port_(devicePath, baudRate){}
    #elif defined(USE_ARDUINO_FRAMEWORK)
        explicit StarDust(Stream& stream) : port_(stream){}
    #endif


        void setMyAddress(uint8_t squadID, uint8_t unitID){
            myAddress_.squadID = squadID;
            myAddress_.unitID  = unitID;
        }

        bool send(uint8_t targetSquad, uint8_t targetUnit, uint16_t functionCode, const uint8_t* payload){
            starDustNS::packet::address_t target{targetSquad, targetUnit};
            return starDustNS::sender::sendPacket(port_, myAddress_, target, functionCode, payload);
        }

    #ifdef USE_DEFAULT_TUMEN_STARNET
        bool send(uint8_t targetSquad, uint8_t targetUnit, Command cmd, const uint8_t* payload){
            return send(targetSquad, targetUnit, static_cast<uint16_t>(cmd), payload);
        }
    #endif

        bool update(uint32_t millis) {
            uint8_t b;
            while (port_.readByte(b)){
                lastResult_ = starDustNS::parser::parsePacket(b, millis, lastPacket_, ctx_);
                if (lastResult_ == Result::OK) {
                    return true;
                }
            }
            return false;
        }

        const Packet& lastPacket() const{
            return lastPacket_;
        }
        Result lastResult() const{
            return lastResult_;
        }

        bool isPortOpen() const {
        #if defined(USE_LINUX_FRAMEWORK)
            return port_.isOpen();
        #else
            return true;
        #endif
        }
    
    private:
        starDustNS::packet::address_t   myAddress_{};
        starDustNS::parser::parserCTX_t ctx_{};
        Packet lastPacket_{};
        Result lastResult_ = Result::WAIT;
 
    #if defined(USE_LINUX_FRAMEWORK)
        starDustNS::port::linuxSerialPort port_;
    #elif defined(USE_ARDUINO_FRAMEWORK)
        starDustNS::port::arduinoStreamPort port_;
    #endif
};
