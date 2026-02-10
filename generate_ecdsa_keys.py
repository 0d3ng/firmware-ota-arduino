#!/usr/bin/env python3
"""
Generate ECDSA-SHA256-P256 keypair for OTA firmware signing

Algorithm: ECDSA signature with SHA-256 hash and P-256 curve (secp256r1)

Usage:
    python generate_ecdsa_keys.py

Output:
    - Private key (32 bytes hex) - Save to GitHub Secrets as ECDSA_PRIVATE_KEY_HEX
    - Public key (64 bytes hex) - Update config.h PUBLIC_KEY_HEX
"""

from cryptography.hazmat.primitives.asymmetric import ec
from cryptography.hazmat.backends import default_backend
from cryptography.hazmat.primitives import serialization
import secrets

def generate_ecdsa_keypair():
    """Generate ECDSA P-256 (secp256r1) keypair"""
    
    # Generate private key
    private_key = ec.generate_private_key(ec.SECP256R1(), default_backend())
    
    # Get private key value (32 bytes)
    private_numbers = private_key.private_numbers()
    private_bytes = private_numbers.private_value.to_bytes(32, byteorder='big')
    private_hex = private_bytes.hex()
    
    # Get public key (X and Y coordinates, 32 bytes each)
    public_key = private_key.public_key()
    public_numbers = public_key.public_numbers()
    
    x_bytes = public_numbers.x.to_bytes(32, byteorder='big')
    y_bytes = public_numbers.y.to_bytes(32, byteorder='big')
    public_bytes = x_bytes + y_bytes  # Uncompressed (64 bytes)
    public_hex = public_bytes.hex()
    
    # Also save PEM format for reference
    private_pem = private_key.private_bytes(
        encoding=serialization.Encoding.PEM,
        format=serialization.PrivateFormat.PKCS8,
        encryption_algorithm=serialization.NoEncryption()
    ).decode('utf-8')
    
    public_pem = public_key.public_bytes(
        encoding=serialization.Encoding.PEM,
        format=serialization.PublicFormat.SubjectPublicKeyInfo
    ).decode('utf-8')
    
    return {
        'private_hex': private_hex,
        'public_hex': public_hex,
        'private_pem': private_pem,
        'public_pem': public_pem
    }

if __name__ == "__main__":
    print("=" * 80)
    print("ECDSA-SHA256-P256 Keypair Generator for OTA Firmware Signing")
    print("Algorithm: ECDSA with SHA-256 hash and P-256 curve (secp256r1)")
    print("=" * 80)
    
    keys = generate_ecdsa_keypair()
    
    print("\n### PRIVATE KEY (32 bytes hex) ###")
    print(f"Length: {len(keys['private_hex'])} characters (32 bytes)")
    print(f"Hex: {keys['private_hex']}")
    print("\n⚠️  SAVE THIS TO GITHUB SECRETS AS: ECDSA_PRIVATE_KEY_HEX")
    print("   Settings > Secrets > Actions > New repository secret")
    print("   Name: ECDSA_PRIVATE_KEY_HEX")
    print(f"   Value: {keys['private_hex']}")
    
    print("\n\n### PUBLIC KEY (64 bytes hex) ###")
    print(f"Length: {len(keys['public_hex'])} characters (64 bytes)")
    print(f"Hex: {keys['public_hex']}")
    print("\n📝 UPDATE config.h WITH THIS PUBLIC KEY:")
    print(f'   #define PUBLIC_KEY_HEX "{keys["public_hex"]}"')
    
    print("\n\n### PRIVATE KEY (PEM FORMAT) - For Reference ###")
    print(keys['private_pem'])
    
    print("\n### PUBLIC KEY (PEM FORMAT) - For Reference ###")
    print(keys['public_pem'])
    
    print("\n" + "=" * 80)
    print("✅ Keypair generated successfully!")
    print("=" * 80)
    
    # Save to file
    with open("ecdsa_keys.txt", "w") as f:
        f.write(f"Private Key (hex): {keys['private_hex']}\n")
        f.write(f"Public Key (hex): {keys['public_hex']}\n\n")
        f.write("Private Key (PEM):\n")
        f.write(keys['private_pem'])
        f.write("\n\nPublic Key (PEM):\n")
        f.write(keys['public_pem'])
    
    print("\n📄 Keys also saved to: ecdsa_keys.txt")
    print("⚠️  KEEP ecdsa_keys.txt SECURE! Delete after copying to GitHub Secrets.\n")
