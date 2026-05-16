#!/usr/bin/env python3
"""
ZXDoc script: Extract public key to C header (public_key.h)
Usage: Directly run in ZXDoc (no arguments needed)
"""
import sys
import os

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from pkg_generator import extract_public_key_for_c

def __zxdoc_main__():
    base_dir = os.path.dirname(os.path.abspath(__file__))
    pub_pem = os.path.join(base_dir, "keys", "public.pem")
    out_h = os.path.join(base_dir, "AppSw", "Tricore", "App_bootloader", "crypto", "public_key.h")
    
    if not os.path.exists(pub_pem):
        print(f"[ERROR] Public key not found: {pub_pem}")
        print("[ERROR] Please run zxdoc_genkey.py first!")
        return
    
    extract_public_key_for_c(pub_pem, out_h)
    print(f"[extract-pub] Done. Header written to: {out_h}")

if __name__ == "__main__":
    __zxdoc_main__()
