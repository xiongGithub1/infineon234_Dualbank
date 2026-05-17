#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
.pkg 容器文件查看 / 验证工具

用法:
    # 在 ZXDoc 中直接运行（无需参数，读取下方配置区路径）
    # 或命令行运行：python inspect_pkg.py

功能:
    - 解析并显示 .pkg Header 各字段
    - 验证 Header CRC32 和 Payload CRC32
    - 验证 RSA 签名（可选）
    - 导出 Payload / Signature / Header 为文件（可选）
"""

import sys
import os
import struct
import zlib
import time

# ============================================================================
# 用户配置区（请根据实际路径修改为绝对路径）
# ============================================================================

# 要查看的 .pkg 文件路径（绝对路径）
PKG_FILE      = r"D:\workFiles\dualbank\dualbank-main\tc234bootloader\App_dualBank.pkg"

# RSA 公钥路径（绝对路径，用于验证签名，不需要验证可留空）
PUB_KEY       = r"D:\workFiles\dualbank\dualbank-main\BootLoader_dualBank\keys\public.pem"

# 导出选项（不需要导出可留空字符串 ""）
DUMP_PAYLOAD  = r""    # 例如: r"E:\workFiles\IEBS\tc234bootloader\App_dualBank.payload.bin"
DUMP_SIG      = r""    # 例如: r"E:\workFiles\IEBS\tc234bootloader\App_dualBank.sig.bin"
DUMP_HEADER   = r""    # 例如: r"E:\workFiles\IEBS\tc234bootloader\App_dualBank.header.bin"

# ============================================================================
# Constants
# ============================================================================
HEADER_SIZE   = 128
SIG_LEN_2048  = 256


def calc_crc32(data):
    return zlib.crc32(data) & 0xFFFFFFFF


def inspect_pkg(pkg_path, pubkey_path=None,
                dump_payload=None, dump_sig=None,
                dump_header=None):
    """解析并显示 .pkg 文件的详细信息，返回 True/False"""

    pkg_path = os.path.normpath(pkg_path)
    if not os.path.exists(pkg_path):
        print("[inspect] X File not found: %s" % pkg_path)
        return False

    with open(pkg_path, "rb") as f:
        pkg = f.read()

    total_size = len(pkg)
    print("=" * 70)
    print("[inspect] PKG file: %s" % pkg_path)
    print("[inspect] Total size: %d bytes (0x%X)" % (total_size, total_size))
    print("=" * 70)

    if total_size < HEADER_SIZE + SIG_LEN_2048:
        print("[inspect] X File too small to be a valid .pkg")
        return False

    # ------------------------------------------------------------------------
    # Parse Header
    # ------------------------------------------------------------------------
    header = pkg[0:HEADER_SIZE]

    magic           = header[0:4]
    header_size     = struct.unpack_from("<I", header, 4)[0]
    payload_addr    = struct.unpack_from("<I", header, 8)[0]
    payload_len     = struct.unpack_from("<I", header, 12)[0]
    sw_version      = struct.unpack_from("<I", header, 16)[0]
    sw_version_mask = struct.unpack_from("<I", header, 20)[0]
    build_timestamp = struct.unpack_from("<I", header, 24)[0]
    payload_crc     = struct.unpack_from("<I", header, 28)[0]
    header_crc      = struct.unpack_from("<I", header, 32)[0]
    sig_algorithm   = struct.unpack_from("<I", header, 36)[0]
    sig_len         = struct.unpack_from("<I", header, 40)[0]

    print("\n[inspect] --- Header (128 bytes) ---")
    print("  [0x00:0x03] Magic           : %s" % magic)
    print("  [0x04:0x07] HeaderSize      : %d bytes" % header_size)
    print("  [0x08:0x0B] PayloadAddr     : 0x%08X" % payload_addr)
    print("  [0x0C:0x0F] PayloadLength   : %d bytes (0x%X)" % (payload_len, payload_len))
    print("  [0x10:0x13] SW Version      : 0x%08X" % sw_version)
    print("  [0x14:0x17] SW Version Mask : 0x%08X" % sw_version_mask)
    print("  [0x18:0x1B] Build Timestamp : %d (%s)" % (
        build_timestamp,
        time.strftime("%Y-%m-%d %H:%M:%S", time.localtime(build_timestamp)) if build_timestamp > 0 else "N/A"
    ))
    print("  [0x1C:0x1F] Payload CRC32   : 0x%08X" % payload_crc)
    print("  [0x20:0x23] Header CRC32    : 0x%08X" % header_crc)
    print("  [0x24:0x27] Sig Algorithm   : %d (%s)" % (
        sig_algorithm,
        "RSA-2048" if sig_algorithm == 1 else "Unknown"
    ))
    print("  [0x28:0x2B] Sig Length      : %d bytes" % sig_len)
    print("  [0x2C:0x7F] Reserved        : %d bytes" % (128 - 44))

    # ------------------------------------------------------------------------
    # Verify Header CRC (header[0:32])
    # ------------------------------------------------------------------------
    header_crc_calc = calc_crc32(header[0:32])
    if header_crc != header_crc_calc:
        print("\n[inspect] X Header CRC mismatch!")
        print("          Expected: 0x%08X" % header_crc)
        print("          Actual  : 0x%08X" % header_crc_calc)
    else:
        print("\n[inspect] V Header CRC OK (0x%08X)" % header_crc_calc)

    # ------------------------------------------------------------------------
    # Extract Payload
    # ------------------------------------------------------------------------
    expected_min_size = HEADER_SIZE + payload_len + sig_len
    if total_size < expected_min_size:
        print("[inspect] X File size mismatch: expected >= %d, actual %d" % (expected_min_size, total_size))
        return False

    payload   = pkg[HEADER_SIZE : HEADER_SIZE + payload_len]
    signature = pkg[HEADER_SIZE + payload_len : HEADER_SIZE + payload_len + sig_len]

    print("[inspect] Payload   : offset=0x%04X, len=%d bytes" % (HEADER_SIZE, payload_len))
    print("[inspect] Signature : offset=0x%04X, len=%d bytes" % (HEADER_SIZE + payload_len, len(signature)))

    # ------------------------------------------------------------------------
    # Verify Payload CRC
    # ------------------------------------------------------------------------
    payload_crc_calc = calc_crc32(payload)
    if payload_crc != payload_crc_calc:
        print("[inspect] X Payload CRC mismatch!")
        print("          Expected: 0x%08X" % payload_crc)
        print("          Actual  : 0x%08X" % payload_crc_calc)
    else:
        print("[inspect] V Payload CRC OK (0x%08X)" % payload_crc_calc)

    # ------------------------------------------------------------------------
    # Dump files if requested
    # ------------------------------------------------------------------------
    if dump_payload:
        dump_payload = os.path.normpath(dump_payload)
        with open(dump_payload, "wb") as f:
            f.write(payload)
        print("[inspect] V Payload dumped to: %s" % dump_payload)

    if dump_sig:
        dump_sig = os.path.normpath(dump_sig)
        with open(dump_sig, "wb") as f:
            f.write(signature)
        print("[inspect] V Signature dumped to: %s" % dump_sig)

    if dump_header:
        dump_header = os.path.normpath(dump_header)
        with open(dump_header, "wb") as f:
            f.write(header)
        print("[inspect] V Header dumped to: %s" % dump_header)

    # ------------------------------------------------------------------------
    # Verify Signature (optional)
    # ------------------------------------------------------------------------
    if pubkey_path:
        pubkey_path = os.path.normpath(pubkey_path)
        if not os.path.exists(pubkey_path):
            print("[inspect] X Public key not found: %s" % pubkey_path)
            return False

        try:
            from cryptography.hazmat.primitives import hashes, serialization
            from cryptography.hazmat.primitives.asymmetric import padding
        except ImportError:
            print("[inspect] X 'cryptography' library not installed. Run: pip install cryptography")
            return False

        with open(pubkey_path, "rb") as f:
            public_key = serialization.load_pem_public_key(f.read())

        try:
            public_key.verify(
                signature,
                payload,
                padding.PKCS1v15(),
                hashes.SHA256()
            )
            print("[inspect] V RSA Signature OK (Payload only)")
        except Exception as e:
            print("[inspect] X RSA Signature verification failed: %s" % e)

    print("=" * 70)
    return True


def run_inspect():
    """使用配置区路径执行 inspect"""
    return inspect_pkg(
        pkg_path=PKG_FILE,
        pubkey_path=PUB_KEY if PUB_KEY else None,
        dump_payload=DUMP_PAYLOAD if DUMP_PAYLOAD else None,
        dump_sig=DUMP_SIG if DUMP_SIG else None,
        dump_header=DUMP_HEADER if DUMP_HEADER else None,
    )


# ============================================================================
# 命令行 / 独立运行入口
# ============================================================================
if __name__ == "__main__":
    ok = run_inspect()
    sys.exit(0 if ok else 1)


# ============================================================================
# ZXDoc 脚本入口
# ============================================================================
def __zxdoc_main__():
    ok = run_inspect()
    if not ok:
        print("[inspect] ! Inspection finished with errors.")


def __zxdoc_on_exit__():
    pass
