# TLS/MQTT Setup Guide

Guide untuk setup TLS connection yang aman untuk MQTT dan OTA updates.

## 🔒 MQTT TLS (MQTTS)

### Current Implementation

ESP8266 menggunakan **CA certificate verification** untuk MQTT TLS, mirip dengan `esp_crt_bundle_attach` di ESP-IDF.

**File:** [src/mqtt_handler.cpp](../src/mqtt_handler.cpp)
```cpp
#if FIRMWARE_TLS == 1
    _espClient.setTrustAnchors(&caCert);  // Verify server certificate
#endif
```

### Why NOT setInsecure()?

❌ **BAD** - `setInsecure()`: Skip certificate verification
- Vulnerable to Man-in-the-Middle (MITM) attacks
- Anyone can intercept your connection
- NOT recommended for production

✅ **GOOD** - `setTrustAnchors()`: Verify server certificate
- Authenticates server identity
- Prevents MITM attacks
- Similar to ESP-IDF `esp_crt_bundle_attach`

## 📜 Get Your CA Certificate

### Method 1: Using OpenSSL (Recommended)

```bash
# Connect to your MQTT broker and get certificates
openssl s_client -connect ota.sinaungoding.com:8883 -showcerts

# Output will show certificate chain
# Copy the ROOT CA certificate (usually the last one in chain)
```

**Example output:**
```
-----BEGIN CERTIFICATE-----
MIIDrzCCApegAwIBAgIQCDvgVpBCRrGhdWrJWZHHSjANBgkqhkiG9w0BAQUFADBh
...
-----END CERTIFICATE-----
```

Copy everything from `-----BEGIN CERTIFICATE-----` to `-----END CERTIFICATE-----`

### Method 2: From Certificate Authority

