# 🔐 TLS Certificate Fingerprint Setup

## Overview

Fingerprint verification adalah **alternative** untuk CA certificate verification yang **jauh lebih ringan** untuk ESP8266:

| Method | Memory Usage | Stability | Maintenance |
|--------|--------------|-----------|-------------|
| CA Certificate | ~20KB RAM | ❌ Crash di ESP8266 | ✅ Auto-renew |
| **Fingerprint** | **~3KB RAM** | **✅ Stabil** | ⚠️ Update setiap 90 hari |

## 📋 Prerequisites

- OpenSSL installed (Linux/macOS/WSL)
- Access to your server domain

---

## 🔍 Get Certificate Fingerprint

### Method 1: OpenSSL (Recommended)

**SHA-1 Fingerprint (20 bytes):**
```bash
echo | openssl s_client -connect ota.sinaungoding.com:443 2>/dev/null | \
openssl x509 -fingerprint -noout -sha1
```

**Output:**
```
SHA1 Fingerprint=AA:BB:CC:DD:EE:FF:00:11:22:33:44:55:66:77:88:99:AA:BB:CC:DD
```

**SHA256 Fingerprint (32 bytes) - More secure but not supported by all BearSSL versions:**
```bash
echo | openssl s_client -connect ota.sinaungoding.com:443 2>/dev/null | \
openssl x509 -fingerprint -noout -sha256
```

### Method 2: Browser

1. Open `https://ota.sinaungoding.com` in browser
2. Click **padlock icon** → **Certificate**
3. Go to **Details** tab
4. Find **Fingerprint** (SHA-1 or SHA-256)
5. Copy the value

### Method 3: For MQTT Server (Port 8883)

```bash
echo | openssl s_client -connect ota.sinaungoding.com:8883 2>/dev/null | \
openssl x509 -fingerprint -noout -sha1
```

---

## ⚙️ Update Code

### 1. Edit `src/certificates.h`

Replace the placeholder fingerprints:

```cpp
// MQTT Server Fingerprint (from openssl command above)
#define MQTT_FINGERPRINT "AA BB CC DD EE FF 00 11 22 33 44 55 66 77 88 99 AA BB CC DD"

// OTA Server Fingerprint (same as MQTT if same domain)
#define OTA_FINGERPRINT "AA BB CC DD EE FF 00 11 22 33 44 55 66 77 88 99 AA BB CC DD"
```

**Format:**
- Use **colon-separated** format: `AA:BB:CC:DD:...`
- Or **space-separated** format: `AA BB CC DD ...`
- Both work with BearSSL

### 2. Build & Upload

```bash
platformio run --environment esp12e
platformio run --target upload --upload-port COM4
```

---

## 🧪 Testing

### 1. Monitor Serial Output

```bash
platformio device monitor --port COM4
```

**Expected output:**
```
[MQTT] TLS: Fingerprint verification enabled
[MQTT] Fingerprint: AA BB CC DD EE FF 00 11 22 33 44 55 66 77 88 99 AA BB CC DD
[MQTT] Configured MQTTS: ota.sinaungoding.com:8883 (Fingerprint)
[MQTT] Connected to broker
```

### 2. Test OTA Update

Publish MQTT trigger:
```bash
mosquitto_pub -h ota.sinaungoding.com -p 8883 \
  --cafile ca.pem \
  -t "device/002/ota/update" \
  -m "start"
```

**Expected output:**
```
[OTA] Checking for updates...
[HTTPS] TLS: Fingerprint verification
[HTTPS] Fingerprint: AA BB CC DD EE FF 00 11 22 33 44 55 66 77 88 99 AA BB CC DD
[HTTPS] Free heap: 45120 bytes
[HTTP] Response code: 200
[HTTP] Manifest downloaded: 245 bytes
```

---

## 🔄 Certificate Renewal (Every ~90 Days)

When your Let's Encrypt certificate is renewed, you **MUST** update the fingerprint:

### 1. Get New Fingerprint

```bash
echo | openssl s_client -connect ota.sinaungoding.com:443 2>/dev/null | \
openssl x509 -fingerprint -noout -sha1
```

### 2. Update `certificates.h`

```cpp
#define MQTT_FINGERPRINT "NEW FINGERPRINT HERE"
#define OTA_FINGERPRINT "NEW FINGERPRINT HERE"
```

