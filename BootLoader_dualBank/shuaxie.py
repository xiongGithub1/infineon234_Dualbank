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

# 刷写文件路径（.pkg 容器格式，由 pkg_generator.py 生成）
# 只需要一个 .pkg，Bootloader 自动选择对区写入
PKG_FILE = r"E:\workFiles\IEBS\tc234bootloader\tc234bootloader\App_dualBank.pkg"

# 安全访问 DLL 路径（用于 27 服务 Seed->Key 计算）
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
    解析 .pkg 容器文件。
    返回: (payload_addr, payload_len, payload_bytes, signature_bytes, sw_version, payload_crc)
    """
    with open(pkg_path, "rb") as f:
        pkg = f.read()

    if len(pkg) < PKG_HEADER_SIZE + SIG_LEN_RSA2048:
        raise ValueError(f".pkg file too small: {len(pkg)} bytes")

    header = pkg[0:PKG_HEADER_SIZE]

    # Magic
    magic = header[0:4]
    if magic != b"PKG2":
        raise ValueError(f"Invalid .pkg magic: {magic!r}, expected b'PKG2'")

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

    # Verify header CRC (header[0:32], excludes CRC field itself)
    import zlib
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
    print(f"[parse_pkg] Signature={len(signature)} bytes")

    return payload_addr, payload_len, payload, signature, sw_version, payload_crc


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


def file_download(uds, pkg_path):
    """
    34/36/37 文件下载（使用 ZXDoc 内置 file_download API）。
    从 .pkg 容器中提取 Payload，通过 ZXDoc 发送。
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
    if not os.path.exists(pkg_path):
        app.log_e(f"[Download] ❌ File not found: {pkg_path}")
        return False

    # 解析 .pkg
    try:
        payload_addr, payload_len, payload_bytes, signature_bytes, sw_version, payload_crc = parse_pkg_file(pkg_path)
    except Exception as e:
        app.log_e(f"[Download] ❌ Failed to parse .pkg: {e}")
        return False

    # 将 Payload + Signature 一起保存为临时 .bin 文件供 ZXDoc 发送
    # Bootloader 会从 PFlash 末尾读取 Signature 进行验证
    tmp_bin = pkg_path + ".payload.bin"
    combined = payload_bytes + signature_bytes
    with open(tmp_bin, "wb") as f:
        f.write(combined)
    app.log_i(f"[Download] Payload+Sig: {payload_len}+{len(signature_bytes)}={len(combined)} bytes -> {tmp_bin}")

    # 构造下载请求
    block_cfg = FlashDataBlockCfg(
        startAddr=0,
        dataLen=len(combined),
        crc=payload_crc,
        fillByte=0x00,
        mappedAddr=payload_addr,
    )

    dl_req = ZFileDownloadReq(
        filePath=tmp_bin,
        memEraseType=ZMemEraseType.NoErase,  # 已手动擦除，禁用自动擦除
        srcAddr=PHY_ADDR,
        dstAddr=TESTER_ADDR,
        fileBlockCfgs=[block_cfg],
        crcAlgorithm=ZCrcAlgorithm(
            type=ZCrcType.CRC32,
            polynomial=0x04C11DB7,
            initValue=0xFFFFFFFF,
            xorOutput=0xFFFFFFFF,
            reflectInput=True,
            reflectOutput=True,
        ),
        totalCheckCmd=None,  # 手动发送 31 DFFF，不用 ZXDoc 内置
    )

    if uds.file_download(dl_req):
        app.log_i("[Download] ✅ File download success")
        return True
    else:
        app.log_e("[Download] ❌ File download failed")
        return False





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
        #      targetBank: 'A' 或 'B'（Bootloader 自动选择的对区）
        #    只需要一个 .pkg，targetBank 仅用于确定擦除哪个 Bank 的 sector
        # ------------------------------------------------------------
        global TARGET_BANK
        global PKG_FILE
        rsp = uds_request(uds, 0x31, [0x01, 0xFF, 0xFD], "Check Programming")
        if not rsp:
            return
        # ZXDoc rsp.data 不含 SID，格式: [01 subFunc, FF RID_H, FD RID_L, canFlash, targetBank]
        pkg_path = PKG_FILE
        if len(rsp.data) >= 5:
            can_flash = rsp.data[3]
            target_bank_char = rsp.data[4]
            app.log_i(f"[Check] Bootloader reports: canFlash={can_flash}, targetBank={target_bank_char}")
            if can_flash != 1:
                app.log_e(f"[Check] ❌ Programming conditions NOT OK (canFlash={can_flash}), abort flash!")
                return
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
            sys.exit(1)

        # ------------------------------------------------------------
        # 5. 关闭通信 28 03 03 (Disable Rx/Tx)
        #    减少刷写过程中总线负载，防止应用报文干扰
        # ------------------------------------------------------------
        if not uds_request(uds, 0x28, [0x03, 0x03], "Disable Communication"):
            return

        # ------------------------------------------------------------
        # 6. 关闭 DTC 85 02
        # ------------------------------------------------------------
        if not uds_request(uds, 0x85, [0x02], "Disable DTC"):
            return

        # ------------------------------------------------------------
        # 7. 编程会话 10 02
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
        # 8. 预编程条件检查 31 01 02 03 (ISO 14229-1 标准 RID)
        #    正响应: 71 01 02 03 <canFlash> <targetBank>
        # ------------------------------------------------------------
        rsp = uds_request(uds, 0x31, [0x01, 0x02, 0x03], "Check Preconditions")
        if not rsp:
            return
        if len(rsp.data) >= 5:
            can_flash = rsp.data[3]
            if can_flash != 1:
                app.log_e(f"[Precond] ❌ Preconditions NOT OK (canFlash={can_flash}), abort!")
                return
            app.log_i("[Precond] ✅ Preconditions OK")

        # ------------------------------------------------------------
        # 9. 写指纹信息 2E F1 5A 55 55
        #    如需修改指纹内容，请调整 data 字段
        # ------------------------------------------------------------
        if not uds_request(uds, 0x2E, [0xF1, 0x5A, 0x55, 0x55], "Write Fingerprint"):
            return

        # ------------------------------------------------------------
        # 10. 擦除目标 Bank (31 01 FF 00)
        #    逐个 sector 擦除，每次只擦 1 个 sector，单次耗时 < 1s，
        #    不会触发 P2 超时。
        # ------------------------------------------------------------
        if not erase_target_bank(uds):
            app.log_e("[Flash] Erase failed, abort download!")
            return

        # ------------------------------------------------------------
        # 11. 文件下载 (34/36/37，只发送 Payload)
        # ------------------------------------------------------------
        if not file_download(uds, pkg_path):
            return

        # ------------------------------------------------------------
        # 12. 检查编程依赖性 31 01 DFFF
        #     31 DFFF 只传 CRC(4B)，Bootloader 从 PFlash 末尾读 Sig 验证。
        # ------------------------------------------------------------
        try:
            _, payload_len, _, _, _, payload_crc = parse_pkg_file(pkg_path)
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
                return
            if len(rsp.data) >= 5 and rsp.data[4] == 0x01:
                app.log_i("[Verify] ✅ Signature and CRC verified, bank activated")
            else:
                app.log_e("[Verify] ❌ Signature/CRC verification failed!")
                return
        except Exception as e:
            app.log_e(f"[Verify] ❌ Failed to construct 31 DFFF: {e}")
            return
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
