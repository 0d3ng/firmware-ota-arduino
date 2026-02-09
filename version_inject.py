Import("env")
import subprocess
import os
from datetime import datetime

# ==== Handle firmware version from CI/CD ====
firmware_version = os.getenv("FIRMWARE_VERSION")

if not firmware_version:
    # Fallback for local builds
    try:
        # Get git hash
        git_hash = subprocess.check_output(
            ["git", "rev-parse", "--short", "HEAD"],
            stderr=subprocess.DEVNULL
        ).decode().strip()
    except:
        git_hash = "unknown"
    
    # Get build timestamp
    build_date = datetime.now().strftime("%Y%m%dT%H%M")
    
    firmware_version = f"{git_hash}-{build_date}-local"

print(f"Building firmware version: {firmware_version}")

# Add version to compile definitions
env.Append(CPPDEFINES=[
    ("FIRMWARE_VERSION", f'\\"{firmware_version}\\"')
])

# ==== Handle firmware algorithm from CI/CD ====
firmware_algorithm = os.getenv("FIRMWARE_ALGORITHM")

if not firmware_algorithm:
    # Default algorithm for local builds
    firmware_algorithm = "ed25519"

print(f"Using firmware algorithm: {firmware_algorithm}")

# Add algorithm to compile definitions
env.Append(CPPDEFINES=[
    ("FIRMWARE_ALGORITHM", f'\\"{firmware_algorithm}\\"')
])
