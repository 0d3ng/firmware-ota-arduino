# ESP8266 Secure OTA with ED25519

Secure Over-The-Air firmware update system untuk ESP8266 dengan **ED25519 signature verification**.

## 🎯 Features

- ✅ **ED25519 Signature Verification** - FULLY WORKING
- ✅ **SHA-256 Hash Verification** - Integrity check
- ✅ **MQTT Remote Trigger** - Update via MQTT message
- ✅ **NTP Time Sync** - Accurate timestamps
- ✅ **Version Management** - Smart version comparison
- ✅ **OTA Monitoring** - Real-time metrics via MQTT
- ✅ **Auto Version Injection** - Git hash + timestamp
- ✅ **CI/CD Ready** - Environment variable support
- ✅ **Modular Architecture** - Clean code structure

## 📦 Memory Usage

```
RAM:   40.2% (32,972 bytes)
Flash: 35.6% (371,359 bytes)
```

## 🚀 Quick Start

### 1. Install Dependencies

```bash
pio pkg install
```

### 2. Configure WiFi & MQTT

Edit `src/config.h`:

```cpp
#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"

#define MQTT_SERVER "broker.sinaungoding.com"
#define MQTT_PORT 1884
#define MQTT_USER "noureen"
#define MQTT_PASS "1234"
```

### 3. Set Server URLs

Edit `src/config.h`:

```cpp
#define MANIFEST_URL "http://server:8000/api/v1/firmware/manifest.json"
#define FIRMWARE_URL "http://server:8000/api/v1/firmware/firmware-otaq.bin"
```

### 4. Update Public Key

Edit `src/config.h` dengan public key dari server:

```cpp
#define PUBLIC_KEY_HEX "your_64_char_hex_ed25519_public_key"
```

### 5. Build & Flash

```bash
pio run --target upload
pio device monitor
```

### 6. Trigger OTA Update

```bash
mosquitto_pub -h broker.sinaungoding.com -p 1884 \
  -u noureen -P 1234 \
  -t "device/002/ota/update" -m "start"
```

## 📚 Documentation

- **[docs/CICD_SETUP.md](docs/CICD_SETUP.md)** - Complete GitHub Actions CI/CD setup guide
- **[ED25519_GUIDE.md](ED25519_GUIDE.md)** - Complete ED25519 implementation guide
- **[.github/workflows/build-ota.yml](.github/workflows/build-ota.yml)** - Production-ready workflow

## 🔢 Version Management

Firmware version auto-generated saat build menggunakan `version_inject.py`:

**Format**: `{git_hash}-{timestamp}-{suffix}`

**Local Build:**
```bash
pio run
# Output: Building firmware version: 7fb3aeb-20260209T1533-local
```

**CI/CD Build:**
```bash
# Set environment variables
export FIRMWARE_VERSION="1.2.3-production"
export FIRMWARE_ALGORITHM="ed25519"
pio run
# Output: Building firmware version: 1.2.3-production
#         Using firmware algorithm: ed25519
```

**GitHub Actions Example:**
```yaml
- name: Build Firmware
  env:
    FIRMWARE_VERSION: "${{ github.ref_name }}-build${{ github.run_number }}"
    FIRMWARE_ALGORITHM: "ed25519"
  run: pio run
```

**Full CI/CD Workflow:**

Project sudah include GitHub Actions workflow lengkap di [.github/workflows/build-ota.yml](.github/workflows/build-ota.yml) yang akan:

1. **Auto-trigger** saat PR merged ke `master` atau `development`
2. **Build firmware** dengan version auto-generated
3. **Sign firmware** menggunakan ED25519 (dari GitHub Secrets)
4. **Create manifest.json** dengan hash & signature
5. **Upload** ke server OTA via API

**Setup GitHub Secrets:**
```
ED25519_PRIVATE_KEY_HEX = your_64_char_hex_private_key
API_TOKEN = your_bearer_token
```

**Setup GitHub Variables:**
```
API_URL = http://your-server.com:8000/api/v1
```

**Trigger:**
```bash
# Create PR, kemudian merge
git checkout -b feature/new-update
git add .
git commit -m "Update feature"
git push origin feature/new-update
# Merge PR via GitHub UI → Auto build & deploy
```

**GitLab CI Example:**
```yaml
build:
  variables:
    FIRMWARE_VERSION: "$CI_COMMIT_TAG-build$CI_PIPELINE_ID"
    FIRMWARE_ALGORITHM: "ed25519"
  script:
    - pio run
```

## 📡 OTA Monitoring

Setiap stage OTA mengirim metrics ke MQTT topic `ota/metrics`:

