#!/usr/bin/env python3
"""
Verify key compatibility between PyNaCl and cryptography library
"""
from nacl.signing import SigningKey as NaClSigningKey
from cryptography.hazmat.primitives.asymmetric import ed25519
from cryptography.hazmat.primitives import serialization

# Your actual keys from config.h
PRIVATE_KEY_HEX = "ba89c973ffb9836d7c3c9f0b6bc869455cdb6db33aa299c297fd1726f567abd9"
PUBLIC_KEY_HEX = "0bc12f3d7182046866669042d921c91db12f83340e80c4837892828051fafcd8"

print("="*70)
print("Testing Key Compatibility")
print("="*70)

# Test 1: Load with PyNaCl
print("\n1. Load with PyNaCl (nacl.signing.SigningKey)")
seed_bytes = bytes.fromhex(PRIVATE_KEY_HEX)
nacl_key = NaClSigningKey(seed_bytes)
nacl_pubkey = nacl_key.verify_key.encode().hex()
print(f"   Private key: {PRIVATE_KEY_HEX}")
print(f"   Public key:  {nacl_pubkey}")
print(f"   Match: {nacl_pubkey == PUBLIC_KEY_HEX}")

# Test 2: Load with cryptography
print("\n2. Load with cryptography (Ed25519PrivateKey)")
try:
    crypto_key = ed25519.Ed25519PrivateKey.from_private_bytes(seed_bytes)
    crypto_pubkey = crypto_key.public_key().public_bytes(
        encoding=serialization.Encoding.Raw,
        format=serialization.PublicFormat.Raw
    ).hex()
    print(f"   Private key: {PRIVATE_KEY_HEX}")
    print(f"   Public key:  {crypto_pubkey}")
    print(f"   Match: {crypto_pubkey == PUBLIC_KEY_HEX}")
except Exception as e:
    print(f"   ERROR: {e}")

# Test 3: Check if both derive same public key
print("\n3. Public key comparison")
print(f"   PyNaCl:       {nacl_pubkey}")
print(f"   cryptography: {crypto_pubkey}")
print(f"   Expected:     {PUBLIC_KEY_HEX}")
print(f"   PyNaCl matches expected: {nacl_pubkey == PUBLIC_KEY_HEX}")
print(f"   crypto matches expected: {crypto_pubkey == PUBLIC_KEY_HEX}")

# Test 4: Try signing with both libraries
import hashlib
test_data = b"Test firmware data"
test_hash = hashlib.sha256(test_data).digest()

print("\n4. Signature Test")
print(f"   Test hash: {test_hash.hex()}")

# Sign with PyNaCl
nacl_sig = nacl_key.sign(test_hash).signature
print(f"   PyNaCl signature: {nacl_sig.hex()[:64]}...")

# Sign with cryptography
crypto_sig = crypto_key.sign(test_hash)
print(f"   crypto signature: {crypto_sig.hex()[:64]}...")

print(f"   Signatures match: {nacl_sig.hex() == crypto_sig.hex()}")

# Test 5: Verify signatures
print("\n5. Signature Verification")

# Verify PyNaCl signature with PyNaCl
try:
    nacl_key.verify_key.verify(test_hash, nacl_sig)
    print("   ✓ PyNaCl sig verified by PyNaCl")
except Exception as e:
    print(f"   ✗ PyNaCl sig failed: {e}")

# Verify crypto signature with crypto
try:
    crypto_key.public_key().verify(crypto_sig, test_hash)
    print("   ✓ crypto sig verified by crypto")
except Exception as e:
    print(f"   ✗ crypto sig failed: {e}")

# Cross-verify
try:
    nacl_key.verify_key.verify(test_hash, crypto_sig)
    print("   ✓ crypto sig verified by PyNaCl")
except Exception as e:
    print(f"   ✗ crypto sig not verified by PyNaCl: {e}")

print("\n" + "="*70)
print("CONCLUSION:")
print("If signatures DON'T match, then the ESP8266 Crypto library")
print("uses a different Ed25519 variant than standard libraries.")
print("="*70)