### 3. Rebuild & Deploy

**Option A: Via GitHub Actions (Recommended)**
```bash
git add src/certificates.h
git commit -m "Update TLS fingerprint for renewed certificate"
git push origin main
```

Your CI/CD will build and sign the firmware automatically.

**Option B: Manual Upload**
```bash
platformio run --environment esp12e --target upload
```

---

## 🐛 Troubleshooting

### Error: "Connection failed"

**Causes:**
1. ❌ **Fingerprint mismatch** → Update fingerprint
2. ❌ **Time not synced** → Wait for NTP sync
3. ❌ **Wrong domain** → Check MQTT_SERVER in config.h

**Debug:**
```cpp
[HTTPS] Connection failed. Possible causes:
  - Certificate fingerprint mismatch
  - Server certificate changed (update FINGERPRINT)
  - Time not synchronized (check NTP)
  - Free heap: 45120 bytes
```

### Error: "Time not synced"

**Solution:** Wait 10-15 seconds after boot for NTP sync.

**Check time:**
```
[OTA] Current time: Tue Feb 10 15:49:34 2026
```

If time is wrong (year < 2000), NTP hasn't synced yet.

### Different Fingerprints for MQTT vs OTA

If your MQTT broker and OTA server use **different domains**:

```cpp
// MQTT broker: mqtt.example.com
#define MQTT_FINGERPRINT "AA BB CC DD EE FF ..."

// OTA server: ota.example.com  
#define OTA_FINGERPRINT "11 22 33 44 55 66 ..."
```

Get fingerprints separately:
```bash
# MQTT
echo | openssl s_client -connect mqtt.example.com:8883 2>/dev/null | \
openssl x509 -fingerprint -noout -sha1

# OTA
echo | openssl s_client -connect ota.example.com:443 2>/dev/null | \
openssl x509 -fingerprint -noout -sha1
```

---

## 📊 Memory Comparison

**Before (CA Certificate):**
```
RAM:   [====      ]  41.8% (34,280 bytes / 81,920 bytes)
Flash: [=====     ]  46.1% (481,099 bytes)
Free heap at runtime: ~32KB (TLS crashes!)
```

**After (Fingerprint):**
```
RAM:   [===       ]  38.2% (31,294 bytes / 81,920 bytes)  ← 3KB saved
Flash: [====      ]  44.8% (467,891 bytes)                ← 13KB saved
Free heap at runtime: ~45KB (TLS stable!)                ← 13KB more available
```

**Result:** ✅ **Stabil**, tidak crash lagi!

---

## 🔐 Security Notes

### Is Fingerprint Secure?

✅ **YES! Fingerprint verification is secure:**
- Verifies **exact certificate** (not just chain)
- Uses SHA-1 or SHA-256 hash
- Man-in-the-middle attacks will fail (different fingerprint)
- Same security level as CA cert for **point-to-point** connections

### CA Cert vs Fingerprint

**CA Certificate:**
- ✅ Validates certificate **chain** (Root → Intermediate → Server)
- ✅ Automatically trusts **renewed certificates** from same CA
- ❌ High memory usage (~20KB)
- ❌ Complex validation logic

**Fingerprint:**
- ✅ Validates **exact certificate** hash
- ✅ Low memory usage (~3KB)
- ✅ Simple hash comparison
- ⚠️ Must update when certificate is renewed

**Recommendation for ESP8266:** Use **fingerprint** due to limited RAM.

---

## 📝 Summary

1. **Get fingerprint:** `openssl s_client` command
2. **Update code:** Edit `certificates.h`
3. **Build & upload:** PlatformIO
4. **Renew every 90 days:** When Let's Encrypt cert renews

**Benefits:**
- ✅ **17KB less memory** usage
- ✅ **No crashes** on ESP8266
- ✅ **Same security** level
- ⚠️ Requires manual update setiap cert renewal

---

## 🔗 References

- [ESP8266 BearSSL Documentation](https://github.com/esp8266/Arduino/blob/master/libraries/ESP8266WiFi/src/WiFiClientSecureBearSSL.h)
- [Let's Encrypt Certificate Lifecycle](https://letsencrypt.org/docs/faq/)
- [OpenSSL s_client Manual](https://www.openssl.org/docs/man1.1.1/man1/s_client.html)