```json
{
  "stage": "download_firmware",
  "elapsed_ms": 1234,
  "free_heap": 45000,
  "algorithm": "ed25519",
  "timestamp": "2026-02-09T10:30:45"
}
```

**Stages:**
1. `download_manifest` - Download manifest.json
2. `parse_manifest` - Parse version/hash/signature
3. `download_firmware` - Download firmware binary
4. `verify_hash` - SHA-256 verification
5. `verify_signature` - ED25519 verification
6. `flash_firmware` - Flash to ESP8266

## 🔒 Security Flow

```
Download Manifest → Version Check → Download Firmware
     ↓
Calculate SHA-256 Hash → Compare Hash
     ↓
Verify ED25519 Signature → If Valid: Flash Firmware
     ↓
Reboot
```

## 🛠️ Technology Stack

- **Platform**: ESP8266 (Espressif8266)
- **Framework**: Arduino
- **Signature**: ED25519 (rweather/Crypto library)
- **Hash**: SHA-256 (BearSSL)
- **MQTT**: PubSubClient
- **JSON**: ArduinoJson

## 📋 Project Structure

```
firmware-ota-arduino/
├── src/
│   ├── main.cpp              # Main application
│   ├── config.h              # Configuration
│   ├── wifi_manager.h/.cpp   # WiFi management
│   ├── ntp_sync.h/.cpp       # NTP synchronization
│   ├── mqtt_handler.h/.cpp   # MQTT client
│   └── ota_updater.h/.cpp    # OTA with ED25519
├── version_inject.py         # Auto-version injection
├── platformio.ini            # PlatformIO config
├── ED25519_GUIDE.md          # Complete guide
└── README.md                 # This file
```

## 🔑 Key Management

⚠️ **CRITICAL**: ED25519 signing dilakukan di server/CI/CD, bukan di client!

**Client (ESP8266) hanya perlu:**
- Public key (32 bytes hex) di `config.h`
- Tidak perlu private key
- Tidak perlu signing tools

**Server/CI/CD perlu:**
- Private key untuk signing firmware
- Tools untuk generate signature
- Manifest.json dengan hash & signature

## 🌐 Server Setup

Server harus menyediakan:

1. **Firmware Binary**
   - URL: `http://server/firmware-otaq.bin`
   - Signed dengan ED25519 private key

2. **Manifest JSON**
   - URL: `http://server/manifest.json`
   - Format:
   ```json
   {
     "version": "7fb3aeb-20260209T1533-local",
     "hash": "sha256_hex_64_chars",
     "signature": "ed25519_signature_128_hex_chars"
   }
   ```

3. **MQTT Broker**
   - Topic: `device/002/ota/update`
   - Message: `"start"`

**Proses di Server:**
```bash
# Build firmware
pio run

# Sign firmware (server-side tool)
python sign_firmware.py firmware.bin > manifest.json

# Upload both files
scp firmware.bin server:/www/firmware-otaq.bin
scp manifest.json server:/www/manifest.json
```

## 🐛 Troubleshooting

### Signature Verification Failed

**Penyebab**: Public key di `config.h` tidak match dengan private key di server

**Solusi**: 
1. Pastikan public key hex di `config.h` benar
2. Verifikasi signature di server sudah benar
3. Check manifest.json format

### Hash Mismatch

**Penyebab**: File firmware corrupt atau tidak match dengan manifest

**Solusi**:
1. Re-generate manifest.json di server
2. Re-upload firmware.bin
3. Clear cache jika ada

### WiFi Not Connected

**Solusi**: Check `config.h`:
```cpp
#define WIFI_SSID "your_ssid"
#define WIFI_PASSWORD "your_password"
```

### MQTT Connection Failed

**Solusi**: Check `config.h`:
```cpp
#define MQTT_SERVER "broker_address"
#define MQTT_PORT 1884
#define MQTT_USER "username"
#define MQTT_PASS "password"
```

## ✅ Status

- **Build Status**: ✅ SUCCESS
- **ED25519 Verification**: ✅ FULLY IMPLEMENTED
- **Hash Verification**: ✅ WORKING
- **Production Ready**: ✅ YES

## 🙏 Credits

- [rweather/arduinolibs](https://github.com/rweather/arduinolibs) - Crypto library
- [ESP8266 Arduino Core](https://github.com/esp8266/Arduino)
- [PubSubClient](https://github.com/knolleary/pubsubclient)
- [ArduinoJson](https://arduinojson.org/)

## 📄 License

MIT License

---

**Ready to secure your ESP8266 OTA updates! 🔒🚀**
