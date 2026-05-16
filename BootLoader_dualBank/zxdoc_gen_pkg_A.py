#!/usr/bin/env python3
"""
ZXDoc script: Generate .pkg for Bank A (0x80020000)
Usage: Directly run in ZXDoc (no arguments needed)
"""
import sys
import os

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from pkg_generator import generate_pkg

# ============================================
# User Config (modify if needed)
# ============================================
HEX_FILE       = r"..\App_dualBank\Debug\App_dualBank.hex"
BASE_ADDR      = 0x80020000      # HEX 编译基址，用于 offset 平移
PAYLOAD_ADDR   = 0x00000000      # .pkg Header 中填 0（offset 模式，Bootloader 自己加基址）
SW_VERSION     = 0x00010002
SW_MASK        = 0xFFFFFFFF
PRIV_KEY       = r".\keys\private.pem"
OUT_FILE       = r"..\tc234bootloader\App_dualBank.pkg"

def __zxdoc_main__():
    base_dir = os.path.dirname(os.path.abspath(__file__))
    
    hex_path = os.path.normpath(os.path.join(base_dir, HEX_FILE))
    key_path = os.path.normpath(os.path.join(base_dir, PRIV_KEY))
    out_path = os.path.normpath(os.path.join(base_dir, OUT_FILE))
    
    print(f"[gen_pkg_A] HEX:       {hex_path}")
    print(f"[gen_pkg_A] BaseAddr:  0x{BASE_ADDR:08X}")
    print(f"[gen_pkg_A] Key:       {key_path}")
    print(f"[gen_pkg_A] Out:       {out_path}")
    
    generate_pkg(hex_path, PAYLOAD_ADDR, SW_VERSION, SW_MASK, key_path, out_path,
                 base_addr=BASE_ADDR, keep_all_segments=False,
                 write_aligned_hex=True, write_bin=True)
    print(f"[gen_pkg_A] Done! .pkg generated: {out_path}")

if __name__ == "__main__":
    __zxdoc_main__()
