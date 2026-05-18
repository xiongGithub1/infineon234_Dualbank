#!/usr/bin/env python3
"""
.pkg 容器生成器 —— TC234 双区 Bootloader 刷写容器

用法:
    python pkg_generator.py \
        --hex App_dualBank.hex \
        --addr 0x80020000 \
        --version 0x00010002 \
        --key private.pem \
        --out App_dualBank.pkg

    python pkg_generator.py \
        --hex App_dualBank.hex \
        --addr 0x80100000 \
        --version 0x00010002 \
        --key private.pem \
        --out App_dualBank_B.pkg

容器格式 (PKG3 - 多段支持):
    [0:127]        Header (128 bytes)
    [128:128+P]    Payload (多段数据，每段前带 16-byte SEGM 描述头)
                     SEGM 头: [SEGM(4) | flash_addr(4) | data_len(4) | reserved(4)]
                     data_len 不含 SEGM 头本身
    [128+P:128+P+256]  Signature (RSA-2048 PKCS#1 v1.5 + SHA-256)
"""

import argparse
import struct
import zlib
import sys
import os

try:
    from cryptography.hazmat.primitives import hashes, serialization
    from cryptography.hazmat.primitives.asymmetric import padding
except ImportError:
    print("[ERROR] 'cryptography' library not found.")
    print("        Install: pip install cryptography")
    sys.exit(1)

# ============================================================================
# Constants
# ============================================================================
PKG_MAGIC = b"PKG3"          # 4 bytes (v3 = multi-segment support)
HEADER_SIZE = 128            # bytes
SIG_ALGORITHM_RSA2048 = 1
SIG_LEN_RSA2048 = 256
PAGE_SIZE = 32               # TC234 PFlash page size (bytes)

# Multi-segment descriptor (embedded in payload)
SEGMENT_MAGIC = b"SEGM"      # 4 bytes
SEGMENT_HEADER_SIZE = 16     # bytes: [SEGM(4) | addr(4) | len(4) | reserved(4)]


def parse_hex_file(filepath):
    """解析 Intel HEX 文件，返回 {abs_addr: byte_value} 和起始地址"""
    data = {}
    base_addr = 0
    start_addr = None

    with open(filepath, 'r') as f:
        for line_num, line in enumerate(f, 1):
            line = line.strip()
            if not line or line[0] != ':':
                continue
            if len(line) < 11:
                continue

            try:
                byte_count = int(line[1:3], 16)
                addr = int(line[3:7], 16)
                rec_type = int(line[7:9], 16)
            except ValueError:
                continue

            if rec_type == 0x04:  # Extended Linear Address
                base_addr = int(line[9:13], 16) << 16
            elif rec_type == 0x02:  # Extended Segment Address
                base_addr = int(line[9:13], 16) << 4
            elif rec_type == 0x00:  # Data
                abs_addr = base_addr + addr
                for i in range(byte_count):
                    try:
                        data[abs_addr + i] = int(line[9 + i*2:11 + i*2], 16)
                    except (ValueError, IndexError):
                        pass
            elif rec_type == 0x05:  # Start Linear Address
                if len(line) >= 17:
                    start_addr = int(line[9:17], 16)
            elif rec_type == 0x01:  # End of File
                break

    return data, start_addr


def group_segments(data, max_gap=PAGE_SIZE):
    """将数据地址分组为连续段（空隙 <= max_gap 的合并）"""
    if not data:
        return []
    sorted_addrs = sorted(data.keys())
    segments = []
    seg_start = sorted_addrs[0]
    seg_end = sorted_addrs[0] + 1
    prev_addr = sorted_addrs[0]
    for addr in sorted_addrs[1:]:
        if addr <= prev_addr + max_gap:
            seg_end = max(seg_end, addr + 1)
        else:
            segments.append((seg_start, seg_end))
            seg_start = addr
            seg_end = addr + 1
        prev_addr = addr
    segments.append((seg_start, seg_end))
    return segments