If your server uses known CA (Let's Encrypt, DigiCert, etc.):

**Let's Encrypt:**
- Root CA: https://letsencrypt.org/certificates/
- Download: ISRG Root X1 (self-signed)

**DigiCert Global Root CA:**
- Already included as example in `config.h` (MQTT_CA_CERT)
- Used by many commercial CAs

### Method 3: Export from Browser

1. Open `https://ota.sinaungoding.com:8883` in browser
2. Click padlock icon → Certificate
3. Go to "Details" or "Certification Path"
4. Select ROOT certificate
5. Export as PEM format

## 🔧 Update Code with Your CA Cert

Edit [src/certificates.h](../src/certificates.h):

```cpp
// MQTT TLS CA Certificate (only used when FIRMWARE_TLS=1)
// Get your server's CA cert: openssl s_client -connect ota.sinaungoding.com:8883 -showcerts
// Current cert: ISRG Root X1 (Let's Encrypt Root CA)
#if FIRMWARE_TLS == 1
static const char MQTT_CA_CERT[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
YOUR_CA_CERTIFICATE_HERE
PASTE_THE_ENTIRE_CERTIFICATE
INCLUDING_ALL_LINES
-----END CERTIFICATE-----
)EOF";
#endif
```

**Important:**
- Keep `R"EOF(` and `)EOF";` delimiters
- Don't modify certificate content
- Include both BEGIN and END lines
- No extra spaces or newlines

**Note:** CA certificate disimpan di `certificates.h` untuk kemudahan maintenance (pisah concerns: config vs certificates).

## 🧪 Test Connection

### 1. Test with OpenSSL

```bash
# Verify your server's TLS is working
openssl s_client -connect ota.sinaungoding.com:8883 -CAfile ca.pem

# Should output:
#   Verify return code: 0 (ok)
```

### 2. Test with mosquitto_pub

```bash
# Test MQTT publish with TLS
mosquitto_pub -h ota.sinaungoding.com -p 8883 \
  --cafile ca.pem \
  -u uwais -P uw415_4Lqarn1 \
  -t "test/topic" -m "hello"
```

### 3. Test ESP8266

Upload firmware and monitor serial output:

```
[MQTT] TLS mode enabled with CA verification
[MQTT] Configured MQTTS server: ota.sinaungoding.com:8883
[MQTT] Connecting... Connected!
[MQTT] Subscribed to: device/002/ota/update
```

## 🐛 Troubleshooting

### Connection Failed: rc=-2

**Error:** MQTT connection fails with return code -2

**Cause:** Certificate verification failed

**Solutions:**
1. **Check CA certificate**: Verify you have the correct ROOT CA
   ```bash
   # Get certificate chain
   openssl s_client -connect ota.sinaungoding.com:8883 -showcerts
   ```

2. **Check time sync**: ESP8266 must have correct time for cert validation
   ```cpp
   // Ensure NTP is synced before MQTT connect
   ntpSync.initialize();  // Wait for time sync
   mqttHandler.begin();   // Then connect MQTT
   ```

3. **Verify certificate dates**: Certificate not expired?
   ```bash
   openssl x509 -in ca.pem -noout -dates
   ```

### Connection Timeout

**Symptoms:** MQTT connection hangs or times out

**Solutions:**
1. Check firewall allows port 8883
2. Verify DNS resolution works
3. Test with `telnet ota.sinaungoding.com 8883`

### Memory Issues

**Error:** ESP8266 crashes or resets during TLS handshake

**Cause:** TLS requires significant RAM (~20-30KB)

**Solutions:**
1. Increase heap: reduce other buffers
2. Use fingerprint instead of CA cert (smaller memory)
   ```cpp
   _espClient.setFingerprint("AA BB CC DD EE FF ...");
   ```
3. Use HTTP instead of HTTPS for OTA (MQTT still TLS)

## 🔐 Alternative: Certificate Fingerprint

If CA verification is too complex, use SHA-1 fingerprint:

### Get Fingerprint

```bash
# Get certificate fingerprint
openssl s_client -connect ota.sinaungoding.com:8883 < /dev/null 2>/dev/null | \
  openssl x509 -fingerprint -noout -sha1

# Output: SHA1 Fingerprint=AA:BB:CC:DD:EE:FF:00:11:22:33:44:55:66:77:88:99:AA:BB:CC:DD
```

### Use in Code

Edit [src/mqtt_handler.cpp](../src/mqtt_handler.cpp):

```cpp
#if FIRMWARE_TLS == 1
    // Use fingerprint instead of CA cert
    _espClient.setFingerprint("AA BB CC DD EE FF 00 11 22 33 44 55 66 77 88 99 AA BB CC DD");
    Serial.println("[MQTT] TLS mode enabled with fingerprint verification");
#endif
```

**Pros:**
- Less memory usage (~20 bytes vs ~1-2KB)
- Simpler setup

**Cons:**
- Must update firmware when server cert changes
- Less flexible than CA verification

## 📊 Comparison

| Method | Security | Memory | Complexity | Maintenance |
|--------|----------|--------|------------|-------------|
| `setInsecure()` | ❌ Low | Low | Easy | None |
| `setFingerprint()` | ⚠️ Medium | Low | Easy | High (update on cert renewal) |
| `setTrustAnchors()` | ✅ High | Medium | Medium | Low (CA rarely changes) |

## 🌐 OTA HTTPS

OTA updates also use TLS when `FIRMWARE_TLS=1`.

Current implementation in [src/ota_updater.cpp](../src/ota_updater.cpp):
```cpp
#if FIRMWARE_TLS == 1
    WiFiClientSecure secureClient;
    secureClient.setInsecure();  // ⚠️ Currently insecure
    http.begin(secureClient, FIRMWARE_URL);
#endif
```

**TODO:** Apply same CA verification to OTA HTTPS:
```cpp
secureClient.setTrustAnchors(&caCert);  // Use same CA as MQTT
```

## ✅ Best Practices

1. **Always verify certificates in production**
   - Never use `setInsecure()` in production
   - Use CA verification or fingerprint

2. **Keep CA certificates updated**
   - Monitor CA expiration
   - Update firmware before CA expires

3. **Sync time before TLS**
   - TLS requires accurate time for cert validation
   - Always sync NTP before MQTT/HTTPS

4. **Test thoroughly**
   - Test with real certificates
   - Verify connection stability
   - Monitor memory usage

5. **Plan for certificate rotation**
   - Server certs expire (typically 90 days for Let's Encrypt)
   - CA certs rarely change (10-20 years)
   - Use CA verification for easier maintenance

## 📚 References

- [ESP8266 BearSSL](https://arduino-esp8266.readthedocs.io/en/latest/esp8266wifi/bearssl-client-secure-class.html)
- [Let's Encrypt Certificates](https://letsencrypt.org/certificates/)
- [OpenSSL Commands](https://www.openssl.org/docs/man1.1.1/man1/)
- [MQTT Security](https://mosquitto.org/man/mosquitto-tls-7.html)

---

**Secure your IoT devices! 🔒**
