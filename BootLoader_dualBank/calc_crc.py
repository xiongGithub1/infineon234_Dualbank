import struct
import zlib

def parse_intel_hex(hex_path):
    """解析 Intel HEX，返回地址到字节的字典"""
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
            if rec_type == 0x00:
                for i, b in enumerate(payload):
                    data_dict[base_addr + addr + i] = b
            elif rec_type == 0x04:
                base_addr = struct.unpack('>H', payload)[0] << 16
            elif rec_type == 0x01:
                break
    return data_dict

def calc_crc_bank(hex_path, bank_start, bank_size):
    """ECU 方式：创建 bank_size 大小的 buffer，全填 0xFF，然后写入 HEX 数据，计算整个 buffer 的 CRC"""
    data_dict = parse_intel_hex(hex_path)
    bank_data = bytearray([0xFF] * bank_size)
    for addr, b in data_dict.items():
        if bank_start <= addr < bank_start + bank_size:
            offset = addr - bank_start
            bank_data[offset] = b
    crc = zlib.crc32(bank_data) & 0xFFFFFFFF
    return crc

def calc_crc_raw_data(hex_path):
    """只计算 HEX 文件中所有数据字节的 CRC（按地址顺序拼接）"""
    data_dict = parse_intel_hex(hex_path)
    if not data_dict:
        return 0
    min_addr = min(data_dict.keys())
    max_addr = max(data_dict.keys())
    size = max_addr - min_addr + 1
    buf = bytearray([0xFF] * size)  # 用 0xFF 填充 gap
    for addr, b in data_dict.items():
        buf[addr - min_addr] = b
    crc = zlib.crc32(buf) & 0xFFFFFFFF
    return crc

def calc_crc_continuous(hex_path):
    """只拼接 HEX 中实际存在的数据记录（不填充 gap），计算 CRC"""
    data_dict = parse_intel_hex(hex_path)
    sorted_addrs = sorted(data_dict.keys())
    # 按连续地址分块
    blocks = []
    cur_block = bytearray()
    prev_addr = None
    for addr in sorted_addrs:
        if prev_addr is None or addr == prev_addr + 1:
            cur_block.append(data_dict[addr])
        else:
            blocks.append(bytes(cur_block))
            cur_block = bytearray([data_dict[addr]])
        prev_addr = addr
    if cur_block:
        blocks.append(bytes(cur_block))
    # 拼接所有块
    all_data = b''.join(blocks)
    crc = zlib.crc32(all_data) & 0xFFFFFFFF
    return crc, len(all_data)

HEX_FILE = r"E:\workFiles\IEBS\tc234bootloader\App_dualBank\Debug\App_dualBank.hex"
BANK_B_START = 0x80100000
BANK_B_SIZE = 1024 * 1024

data_dict = parse_intel_hex(HEX_FILE)
min_addr = min(data_dict.keys())
max_addr = max(data_dict.keys())
print(f"HEX data range: 0x{min_addr:08X} ~ 0x{max_addr:08X}")
print(f"Total unique bytes in HEX: {len(data_dict)}")

# 统计在 Bank B 范围内的数据
bank_b_bytes = {k: v for k, v in data_dict.items() if BANK_B_START <= k < BANK_B_START + BANK_B_SIZE}
print(f"Bytes within Bank B: {len(bank_b_bytes)}")

# 找出 Bank B 内的数据分布
bank_b_addrs = sorted(bank_b_bytes.keys())
if bank_b_addrs:
    print(f"Bank B data start: 0x{bank_b_addrs[0]:08X}")
    print(f"Bank B data end: 0x{bank_b_addrs[-1]:08X}")

# 方式1: ECU 方式（整个 Bank）
crc1 = calc_crc_bank(HEX_FILE, BANK_B_START, BANK_B_SIZE)
print(f"\n[ECU mode] CRC over full Bank B (1MB, 0xFF padded): 0x{crc1:08X}")

# 方式2: 按 HEX 文件的最小~最大地址范围，0xFF 填充 gap
crc2 = calc_crc_raw_data(HEX_FILE)
print(f"[Raw data range] CRC over HEX min~max range (0xFF padded): 0x{crc2:08X}")

# 方式3: 只拼接实际数据字节，不填充 gap
crc3, total_len = calc_crc_continuous(HEX_FILE)
print(f"[Continuous data only] CRC over {total_len} bytes (no gap fill): 0x{crc3:08X}")

# 方式4: 只计算 Bank B 范围内、HEX 文件覆盖的地址范围
crc4 = calc_crc_bank(HEX_FILE, BANK_B_START, max_addr - BANK_B_START + 1) if max_addr >= BANK_B_START else 0
print(f"[Bank B actual range] CRC over 0x{BANK_B_START:08X}~0x{max_addr:08X}: 0x{crc4:08X}")

print(f"\nECU reported actualCRC: 0x0A522ECB")
print(f"Tester reported expectedCRC: 0x1ABC6AE1")
