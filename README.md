# ESP8266 Secure OTA with ED25519

Secure Over-The-Air firmware update system untuk ESP8266 dengan **ED25519 signature verification**.

## 🎯 Features

- ✅ **ED25519 Signature Verification** - FULLY WORKING
- ✅ **SHA-256 Hash Verification** - Integrity check
- ✅ **MQTT Remote Trigger** - Update via MQTT message
- ✅ **NTP Time Sync** - Accurate timestamps
- ✅ **Version Management** - Smart version comparison
- ✅ **Progress Monitoring** - Real-time download status
- ✅ **Auto-Update Check** - Periodic checks every 5 minutes

## 📦 Memory Usage

```
RAM:   40.0% (32,748 bytes)
Flash: 35.4% (370,211 bytes)
```

## 🚀 Quick Start

### 1. Install Dependencies

```bash
pio pkg install
```

### 2. Configure WiFi & MQTT

Edit `src/main.cpp`:

```cpp
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

#define MQTT_SERVER "broker.sinaungoding.com"
#define MQTT_PORT 1884
```

### 3. Generate ED25519 Keys

**Windows:**
```powershell
cd tools
.\sign_firmware_ed25519.ps1 ..\..\.pio\build\esp12e\firmware.bin
```

**Linux/Mac:**
```bash
cd tools
./sign_firmware_ed25519.sh ../.pio/build/esp12e/firmware.bin
```

### 4. Update Public Key

Copy public key hex from script output to `src/main.cpp`:

```cpp
const char PUBLIC_KEY_HEX[] = "your_64_char_hex_string";
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

- **[ED25519_GUIDE.md](ED25519_GUIDE.md)** - Complete ED25519 implementation guide
- **[QUICKSTART.md](QUICKSTART.md)** - Quick reference (if exists)

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
│   └── main.cpp              # Main firmware code
├── tools/
│   ├── sign_firmware_ed25519.sh   # Signing script (Linux/Mac)
│   └── sign_firmware_ed25519.ps1  # Signing script (Windows)
├── platformio.ini            # PlatformIO configuration
├── ED25519_GUIDE.md          # Complete guide
└── README.md                 # This file
```

## 🔑 Key Management

⚠️ **CRITICAL**: Keep `private_ed25519.pem` SECRET!

- Never commit private key to git
- Store in secure location
- Backup securely
- Use HSM in production

## 🐛 Troubleshooting

### Signature Verification Failed

**Solution**: Ensure public key in code matches private key used for signing.

```bash
# Re-sign firmware and get public key
cd tools
./sign_firmware_ed25519.sh ../.pio/build/esp12e/firmware.bin

# Copy output to main.cpp
```

### Hash Mismatch

**Solution**: Re-sign and re-upload firmware to server.

### More Issues?

See [ED25519_GUIDE.md](ED25519_GUIDE.md) troubleshooting section.

## 📊 Manifest Format

```json
{
  "version": "abc123-20260209T1530-build1",
  "hash": "sha256_hex_64_chars",
  "signature": "ed25519_signature_128_hex_chars"
}
```

## 🌐 Server Setup

1. **Build & Sign Firmware**
   ```bash
   pio run
   cd tools
   ./sign_firmware_ed25519.sh ../.pio/build/esp12e/firmware.bin
   ```

2. **Upload to Server**
   - `firmware.bin` → `http://server/firmware-otaq.bin`
   - `manifest.json` → `http://server/manifest.json`

3. **Trigger Update**
   - Via MQTT: `device/002/ota/update` with message `"start"`
   - Or wait for auto-check (5 minutes)

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
