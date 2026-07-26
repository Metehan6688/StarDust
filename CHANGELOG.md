# Changelog

All notable changes to this project are documented here.

## [4.4] - 2026-07-27
> Added broadcast & multicast support to the parser via receiver address filtering.
- New config flags: `ENABLE_BROADCAST_IN_PARSER`, `ENABLE_MULTICAST_IN_PARSER` (independently toggleable in config.hpp).
- `WILDCARD_BYTE` (0xFF) reserved for squadID/unitID when either flag is enabled:
- squadID == WILDCARD_BYTE -> accepted by everyone regardless of unitID (true broadcast), only when ENABLE_BROADCAST_IN_PARSER is defined.
- squadID matches mine + unitID == WILDCARD_BYTE -> accepted by every unit in my squad, only when ENABLE_MULTICAST_IN_PARSER is defined.
- If a flag is disabled, its corresponding field requires an exact address match — WILDCARD_BYTE is not treated specially for that field.
- New parseResult values: ANOTHER_SQUAD, ANOTHER_UNIT — packet passed CRC but wasn't addressed to us; sender/receiver are still populated in the export struct for diagnostics.
- setMyAddress() now takes std::array<uint8_t, 2> and writes to config::myAddress (previously an instance member on StarDust).

## [4.3] - 2026-07-26
- Fixing bugs and added crypto key can change in runtime now with setMyCryptoKey([16]) function

## [4.2] - 2026-07-26
- Fixing some bugs and changed crc16 calcualte logic. more flash area, less ram. good deal for this verison.

## [4.1] - 2026-07-24
- Fixing some bugs

## [4.0]
> Rewritten from scratch; parsing, CRC, etc. fully changed.
- This version changed every part of library. I tried to use modern cpp more in this library. I create files for modularity, I changed packet types and this is suitable for swarm systems now. this version have 32 byte packets, 16 byte payloads and 2 byte function codes. With this verison we can make custom packets witch function code. every function code can make special payload. I changed crc16, now that is more simillar with MODBUS protocol. That tested with arduino - linux pc.

## [3.0]
> This verison has been "okay" for me first time.
- That have a god class and one stardust.h and stardust.cpp, this have a crc16, 16byte security key and bit shift. This version's frame struct: header + payload + crc, all of them 69 bytes. No custom functions, all of them integred with main class. this verison have linux and serial port support for arduino framework. tested in esp32 - arduino nano, esp32 - linux pc, arduino nano - linux pc.

## [2.0] (never released)
- Some bug fixes but actually this isn't a real version.

## [1.0]
- This version I added basic XOR checksum

## [0.x] / Alpha / Base
> Prototype/internal versions, never released.
- This idea started with a personal/hobby project. I wanted to create my own protocol for my future projects, because I lie make my own standarts. This versions doesn't have any crc16 or crypto files. Only sending packets with internal functions, no custom payloads.
