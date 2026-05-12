from ZXDoc import *
import time
import os
import struct
import zlib

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
#  10. 34/36/37   -> 文件下载 (ZxDoc file_download API)
#  11. 31 01 DFFF -> 验证并激活 Bank
#  12. 11 01      -> ECU 复位
# ============================================================================

# ========== 用户配置区（请根据实际情况修改） ==========

# CAN 诊断 ID
PHY_ADDR = 0x74C           # ECU 物理请求地址 (RX)
TESTER_ADDR = 0x75C        # Tester 响应地址 (TX)
FUNC_ADDR = 0x7DF          # 功能地址（会话保持用）
CHANNEL = 1                # CAN 通道号

# 刷写文件路径（.hex 或 .bin，需与 LSL 中的 Bank 地址匹配）
# HEX_FILE = r"E:\workFiles\IEBS\zhenChuangCode\ZhenchuangTest\IEBS_wulin_DiagnosisTest_20250723\Debug\IEBS_wulin_DiagnosisTest_20250723.hex"
HEX_FILE = r"E:\workFiles\IEBS\tc234bootloader\App_dualBank\Debug\App_dualBank.hex"
HEX_FILE_B=r"E:\workFiles\IEBS\tc234bootloader\App_dualBank\App_dualBank.hex"
# 安全访问 DLL 路径（用于 27 服务 Seed->Key 计算）
KEY_DLL = r"E:\visualStudioCode\ZcanProDll\Debug\ZcanProDll.dll"

# 刷写目标 Bank: "A" 或 "B"
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


def parse_intel_hex(hex_path):
    """解析 Intel HEX 文件，返回 (起始地址, bytearray 数据)"""
    data_dict = {}
    base_addr = 0
    with open(hex_path, 'r') as f:
        for line in f:
            line = line.strip()
            if not line or line[0] != ':':
                continue
            byte_count = int(line[1:3], 16)
            addr = int(line[3:7], 16)
            rec_type = int(line[7:9], 16)
            payload = bytes.fromhex(line[9:9 + byte_count * 2])
            if rec_type == 0x00:           # Data
                for i, b in enumerate(payload):
                    data_dict[base_addr + addr + i] = b
            elif rec_type == 0x04:         # Extended Linear Address
                base_addr = struct.unpack('>H', payload)[0] << 16
            elif rec_type == 0x01:         # End
                break
    if not data_dict:
        return 0, bytearray()
    min_addr = min(data_dict.keys())
    max_addr = max(data_dict.keys())
    size = max_addr - min_addr + 1
    buf = bytearray([0xFF] * size)
    for addr, b in data_dict.items():
        buf[addr - min_addr] = b
    return min_addr, buf


def calc_bank_crc32(hex_path, bank_start, bank_size):
    """
    计算 HEX 文件在指定 Bank 范围内的 CRC32（与 Bootloader 内部计算一致）。
    未编程区域按 0xFF 填充，与 Flash 擦除后的状态一致。
    """
    start_addr, data = parse_intel_hex(hex_path)
    offset = bank_start - start_addr
    if offset < 0:
        app.log_e(f"[CRC] HEX start 0x{start_addr:08X} > Bank start 0x{bank_start:08X}")
        return None
    bank_data = bytearray([0xFF] * bank_size)
    copy_len = min(len(data), bank_size - offset)
    bank_data[offset:offset + copy_len] = data[:copy_len]
    crc = zlib.crc32(bank_data) & 0xFFFFFFFF
    return crc


