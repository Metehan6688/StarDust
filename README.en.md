🇹🇷 [Türkçe](./README.md) | 🇬🇧 **English**

# StarDust Communication Protocol — v4.0

A **packet-based, CRC16-validated, signature-checked, lightly encrypted, non-blocking** communication protocol library for embedded systems (Arduino, Linux/POSIX serial port).

v4.0 is a ground-up rewrite of the library. Where v3.0 had a single monolithic `StarDust` class, v4.0 introduces a modular namespace layout, a platform-agnostic port abstraction, and a generic packet format.

> This project is under active development. Please read the **Security Notes** and **Known Limitations** sections below before deploying it in production.

---

## Table of Contents

- [Overview](#overview)
- [What's New in v4.0](#whats-new-in-v40)
- [Architecture / File Layout](#architecture--file-layout)
- [Packet Format](#packet-format)
- [Installation](#installation)
- [Configuration (config.hpp)](#configuration-confighpp)
- [Usage Examples](#usage-examples)
- [Command Set (StarNet)](#command-set-starnet)
- [Port Layer & Extending](#port-layer--extending)
- [Security Notes](#security-notes)
- [Migration Guide: v3.0 → v4.0](#migration-guide-v30--v40)
- [Known Limitations / Roadmap](#known-limitations--roadmap)
- [Contributing](#contributing)
- [License](#license)
- [Contact](#contact)

---

## Overview

StarDust is a header-only C++ library designed for reliable packet exchange between two or more devices over a serial link (UART/USB-Serial). Key features:

- **Non-blocking, byte-by-byte parser**: processes each incoming byte through a state machine without blocking the main loop.
- **CRC16** for data integrity checking.
- **A fixed 6-byte signature** to verify that packets on the line belong to the protocol.
- **XOR + bit-rotate based lightweight payload encryption**.
- **Platform-independent port layer**: the same high-level API works over both Arduino `Stream` and native Linux POSIX serial ports.
- **Extensible command set**: use the default "Tumen StarNet" system commands, or define your own `funcCode_t` (uint16_t) codes.

---

## What's New in v4.0

Quick summary — see [`CHANGELOG.en.md`](./CHANGELOG.en.md) for the full list:

- The single-file `StarDust.h/.cpp` layout was split into modular headers under `starDustNS` (`config`, `packet`, `security`, `parser`, `sender`, `port`, `commandset`).
- Flat single-byte addressing (`source`/`target`) was replaced with two-level hierarchical addressing via `address_t` (squad + unit).
- Per-message-type structs and dedicated `send*/receive*` functions (REQUEST, TELEMETRY, COMMAND, BETRAYAL, etc.) were removed in favor of a single **generic payload + `functionCode` (uint16_t)** model.
- Payload size dropped from 64 bytes to **16 bytes** (optimized for smaller, more frequent packets).
- A **6-byte fixed signature field** was added to every packet (didn't exist in v3).
- The CRC algorithm changed from **CRC16-CCITT (poly 0x1021)** to a **CRC16/Modbus-style algorithm (poly 0xA001)**.
- A **port abstraction** (`internalPort`) was introduced; Arduino `Stream` is no longer a hard requirement — native Linux POSIX serial support is now built in.
- The encryption key moved from a runtime-configurable field to a **compile-time constant** in `config.hpp`; the v3 `setCryptoKey()` runtime key-swap feature is gone in v4.
- `BROADCAST_ID` no longer exists.

---

## Architecture / File Layout

```
StarDust/
├── config.hpp        # All protocol constants + platform selection macros
├── packet.hpp         # Raw packet/header structs (packed)
├── security.hpp       # CRC16, signature verification, encryption/decryption
├── parser.hpp          # Non-blocking state-machine parser
├── sender.hpp           # Packet preparation + transmission
├── commandset.hpp        # Default "Tumen StarNet" system commands
├── starDust.hpp            # Top-level StarDust class wrapping everything
└── port/
    ├── port.hpp             # internalPort abstract interface
    ├── arduino.hpp           # Arduino Stream implementation
    ├── linux.hpp / linux.cpp # Linux POSIX termios implementation
    └── (espidf.hpp)          # Planned, not yet implemented
```

| File | Responsibility |
|---|---|
| `config.hpp` | Frame bytes, payload/signature lengths, CRC parameters, encryption key, timeout, platform selection macros |
| `packet.hpp` | `header_t`, `address_t`, `packet_t` — the raw on-wire byte layout |
| `security.hpp` | CRC16 calculation, constant-time signature comparison, XOR+rotate encryption |
| `parser.hpp` | `parsePacket()` — turns incoming bytes into a packet via a state machine |
| `sender.hpp` | `sendPacket()` — prepares a packet (encrypt, sign, add CRC) and writes it via the port |
| `commandset.hpp` | `starNetCodes` enum — optional default system command set |
| `starDust.hpp` | `StarDust` class — the single entry point for users |
| `port/*.hpp` | Platform-specific byte read/write implementations |

---

## Packet Format

`packet_t` is transmitted on the wire as a total of **32 bytes** (`[[gnu::packed]]`):

```
Offset  Size   Field
------  -----  ----------------------------------
0       1      firstByte    (fixed 0xAA)
1       1      secondByte   (fixed 0xAA)
2       1      sender.squadID
3       1      sender.unitID
4       1      receiver.squadID
5       1      receiver.unitID
6       2      functionCode (uint16_t)
------  -----  ---------------------------------- (header = 8 bytes)
8       16     payload      (encrypted)
24      6      signature    (fixed protocol signature)
30      2      crc16
------  -----  ----------------------------------
                Total: 32 bytes
```

**Parser flow** (`parser::parsePacket`):

```
IDLE ──0xAA──▶ READING_HEADER ──header complete──▶ READING_BODY ──payload+signature complete──▶ READING_CRC ──▶ [CRC check] ──▶ [signature check] ──▶ [decrypt] ──▶ OK
```

At every state transition, if the gap between two bytes exceeds `config::TIMEOUT_MS`, the parser automatically resets to `IDLE` (a partial packet never permanently locks up the line).

---

## Installation

### Arduino (IDE / PlatformIO)

1. Clone this repo into `Arduino/libraries/StarDust`, **or**, in PlatformIO:
   ```ini
   lib_deps =
       https://github.com/<your-username>/StarDust.git
   ```
2. In `config.hpp`, uncomment `USE_ARDUINO_FRAMEWORK` and comment out `USE_LINUX_FRAMEWORK`.
3. Include it in your project with `#include "starDust.hpp"`.

### Linux (POSIX serial port)

Since the library is header-only, no separate build step is required — just add it to your include path:

```bash
g++ -std=c++20 -Iinclude/StarDust main.cpp -o app
```

`USE_LINUX_FRAMEWORK` should be active and all other platform macros disabled in `config.hpp` (this is the default state).

> Note: the library requires `c++20` (due to `[[gnu::packed]]`, `inline constexpr` namespace members, and `enum class` usage; `c++17` is likely the practical minimum, but `c++20` is what's been tested).

---

## Configuration (config.hpp)

| Constant | Description | Default |
|---|---|---|
| `FIRST_BYTE`, `SECOND_BYTE` | Frame start marker | `0xAA, 0xAA` |
| `PAYLOAD_LEN` | Payload size (bytes) | `16` |
| `SIGNATURE_LEN` / `SIGNATURE` | Fixed protocol signature and its length | `6` bytes |
| `CRYPTO_KEY` | 16-byte XOR+rotate key | **All `0x00` — change this before deploying** |
| `CRC16_INIT` / `CRC16_POLY` | CRC16 initial value and polynomial | `0xFFFF` / `0xA001` |
| `TIMEOUT_MS` | Maximum allowed gap between bytes | `50` ms |

Platform/feature macros (also in `config.hpp`; exactly one platform macro should be active):

| Macro | Description |
|---|---|
| `USE_DEFAULT_TUMEN_STARNET` | Enables the default system command set in `commandset.hpp` |
| `USE_ARDUINO_FRAMEWORK` | Use the Arduino `Stream` port |
| `USE_ESPIDF_FRAMEWORK` | Declared but **not yet implemented** (see Roadmap) |
| `USE_LINUX_FRAMEWORK` | Use the Linux POSIX serial port implementation |

---

## Usage Examples

### Linux

```cpp
#include "starDust.hpp"

int main() {
    StarDust node("/dev/ttyUSB0", 115200);
    node.setMyAddress(/*squadID=*/1, /*unitID=*/1);

    if (!node.isPortOpen()) {
        // error handling
    }

    uint8_t payload[16] = {0};
    node.send(/*targetSquad=*/1, /*targetUnit=*/2,
               StarDust::Command::sysPingCode, payload);

    while (true) {
        uint32_t nowMs = /* your own millis() implementation */;
        if (node.update(nowMs)) {
            const auto& pkt = node.lastPacket();
            // pkt.sender, pkt.receiver, pkt.functionCode, pkt.payload
        }
    }
}
```

### Arduino

```cpp
#include "starDust.hpp"

StarDust node(Serial);

void setup() {
    Serial.begin(115200);
    node.setMyAddress(1, 1);
}

void loop() {
    if (node.update(millis())) {
        const auto& pkt = node.lastPacket();
        // handle pkt
    }
}
```

---

## Command Set (StarNet)

When `USE_DEFAULT_TUMEN_STARNET` is enabled, `starDustNS::useDefaultTumenStarNet::starNetCodes` provides a ready-made command set for network management (ping/pong, handshake, name/version query, resend, leadpoint, device banning, etc.). All codes fall in the `0x00–0xFF` range and only use the low byte of the `functionCode` field (uint16_t).

To define your own command set, disable `USE_DEFAULT_TUMEN_STARNET`, define your own `enum class` compatible with `functionCode_t` (uint16_t), and use the generic (`uint16_t`) overload of `send()`.

---

## Port Layer & Extending

To add a new transport layer (e.g. BLE, TCP socket, ESP-IDF UART), simply implement the `port::internalPort` interface:

```cpp
class internalPort {
public:
    virtual ~internalPort() = default;
    virtual size_t writeByte(const uint8_t* data, size_t len) = 0;
    virtual bool readByte(uint8_t& outByte) = 0;
};
```

Then add your platform macro and include it among the `#if defined(...)` blocks in `starDust.hpp`.

---

## Security Notes

Don't skip this section — it matters if you're deploying this protocol outside a closed, trusted environment (e.g. over a line that could be exposed or tapped):

- **CRC16 is an integrity check, not authentication.** A malicious sender can produce arbitrary data with a valid CRC. CRC only catches random corruption on the wire.
- **The `SIGNATURE` is a fixed, shared constant — not a per-message MAC/HMAC.** Anyone sniffing the line can read and replay it directly. Its purpose is to filter "is this a StarDust packet," not to provide cryptographic authentication.
- **The encryption (XOR + 3-bit rotate) is not cryptographically secure.** There's no nonce/IV; the same key and same byte always produce the same output deterministically, making it vulnerable to frequency analysis and known-plaintext attacks. If real confidentiality is required, replacing this layer (e.g. with ChaCha20 or AES-CTR + nonce) is recommended.
- **`CRYPTO_KEY` defaults to all zeros.** Make sure to change it to a real key in `config.hpp` before deploying. Unlike v3's `setCryptoKey()`, there is currently no runtime key-change mechanism in v4 — see Roadmap.
- In v4, CRC is calculated **over the encrypted (wire) bytes** (in v3 it was calculated over plaintext, before encryption). This is arguably the more correct design for transmission-integrity purposes, but it does not guarantee the decrypted payload is "meaningful" — only that it arrived without corruption.

---

## Migration Guide: v3.0 → v4.0

This is a **breaking** release; there is no direct API compatibility:

| v3.0 | v4.0 |
|---|---|
| `StarDust node; node.begin(Serial, myID, targetID);` | `StarDust node(Serial);` (Arduino) / `StarDust node(devicePath, baud);` (Linux), then `setMyAddress(squad, unit)` |
| Single-byte `myID` / `targetID` | `squadID` + `unitID` pair |
| `sendTelemetry(...)`, `sendCommand(...)`, ... (type-specific functions) | `send(squad, unit, functionCode, payload)` — generic |
| `TelemetryPayload`, `CommandPayload`, etc. structs | Define your own struct and `memcpy` it into `uint8_t payload[16]` |
| `PAYLOADSIZE = 64` | `PAYLOAD_LEN = 16` — you'll need your own fragmentation logic for larger payloads |
| `setCryptoKey()` (runtime) | `config::CRYPTO_KEY` (compile-time constant) |
| `BROADCAST_ID` | Not available — sending to multiple targets requires looping and sending individually |
| Single platform: Arduino `Stream` (+ PC stub) | Arduino `Stream` **and** Linux POSIX serial port, extensible via `internalPort` |

The most time-consuming part of migrating a v3-based project will likely be defining your own payload structs to replace the type-specific `send*/receive*` functions, and adapting your addressing scheme to the squad/unit model.

---

## Known Limitations / Roadmap

- [ ] `USE_ESPIDF_FRAMEWORK` is defined but the corresponding `port/espidf.hpp` implementation doesn't exist yet.
- [ ] No runtime encryption key change (similar to v3's `setCryptoKey()`).
- [ ] No broadcast addressing.
- [ ] No fragmentation/reassembly mechanism for data larger than 16 bytes.
- [ ] No unit tests or `examples/` folder yet.
- [ ] No `CMakeLists.txt` / `platformio.ini` templates yet (happy to prepare these too if wanted).

---

## Contributing

Pull requests and issues are welcome. Before submitting changes, please reset the platform macros in `config.hpp` back to their default state (only `USE_LINUX_FRAMEWORK` active).

---

## License

MIT License — see [`LICENSE`](./LICENSE).

Copyright (c) 2026 Metehan Semerci

---

## Contact

**Metehan Semerci** — furkanmetehansemerci@gmail.com
