from ZXDoc import *
from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.asymmetric import padding
import struct
import zlib

# ============================================================================
# Constants (must match pkg_generator.py)
# ============================================================================
HEADER_SIZE = 128
SIG_LEN_RSA2048 = 256

# ============================================================================
# Paths
# ============================================================================
# IMPORTANT: public key must be PEM format, NOT C header (.h)
public_k_path = r"E:\workFiles\IEBS\tc234bootloader\BootLoader_dualBank\keys\public.pem"
pkgpath = r"E:\workFiles\IEBS\tc234bootloader\tc234bootloader\App_dualBank.pkg"


def calc_crc32(data):
    """计算 CRC32，与 Bootloader 流式 CRC 兼容（IEEE 802.3）"""
    return zlib.crc32(data) & 0xFFFFFFFF


def verify_pkg(pkg_path, public_key_path):
    """验证 .pkg 文件的签名（用于测试）"""
    with open(public_key_path, "rb") as f:
        public_key = serialization.load_pem_public_key(f.read())

    with open(pkg_path, "rb") as f:
        pkg = f.read()

    header = pkg[0:HEADER_SIZE]
    signature = pkg[-SIG_LEN_RSA2048:]
    payload = pkg[HEADER_SIZE:-SIG_LEN_RSA2048]

    # 验证 Header CRC (不含 CRC 字段本身)
    header_crc_expected = struct.unpack_from("<I", header, 32)[0]
    header_crc_actual = calc_crc32(header[0:32])
    if header_crc_expected != header_crc_actual:
        print(f"[FAIL] Header CRC mismatch: expected 0x{header_crc_expected:08X}, actual 0x{header_crc_actual:08X}")
        return False
    print(f"[PASS] Header CRC OK (0x{header_crc_actual:08X})")

    # 验证 Payload CRC
    payload_crc_expected = struct.unpack_from("<I", header, 28)[0]
    payload_crc_actual = calc_crc32(payload)
    if payload_crc_expected != payload_crc_actual:
        print(f"[FAIL] Payload CRC mismatch: expected 0x{payload_crc_expected:08X}, actual 0x{payload_crc_actual:08X}")
        return False
    print(f"[PASS] Payload CRC OK (0x{payload_crc_actual:08X})")

    # 验证签名
    data_to_verify = payload
    try:
        public_key.verify(
            signature,
            data_to_verify,
            padding.PKCS1v15(),
            hashes.SHA256()
        )
        print("[PASS] Signature OK")
        return True
    except Exception as e:
        print(f"[FAIL] Signature verification failed: {e}")
        return False


def __zxdoc_init__():
    pass


def __zxdoc_main__():
    global pkgpath
    global public_k_path
    verify_pkg(pkgpath, public_k_path)


def __zxdoc_on_exit__():
    pass