def file_download(uds):
    """
    34/36/37 文件下载（使用 ZXDoc 内置 file_download API）。
    注意: 由于我们在调用本函数前已经手动完成了 Flash 擦除，
    如果 ZxDoc 的 file_download 内部也尝试自动擦除，可能会因命令格式
    不兼容而失败。建议:
      1. 先测试观察 file_download 是否自动发送了 31 擦除命令；
      2. 如有冲突，将 memEraseType 改为不自动擦除的模式
         （如 ZMemEraseType.WithoutErase，具体枚举名请参考 ZXDoc 文档）。
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
            f"Proceeding to RequestDownload (0x34)."
        )
    if not os.path.exists(HEX_FILE):
        app.log_e(f"[Download] ❌ File not found: {HEX_FILE}")
        return False

    dl_req = ZFileDownloadReq(
        filePath=HEX_FILE,
        # memEraseType=ZMemEraseType.WithParam,  # 如需禁用自动擦除请修改此处
        srcAddr=PHY_ADDR,
        dstAddr=TESTER_ADDR,
        crcAlgorithm=ZCrcAlgorithm(
            type=ZCrcType.CRC32,
            polynomial=0x04C11DB7,
            initValue=0xFFFFFFFF,
            xorOutput=0xFFFFFFFF,
            reflectInput=True,
            reflectOutput=True,
        ),
        totalCheckCmd=TOTAL_CHECK_CMD,
    )

    if uds.file_download(dl_req):
        app.log_i("[Download] ✅ File download success")
        return True
    else:
        app.log_e("[Download] ❌ File download failed")
        return False

# ============================================================
# 手动实现 34/36/37 下载（绕过 ZXDoc file_download 内部检查）
# ============================================================
def parse_hex_file(hex_path):
    """
    解析 Intel HEX 文件，返回 [(address, data_bytes), ...] 的列表
    按连续地址合并 segment
    """
    segments = []
    base_addr = 0
    cur_seg = None

    with open(hex_path, 'r') as f:
        for line in f:
            line = line.strip()
            if not line or not line.startswith(':'):
                continue
            byte_count = int(line[1:3], 16)
            addr = int(line[3:7], 16)
            rec_type = int(line[7:9], 16)
            payload = bytes.fromhex(line[9:9+byte_count*2])

            if rec_type == 0x04:  # Extended Linear Address
                base_addr = struct.unpack(">H", payload)[0] << 16
            elif rec_type == 0x00:  # Data
                abs_addr = base_addr + addr
                if cur_seg is None or abs_addr != (cur_seg[0] + len(cur_seg[1])):
                    if cur_seg is not None:
                        segments.append(cur_seg)
                    cur_seg = [abs_addr, bytearray()]
                cur_seg[1].extend(payload)
            # 忽略 0x01 (EOF), 0x05 (Start Linear Address) 等

    if cur_seg is not None:
        segments.append(cur_seg)

    # 转换为 (addr, bytes) 元组列表
    return [(s[0], bytes(s[1])) for s in segments]


def file_download_manual(uds):
    """
    手动发送 34/36/37 序列下载 HEX 文件到 TARGET_BANK。
    完全绕过 ZXDoc file_download API，避免任何内部隐藏检查。
    """
    # 1. 擦除检查
    expected_sectors = set(BANK_SECTORS.get(TARGET_BANK, BANK_SECTORS["A"]))
    actual_sectors = set(_g_erased_sectors)
    missing = expected_sectors - actual_sectors
    if missing:
        app.log_e(
            f"[Download] ❌ Erase incomplete! Missing sectors: {sorted(missing)}. "
            f"Expected {len(expected_sectors)}, actually erased {len(actual_sectors)}."
        )
        return False
    app.log_i(f"[Download] ✅ Erase check passed, proceeding to manual 34/36/37 download.")

    if not os.path.exists(HEX_FILE):
        app.log_e(f"[Download] ❌ File not found: {HEX_FILE}")
        return False

    # 2. 解析 HEX
    segments = parse_hex_file(HEX_FILE)
    if not segments:
        app.log_e("[Download] ❌ No valid data segments found in HEX file")
        return False

    # 3. 过滤只下载到目标 Bank 的段
    bank_start = BANK_A_START_ADDR if TARGET_BANK == "A" else BANK_B_START_ADDR
    bank_end = bank_start + (BANK_APP_A_SIZE if TARGET_BANK == "A" else BANK_APP_B_SIZE)
    filtered_segments = []
    for addr, data in segments:
        seg_start = max(addr, bank_start)
        seg_end = min(addr + len(data), bank_end)
        if seg_start < seg_end:
            offset = seg_start - addr
            filtered_segments.append((seg_start, data[offset:offset + (seg_end - seg_start)]))

    if not filtered_segments:
        app.log_e(f"[Download] ❌ No data in HEX falls within target bank {TARGET_BANK} "
                  f"range [{hex(bank_start)} - {hex(bank_end)}]")
        return False

    app.log_i(f"[Download] Target bank: {TARGET_BANK}, {len(filtered_segments)} segment(s) to download")

    # 4. 逐段下载
    MAX_TX_DATA = 4095  # CAN FD 单帧最大数据，留点余量
    sequence = 1

    for seg_idx, (addr, data) in enumerate(filtered_segments):
        data_len = len(data)
        app.log_i(f"[Download] Segment {seg_idx+1}/{len(filtered_segments)}: "
                  f"addr={hex(addr)}, len={data_len}")

        # ---- 0x34 RequestDownload ----
        # 地址格式: 0x44 = 4字节地址 + 4字节长度
        addr_bytes = struct.pack(">I", addr)
        len_bytes = struct.pack(">I", data_len)
        req34_data = [0x00, 0x44] + list(addr_bytes) + list(len_bytes)
        req34 = ZUdsRequest(
            src_addr=PHY_ADDR,
            dst_addr=TESTER_ADDR,
            is_extend=False,
            suppress_response=False,
            sid=0x34,
            data=req34_data,
        )
        resp34 = uds.request(req34)
        if resp34 is None or resp34.status != UDS_RSP_STATUS_OK or len(resp34.data) < 2:
            app.log_e(f"[Download] ❌ 0x34 RequestDownload failed or NRC")
            return False

        # 解析 0x74 响应中的 maxNumberOfBlockLength
        # resp34.data 不含 SID，格式: [lengthFormatIdentifier, maxNumberOfBlockLength...]
        lfi = resp34.data[0]
        # UDS 规范: bit3-0 编码为 (字节数 - 1)，即实际字节数 = (lfi & 0x0F) + 1
        len_size = (lfi & 0x0F) + 1
        if len_size == 1 and len(resp34.data) >= 2:
            max_block_len = resp34.data[1]
        elif len_size == 2 and len(resp34.data) >= 3:
            max_block_len = (resp34.data[1] << 8) | resp34.data[2]
        else:
            max_block_len = MAX_TX_DATA
        app.log_i(f"[Download] Server max block length: {max_block_len}")
        # 实际可用数据长度 = max_block_len - 2 (减去 SID + blockSequenceCounter)
        max_tx = max_block_len - 2 if max_block_len > 2 else MAX_TX_DATA

        # ---- 0x36 TransferData ----
        offset = 0
        while offset < data_len:
            chunk = data[offset:offset + max_tx]
            block_seq = sequence & 0xFF
            req36_data = [block_seq] + list(chunk)
            req36 = ZUdsRequest(
                src_addr=PHY_ADDR,
                dst_addr=TESTER_ADDR,
                is_extend=False,
                suppress_response=False,
                sid=0x36,
                data=req36_data,
            )
            resp36 = uds.request(req36)
            if resp36 is None or resp36.status != UDS_RSP_STATUS_OK:
                app.log_e(f"[Download] ❌ 0x36 TransferData failed at offset {offset}, seq={block_seq}")
                return False
            # 0x76 正响应: resp36.data[0] 应为 blockSequenceCounter
            if len(resp36.data) < 1 or resp36.data[0] != block_seq:
                app.log_e(f"[Download] ❌ 0x76 block sequence mismatch, expected {block_seq}, got {resp36.data.hex() if resp36.data else 'empty'}")
                return False

            offset += len(chunk)
            sequence += 1
            if sequence > 0xFF:
                sequence = 0

            # 每 10 个数据包打印一次进度
            if offset % (max_tx * 10) < max_tx:
                pct = offset * 100 / data_len
                app.log_i(f"[Download] Segment {seg_idx+1} progress: {offset}/{data_len} ({pct:.1f}%)")

        # ---- 0x37 RequestTransferExit ----
        req37 = ZUdsRequest(
            src_addr=PHY_ADDR,
            dst_addr=TESTER_ADDR,
            is_extend=False,
            suppress_response=False,
            sid=0x37,
            data=[],
        )
        resp37 = uds.request(req37)
        if resp37 is None or resp37.status != UDS_RSP_STATUS_OK:
            app.log_e(f"[Download] ❌ 0x37 RequestTransferExit failed")
            return False

        app.log_i(f"[Download] ✅ Segment {seg_idx+1}/{len(filtered_segments)} downloaded successfully")

    app.log_i("[Download] ✅ All segments downloaded successfully")
    return True
def do_flash_process():
    """完整刷写主流程"""
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
        return

    # 启动会话保持（功能地址 0x7DF，周期 5000ms）
    uds.start_session_keep(FUNC_ADDR, 5000, True)

    try:
        # ------------------------------------------------------------
        # 1. 默认会话 10 01
        # ------------------------------------------------------------
        if not session_control(uds, 0x01, "Default Session"):
            return

        # ------------------------------------------------------------
        # 2. 扩展会话 10 03
        # ------------------------------------------------------------
        if not session_control(uds, 0x03, "Extended Session"):
            return

        # ------------------------------------------------------------
        # 3. 安全访问 Level 1（扩展会话下解锁）
        # ------------------------------------------------------------
        if not security_access(uds, 1, "SA L1"):
            return

        # ------------------------------------------------------------
        # 4. 检查编程条件 31 01 DF FD
        #    正响应: 71 01 FF FD <canFlash> <targetBank>
        #      canFlash  : 1=能刷写, 0=不能
        #      targetBank: 'A' 或 'B'（推荐刷写的 Bank）
        # ------------------------------------------------------------
        global TARGET_BANK
        global HEX_FILE
        rsp = uds_request(uds, 0x31, [0x01, 0xFF, 0xFD], "Check Programming")
        if not rsp:
            return
        # ZXDoc rsp.data 不含 SID，格式: [01 subFunc, FF RID_H, FD RID_L, canFlash, targetBank]
        if len(rsp.data) >= 5:
            can_flash = rsp.data[3]
            target_bank_char = rsp.data[4]
            app.log_i(f"[Check] Bootloader reports: canFlash={can_flash}, targetBank={target_bank_char}")
            if can_flash != 1:
                app.log_e(f"[Check] ❌ Programming conditions NOT OK (canFlash={can_flash}), abort flash!")
                return
            if target_bank_char ==0x0A:
                TARGET_BANK = 'A'
                HEX_FILE= r"E:\workFiles\IEBS\tc234bootloader\App_dualBank\Debug\App_dualBank.hex"
                app.log_i(f"[Check] ✅ Target bank dynamically set to Bank {TARGET_BANK}")
            elif target_bank_char==0x0B:
                TARGET_BANK ='B'
                HEX_FILE=r"E:\workFiles\IEBS\tc234bootloader\App_dualBank\App_dualBank.hex"
            else:
                app.log_w(f"[Check] ⚠️ Unexpected targetBank char=0x{rsp.data[4]:02X}, keep default={TARGET_BANK}")
        else:
            app.log_w(f"[Check] ⚠️ Short response, keep default target bank={TARGET_BANK}")

        # ------------------------------------------------------------
        # 5. 关闭 DTC 85 02
        # ------------------------------------------------------------
        if not uds_request(uds, 0x85, [0x02], "Disable DTC"):
            return

        # ------------------------------------------------------------
        # 6. 编程会话 10 02
        # ------------------------------------------------------------
        if not session_control(uds, 0x02, "Programming Session"):
            return

        # ------------------------------------------------------------
        # 7. 安全访问 Level 2（编程会话必须解锁 Level 2）
        #    BootLoader 中 31 01 FF 00 / 31 01 DFFF 都要求 SECURITY_LEVEL_2
        # ------------------------------------------------------------
        if not security_access(uds, 3, "SA L2"):
            return

        # ------------------------------------------------------------
        # 8. 写指纹信息 2E F1 5A 55 55
        #    如需修改指纹内容，请调整 data 字段
        # ------------------------------------------------------------
        if not uds_request(uds, 0x2E, [0xF1, 0x5A, 0x55, 0x55], "Write Fingerprint"):
            return

        # ------------------------------------------------------------
        # 9. 擦除目标 Bank (31 01 FF 00)
        #    逐个 sector 擦除，每次只擦 1 个 sector，单次耗时 < 1s，
        #    不会触发 P2 超时。
        # ------------------------------------------------------------
        if not erase_target_bank(uds):
            app.log_e("[Flash] Erase failed, abort download!")
            return

        # ------------------------------------------------------------
        # 10. 文件下载 (34/36/37，不自动发送 totalCheckCmd)
        # ------------------------------------------------------------
        if not file_download(uds):
            return

        # # 替换原来调用 file_download(uds) 的地方
        # if not file_download_manual(uds):
        #     app.log_e("[Flash] ❌ File download failed, aborting.")
        #     return
        # ------------------------------------------------------------
        # 11. 手动发送 totalCheckCmd 验证 CRC
        #     31 01 DF FF <CRC32(4 bytes, big-endian)>
        #     Bootloader 收到后会自行计算 Flash CRC 并与传入值比较。
        # ------------------------------------------------------------
        # bank_start = BANK_B_START_ADDR if TARGET_BANK == "A" else BANK_A_START_ADDR
        # bank_size  = BANK_APP_B_SIZE  if TARGET_BANK == "A" else BANK_APP_A_SIZE

        # file_crc = calc_bank_crc32(HEX_FILE, bank_start, bank_size)
        # if file_crc is None:
        #     app.log_e("[Verify] ❌ CRC calculation failed, abort ECU reset!")
        #     return

        # app.log_i(f"[Verify] HEX file CRC32 = 0x{file_crc:08X}")

        # total_check_data = [
        #     0x01, 0xDF, 0xFF,
        #     (file_crc >> 24) & 0xFF,
        #     (file_crc >> 16) & 0xFF,
        #     (file_crc >> 8)  & 0xFF,
        #     file_crc & 0xFF,
        # ]

        # rsp = uds_request(uds, 0x31, total_check_data, "Verify CRC")
        # if not rsp:
        #     app.log_e("[Verify] ❌ No response from 31 01 DFFF, abort ECU reset!")
        #     return

        # # rsp.data: [0x01=subFunc, 0xDF=RID_H, 0xFF=RID_L, 0x01=result]
        # if len(rsp.data) < 4 or rsp.data[3] != 0x01:
        #     app.log_e(
        #         f"[Verify] ❌ Bank {TARGET_BANK} CRC verification FAILED! "
        #         f"rsp.data={rsp.data.hex() if rsp.data else 'empty'}, abort ECU reset!"
        #     )
        #     return
        # app.log_i(f"[Verify] ✅ Bank {TARGET_BANK} CRC OK, bank activated.")

        # ------------------------------------------------------------
        # 12. ECU 复位 11 01
        # ------------------------------------------------------------
        uds_request(uds, 0x11, [0x03], "ECU Reset")

    finally:
        uds.stop_session_keep()


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
