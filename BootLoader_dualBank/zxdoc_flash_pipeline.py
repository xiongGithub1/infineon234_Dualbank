#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# vim: set fileencoding=utf-8 :
"""
ZXDoc 统一刷写流水线 - TC234 A/B DualBank Bootloader

完整流程（一键完成）:
    1. align_hex   : 编译器 HEX -> 32B 对齐 HEX (.aligned.hex)
    2. pkg_generator: 对齐 HEX -> .pkg 容器 (Header + Payload + RSA Signature)
    3. shuaxie     : .pkg -> UDS 刷写到 ECU (擦除 -> 下载 -> 验证 -> 复位)

核心设计（单一 HEX，不分 Bank A/B）:
    - 只需要一个编译器生成的 HEX（基址固定，如 0x80020000）
    - .pkg 的 PayloadAddr 填 0（offset 模式）
    - Bootloader 通过 31 FF FD 自动选择 targetBank（A/B）
    - Bootloader 内部根据 targetBank 把 offset 映射到实际物理地址

用法:
    直接在 ZXDoc 中运行本脚本（无需命令行参数）。
    修改下方 "用户配置区" 的路径即可。
"""

import sys
import os
import time
from ZXDoc import *
# ============================================================================
# 把本脚本所在目录加入 Python 路径，以便导入同目录下的 align_hex / pkg_generator
# ============================================================================
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, SCRIPT_DIR)

from align_hex import align_hex_file
from pkg_generator import generate_pkg

# ============================================================================
# 用户配置区（请根据实际工程路径修改）
# ============================================================================

# 编译器原始 HEX（绝对路径，只需一份，不区分 Bank A/B）
HEX_IN        = r"E:\workFiles\IEBS\tc234bootloader\App_dualBank\Debug\App_dualBank.hex"

# 对齐后的 HEX 输出路径（绝对路径）
HEX_OUT       = r"E:\workFiles\IEBS\tc234bootloader\tc234bootloader\App_dualBank.aligned.hex"

# .pkg 输出路径（绝对路径）
PKG_OUT       = r"E:\workFiles\IEBS\tc234bootloader\tc234bootloader\App_dualBank.pkg"

# HEX 编译基址，用于 pkg_generator 的 offset 平移
# 必须与 APP 工程的 LSL 链接基址一致
BASE_ADDR     = 0x80020000

# RSA 私钥路径（绝对路径）
PRIV_KEY      = r"E:\workFiles\IEBS\tc234bootloader\BootLoader_dualBank\keys\private.pem"

# 软件版本号（写入 .pkg Header）
SW_VERSION    = 0x00010002
SW_MASK       = 0xFFFFFFFF

# .pkg Header 中 PayloadAddr 填 0（offset 模式，Bootloader 自己加基址）
PAYLOAD_ADDR  = 0x00000000

# TC234 PFlash page 对齐参数
ALIGN_SIZE    = 32
FILL_BYTE     = 0x00


def run_pipeline():
    """
    执行三步流水线，返回 True/False
    """
    # 直接使用配置的绝对路径
    hex_in   = os.path.normpath(HEX_IN)
    hex_out  = os.path.normpath(HEX_OUT)
    pkg_out  = os.path.normpath(PKG_OUT)
    key_path = os.path.normpath(PRIV_KEY)

    print("=" * 70)
    print("[Pipeline] Unified flash pipeline (single HEX, auto Bank A/B)")
    print("[Pipeline] HEX_IN  = %s" % hex_in)
    print("[Pipeline] HEX_OUT = %s" % hex_out)
    print("[Pipeline] PKG_OUT = %s" % pkg_out)
    print("[Pipeline] BASE_ADDR = 0x%08X" % BASE_ADDR)
    print("=" * 70)

    # ------------------------------------------------------------------------
    # Step 1: Align HEX
    # ------------------------------------------------------------------------
    print("\n[Pipeline] Step 1/3: Align HEX (%dB boundary, fill=0x%02X)" % (ALIGN_SIZE, FILL_BYTE))
    if not os.path.exists(hex_in):
        print("[Pipeline] X HEX input not found: %s" % hex_in)
        return False

    ok = align_hex_file(hex_in, hex_out, align=ALIGN_SIZE, fill_byte=FILL_BYTE)
    if not ok:
        print("[Pipeline] X Align HEX failed")
        return False
    print("[Pipeline] V Align HEX done")

    # ------------------------------------------------------------------------
    # Step 2: Generate .pkg
    # ------------------------------------------------------------------------
    print("\n[Pipeline] Step 2/3: Generate .pkg container")
    if not os.path.exists(key_path):
        print("[Pipeline] X Private key not found: %s" % key_path)
        return False

    try:
        generate_pkg(
            hex_path=hex_out,
            payload_addr=PAYLOAD_ADDR,
            sw_version=SW_VERSION,
            sw_version_mask=SW_MASK,
            private_key_path=key_path,
            out_path=pkg_out,
            base_addr=BASE_ADDR,
            keep_all_segments=False,
            write_aligned_hex=False,   # 已对齐，不再重复生成
            write_bin=True             # 同时输出 .bin 便于调试
        )
    except SystemExit as e:
        # generate_pkg 在错误时 sys.exit(1)
        print("[Pipeline] X PKG generation failed (exit code %s)" % e.code)
        return False
    except Exception as e:
        print("[Pipeline] X PKG generation failed: %s" % e)
        return False

    print("[Pipeline] V PKG generation done")

    # ------------------------------------------------------------------------
    # Step 3: Flash ECU via UDS
    # ------------------------------------------------------------------------
    print("\n[Pipeline] Step 3/3: Flash ECU via UDS")
    if not os.path.exists(pkg_out):
        print("[Pipeline] X PKG file not found: %s" % pkg_out)
        return False

    try:
        from shuaxie import do_flash_process
    except ImportError as e:
        print("[Pipeline] ! Cannot import shuaxie (ZXDoc module not available): %s" % e)
        print("[Pipeline] V Step 1/2 completed (HEX aligned + PKG generated).")
        print("[Pipeline]   Step 3 (flash) requires ZXDoc environment.")
        print("=" * 70)
        return True  # 非 ZXDoc 环境只生成 pkg，不算失败

    flash_ok = do_flash_process(pkg_path=pkg_out)
    if not flash_ok:
        print("[Pipeline] X Flash failed")
        return False

    print("\n" + "=" * 70)
    print("[Pipeline] V All steps completed successfully!")
    print("=" * 70)
    return True


# ============================================================================
# 命令行 / 独立运行入口
# ============================================================================
# if __name__ == "__main__":
#     ok = run_pipeline()
#     sys.exit(0 if ok else 1)


# ============================================================================
# ZXDoc 脚本入口
# ============================================================================
def __zxdoc_main__():
    if not measurement.is_started():
        measurement.start()

    ok = run_pipeline()

    # 保持测量运行一段时间，便于观察最后几帧报文
    time.sleep(2)

    if not ok:
        print("[Pipeline] ! Pipeline finished with errors.")


def __zxdoc_on_exit__():
    measurement.stop()
