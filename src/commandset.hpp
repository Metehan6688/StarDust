#pragma once

#include <stdint.h>
#include <config.hpp>


#ifdef USE_DEFAULT_TUMEN_STARNET
namespace starDustNS::useDefaultTumenStarNet{
    enum class starNetCodes : uint8_t {
        sysDefCode = 0x0,
        sysPingCode = 0x1,
        sysPongCode = 0x2,
        sysAskName = 0x3,
        sysSayName = 0x4,
        sysHandshakeAck = 0x5,
        sysHandshakeNack = 0x6,
        sysAskSoftwareVersion = 0x7,
        sysSaySoftwareVersion = 0x8,
        sysAskLangCount = 0x9,
        sysSayLangCount = 0xA,
        sysAskLangName = 0xB,
        sysSayLangName = 0xC,
        sysAskTimestamp = 0xD,
        sysSayTimestamp = 0xE,
        sysAskResend = 0xF,
        sysSayResend = 0x10,
        sysResendAck = 0x11,
        sysResendNack = 0x12,
        sysAskLeadpoint = 0x13,
        sysSayLeadpoint = 0x14,
        sysAskHardwareInfo = 0x15,
        sysSayHardwareInfo = 0x16,
        sysAskJoinStarNet = 0x17,
        sysSayJoinStarNet = 0x18,
        sysAskLeaveStarNet = 0x19,
        sysSayLeaveStarNet = 0x20,
        sysBanDevice = 0x21,
        sysError = 0xFF
    };
}
#endif