def align_segments(segments, page_size=PAGE_SIZE):
    """对齐段边界到 page_size"""
    aligned = []
    for seg_start, seg_end in segments:
        a_start = seg_start & ~(page_size - 1)
        a_end = (seg_end + page_size - 1) & ~(page_size - 1)
        aligned.append((a_start, a_end))
    return aligned


def write_intel_hex(data, segments, out_path, page_size=PAGE_SIZE):
    """生成 Intel HEX 文件（段已对齐）"""
    with open(out_path, 'w') as f:
        current_base = -1
        for seg_start, seg_end in segments:
            addr = seg_start
            while addr < seg_end:
                base = addr >> 16
                if base != current_base:
                    current_base = base
                    cs = (2 + 0 + 0 + 4 + ((base >> 8) & 0xFF) + (base & 0xFF)) & 0xFF
                    cs = ((-cs) & 0xFF)
                    f.write(f":02000004{base:04X}{cs:02X}\n")
                offset = addr & 0xFFFF
                line_data = bytearray()
                for i in range(page_size):
                    line_data.append(data.get(addr + i, 0x00))
                cs = page_size + ((offset >> 8) & 0xFF) + (offset & 0xFF)
                for b in line_data:
                    cs += b
                cs = ((-cs) & 0xFF)
                f.write(f":{page_size:02X}{offset:04X}00{line_data.hex().upper()}{cs:02X}\n")
                addr += page_size
        f.write(":00000001FF\n")


def extract_binary(data, expected_start_addr):
    """
    从 HEX 解析结果中提取连续二进制数据（旧方法，保留空洞）。
    返回起始地址对齐后的 bytearray。
    """
    if not data:
        return bytearray(), 0

    sorted_addrs = sorted(data.keys())
    min_addr = sorted_addrs[0]
    max_addr = sorted_addrs[-1]

    # 如果提供了期望的起始地址，使用它；否则使用 HEX 中的最小地址
    start_addr = expected_start_addr if expected_start_addr else min_addr

    # 确保 start_addr <= min_addr，否则前面会缺数据
    if start_addr > min_addr:
        print(f"[WARN] expected start_addr 0x{start_addr:08X} > HEX min_addr 0x{min_addr:08X}")
        start_addr = min_addr

    length = max_addr - start_addr + 1
    binary = bytearray(length)

    for addr, val in data.items():
        offset = addr - start_addr
        if 0 <= offset < length:
            binary[offset] = val

    return binary, start_addr


def extract_binary_compact(data, base_addr=None, page_size=PAGE_SIZE,
                             keep_all_segments=False):
    """
    从 HEX 解析结果中提取有效数据，合并成一个紧凑的连续块。
    按 page_size 对齐总长度（填充 0x00），空洞不填充。
    
    Args:
        data: {abs_addr: byte_value}
        base_addr: 基址，用于 offset 平移计算。None=使用第一个段地址
        page_size: Flash page 对齐大小
        keep_all_segments: True=保留所有对齐段；False=只保留最大段（推荐）
    
    返回: (binary, first_seg_addr, aligned_segments)
    """
    if not data:
        return bytearray(), 0, []

    # 1. 分组连续段（空隙 <= page_size 的合并）
    segments = group_segments(data, max_gap=page_size)

    # 2. 对齐段边界
    aligned_segments = align_segments(segments, page_size)

    # 3. 如果只保留最大段（默认）
    if not keep_all_segments and len(aligned_segments) > 1:
        main_seg = max(aligned_segments, key=lambda s: s[1] - s[0])
        aligned_segments = [main_seg]
        print(f"[INFO] Multi-segment HEX detected ({len(segments)} segments)")
        print(f"[INFO] Keeping ONLY the main segment: 0x{main_seg[0]:08X} - 0x{main_seg[1]-1:08X}")
        print(f"[INFO] Use keep_all_segments=True if you need all segments")

    # 4. 提取数据为紧凑连续块
    binary = bytearray()
    for a_start, a_end in aligned_segments:
        for addr in range(a_start, a_end):
            binary.append(data.get(addr, 0x00))

    raw_len = len(binary)

    # 5. 按 page_size 对齐总长度
    if raw_len % page_size != 0:
        pad_len = page_size - (raw_len % page_size)
        binary.extend(b'\x00' * pad_len)
        print(f"[INFO] Padded {pad_len} bytes to align {page_size}B (erase state=0x00)")

    first_addr = aligned_segments[0][0] if aligned_segments else 0
    return binary, first_addr, aligned_segments


