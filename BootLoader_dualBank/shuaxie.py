# -*- coding: utf-8 -*-
from ZXDoc import *
import time
import os
import struct
import zlib
import sys
# ============================================================================
# BootLoader20250714_UDS_tasking622 - ZXDoc 完整刷写脚本
# ============================================================================
# 适用场景: 基于 TC234 的 A/B 双区 Bootloader 刷写
# 刷写流程对照 ZxDoc 配置截图:
#   1. 10 01      -> 默认会话
#   2. 10 03      -> 扩展会话
#   3. 27 01/02   -> 安全访问 Level 1
#   4. 31 01 FF FD-> 检查编程条件
#   5. 85 02      -> 关闭 DTC
#   6. 10 02      -> 编程会话
#   7. 27 01/02   -> 安全访问 Level 2 (编程会话必须)
#   8. 2E F1 5A...-> 写指纹信息
#   9. 31 01 FF 00-> 逐个擦除目标 Bank sector
#  10. 34/36/37   -> 文件下载 (Payload + Signature)
#  11. 31 01 DFFF -> 验证并激活 Bank (CRC only, Sig read from PFlash)
#  12. 11 01      -> ECU 复位
# ============================================================================

# ========== 用户配置区（请根据实际情况修改） ==========

# CAN 诊断 ID
PHY_ADDR = 0x74C           # ECU 物理请求地址 (RX)
TESTER_ADDR = 0x75C        # Tester 响应地址 (TX)
FUNC_ADDR = 0x7DF          # 功能地址（会话保持用）
CHANNEL = 1                # CAN 通道号

# 刷写文件路径（.pkg 容器格式，绝对路径）
# 只需要一个 .pkg，Bootloader 自动选择对区写入
PKG_FILE = r"E:\workFiles\IEBS\tc234bootloader\tc234bootloader\App_dualBank.pkg"

# 安全访问 DLL 路径（绝对路径，用于 27 服务 Seed->Key 计算）
KEY_DLL = r"E:\visualStudioCode\ZcanProDll\Debug\ZcanProDll.dll"

# 刷写目标 Bank: 由 Bootloader 通过 31 FF FD 动态返回，无需手动配置
# Bank A: 0x80020000 ~ 0x800FFFFF (S8~S22, 896KB)
# Bank B: 0x80100000 ~ 0x801FFFFF (S23~S26, 1MB)
TARGET_BANK = "B"

# ========== Bank 地址与大小（与 Bootloader LSL 保持一致） ==========
BANK_A_START_ADDR = 0x80020000
BANK_B_START_ADDR = 0x80100000
BANK_APP_A_SIZE   = 896 * 1024   # 0x000E0000
BANK_APP_B_SIZE   = 1024 * 1024  # 0x00100000

# Bank 对应的 sector 列表（对应 IfxFlash_pFlashTableLog 索引）
BANK_SECTORS = {
    "A": list(range(8, 23)),    # S8 ~ S22
    "B": list(range(23, 27)),   # S23 ~ S26
}

# 全局变量：记录手动擦除成功的 sector 列表，供 file_download 前置检查使用
_g_erased_sectors = []

TOTAL_CHECK_CMD = bytes.fromhex("31 01 DF FF")

# .pkg 容器 Header 大小
PKG_HEADER_SIZE = 128
SIG_LEN_RSA2048 = 256

def uds_request(uds, sid, data, desc="UDS"):
    """
    发送 UDS 请求并自动检查响应状态。
    :return: ZUdsResponse 对象（成功时）；None（失败或超时）
    """
    req = ZUdsRequest(
        src_addr=PHY_ADDR,
        dst_addr=TESTER_ADDR,
        is_extend=False,
        suppress_response=False,
        sid=sid,
        data=data,
    )
    rsp = uds.request(req)
    if not rsp:
        app.log_e(f"[{desc}] ❌ No response (timeout)")
        return None
    if rsp.status != UDS_RSP_STATUS_OK:
        app.log_e(f"[{desc}] ❌ NRC 0x{rsp.status:02X}: {uds.get_error_message(rsp)}")
        return None
    app.log_i(f"[{desc}] ✅ OK, rsp: {rsp.data.hex()}")
    return rsp


