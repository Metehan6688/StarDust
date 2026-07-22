🇹🇷 **Türkçe** | 🇬🇧 [English](./README.en.md)

# StarDust Communication Protocol — v4.0

Gömülü sistemler (Arduino, Linux/POSIX seri port) arasında **paket tabanlı, CRC16 doğrulamalı, imza kontrollü ve hafif şifrelemeli** haberleşme sağlayan, non-blocking (bloklamayan) bir protokol kütüphanesi.

v4.0, kütüphanenin baştan yazılmış (rewrite) sürümüdür. v3.0'da tek bir monolitik `StarDust` sınıfı olan yapı; v4.0'da modüler namespace'lere, platform bağımsız bir port soyutlamasına ve genel amaçlı (generic) bir paket formatına dönüştürülmüştür.

> Bu proje aktif geliştirme aşamasındadır. Aşağıdaki **Bilinen Sınırlamalar** ve **Güvenlik Notları** bölümlerini üretim (production) ortamına almadan önce mutlaka okuyun.

---

## İçindekiler

- [Genel Bakış](#genel-bakış)
- [v4.0'da Neler Değişti?](#v40da-neler-değişti)
- [Mimari / Dosya Yapısı](#mimari--dosya-yapısı)
- [Paket Formatı](#paket-formatı)
- [Kurulum](#kurulum)
- [Yapılandırma (config.hpp)](#yapılandırma-confighpp)
- [Kullanım Örnekleri](#kullanım-örnekleri)
- [Komut Seti (StarNet)](#komut-seti-starnet)
- [Port Katmanı ve Genişletme](#port-katmanı-ve-genişletme)
- [Güvenlik Notları](#güvenlik-notları)
- [v3.0 → v4.0 Geçiş Rehberi](#v30--v40-geçiş-rehberi)
- [Bilinen Sınırlamalar / Yol Haritası](#bilinen-sınırlamalar--yol-haritası)
- [Katkıda Bulunma](#katkıda-bulunma)
- [Lisans](#lisans)
- [İletişim](#iletişim)

---

## Genel Bakış

StarDust; iki ya da daha fazla cihaz arasında seri hat (UART/USB-Serial) üzerinden güvenilir paket alışverişi yapmak için tasarlanmış, header-only bir C++ kütüphanesidir. Temel özellikleri:

- **Non-blocking byte-by-byte parser**: Ana döngüyü (`loop()`) bloklamadan, gelen her byte'ı state machine ile işler.
- **CRC16** ile veri bütünlüğü kontrolü.
- **6 byte'lık sabit imza** ile hat üzerindeki paketlerin protokole ait olduğunun doğrulanması.
- **XOR + bit-rotate tabanlı hafif payload şifrelemesi**.
- **Platform bağımsız port katmanı**: Aynı üst seviye API, hem Arduino `Stream` hem de Linux POSIX seri port üzerinde çalışır.
- **Genişletilebilir komut seti**: Varsayılan "Tumen StarNet" sistem komutları veya kendi `funcCode_t` (uint16_t) tanımlarınız.

---

## v4.0'da Neler Değişti?

Kısa özet — detaylı liste için [`CHANGELOG.md`](./CHANGELOG.md):

- Tek dosyalık `StarDust.h/.cpp` yapısı, `starDustNS` altında modüler header'lara bölündü (`config`, `packet`, `security`, `parser`, `sender`, `port`, `commandset`).
- Tek byte'lık düz adresleme (`source`/`target`) yerine **squad + unit** ile iki katmanlı hiyerarşik adresleme (`address_t`) geldi.
- Her mesaj tipi için ayrı struct + ayrı `send*/receive*` fonksiyonu (REQUEST, TELEMETRY, COMMAND, vs.) kaldırıldı; yerine tek tip **generic payload + `functionCode` (uint16_t)** modeli geldi.
- Payload boyutu 64 byte'tan **16 byte'a** düştü (daha küçük, daha sık paket senaryosuna göre optimize edilmiş).
- Paket başına **6 byte'lık sabit imza alanı** eklendi (v3'te yoktu).
- CRC algoritması **CRC16-CCITT (poly 0x1021)**'den **CRC16/Modbus tarzı (poly 0xA001)**'a değişti.
- **Port soyutlaması** (`internalPort`) eklendi; artık Arduino `Stream` zorunluluğu yok, Linux POSIX seri port native destekleniyor.
- Şifreleme anahtarı artık `config.hpp` içinde **derleme zamanı sabiti**; v3'teki `setCryptoKey()` ile çalışma zamanında anahtar değiştirme özelliği v4'te yok.
- `BROADCAST_ID` kavramı v4'te yok.

---

## Mimari / Dosya Yapısı

```
StarDust/
├── config.hpp        # Tüm protokol sabitleri + platform seçim makroları
├── packet.hpp         # Ham paket/header struct'ları (packed)
├── security.hpp       # CRC16, imza doğrulama, şifreleme/deşifreleme
├── parser.hpp          # Non-blocking state machine parser
├── sender.hpp           # Paket hazırlama + gönderme
├── commandset.hpp        # Varsayılan "Tumen StarNet" sistem komutları
├── starDust.hpp            # Her şeyi saran üst seviye StarDust sınıfı
└── port/
    ├── port.hpp             # internalPort soyut arayüzü
    ├── arduino.hpp           # Arduino Stream implementasyonu
    ├── linux.hpp / linux.cpp # Linux POSIX termios implementasyonu
    └── (espidf.hpp)          # Planlanan, henüz implemente edilmedi
```

| Dosya | Sorumluluk |
|---|---|
| `config.hpp` | Frame byte'ları, payload/imza uzunlukları, CRC parametreleri, şifreleme anahtarı, timeout, platform seçim makroları |
| `packet.hpp` | `header_t`, `address_t`, `packet_t` — hattaki ham bayt düzeni |
| `security.hpp` | CRC16 hesaplama, sabit zamanlı imza karşılaştırma, XOR+rotate şifreleme |
| `parser.hpp` | `parsePacket()` — gelen byte'ları state machine ile paket haline getirir |
| `sender.hpp` | `sendPacket()` — paketi hazırlar (şifrele, imzala, CRC ekle) ve port üzerinden yazar |
| `commandset.hpp` | `starNetCodes` enum — opsiyonel varsayılan sistem komut kümesi |
| `starDust.hpp` | `StarDust` sınıfı — kullanıcıya bakan tek giriş noktası |
| `port/*.hpp` | Platforma özgü byte okuma/yazma implementasyonları |

---

## Paket Formatı

`packet_t` toplamda **32 byte** olarak hatta gönderilir (`[[gnu::packed]]`):

```
Offset  Boyut  Alan
------  -----  ----------------------------------
0       1      firstByte    (0xAA sabit)
1       1      secondByte   (0xAA sabit)
2       1      sender.squadID
3       1      sender.unitID
4       1      receiver.squadID
5       1      receiver.unitID
6       2      functionCode (uint16_t)
------  -----  ---------------------------------- (header = 8 byte)
8       16     payload      (şifreli)
24      6      signature    (sabit protokol imzası)
30      2      crc16
------  -----  ----------------------------------
                Toplam: 32 byte
```

**Parser akışı** (`parser::parsePacket`):

```
IDLE ──0xAA──▶ READING_HEADER ──header tamam──▶ READING_BODY ──payload+imza tamam──▶ READING_CRC ──▶ [CRC kontrolü] ──▶ [imza kontrolü] ──▶ [deşifre] ──▶ OK
```

Her state geçişinde, iki byte arası süre `config::TIMEOUT_MS`'i aşarsa parser otomatik olarak `IDLE`'a resetlenir (yarım kalan paketler hattı kilitlemez).

---

## Kurulum

### Arduino (IDE / PlatformIO)

1. Bu repoyu `Arduino/libraries/StarDust` altına klonlayın **veya** PlatformIO'da:
   ```ini
   lib_deps =
       https://github.com/Metehan6688/StarDust.git
   ```
2. `config.hpp` içinde `USE_ARDUINO_FRAMEWORK` satırının başındaki yorumu kaldırın, `USE_LINUX_FRAMEWORK`'ü yorum satırı yapın.
3. `#include "starDust.hpp"` ile projenize dahil edin.

### Linux (POSIX seri port)

Header-only olduğu için ayrı bir derleme adımı gerekmez; sadece include path'e ekleyin:

```bash
g++ -std=c++20 -Iinclude/StarDust main.cpp -o app
```

`config.hpp` içinde `USE_LINUX_FRAMEWORK` aktif, diğer platform makroları kapalı olmalı (varsayılan durum budur).

> Not: Kütüphane `c++20` gerektirir (`[[gnu::packed]]`, `inline constexpr` namespace üyeleri, `enum class` kullanımı nedeniyle en az `c++17` önerilir; test edilen sürüm `c++20`'dir).

---

## Yapılandırma (config.hpp)

| Sabit | Açıklama | Varsayılan |
|---|---|---|
| `FIRST_BYTE`, `SECOND_BYTE` | Frame başlangıç imzası | `0xAA, 0xAA` |
| `PAYLOAD_LEN` | Payload boyutu (byte) | `16` |
| `SIGNATURE_LEN` / `SIGNATURE` | Sabit protokol imzası ve uzunluğu | `6` byte |
| `CRYPTO_KEY` | 16 byte'lık XOR+rotate anahtarı | **Hepsi `0x00` — mutlaka değiştirin** |
| `CRC16_INIT` / `CRC16_POLY` | CRC16 başlangıç değeri ve polinomu | `0xFFFF` / `0xA001` |
| `TIMEOUT_MS` | Byte'lar arası izin verilen maksimum boşluk | `50` ms |

Platform/özellik makroları (yine `config.hpp` içinde, tek satır aktif olmalı):

| Makro | Açıklama |
|---|---|
| `USE_DEFAULT_TUMEN_STARNET` | `commandset.hpp`'deki varsayılan sistem komut kümesini aktif eder |
| `USE_ARDUINO_FRAMEWORK` | Arduino `Stream` portunu kullan |
| `USE_ESPIDF_FRAMEWORK` | Şu an tanımlı ama **implementasyonu yok** (bkz. Yol Haritası) |
| `USE_LINUX_FRAMEWORK` | Linux POSIX seri port implementasyonunu kullan |

---

## Kullanım Örnekleri

### Linux

```cpp
#include "starDust.hpp"

int main() {
    StarDust node("/dev/ttyUSB0", 115200);
    node.setMyAddress(/*squadID=*/1, /*unitID=*/1);

    if (!node.isPortOpen()) {
        // hata yönetimi
    }

    uint8_t payload[16] = {0};
    node.send(/*targetSquad=*/1, /*targetUnit=*/2,
               StarDust::Command::sysPingCode, payload);

    while (true) {
        uint32_t nowMs = /* kendi millis() implementasyonunuz */;
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
        // pkt işleme
    }
}
```

---

## Komut Seti (StarNet)

`USE_DEFAULT_TUMEN_STARNET` aktifken `starDustNS::useDefaultTumenStarNet::starNetCodes` enum'u, ağ yönetimi için hazır bir komut kümesi sunar (ping/pong, handshake, isim/versiyon sorgulama, resend, leadpoint, cihaz banlama, vb.). Tüm kod aralığı `0x00–0xFF` arasında, `functionCode` alanının (uint16_t) sadece alt baytını kullanır.

Kendi komut setinizi tanımlamak isterseniz `USE_DEFAULT_TUMEN_STARNET`'i kapatıp kendi `enum class`'ınızı `functionCode_t` (uint16_t) ile uyumlu şekilde tanımlayabilir, `send()`'in generic (`uint16_t`) overload'ını kullanabilirsiniz.

---

## Port Katmanı ve Genişletme

Yeni bir taşıma katmanı (örn. BLE, TCP soket, ESP-IDF UART) eklemek için `port::internalPort` arayüzünü implemente etmeniz yeterli:

```cpp
class internalPort {
public:
    virtual ~internalPort() = default;
    virtual size_t writeByte(const uint8_t* data, size_t len) = 0;
    virtual bool readByte(uint8_t& outByte) = 0;
};
```

Ardından `starDust.hpp` içindeki `#if defined(...)` blokları arasına kendi platform makronuzu ve include'unuzu ekleyin.

---

## Güvenlik Notları

Bu bölümü atlamayın — protokolü "kapalı devre, güvenilir ortam" dışında (örn. üzerinden hassas veri geçen, dışarıdan erişilebilir bir hat) kullanacaksanız özellikle önemli:

- **CRC16, kimlik doğrulama değil bütünlük kontrolüdür.** Kötü niyetli bir gönderici, geçerli bir CRC ile istediği veriyi üretebilir. CRC yalnızca hat üzerindeki rastgele bozulmaları yakalar.
- **İmza (`SIGNATURE`), sabit ve paylaşılan bir sabittir — mesaj bazlı bir MAC/HMAC değildir.** Hattı dinleyen biri bu imzayı doğrudan görüp kopyalayabilir. Amacı "bu StarDust paketidir" filtrelemesi yapmak, kriptografik doğrulama sağlamak değil.
- **Şifreleme (XOR + 3-bit rotate) kriptografik olarak güvenli değildir.** Nonce/IV yok, aynı anahtar ve aynı byte deterministik olarak aynı çıktıyı üretir; frekans analizine ve known-plaintext saldırılarına açıktır. Gerçek gizlilik gerekiyorsa (örn. ChaCha20 veya AES-CTR + nonce) bu katmanın değiştirilmesi önerilir.
- **`CRYPTO_KEY` varsayılan olarak tamamen sıfır.** Üretime almadan önce `config.hpp`'de mutlaka gerçek bir anahtarla değiştirin. v3'teki `setCryptoKey()` gibi çalışma zamanı anahtar değişimi v4'te yok — bkz. Yol Haritası.
- CRC, v4'te **şifrelenmiş (wire) baytlar üzerinden** hesaplanıyor (v3'te düz metin üzerinden hesaplanıp sonra şifreleniyordu). Bu, CRC'nin transmisyon bütünlüğünü doğru şekilde koruması açısından mantıklı bir tasarım, ancak decrypt sonrası payload'un "anlamlı" olduğunu garanti etmez — sadece hatasız iletildiğini garanti eder.

---

## v3.0 → v4.0 Geçiş Rehberi

Bu **kırıcı (breaking)** bir sürümdür, doğrudan API uyumluluğu yoktur:

| v3.0 | v4.0 |
|---|---|
| `StarDust node; node.begin(Serial, myID, targetID);` | `StarDust node(Serial);` (Arduino) / `StarDust node(devicePath, baud);` (Linux), sonra `setMyAddress(squad, unit)` |
| Tek byte `myID` / `targetID` | `squadID` + `unitID` çifti |
| `sendTelemetry(...)`, `sendCommand(...)`, ... (tipe özel fonksiyonlar) | `send(squad, unit, functionCode, payload)` — generic |
| `TelemetryPayload`, `CommandPayload`, vb. struct'lar | Kendi struct'ınızı tanımlayıp `uint8_t payload[16]`'a `memcpy` edersiniz |
| `PAYLOADSIZE = 64` | `PAYLOAD_LEN = 16` — büyük payload'lar için parçalama (fragmentation) mantığı kendiniz eklemelisiniz |
| `setCryptoKey()` (çalışma zamanı) | `config::CRYPTO_KEY` (derleme zamanı sabiti) |
| `BROADCAST_ID` | Yok — çoklu hedefe göndermek için döngüyle tek tek gönderim gerekir |
| Tek platform: Arduino `Stream` (+ PC stub) | Arduino `Stream` **ve** Linux POSIX seri port, `internalPort` ile genişletilebilir |

Mevcut v3 tabanlı bir projeyi taşırken en çok zaman alacak kısım, tipe özel `send*/receive*` fonksiyonlarının yerine geçecek payload struct'larınızı tanımlamak ve adresleme şemanızı squad/unit modeline uyarlamak olacaktır.

---

## Bilinen Sınırlamalar / Yol Haritası

- [ ] `USE_ESPIDF_FRAMEWORK` makrosu tanımlı ama karşılık gelen `port/espidf.hpp` implementasyonu henüz yok.
- [ ] Çalışma zamanında şifreleme anahtarı değiştirme (v3'teki `setCryptoKey()` benzeri) yok.
- [ ] Broadcast adresleme yok.
- [ ] 16 byte'ı aşan veriler için fragmentation/reassembly mekanizması yok.
- [ ] Birim testleri ve `examples/` klasörü henüz repoya eklenmedi.
- [ ] `CMakeLists.txt` / `platformio.ini` şablonları henüz yok (istersen bunları da hazırlayabilirim).

---

## Katkıda Bulunma

Pull request ve issue'lar açıktır. Değişiklik göndermeden önce lütfen `config.hpp`'deki platform makrolarını varsayılan haline (yalnızca `USE_LINUX_FRAMEWORK` aktif) döndürün.

---

## Lisans

MIT License — bkz. [`LICENSE`](./LICENSE).

Copyright (c) 2026 Metehan Semerci

---

## İletişim

**Metehan Semerci** — furkanmetehansemerci@gmail.com