def calc_crc32(data):
    """计算 CRC32，与 Bootloader 流式 CRC 兼容（IEEE 802.3）"""
    return zlib.crc32(data) & 0xFFFFFFFF


def build_segment_header(flash_addr, data_len, reserved=0):
    """构建 16 字节段描述头
    
    flash_addr: 数据在 payload 中的 offset（用于 Bootloader 解析定位）
    data_len:   数据长度（不含 SEGM 头）
    """
    seg = bytearray(SEGMENT_HEADER_SIZE)
    seg[0:4] = SEGMENT_MAGIC
    struct.pack_into("<I", seg, 4, flash_addr)
    struct.pack_into("<I", seg, 8, data_len)
    struct.pack_into("<I", seg, 12, reserved)
    return seg


def build_header(payload_addr, payload_len, sw_version, sw_version_mask,
                 build_timestamp, payload_crc, header_crc, num_segments=1,
                 sig_algorithm=SIG_ALGORITHM_RSA2048, sig_len=SIG_LEN_RSA2048):
    """构建 128 字节 Header (PKG3 多段格式)"""
    header = bytearray(HEADER_SIZE)

    # [0:3] Magic
    header[0:4] = PKG_MAGIC
    # [4:7] Header Size
    struct.pack_into("<I", header, 4, HEADER_SIZE)
    # [8:11] Payload Address (保留兼容性，offset 模式填 0)
    struct.pack_into("<I", header, 8, payload_addr)
    # [12:15] Payload Length (含所有 SEGM 头 + 数据)
    struct.pack_into("<I", header, 12, payload_len)
    # [16:19] SW Version
    struct.pack_into("<I", header, 16, sw_version)
    # [20:23] SW Version Mask
    struct.pack_into("<I", header, 20, sw_version_mask)
    # [24:27] Build Timestamp
    struct.pack_into("<I", header, 24, build_timestamp)
    # [28:31] Payload CRC32
    struct.pack_into("<I", header, 28, payload_crc)
    # [32:35] Header CRC32 (先填 0，后面计算)
    struct.pack_into("<I", header, 32, 0)
    # [36:39] Signature Algorithm
    struct.pack_into("<I", header, 36, sig_algorithm)
    # [40:43] Signature Length
    struct.pack_into("<I", header, 40, sig_len)
    # [44:47] Number of Segments (PKG3 新增)
    struct.pack_into("<I", header, 44, num_segments)
    # [48:127] Reserved (zeros)

    # 计算 Header CRC32 (Header[0:32], 不含 CRC 字段本身)
    header_crc_value = calc_crc32(header[0:32])
    struct.pack_into("<I", header, 32, header_crc_value)

    return header, header_crc_value


