#!/usr/bin/env python3
"""
ZXDoc script: Generate RSA-2048 key pair
Usage: Directly run in ZXDoc (no arguments needed)
"""
import sys
import os

# Ensure we can import pkg_generator
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from pkg_generator import generate_key_pair

def __zxdoc_main__():
    out_dir = os.path.join(os.path.dirname(os.path.abspath(__file__)), "keys")
    os.makedirs(out_dir, exist_ok=True)
    priv, pub = generate_key_pair(out_dir)
    print(f"[genkey] Done. Keys saved to: {out_dir}")
    print(f"[genkey] Private: {priv}")
    print(f"[genkey] Public:  {pub}")

if __name__ == "__main__":
    __zxdoc_main__()