def parse_pkg_file(pkg_path):
    """
    解析 .pkg 容器文件（支持 PKG2 单段 和 PKG3 多段）。
    返回: (payload_addr, payload_len, payload_bytes, signature_bytes, sw_version, payload_crc, segments)
        segments: 段信息列表 [(flash_addr, data_bytes), ...]，仅 PKG3 有效；PKG2 时为 []
    """
    with open(pkg_path, "rb") as f:
        pkg = f.read()

    if len(pkg) < PKG_HEADER_SIZE + SIG_LEN_RSA2048:
        raise ValueError(f".pkg file too small: {len(pkg)} bytes")

    header = pkg[0:PKG_HEADER_SIZE]

    # Magic
    magic = header[0:4]
    if magic not in (b"PKG2", b"PKG3"):
        raise ValueError(f"Invalid .pkg magic: {magic!r}, expected b'PKG2' or b'PKG3'")

    # Header fields (little-endian)
    header_size = struct.unpack_from("<I", header, 4)[0]
    payload_addr = struct.unpack_from("<I", header, 8)[0]
    payload_len = struct.unpack_from("<I", header, 12)[0]
    sw_version = struct.unpack_from("<I", header, 16)[0]
    sw_version_mask = struct.unpack_from("<I", header, 20)[0]
    build_timestamp = struct.unpack_from("<I", header, 24)[0]
    payload_crc = struct.unpack_from("<I", header, 28)[0]
    header_crc = struct.unpack_from("<I", header, 32)[0]
    sig_algorithm = struct.unpack_from("<I", header, 36)[0]
    sig_len = struct.unpack_from("<I", header, 40)[0]
    num_segments = struct.unpack_from("<I", header, 44)[0] if magic == b"PKG3" else 1

    # Verify header CRC (header[0:32], excludes CRC field itself)
    header_crc_calc = zlib.crc32(header[0:32]) & 0xFFFFFFFF
    if header_crc != header_crc_calc:
        raise ValueError(f"Header CRC mismatch: file=0x{header_crc:08X}, calc=0x{header_crc_calc:08X}")

    # Extract payload and signature
    payload = pkg[PKG_HEADER_SIZE : PKG_HEADER_SIZE + payload_len]
    signature = pkg[PKG_HEADER_SIZE + payload_len : PKG_HEADER_SIZE + payload_len + sig_len]

    if len(payload) != payload_len:
        raise ValueError(f"Payload length mismatch: header says {payload_len}, actual {len(payload)}")

    # Verify payload CRC
    payload_crc_calc = zlib.crc32(payload) & 0xFFFFFFFF
    if payload_crc != payload_crc_calc:
        raise ValueError(f"Payload CRC mismatch: header=0x{payload_crc:08X}, calc=0x{payload_crc_calc:08X}")

    print(f"[parse_pkg] Magic={magic!r}, HeaderSize={header_size}")
    print(f"[parse_pkg] PayloadAddr=0x{payload_addr:08X}, PayloadLen={payload_len}")
    print(f"[parse_pkg] SWVersion=0x{sw_version:08X}, BuildTime={build_timestamp}")
    print(f"[parse_pkg] PayloadCRC=0x{payload_crc:08X}, HeaderCRC=0x{header_crc:08X}")
    print(f"[parse_pkg] SigAlgorithm={sig_algorithm}, SigLen={sig_len}")
    print(f"[parse_pkg] NumSegments={num_segments}")
    print(f"[parse_pkg] Signature={len(signature)} bytes")

    # Parse segments for PKG3 (new format: data first, segment table at end)
    segments = []
    if magic == b"PKG3" and num_segments > 0:
        seg_table_size = num_segments * 16
        if payload_len < seg_table_size:
            raise ValueError(f"Payload too small for segment table: {payload_len} < {seg_table_size}")
        data_total_len = payload_len - seg_table_size
        seg_table_offset = data_total_len
        
        print(f"[parse_pkg]   Data total: {data_total_len} bytes, Seg table: {seg_table_size} bytes at offset {seg_table_offset}")
        
        for i in range(num_segments):
            seg_entry_offset = seg_table_offset + i * 16
            seg_magic = payload[seg_entry_offset:seg_entry_offset+4]
            data_offset = struct.unpack_from("<I", payload, seg_entry_offset+4)[0]
            data_len = struct.unpack_from("<I", payload, seg_entry_offset+8)[0]
            print(f"[parse_pkg]   Seg {i+1}: magic={seg_magic!r}, data_offset={data_offset}, data_len={data_len}")
            
            if seg_magic != b"SEGM":
                print(f"[parse_pkg]   WARN: seg magic mismatch: {seg_magic!r}")
            
            if data_offset + data_len > data_total_len:
                raise ValueError(f"Segment {i+1}: data range [{data_offset}:{data_offset+data_len}] exceeds data total {data_total_len}")
            
            seg_data = payload[data_offset : data_offset + data_len]
            
            # flash_addr = base_addr + data_offset (base_addr from header payload_addr)
            # For offset mode, payload_addr=0, use 0x80020000 as default base
            base_addr = payload_addr if payload_addr != 0 else 0x80020000
            flash_addr = base_addr + data_offset
            segments.append((flash_addr, seg_data))
            print(f"[parse_pkg]     -> flash_addr=0x{flash_addr:08X}, len={len(seg_data)}")

    return payload_addr, payload_len, payload, signature, sw_version, payload_crc, segments