def extract_binary_multisegment(data, base_addr=None, page_size=PAGE_SIZE):
    """
    从 HEX 解析结果中提取多段数据。
    
    PKG3 格式 payload 结构（连续写入 Flash，Bootloader 内部解析）：
        [data1] [data2] ... [data_n] [SEGM1] [SEGM2] ... [SEGMn]
    
    其中 SEGM 头（16 bytes）描述每段数据的位置和长度：
        - magic(4):   "SEGM"
        - offset(4):  该段数据在 payload 中的起始 offset
        - data_len(4): 该段数据长度（不含 SEGM 头）
        - reserved(4): 保留
    
    这样 Bootloader 可以连续下载完整 payload，然后从末尾解析段表，
    跳跃写入 Flash 的对应位置。
    
    Args:
        data: {abs_addr: byte_value}
        base_addr: 基址，用于 offset 平移。None=不平移
        page_size: Flash page 对齐大小
    
    返回: (payload_bytes, seg_infos, num_segments)
        payload_bytes: 含段表在内的完整 payload
        seg_infos: [(flash_addr, data_len, payload_offset), ...]
        num_segments: 段数
    """
    if not data:
        return bytearray(), [], 0

    # 1. 分组连续段（空隙 <= page_size 的合并）
    raw_segments = group_segments(data, max_gap=page_size)
    # 2. 对齐段边界
    aligned_segments = align_segments(raw_segments, page_size)

    # 3. 先收集每段的原始数据
    seg_data_list = []
    for a_start, a_end in aligned_segments:
        seg_data = bytearray()
        for addr in range(a_start, a_end):
            seg_data.append(data.get(addr, 0x00))
        
        # 按 page_size 对齐段长度
        raw_len = len(seg_data)
        if raw_len % page_size != 0:
            pad_len = page_size - (raw_len % page_size)
            seg_data.extend(b'\x00' * pad_len)
        
        seg_data_list.append((a_start, seg_data))
        print(f"[INFO]   Seg raw: addr=0x{a_start:08X}, len={len(seg_data)} bytes")

    # 4. 计算 Flash 地址（offset 平移）
    seg_infos = []
    for i, (a_start, seg_data) in enumerate(seg_data_list):
        flash_addr = a_start
        if base_addr is not None:
            flash_addr = base_addr + (a_start - aligned_segments[0][0])
        seg_infos.append((flash_addr, len(seg_data)))

    # 5. 构建 payload：data 在前，段表在后
    # 先算 data 部分总长度
    data_total_len = sum(len(sd) for _, sd in seg_data_list)
    # 段表起始 offset = data 总长度
    seg_table_offset = data_total_len

    payload = bytearray()
    
    # 5a. 写入所有 data
    for _, seg_data in seg_data_list:
        payload.extend(seg_data)
    
    # 5b. 写入段表（SEGM 头数组）
    current_data_offset = 0
    for i, (flash_addr, seg_data) in enumerate(seg_data_list):
        data_len = len(seg_data)
        # offset = 该段数据在 payload 中的起始位置
        seg_header = build_segment_header(current_data_offset, data_len)
        payload.extend(seg_header)
        # 更新 seg_infos 加入 payload_offset
        seg_infos[i] = (flash_addr, data_len, current_data_offset)
        current_data_offset += data_len
        print(f"[INFO]   Seg table: flash=0x{flash_addr:08X}, len={data_len}, payload_offset={seg_infos[i][2]}")

    return payload, seg_infos, len(seg_infos)


def sign_payload(private_key_path, header, payload):
    """用 RSA 私钥对 Payload 的 SHA-256 进行签名"""
    with open(private_key_path, "rb") as key_file:
        private_key = serialization.load_pem_private_key(key_file.read(), password=None)

    data_to_sign = payload

    signature = private_key.sign(
        data_to_sign,
        padding.PKCS1v15(),
        hashes.SHA256()
    )

    return signature


