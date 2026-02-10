#!/usr/bin/env python3
"""
Local test script untuk simulate GitHub Actions workflow
Test build, sign, dan package firmware sebelum push ke GitHub
"""

import os
import sys
import json
import hashlib
import subprocess
from datetime import datetime

def run_command(cmd, description):
    """Run shell command and print output"""
    print(f"\n{'='*60}")
    print(f"🔧 {description}")
    print(f"{'='*60}")
    result = subprocess.run(cmd, shell=True, capture_output=True, text=True)
    print(result.stdout)
    if result.stderr:
        print(result.stderr, file=sys.stderr)
    if result.returncode != 0:
        print(f"❌ Failed with exit code {result.returncode}")
        sys.exit(1)
    return result

def main():
    print("🚀 ESP8266 OTA Build & Sign Test")
    print("="*60)
    
    # Step 1: Set firmware version
    print("\n📋 Step 1: Generate firmware version")
    try:
        git_hash = subprocess.check_output(
            ["git", "rev-parse", "--short", "HEAD"],
            stderr=subprocess.DEVNULL
        ).decode().strip()
    except:
        git_hash = "unknown"
    
    build_date = datetime.now().strftime("%Y%m%dT%H%M")
    firmware_version = f"{git_hash}-{build_date}-local-test"
    firmware_algorithm = "ed25519"
    
    os.environ["FIRMWARE_VERSION"] = firmware_version
    os.environ["FIRMWARE_ALGORITHM"] = firmware_algorithm
    
    print(f"✅ FIRMWARE_VERSION: {firmware_version}")
    print(f"✅ FIRMWARE_ALGORITHM: {firmware_algorithm}")
    
    # Step 2: Clean build
    run_command("pio run --target clean", "Clean previous build")
    
    # Step 3: Build firmware
    run_command("pio run --environment esp12e", "Build firmware")
    
    # Step 4: Copy firmware
    print("\n📦 Step 4: Prepare firmware binary")
    os.makedirs("build", exist_ok=True)
    
    import shutil
    shutil.copy(".pio/build/esp12e/firmware.bin", "build/firmware-otaq.bin")
    
    fw_size = os.path.getsize("build/firmware-otaq.bin")
    print(f"✅ Firmware copied: {fw_size:,} bytes")
    
    # Step 5: Calculate hash
    print("\n🔐 Step 5: Calculate SHA-256 hash")
    with open("build/firmware-otaq.bin", "rb") as f:
        fw_data = f.read()
    
    fw_hash = hashlib.sha256(fw_data).hexdigest()
    print(f"✅ SHA-256: {fw_hash}")
    
    # Step 6: Sign firmware (simulation)
    print("\n✍️  Step 6: Sign firmware (simulation)")
    print("⚠️  NOTE: Actual signing requires ED25519_PRIVATE_KEY_HEX")
    print("⚠️  In GitHub Actions, this comes from GitHub Secrets")
    
    # Check if pynacl is installed
    try:
        from nacl.signing import SigningKey
        
        # Try to get private key from environment
        private_key_hex = os.environ.get("ED25519_PRIVATE_KEY_HEX", "")
        
        if private_key_hex:
            print("🔑 Using ED25519_PRIVATE_KEY_HEX from environment")
            seed = bytes.fromhex(private_key_hex)
            sk = SigningKey(seed)
            signature = sk.sign(bytes.fromhex(fw_hash)).signature.hex()
            print(f"✅ Signature: {signature[:32]}...{signature[-32:]}")
        else:
            print("⚠️  ED25519_PRIVATE_KEY_HEX not set, generating test key")
            sk = SigningKey.generate()
            signature = sk.sign(bytes.fromhex(fw_hash)).signature.hex()
            print(f"✅ Test Signature: {signature[:32]}...{signature[-32:]}")
            print(f"🔑 Test Public Key: {sk.verify_key.encode().hex()}")
    except ImportError:
        print("❌ pynacl not installed, using dummy signature")
        signature = "0" * 128
    
    # Step 7: Create manifest
    print("\n📄 Step 7: Create manifest.json")
    manifest = {
        "version": firmware_version,
        "hash": fw_hash,
        "signature": signature
    }
    
    with open("manifest.json", "w") as f:
        json.dump(manifest, f, indent=2)
    
    print("✅ Manifest created:")
    print(json.dumps(manifest, indent=2))
    
    # Step 8: Create zip package
    print("\n📦 Step 8: Create firmware.zip")
    run_command(
        "zip -j firmware.zip build/firmware-otaq.bin manifest.json",
        "Package firmware and manifest"
    )
    
    zip_size = os.path.getsize("firmware.zip")
    print(f"✅ Package created: {zip_size:,} bytes")
    
    # Summary
    print("\n" + "="*60)
    print("✅ BUILD & SIGN TEST COMPLETE")
    print("="*60)
    print(f"Firmware Version: {firmware_version}")
    print(f"Firmware Size: {fw_size:,} bytes")
    print(f"Package: firmware.zip ({zip_size:,} bytes)")
    print("\n📋 Files created:")
    print("  - build/firmware-otaq.bin")
    print("  - manifest.json")
    print("  - firmware.zip")
    print("\n🚀 Next steps:")
    print("  1. Review manifest.json")
    print("  2. Test signature verification on ESP8266")
    print("  3. Push to GitHub to trigger actual workflow")
    print("\n⚠️  NOTE: This is a local test.")
    print("   Real signing in GitHub Actions uses GitHub Secrets.")

if __name__ == "__main__":
    main()