def session_control(uds, session_type, desc):
    """10 服务：诊断会话控制"""
    return uds_request(uds, 0x10, [session_type], desc) is not None


def security_access(uds, level, desc):
    """
    27 服务：安全访问。
    Level 1: 扩展会话解锁；Level 2: 编程会话解锁（必须）。
    """
    sa_req = ZSecurityAccessReq(
        keyDllPath=KEY_DLL,
        srcAddr=PHY_ADDR,
        dstAddr=TESTER_ADDR,
        securityLevel=level,
        isExtend=False,
    )
    if uds.security_access(sa_req):
        app.log_i(f"[{desc}] ✅ Security Level {level} passed")
        return True
    else:
        app.log_e(f"[{desc}] ❌ Security Level {level} failed")
        return False


def erase_target_bank(uds):
    """
    31 01 FF 00: 根据 TARGET_BANK 配置，逐个擦除目标 Bank 的所有 sector。
    由于 BootLoader 里每个 31 01 FF 00 只擦除单个 sector，
    因此需要循环发送（S8~S22 共 15 次，S23~S26 共 4 次）。
    """
    global _g_erased_sectors
    _g_erased_sectors.clear()

    sectors = BANK_SECTORS.get(TARGET_BANK, BANK_SECTORS[TARGET_BANK])
    app.log_i(f"[Erase] Start erasing Bank {TARGET_BANK}, sectors: {sectors}")

    for sec in sectors:
        high = (sec >> 8) & 0xFF
        low = sec & 0xFF
        rsp = uds_request(
            uds, 0x31,
            [0x01, 0xFF, 0x00, high, low],
            f"Erase S{sec}",
        )
        if not rsp:
            app.log_e(f"[Erase] Bank {TARGET_BANK} aborted at S{sec}")
            return False

        # 校验正响应: ZxDoc rsp.data 不包含 SID(0x71)，格式为:
        #   byte0: 0x01 (routineControlType)
        #   byte1: 0xFF (RID high)
        #   byte2: 0x00 (RID low)
        #   byte3: sector num
        #   byte4: result (0x01 = success)
        if len(rsp.data) >= 5 and rsp.data[0] == 0x01:
            result = rsp.data[4]
            if result == 0x01:
                _g_erased_sectors.append(sec)
                app.log_i(f"[Erase] S{sec} done, result=0x{result:02X}")
            else:
                app.log_e(f"[Erase] S{sec} returned error result=0x{result:02X}")
                return False
        else:
            app.log_w(f"[Erase] S{sec} unexpected rsp: {rsp.data.hex()}")

        # 延时 50ms，避免 CAN 总线报文过于密集
        time.sleep(0.05)

    app.log_i(f"[Erase] Bank {TARGET_BANK} all sectors erased successfully ({len(_g_erased_sectors)}/{len(sectors)})")
    return True


def _build_34_request(addr, length):
    """构建 0x34 RequestDownload 请求数据 (addr=4B, len=4B)."""
    addr_bytes = [(addr >> 24) & 0xFF, (addr >> 16) & 0xFF, (addr >> 8) & 0xFF, addr & 0xFF]
    len_bytes = [(length >> 24) & 0xFF, (length >> 16) & 0xFF, (length >> 8) & 0xFF, length & 0xFF]
    return [0x00, 0x44] + addr_bytes + len_bytes