def generate_pkg(hex_path, payload_addr, sw_version, sw_version_mask,
                 private_key_path, out_path, build_timestamp=None,
                 base_addr=None, keep_all_segments=False,
                 write_aligned_hex=False, write_bin=False):
    """
    主函数：HEX → .pkg (PKG3 多段格式)
    
    Args:
        hex_path: 输入 HEX 文件路径
        payload_addr: 写入 .pkg Header 的 PayloadAddr 字段（offset 模式建议填 0）
        sw_version: 软件版本号
        sw_version_mask: 软件版本掩码
        private_key_path: RSA 私钥路径
        out_path: 输出 .pkg 路径
        build_timestamp: 构建时间戳（None=自动）
        base_addr: 基址（如 0x80020000），用于 offset 平移。None=不平移
        keep_all_segments: 保留（为兼容性，现在总是保留所有段）
        write_aligned_hex: True=生成 .aligned.hex 中间文件
        write_bin: True=生成 .bin 中间文件
    """

    if not os.path.exists(hex_path):
        print(f"[ERROR] HEX file not found: {hex_path}")
        sys.exit(1)

    if not os.path.exists(private_key_path):
        print(f"[ERROR] Private key not found: {private_key_path}")
        sys.exit(1)

    # 1. 解析 HEX
    print(f"[INFO] Parsing HEX: {hex_path}")
    data, hex_start_addr = parse_hex_file(hex_path)
    if not data:
        print("[ERROR] No data found in HEX file")
        sys.exit(1)

    # 2. 提取多段 Binary（PKG3 格式：每段前加 SEGM 头）
    payload, seg_infos, num_segments = extract_binary_multisegment(
        data, base_addr=base_addr, page_size=PAGE_SIZE
    )
    payload_len = len(payload)

    print(f"[INFO] Segments: {num_segments}")
    total_raw = sum(dlen for _, dlen in seg_infos)
    print(f"[INFO] Raw data: {total_raw} bytes, Payload with headers: {payload_len} bytes")
    if base_addr is not None:
        print(f"[INFO] Base addr: 0x{base_addr:08X}, Offset mode")
    else:
        print(f"[INFO] First seg addr: 0x{seg_infos[0][0]:08X}")

    # 3. 可选：生成对齐后的 HEX 文件
    if write_aligned_hex:
        # 提取原始对齐段用于 HEX 输出
        raw_segments = group_segments(data, max_gap=PAGE_SIZE)
        aligned_segments = align_segments(raw_segments, PAGE_SIZE)
        hex_out = os.path.splitext(out_path)[0] + ".aligned.hex"
        write_intel_hex(data, aligned_segments, hex_out, page_size=PAGE_SIZE)
        print(f"[INFO] Aligned HEX:   {hex_out}")

    # 4. 可选：生成 BIN 文件
    if write_bin:
        bin_out = os.path.splitext(out_path)[0] + ".bin"
        with open(bin_out, "wb") as f:
            f.write(payload)
        print(f"[INFO] BIN file:      {bin_out}")

    # 5. 计算 Payload CRC（只对 data 部分，不含 SEGM 段表）
    data_only = payload[0:data_total_len]
    payload_crc = calc_crc32(data_only)
    print(f"[INFO] Payload CRC32: 0x{payload_crc:08X} (over {data_total_len} bytes of data)")

    # 6. 构建 Header
    if build_timestamp is None:
        import time
        build_timestamp = int(time.time())

    header, header_crc = build_header(
        payload_addr=payload_addr,
        payload_len=payload_len,
        sw_version=sw_version,
        sw_version_mask=sw_version_mask,
        build_timestamp=build_timestamp,
        payload_crc=payload_crc,
        header_crc=0,  # 内部计算
        num_segments=num_segments
    )
    print(f"[INFO] Header CRC32:  0x{header_crc:08X}")

    # 7. 签名（只对 data 部分做 SHA-256 + RSA，不含 SEGM 段表）
    print(f"[INFO] Signing with:  {private_key_path}")
    signature = sign_payload(private_key_path, header, data_only)
    print(f"[INFO] Signature:     {len(signature)} bytes")

    if len(signature) != SIG_LEN_RSA2048:
        print(f"[WARN] Signature length {len(signature)} != expected {SIG_LEN_RSA2048}")

    # 8. 组装 .pkg
    pkg = header + payload + signature

    with open(out_path, "wb") as f:
        f.write(pkg)

    print(f"[INFO] Output:        {out_path}")
    print(f"[INFO]   Total size:  {len(pkg)} bytes")
    print(f"[INFO]   Header:      {HEADER_SIZE} bytes")
    print(f"[INFO]   Payload:     {payload_len} bytes ({num_segments} segments with SEGM headers)")
    print(f"[INFO]   Signature:   {len(signature)} bytes")
    print("[INFO] Done.")


def generate_key_pair(out_dir):
    """生成 RSA-2048 密钥对（用于首次配置）"""
    from cryptography.hazmat.primitives.asymmetric import rsa

    private_key = rsa.generate_private_key(
        public_exponent=65537,
        key_size=2048,
    )

    priv_path = os.path.join(out_dir, "private.pem")
    pub_path = os.path.join(out_dir, "public.pem")

    with open(priv_path, "wb") as f:
        f.write(private_key.private_bytes(
            encoding=serialization.Encoding.PEM,
            format=serialization.PrivateFormat.TraditionalOpenSSL,
            encryption_algorithm=serialization.NoEncryption()
        ))

    with open(pub_path, "wb") as f:
        f.write(private_key.public_key().public_bytes(
            encoding=serialization.Encoding.PEM,
            format=serialization.PublicFormat.SubjectPublicKeyInfo
        ))

    print(f"[INFO] Generated RSA-2048 key pair:")
    print(f"       Private: {priv_path}")
    print(f"       Public:  {pub_path}")
    return priv_path, pub_path


