# StarDust Communication Protocol — v4.4

A packet-based, CRC16-validated, signature-checked, lightly encrypted, non-blocking communication protocol library for embedded systems (Arduino, Linux/POSIX serial port).

> This project is under active development. Read **Security Notes** and **Known Limitations** before using it in production.

## Contents

- [Overview](#overview)
- [File Layout](#file-layout)
- [Packet Format](#packet-format)
- [Installation](#installation)
- [Configuration](#configuration)
- [Usage](#usage)
- [Command Set (StarNet)](#command-set-starnet)
- [Adding a New Port](#adding-a-new-port)
- [Security Notes](#security-notes)
- [Known Limitations](#known-limitations)
- [License](#license)

## Overview

StarDust is a header-only C++20 library for exchanging packets between devices over a serial link (UART/USB-Serial).

- Non-blocking, byte-by-byte parser (state machine, no busy-waiting or blocking reads).
- CRC16 integrity check.
- Fixed 6-byte signature to filter non-StarDust traffic on the line.
- Lightweight XOR + bit-rotate payload encryption with a runtime-settable key.
- One high-level API for both Arduino `Stream` and native Linux POSIX serial ports.
- Generic `functionCode` (uint16_t) instead of per-message-type structs — define your own command set freely.


## Packet Format

32 bytes total, `[[gnu::packed]]`:

```
Offset  Size   Field
0       1      firstByte     (0xAA)
1       1      secondByte    (0xAA)
2       1      sender.squadID
3       1      sender.unitID
4       1      receiver.squadID
5       1      receiver.unitID
6       2      functionCode  (uint16_t)
8       16     payload       (encrypted)
24      6      signature
30      2      crc16
```

Parser flow:

```
IDLE ─0xAA→ READING_HEADER ─header done→ READING_BODY ─payload+signature done→ READING_CRC
     → CRC check → signature check → decrypt → OK
```

If the gap between two incoming bytes exceeds `config::TIMEOUT_MS`, the parser resets to `IDLE` — a partial packet never locks up the line.

## Installation

**Arduino (IDE / PlatformIO)**
1. Clone into `Arduino/libraries/StarDust`, or in PlatformIO: `lib_deps = https://github.com/Metehan6688/StarDust.git`
2. In `config.hpp`, uncomment `USE_ARDUINO_FRAMEWORK` and comment out `USE_LINUX_FRAMEWORK`.
3. `#include "starDust.hpp"`

**Linux (POSIX serial)**
Header-only, just add it to your include path:
```bash
g++ -std=c++20 -Iinclude/StarDust main.cpp -o app
```
`USE_LINUX_FRAMEWORK` should be the only active platform macro (default state).

> Requires C++20 (`[[gnu::packed]]`, `inline constexpr` namespace members, `enum class`).

## Configuration

`config.hpp`:

| Constant | Description | Default |
|---|---|---|
| `FIRST_BYTE`, `SECOND_BYTE` | Frame start marker | `0xAA, 0xAA` |
| `PAYLOAD_LEN` | Payload size (bytes) | `16` |
| `SIGNATURE_LEN` / `SIGNATURE` | Fixed protocol signature and length | `6` bytes |
| `CRYPTO_KEY` | 16-byte XOR+rotate key (default value; can be overridden at runtime via `setMyCryptoKey()`) | all `0x00` |
| `CRC16_INIT` / `CRC16_POLY` | CRC16 initial value and polynomial | `0xFFFF` / `0xA001` |
| `TIMEOUT_MS` | Max allowed gap between bytes | `50` ms |

Platform/feature macros (exactly one platform macro should be active):

| Macro | Description |
|---|---|
| `USE_DEFAULT_TUMEN_STARNET` | Enables the default command set in `commandset.hpp` |
| `USE_ARDUINO_FRAMEWORK` | Use the Arduino `Stream` port |
| `USE_LINUX_FRAMEWORK` | Use the Linux POSIX serial port |
| `USE_ESPIDF_FRAMEWORK` | Declared, not yet implemented |

## Usage

**Linux**

```cpp
#include "starDust.hpp"

int main() {
    StarDust node("/dev/ttyUSB0", 115200);
    node.setMyAddress(/*squadID=*/1, /*unitID=*/1);
    node.setMyCryptoKey({ /* your 16-byte key */ });

    if (!node.isPortOpen()) {
        // error handling
    }

    uint8_t payload[16] = {0};
    node.send(/*targetSquad=*/1, /*targetUnit=*/2,
               StarDust::Command::sysPingCode, payload);

    while (true) {
        uint32_t nowMs = /* your millis() implementation */;
        if (node.update(nowMs)) {
            const auto& pkt = node.lastPacket();
            // pkt.sender, pkt.receiver, pkt.functionCode, pkt.payload
        }
    }
}
```

**Arduino**

```cpp
#include "starDust.hpp"

StarDust node(Serial);

void setup() {
    Serial.begin(115200);
    node.setMyAddress(1, 1);
    node.setMyCryptoKey({ /* your 16-byte key */ });
}

void loop() {
    if (node.update(millis())) {
        const auto& pkt = node.lastPacket();
        // handle pkt
    }
}
```

> Note: `setMyCryptoKey()` sets the key process-wide (it writes to the shared `config::CRYPTO_KEY`), not per-instance. Set it once, before sending/receiving.

## Command Set (StarNet)

With `USE_DEFAULT_TUMEN_STARNET` enabled, `starDustNS::useDefaultTumenStarNet::starNetCodes` provides ready-made codes for network management (ping/pong, handshake, name/version query, resend, leadpoint, device ban, etc.), all within `0x00–0xFF`.

To use your own command set: disable `USE_DEFAULT_TUMEN_STARNET`, define your own `enum class` compatible with `functionCode_t` (uint16_t), and use the generic `uint16_t` overload of `send()`.

## Adding a New Port

Implement `port::internalPort`:

```cpp
class internalPort {
public:
    virtual ~internalPort() = default;
    virtual size_t writeByte(const uint8_t* data, size_t len) = 0;
    virtual bool readByte(uint8_t& outByte) = 0;
};
```

Then add your platform macro and header include next to the other `#if defined(...)` blocks in `starDust.hpp`.

## Security Notes

Read this before using StarDust on a line that isn't fully closed/trusted:

- **CRC16 checks integrity, not authenticity.** A malicious sender can produce arbitrary data with a valid CRC — CRC only catches random corruption.
- **`SIGNATURE` is a fixed shared constant, not a per-message MAC/HMAC.** Anyone sniffing the line can read and replay it. It only filters "is this a StarDust packet," not who sent it.
- **The XOR + 3-bit-rotate encryption is not cryptographically secure.** There's no nonce/IV, so the same key + same byte always produces the same output — vulnerable to frequency analysis and known-plaintext attacks. For real confidentiality, replace this layer (e.g. ChaCha20 or AES-CTR + nonce).
- **`CRYPTO_KEY` defaults to all zeros.** Set a real key via `setMyCryptoKey()` before deploying.
- CRC is computed over the encrypted (wire) bytes, so a valid CRC only guarantees corruption-free transit — not that the decrypted payload is meaningful.

## Known Limitations

- `USE_ESPIDF_FRAMEWORK` is declared but `port/espidf.hpp` doesn't exist yet.
- No broadcast addressing.
- No fragmentation/reassembly for payloads larger than 16 bytes.
- No unit tests or `examples/` folder yet.
- No `CMakeLists.txt` / `platformio.ini` templates yet.

## License

MIT License — see [`LICENSE`](./LICENSE).

Copyright (c) 2026 Metehan Semerci

## Contact

**Metehan Semerci** — furkanmetehansemerci@gmail.com