def _download_single_segment(uds, seg_offset, seg_data, desc="Seg"):
    """
    下载单段数据：0x34 -> 0x36(blocks) -> 0x37.
    seg_offset: 段在目标 bank 中的偏移地址.
    seg_data:   段数据 bytes.
    返回 True/False.
    """
    # 1. RequestDownload (0x34)
    req_34 = _build_34_request(seg_offset, len(seg_data))
    rsp = uds_request(uds, 0x34, req_34, f"{desc} 0x34")
    if not rsp:
        app.log_e(f"[{desc}] 0x34 failed, abort segment")
        return False

    # Parse max block size from 0x74 response (rsp.data[1:3] = maxNumberOfBlockLength)
    max_block = 0x80  # default 128 bytes
    if len(rsp.data) >= 3:
        max_block = (rsp.data[1] << 8) | rsp.data[2]
        # Bootloader buf size = 300, actual data = max_block - 2 (SID+SN)
        max_block = min(max_block - 2, 256)  # cap at 256 for safety
    app.log_i(f"[{desc}] Max block size: {max_block} bytes/frame")

    # 2. TransferData (0x36) - send data in blocks
    seq_num = 1
    total_sent = 0
    for i in range(0, len(seg_data), max_block):
        block = seg_data[i:i + max_block]
        block_with_sn = bytes([seq_num]) + block
        rsp = uds_request(uds, 0x36, list(block_with_sn), f"{desc} 0x36[{seq_num}]")
        if not rsp:
            app.log_e(f"[{desc}] 0x36 block {seq_num} failed, abort segment")
            return False
        seq_num = (seq_num + 1) & 0xFF
        if seq_num == 0:
            seq_num = 1
        total_sent += len(block)
        if total_sent % 1024 == 0 or total_sent == len(seg_data):
            app.log_i(f"[{desc}] Progress: {total_sent}/{len(seg_data)} bytes")

    # 3. RequestTransferExit (0x37)
    rsp = uds_request(uds, 0x37, [], f"{desc} 0x37")
    if not rsp:
        app.log_e(f"[{desc}] 0x37 failed, abort segment")
        return False

    app.log_i(f"[{desc}] ✅ Segment complete: {len(seg_data)} bytes at offset 0x{seg_offset:08X}")
    return True


