#!/usr/bin/env python3
"""
Debug script: verify .pkg signature locally to isolate the failure.
"""
import sys
import os
import struct
import zlib

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from cryptography.hazmat.primitives import serialization, hashes
from cryptography.hazmat.primitives.asymmetric import padding

def verify_pkg(pkg_path, pub_key_path):
    with open(pkg_path, "rb") as f:
        pkg = f.read()
    with open(pub_key_path, "rb") as f:
        public_key = serialization.load_pem_public_key(f.read())

    HEADER_SIZE = 128
    SIG_LEN = 256

    header = pkg[0:HEADER_SIZE]
    payload_len = struct.unpack_from("<I", header, 12)[0]
    payload_crc_expected = struct.unpack_from("<I", header, 28)[0]

    payload = pkg[HEADER_SIZE : HEADER_SIZE + payload_len]
    signature = pkg[HEADER_SIZE + payload_len : HEADER_SIZE + payload_len + SIG_LEN]

    print(f"[verify] pkg_path:    {pkg_path}")
    print(f"[verify] payload_len: {payload_len}")
    print(f"[verify] payload_crc: 0x{payload_crc_expected:08X}")
    print(f"[verify] signature:   {len(signature)} bytes")

    # 1. Check Payload CRC
    payload_crc_actual = zlib.crc32(payload) & 0xFFFFFFFF
    print(f"[verify] calc_crc:    0x{payload_crc_actual:08X}")
    if payload_crc_expected != payload_crc_actual:
        print("[FAIL] Payload CRC mismatch!")
        return False
    print("[PASS] Payload CRC OK")

    # 2. Check Signature over Payload only
    data_to_verify = payload
    print(f"[verify] data_to_verify len: {len(data_to_verify)}")
    try:
        public_key.verify(
            signature,
            data_to_verify,
            padding.PKCS1v15(),
            hashes.SHA256()
        )
        print("[PASS] Signature OK (Payload only)")
    except Exception as e:
        print(f"[FAIL] Signature verification failed: {e}")
        return False

    # 3. Also try Header + Payload (old behavior) to confirm mismatch
    try:
        public_key.verify(
            signature,
            header + payload,
            padding.PKCS1v15(),
            hashes.SHA256()
        )
        print("[PASS] Signature OK (Header + Payload) -- OLD BEHAVIOR")
    except Exception as e:
        print(f"[INFO] Signature fails with Header+Payload (expected after fix): {e}")

    return True

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: python verify_debug.py <pkg_file> <public.pem>")
        sys.exit(1)
    ok = verify_pkg(sys.argv[1], sys.argv[2])
    sys.exit(0 if ok else 1)
