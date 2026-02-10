# GitHub Actions CI/CD Setup

Workflow otomatis untuk build, sign, dan deploy firmware ESP8266 ke OTA server.

## 🎯 Overview

Workflow [build-ota.yml](../.github/workflows/build-ota.yml) akan:

1. ✅ Build firmware dengan PlatformIO
2. ✅ Auto-generate version dari git hash + timestamp + build number
3. ✅ Sign firmware dengan ED25519
4. ✅ Create manifest.json dengan hash & signature
5. ✅ Upload ke OTA server via API

## 🔧 Setup

### 1. GitHub Secrets

Masuk ke **Settings → Secrets and variables → Actions → New repository secret**

**Required Secrets:**

| Secret Name | Description | Format | Example |
|-------------|-------------|--------|---------|
| `ED25519_PRIVATE_KEY_HEX` | ED25519 private key | 64-char hex | `a1b2c3d4...` |
| `API_TOKEN` | Bearer token for upload API | String | `your_api_token` |

**Generate ED25519 Key:**

```bash
# Server-side script (Python)
python3 -c "
from nacl.signing import SigningKey
sk = SigningKey.generate()
print('Private Key (HEX):', sk.encode().hex())
print('Public Key (HEX):', sk.verify_key.encode().hex())
"
```

Copy **Private Key** → GitHub Secret `ED25519_PRIVATE_KEY_HEX`  
Copy **Public Key** → `src/config.h` `PUBLIC_KEY_HEX`

### 2. GitHub Variables

Masuk ke **Settings → Secrets and variables → Actions → Variables → New repository variable**

**Required Variables:**

| Variable Name | Description | Example |
|---------------|-------------|---------|
| `API_URL` | OTA server API base URL | `http://ota.example.com:8000/api/v1` |

### 3. GitHub Environment

Create environment **"firmware env"** untuk approval (optional):

**Settings → Environments → New environment → "firmware env"**

Options:
- ✅ Required reviewers (optional)
- ✅ Wait timer (optional)
- ✅ Deployment branches (recommended: only `master`, `development`)

## 🚀 Usage

### Trigger Build

**Method 1: Pull Request Merge** (Recommended)

```bash
# 1. Create feature branch
git checkout -b feature/add-sensor-logging
git add .
git commit -m "Add sensor logging feature"
git push origin feature/add-sensor-logging

# 2. Create Pull Request via GitHub UI
# Target branch: master or development

# 3. Merge PR → Workflow auto-runs
```

**Method 2: Push to Main Branch**

```bash
git checkout master
git add .
git commit -m "Update firmware"
git push origin master
# Workflow auto-runs after merge
```

### Monitor Build

1. Go to **Actions** tab
2. Click running workflow
3. Monitor each step:
   - ✅ Checkout repository
   - ✅ Setup Python & PlatformIO
   - ✅ Build firmware
   - ✅ Sign firmware
   - ✅ Upload to server

### Check Output

Build output akan show:

```
Building firmware version: fb6828c-20260210T0830-build42
Using firmware algorithm: ed25519
RAM:   [====      ]  40.2% (used 32972 bytes from 81920 bytes)
Flash: [====      ]  35.6% (used 371359 bytes from 1044464 bytes)
✅ Upload successful
```

## 📦 Artifacts

Setelah build sukses, firmware tersedia di:

**Server:**
- `http://your-server.com:8000/api/v1/firmware/firmware-otaq.bin`
- `http://your-server.com:8000/api/v1/firmware/manifest.json`

**GitHub (if enabled):**
- Actions → Workflow run → Artifacts → `platformio-build-logs` (jika build fail)

## 🔒 Security

### Private Key Protection

⚠️ **CRITICAL**: ED25519 private key harus dijaga ketat!

- ✅ Stored in GitHub Secrets (encrypted at rest)
- ✅ Never logged or exposed in workflow
- ✅ Only accessible during workflow execution
- ✅ Access controlled via environment protection rules

### Best Practices