def extract_public_key_for_c(pub_path, out_c_path):
    """从 PEM 公钥提取 modulus + exponent，生成 C 头文件"""
    with open(pub_path, "rb") as f:
        public_key = serialization.load_pem_public_key(f.read())

    from cryptography.hazmat.primitives.asymmetric import rsa
    if not isinstance(public_key, rsa.RSAPublicKey):
        print("[ERROR] Only RSA public keys are supported")
        sys.exit(1)

    pub_num = public_key.public_numbers()
    modulus = pub_num.n
    exponent = pub_num.e

    # 将 modulus 拆分为 256 字节（2048 位）大端数组
    mod_bytes = modulus.to_bytes(256, byteorder='big')
    exp_bytes = exponent.to_bytes(4, byteorder='big')

    lines = [
        "/* Auto-generated public key header for RSA-2048 verification */",
        "#ifndef CRYPTO_PUBLIC_KEY_H_",
        "#define CRYPTO_PUBLIC_KEY_H_",
        "",
        "#define RSA_MODULUS_LEN   (256u)",
        "#define RSA_EXPONENT_LEN  (4u)",
        "",
        "static const uint8 rsa_public_modulus[RSA_MODULUS_LEN] =",
        "{",
    ]

    for i in range(0, 256, 8):
        row = "    " + ", ".join(f"0x{mod_bytes[j]:02X}" for j in range(i, min(i+8, 256)))
        lines.append(row + ",")
    lines[-1] = lines[-1].rstrip(",")  # 去掉最后一行的逗号
    lines.append("};")
    lines.append("")
    lines.append("static const uint8 rsa_public_exponent[RSA_EXPONENT_LEN] =")
    lines.append("{")
    lines.append("    " + ", ".join(f"0x{exp_bytes[j]:02X}" for j in range(4)))
    lines.append("};")
    lines.append("")
    lines.append("#endif /* CRYPTO_PUBLIC_KEY_H_ */")
    lines.append("")

    with open(out_c_path, "w") as f:
        f.write("\n".join(lines))

    print(f"[INFO] Generated C public key header: {out_c_path}")


def verify_pkg(pkg_path, public_key_path):
    """验证 .pkg 文件的签名（用于测试），支持 PKG3 多段格式"""
    with open(public_key_path, "rb") as f:
        public_key = serialization.load_pem_public_key(f.read())

    with open(pkg_path, "rb") as f:
        pkg = f.read()

    header = pkg[0:HEADER_SIZE]
    signature = pkg[-SIG_LEN_RSA2048:]
    payload = pkg[HEADER_SIZE:-SIG_LEN_RSA2048]

    magic = header[0:4]
    num_segments = struct.unpack_from("<I", header, 44)[0]

    print(f"[verify] Magic: {magic}, Segments: {num_segments}")

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

    # 如果是 PKG3 多段格式，解析并显示段信息
    if magic == b"PKG3" and num_segments > 0:
        print(f"[verify] --- Multi-segment payload ---")
        offset = 0
        for i in range(num_segments):
            if offset + SEGMENT_HEADER_SIZE > len(payload):
                print(f"[FAIL] Segment {i+1}: truncated at offset {offset}")
                break
            seg_magic = payload[offset:offset+4]
            seg_addr = struct.unpack_from("<I", payload, offset+4)[0]
            seg_len = struct.unpack_from("<I", payload, offset+8)[0]
            print(f"[verify]   Seg {i+1}: magic={seg_magic}, addr=0x{seg_addr:08X}, len={seg_len}")
            if seg_magic != SEGMENT_MAGIC:
                print(f"[WARN]   Seg {i+1}: magic mismatch, expected {SEGMENT_MAGIC}")
            offset += SEGMENT_HEADER_SIZE + seg_len

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