def file_download(uds, pkg_path):
    """
    多段文件下载（PKG3 格式）。
    每段独立 0x34/0x36/0x37，最后一段为签名（addr=0, len=256）。
    签名段下载后发送 0x31 DFFF 进行 CRC+签名验证。
    """
    expected_sectors = set(BANK_SECTORS.get(TARGET_BANK, BANK_SECTORS["A"]))
    actual_sectors = set(_g_erased_sectors)
    missing = expected_sectors - actual_sectors
    if missing:
        app.log_e(
            f"[Download] ❌ Erase incomplete! Missing sectors: {sorted(missing)}. "
            f"Expected {len(expected_sectors)}, actually erased {len(actual_sectors)}. "
            f"Abort file download to prevent writing to un-erased flash."
        )
        return False
    else:
        app.log_i(
            f"[Download] ✅ Erase check passed: {len(actual_sectors)}/{len(expected_sectors)} sectors erased. "
            f"Proceeding to multi-segment download."
        )

    if not os.path.exists(pkg_path):
        app.log_e(f"[Download] ❌ File not found: {pkg_path}")
        return False

    # 解析 .pkg
    try:
        payload_addr, payload_len, payload_bytes, signature_bytes, sw_version, payload_crc, segments = parse_pkg_file(pkg_path)
    except Exception as e:
        app.log_e(f"[Download] ❌ Failed to parse .pkg: {e}")
        return False

    # 判断格式
    if not segments:
        # PKG2 fallback: use ZXDoc built-in file_download
        app.log_w("[Download] PKG2 detected, using legacy single-segment mode")
        tmp_bin = pkg_path + ".payload.bin"
        combined = payload_bytes + signature_bytes
        with open(tmp_bin, "wb") as f:
            f.write(combined)
        block_cfgs = [FlashDataBlockCfg(
            startAddr=0,
            dataLen=len(combined),
            crc=payload_crc,
            fillByte=0x00,
            mappedAddr=payload_addr,
        )]
        dl_req = ZFileDownloadReq(
            filePath=tmp_bin,
            memEraseType=ZMemEraseType.NoErase,
            srcAddr=PHY_ADDR,
            dstAddr=TESTER_ADDR,
            fileBlockCfgs=block_cfgs,
            crcAlgorithm=ZCrcAlgorithm(
                type=ZCrcType.CRC32,
                polynomial=0x04C11DB7,
                initValue=0xFFFFFFFF,
                xorOutput=0xFFFFFFFF,
                reflectInput=True,
                reflectOutput=True,
            ),
            totalCheckCmd=None,
        )
        return uds.file_download(dl_req)

    # PKG3 multi-segment mode
    app.log_i(f"[Download] PKG3 multi-segment mode: {len(segments)} data segments + 1 signature segment")

    # Base address for offset calculation
    base_addr = payload_addr if payload_addr != 0 else 0x80020000
    app.log_i(f"[Download] Base addr: 0x{base_addr:08X}")

    # Download each data segment
    for idx, (flash_addr, seg_data) in enumerate(segments):
        seg_offset = flash_addr - base_addr
        app.log_i(f"[Download] --- Segment {idx+1}/{len(segments)}: flash=0x{flash_addr:08X}, offset=0x{seg_offset:08X}, len={len(seg_data)} ---")
        if not _download_single_segment(uds, seg_offset, seg_data, f"Seg{idx+1}"):
            app.log_e(f"[Download] ❌ Segment {idx+1} failed, abort download")
            return False

    # Download signature segment (addr=0, len=256)
    app.log_i(f"[Download] --- Signature segment: offset=0x00000000, len={len(signature_bytes)} ---")
    if not _download_single_segment(uds, 0, signature_bytes, "Signature"):
        app.log_e("[Download] ❌ Signature segment failed, abort download")
        return False

    # CRC + Signature verification (0x31 01 DFFF)
    app.log_i("[Download] --- Verifying CRC + Signature (0x31 DFFF) ---")
    crc_be = [(payload_crc >> 24) & 0xFF, (payload_crc >> 16) & 0xFF,
              (payload_crc >> 8) & 0xFF, payload_crc & 0xFF]
    rsp = uds_request(uds, 0x31, [0x01, 0xDF, 0xFF] + crc_be, "Verify CRC+Sig")
    if not rsp:
        app.log_e("[Download] ❌ Verification request failed")
        return False

    if len(rsp.data) >= 5 and rsp.data[0] == 0x01:
        result = rsp.data[4]
        if result == 0x00:
            app.log_i("[Download] ✅ Verification passed! Bank marked valid.")
            return True
        elif result == 0x01:
            app.log_e("[Download] ❌ CRC mismatch!")
        elif result == 0x02:
            app.log_e("[Download] ❌ Signature verification failed!")
        else:
            app.log_e(f"[Download] ❌ Unknown verification result: 0x{result:02X}")
    else:
        app.log_w(f"[Download] ⚠️ Unexpected verification response: {rsp.data.hex()}")

    return False





