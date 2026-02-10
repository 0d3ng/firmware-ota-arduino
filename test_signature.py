#!/usr/bin/env python3
"""
Test ED25519 signature compatibility between Python and ESP8266
"""
import hashlib
from nacl.signing import SigningKey
from nacl.encoding import HexEncoder

# Your keys from config.h
PRIVATE_KEY_HEX = "ba89c973ffb9836d7c3c9f0b6bc869455cdb6db33aa299c297fd1726f567abd9"
PUBLIC_KEY_HEX = "0bc12f3d7182046866669042d921c91db12f83340e80c4837892828051fafcd8"

# Test message (simulating firmware hash)
test_message = b"Hello, this is a test message for ED25519 signature verification!"
message_hash = hashlib.sha256(test_message).digest()

print("="*60)
print("ED25519 Signature Test")
print("="*60)

# Load key from hex
seed = bytes.fromhex(PRIVATE_KEY_HEX)
sk = SigningKey(seed)

# Get public key and verify
pk_bytes = sk.verify_key.encode()
print(f"Private key (seed): {PRIVATE_KEY_HEX}")
print(f"Public key derived: {pk_bytes.hex()}")
print(f"Public key expected: {PUBLIC_KEY_HEX}")
print(f"Public key match: {pk_bytes.hex() == PUBLIC_KEY_HEX}")
print()

# Test 1: Sign the raw message
print("Test 1: Sign RAW message (what PyNaCl does by default)")
signed_msg = sk.sign(test_message)
sig1 = signed_msg.signature
print(f"Message: {test_message[:32]}...")
print(f"Signature: {sig1.hex()[:64]}...")
print(f"Signature length: {len(sig1)} bytes")
print()

# Test 2: Sign the hash (what we're trying to do)
print("Test 2: Sign HASH of message (what we want)")
signed_hash = sk.sign(message_hash)
sig2 = signed_hash.signature
print(f"Message hash: {message_hash.hex()}")
print(f"Signature: {sig2.hex()[:64]}...")
print(f"Signature length: {len(sig2)} bytes")
print()

# Verify both
from nacl.signing import VerifyKey
vk = VerifyKey(pk_bytes)

print("Verification Test 1: Verify signature of RAW message")
try:
    vk.verify(test_message, sig1)
    print("✓ PASS: Signature valid for raw message")
except Exception as e:
    print(f"✗ FAIL: {e}")

print()
print("Verification Test 2: Verify signature of HASH")
try:
    vk.verify(message_hash, sig2)
    print("✓ PASS: Signature valid for hash")
except Exception as e:
    print(f"✗ FAIL: {e}")

print()
print("="*60)
print("PROBLEM: PyNaCl's sign() expects to receive the ORIGINAL")
print("message, NOT a pre-computed hash. It will hash the input")
print("again internally with SHA-512 (Ed25519 standard).")
print()
print("ESP8266 Crypto library Ed25519::verify() expects:")
print("  - signature (64 bytes)")
print("  - public key (32 bytes)")
print("  - message to verify (can be hash)")
print()
print("SOLUTION: We need to use a library that supports")
print("'raw' Ed25519 signing without internal hashing,")
print("or change the approach.")
print("="*60)