1. **Rotate Keys** - Generate new ED25519 keypair setiap 6-12 bulan
2. **Separate Keys** - Gunakan key berbeda untuk development & production
3. **Audit Logs** - Review GitHub Actions logs regularly
4. **Access Control** - Limit repository access, require branch protection

## 🐛 Troubleshooting

### Build Failed: PlatformIO Error

**Check:**
```bash
# Test locally first
pio run --environment esp12e
```

**Common Issues:**
- Missing dependencies in `platformio.ini`
- Code syntax error
- RAM/Flash overflow

### Signing Failed: Invalid Private Key

**Error:** `ValueError: Invalid ED25519 private key length`

**Solution:**
```bash
# Verify key length
echo -n "YOUR_KEY" | wc -c
# Should output: 64

# Regenerate if needed
python3 -c "from nacl.signing import SigningKey; print(SigningKey.generate().encode().hex())"
```

### Upload Failed: API Error

**Check:**
1. `API_URL` variable correct?
2. `API_TOKEN` secret valid?
3. Server endpoint accessible?

```bash
# Test API manually
curl -X POST "http://your-server.com:8000/api/v1/firmware/upload" \
  -H "Authorization: Bearer YOUR_TOKEN" \
  -F "file=@firmware.zip"
```

### Workflow Not Triggering

**Check:**
1. Workflow file in `.github/workflows/` directory?
2. Branch name matches trigger? (master/development)
3. PR actually merged? (not just closed)

```yaml
# Verify trigger in build-ota.yml
on:
  pull_request:
    branches:
      - master        # Check your branch name
      - development
    types: [closed]   # Must be merged, not just closed
```

## 📊 Workflow Details

### Build Steps

1. **Checkout** - Clone repository dengan history
2. **Setup Python** - Install Python 3.11
3. **Cache PlatformIO** - Speed up subsequent builds
4. **Install deps** - Install zip, curl, platformio, pynacl
5. **Set version** - Generate from git hash + timestamp
6. **Build** - `pio run --environment esp12e`
7. **Sign** - ED25519 signature via Python nacl
8. **Package** - Create firmware.zip with manifest.json
9. **Upload** - POST to API server

### Build Time

Typical build times:
- First build: ~5-7 minutes (download packages)
- Cached build: ~2-3 minutes
- Clean build: ~3-4 minutes

### Version Format

Auto-generated version:
```
{git_hash}-{timestamp}-build{run_number}

Example: fb6828c-20260210T0830-build42
```

Where:
- `fb6828c` - Short git commit hash (7 chars)
- `20260210T0830` - Build timestamp (YYYYMMDDTHHmm)
- `build42` - GitHub Actions run number

## 🔄 Update Workflow

Edit [.github/workflows/build-ota.yml](../.github/workflows/build-ota.yml):

**Add board:**
```yaml
- name: Build firmware
  run: |
    pio run --environment esp12e
    pio run --environment esp32dev  # Add more boards
```

**Change trigger:**
```yaml
on:
  push:
    branches: [master]  # Trigger on every push
  pull_request:
    branches: [master]
    types: [closed]
```

**Add testing:**
```yaml
- name: Run tests
  run: pio test --environment esp12e
```

## 📚 References

- [GitHub Actions Docs](https://docs.github.com/en/actions)
- [PlatformIO CI/CD](https://docs.platformio.org/en/latest/integration/ci/index.html)
- [ED25519 Signature](https://ed25519.cr.yp.to/)
- [PyNaCl Library](https://pynacl.readthedocs.io/)

## ✅ Checklist

Before enabling workflow:

- [ ] ED25519 keypair generated
- [ ] Private key added to GitHub Secrets
- [ ] Public key added to `src/config.h`
- [ ] API_URL variable configured
- [ ] API_TOKEN secret added
- [ ] Server endpoint ready
- [ ] Test local build: `pio run`
- [ ] Test signing script manually
- [ ] Create test PR and merge

---

**Ready for automated CI/CD deployment! 🚀**