def do_flash_process(pkg_path=None):
    """完整刷写主流程，返回 True/False"""
    actual_pkg = pkg_path if pkg_path else PKG_FILE
    
    uds = ZDoCanInterface(
        channelIndex=CHANNEL,
        cfg=ZUdsCANCfg(
            frameType=CAN_FRAME_TYPE_CAN,
            protocolVersion=CAN_TP_ISO15765_2_2016,
            fillByte=0xAA,
            isFillByte=True,
            p2Timeout=2000,       # P2 超时 2s（单帧响应）
            p2xTimeout=5000,      # P2* 超时 5s（多帧/长操作）
            isReplaceEcuSTmin=False,
            remoteSTmin=0,
            localSTmin=0,
            blockSize=0,          # 0 = 不限制 BlockSize
            fcTimeout=1000,
        ),
    )

    if uds.handle() < 0:
        app.log_e("[Init] ❌ Failed to create UDS interface")
        return False

    # 启动会话保持（功能地址 0x7DF，周期 5000ms）
    uds.start_session_keep(FUNC_ADDR, 5000, True)

    flash_ok = False
    try:
        # ------------------------------------------------------------
        # 1. 默认会话 10 01
        # ------------------------------------------------------------
        if not session_control(uds, 0x01, "Default Session"):
            return False

        # ------------------------------------------------------------
        # 2. 扩展会话 10 03
        # ------------------------------------------------------------
        if not session_control(uds, 0x03, "Extended Session"):
            return False

        # ------------------------------------------------------------
        # 3. 安全访问 Level 1（扩展会话下解锁）
        # ------------------------------------------------------------
        if not security_access(uds, 1, "SA L1"):
            return False

        # ------------------------------------------------------------
        # 4. 检查编程条件 31 01 DF FD
        #    正响应: 71 01 FF FD <canFlash> <targetBank>
        #      canFlash  : 1=能刷写, 0=不能
        #      targetBank: 'A' 或 'B'（Bootloader 自动选择的对区）
        #    只需要一个 .pkg，targetBank 仅用于确定擦除哪个 Bank 的 sector
        # ------------------------------------------------------------
        global TARGET_BANK
        rsp = uds_request(uds, 0x31, [0x01, 0xFF, 0xFD], "Check Programming")
        if not rsp:
            return False
        # ZXDoc rsp.data 不含 SID，格式: [01 subFunc, FF RID_H, FD RID_L, canFlash, targetBank]
        pkg_path = actual_pkg
        if len(rsp.data) >= 5:
            can_flash = rsp.data[3]
            target_bank_char = rsp.data[4]
            app.log_i(f"[Check] Bootloader reports: canFlash={can_flash}, targetBank={target_bank_char}")
            if can_flash != 1:
                app.log_e(f"[Check] ❌ Programming conditions NOT OK (canFlash={can_flash}), abort flash!")
                return False
            if target_bank_char == 0x0A:
                TARGET_BANK = 'A'
                app.log_i(f"[Check] ✅ Bootloader selected Bank A (opposite of current)")
            elif target_bank_char == 0x0B:
                TARGET_BANK = 'B'
                app.log_i(f"[Check] ✅ Bootloader selected Bank B (opposite of current)")
            else:
                app.log_w(f"[Check] ⚠️ Unexpected targetBank char=0x{rsp.data[4]:02X}, keep default={TARGET_BANK}")
        else:
            app.log_w(f"[Check] ⚠️ Short response, keep default target bank={TARGET_BANK}")

        if not os.path.exists(pkg_path):
            app.log_e(f"[Check] ❌ PKG file not found: {pkg_path}")
            return False

        # ------------------------------------------------------------
        # 5. 关闭通信 28 03 03 (Disable Rx/Tx)
        #    减少刷写过程中总线负载，防止应用报文干扰
        # ------------------------------------------------------------
        if not uds_request(uds, 0x28, [0x03, 0x03], "Disable Communication"):
            return False

        # ------------------------------------------------------------
        # 6. 关闭 DTC 85 02
        # ------------------------------------------------------------
        if not uds_request(uds, 0x85, [0x02], "Disable DTC"):
            return False

        # ------------------------------------------------------------
        # 7. 编程会话 10 02
        # ------------------------------------------------------------
        if not session_control(uds, 0x02, "Programming Session"):
            return False

        # ------------------------------------------------------------
        # 7. 安全访问 Level 2（编程会话必须解锁 Level 2）
        #    BootLoader 中 31 01 FF 00 / 31 01 DFFF 都要求 SECURITY_LEVEL_2
        # ------------------------------------------------------------
        if not security_access(uds, 3, "SA L2"):
            return False

        # ------------------------------------------------------------
        # 8. 预编程条件检查 31 01 02 03 (ISO 14229-1 标准 RID)
        #    正响应: 71 01 02 03 <canFlash> <targetBank>
        # ------------------------------------------------------------
        rsp = uds_request(uds, 0x31, [0x01, 0x02, 0x03], "Check Preconditions")
        if not rsp:
            return False
        if len(rsp.data) >= 5:
            can_flash = rsp.data[3]
            if can_flash != 1:
                app.log_e(f"[Precond] ❌ Preconditions NOT OK (canFlash={can_flash}), abort!")
                return False
            app.log_i("[Precond] ✅ Preconditions OK")

        # ------------------------------------------------------------
        # 9. 写指纹信息 2E F1 5A 55 55
        #    如需修改指纹内容，请调整 data 字段
        # ------------------------------------------------------------
        if not uds_request(uds, 0x2E, [0xF1, 0x5A, 0x55, 0x55], "Write Fingerprint"):
            return False

        # ------------------------------------------------------------
        # 10. 擦除目标 Bank (31 01 FF 00)
        #    逐个 sector 擦除，每次只擦 1 个 sector，单次耗时 < 1s，
        #    不会触发 P2 超时。
        # ------------------------------------------------------------
        if not erase_target_bank(uds):
            app.log_e("[Flash] Erase failed, abort download!")
            return False

        # ------------------------------------------------------------
        # 11. 文件下载 (34/36/37，只发送 Payload)
        # ------------------------------------------------------------
        if not file_download(uds, pkg_path):
            return False

        # ------------------------------------------------------------
        # 12. 检查编程依赖性 31 01 DFFF
        #     31 DFFF 只传 CRC(4B)，Bootloader 从 PFlash 末尾读 Sig 验证。
        # ------------------------------------------------------------
        try:
            _, payload_len, _, _, _, payload_crc, _ = parse_pkg_file(pkg_path)
            # 31 01 DF FF + CRC(4B, big-endian)
            # PayloadLen is not needed: Bootloader knows it from 0x34 DataLen - SigLen.
            dfff_data = bytes([0x01, 0xDF, 0xFF])
            dfff_data += bytes([
                (payload_crc >> 24) & 0xFF,
                (payload_crc >> 16) & 0xFF,
                (payload_crc >> 8) & 0xFF,
                payload_crc & 0xFF,
            ])
            rsp = uds_request(uds, 0x31, list(dfff_data), "Verify & Activate")
            if not rsp:
                app.log_e("[Verify] ❌ No response from 31 DFFF")
                return False
            if len(rsp.data) >= 4 and rsp.data[3] == 0x00:
                app.log_i("[Verify] ✅ Signature and CRC verified, bank activated")
            else:
                app.log_e("[Verify] ❌ Signature/CRC verification failed!")
                return False
        except Exception as e:
            app.log_e(f"[Verify] ❌ Failed to construct 31 DFFF: {e}")
            return False
        # ------------------------------------------------------------
        # 13. 后编程阶段 - 恢复系统正常工作状态
        # ------------------------------------------------------------
        app.log_i("[Post] ====== Post-Programming Phase ======")

        # 13.1 切换到扩展会话 10 03
        if not session_control(uds, 0x03, "Extended Session (Post)"):
            app.log_w("[Post] ⚠️ Failed to enter Extended Session, continue...")

        # 13.2 恢复通信 28 00 03 (Enable Rx/Tx)
        if not uds_request(uds, 0x28, [0x00, 0x03], "Enable Communication"):
            app.log_w("[Post] ⚠️ Failed to enable communication, continue...")

        # 13.3 恢复 DTC 85 01
        if not uds_request(uds, 0x85, [0x01], "Enable DTC"):
            app.log_w("[Post] ⚠️ Failed to enable DTC, continue...")

        # 13.4 清除所有 DTC 14 FF FF FF
        if not uds_request(uds, 0x14, [0xFF, 0xFF, 0xFF], "Clear All DTC"):
            app.log_w("[Post] ⚠️ Failed to clear DTC, continue...")

        # 13.5 回到默认会话 10 01
        if not session_control(uds, 0x01, "Default Session (Post)"):
            app.log_w("[Post] ⚠️ Failed to enter Default Session, continue...")

        # ------------------------------------------------------------
        # 14. ECU 复位 11 03 (SoftReset)
        # ------------------------------------------------------------
        uds_request(uds, 0x11, [0x03], "ECU Reset")

        flash_ok = True

    finally:
        uds.stop_session_keep()

    return flash_ok


# ============================================================================
# ZXDoc 脚本入口
# ============================================================================
def __zxdoc_main__():
    


    if not measurement.is_started():
        measurement.start()
    
    do_flash_process()

    # 保持测量运行一段时间，便于观察最后几帧报文
    time.sleep(2)


def __zxdoc_on_exit__():
    measurement.stop()
