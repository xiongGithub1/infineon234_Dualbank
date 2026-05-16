from ZXDoc import *




#!/usr/bin/env python3
"""
Intel HEX 文件对齐工具（AURIX TC234 专用）。
将 HEX 文件中的数据段对齐到指定边界（默认 32 字节），
空隙用 0x00 填充（AURIX PFlash 擦除状态）。
"""

import sys
import os


def parse_hex_file(filepath):
    """解析 Intel HEX 文件，返回 {abs_addr: byte_value} 和起始地址"""
    data = {}
    base_addr = 0
    start_addr = None

    with open(filepath, 'r') as f:
        for line_num, line in enumerate(f, 1):
            line = line.strip()
            if not line:
                continue
            if line[0] != ':':
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


def group_segments(data, max_gap=32):
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


def write_hex_record(f, byte_count, offset, rec_type, data_bytes):
    """写入一条 Intel HEX 记录"""
    record = f"{byte_count:02X}{offset:04X}{rec_type:02X}"
    if data_bytes:
        record += data_bytes.hex().upper()

    # 计算校验和：所有字节的和取低8位，再取补码
    checksum = byte_count + ((offset >> 8) & 0xFF) + (offset & 0xFF) + rec_type
    for b in data_bytes:
        checksum += b
    checksum = ((-checksum) & 0xFF)

    record += f"{checksum:02X}"
    f.write(f":{record}\n")


def align_hex_file(input_path, output_path, align=32, fill_byte=0x00):
    """
    对齐 Intel HEX 文件。

    Args:
        input_path:  输入 HEX 文件路径
        output_path: 输出 HEX 文件路径
        align:       对齐边界（默认 32 字节，TC234 PFlash page size）
        fill_byte:   填充字节（默认 0x00，AURIX PFlash 擦除状态）
    """
    data, start_addr = parse_hex_file(input_path)

    if not data:
        print(f"[align_hex] Error: No data found in {input_path}")
        return False

    segments = group_segments(data, max_gap=align)

    with open(output_path, 'w') as f:
        current_base = -1

        for seg_start, seg_end in segments:
            aligned_start = seg_start & ~(align - 1)
            aligned_end = (seg_end + align - 1) & ~(align - 1)

            addr = aligned_start
            while addr < aligned_end:
                # 检查是否需要发送扩展线性地址记录
                base = addr >> 16
                if base != current_base:
                    current_base = base
                    write_hex_record(f, 2, 0x0000, 0x04,
                                     bytes([base >> 8, base & 0xFF]))

                offset = addr & 0xFFFF
                line_data = bytearray()
                for i in range(align):
                    line_data.append(data.get(addr + i, fill_byte))

                write_hex_record(f, align, offset, 0x00, bytes(line_data))
                addr += align

        # 起始地址记录（如果有）
        if start_addr is not None:
            write_hex_record(f, 4, 0x0000, 0x05,
                             bytes([(start_addr >> 24) & 0xFF,
                                    (start_addr >> 16) & 0xFF,
                                    (start_addr >> 8) & 0xFF,
                                    start_addr & 0xFF]))

        # 结束记录
        write_hex_record(f, 0, 0x0000, 0x01, b'')

    # 统计信息
    total_data = len(data)
    aligned_total = sum(
        ((seg_end + align - 1) & ~(align - 1)) - (seg_start & ~(align - 1))
        for seg_start, seg_end in segments
    )
    added = aligned_total - total_data

    print(f"[align_hex] Aligned HEX file saved to: {output_path}")
    print(f"[align_hex]   Original data bytes: {total_data}")
    print(f"[align_hex]   Aligned total bytes: {aligned_total}")
    print(f"[align_hex]   Added fill bytes:    {added} ({added/total_data*100:.1f}%)")
    print(f"[align_hex]   Alignment:           {align} bytes")
    print(f"[align_hex]   Fill byte:           0x{fill_byte:02X}")

    return True


if __name__ == "__main__":

    sys.exit(0 if success else 1)


# def __zxdoc_init__():
#     pass


def __zxdoc_main__():
    
    # if len(sys.argv) < 3:
        # print("Usage: python align_hex.py <input.hex> <output.hex> [alignment]")
    # sys.exit(1)

    input_file = r"E:\workFiles\IEBS\tc234bootloader\App_dualBank\Debug\App_dualBank.hex"
    output_file = r"E:\workFiles\IEBS\tc234bootloader\tc234bootloader\App_dualBank.hex"
    alignment = int(sys.argv[3]) if len(sys.argv) > 3 else 32

    if not os.path.exists(input_file):
        print(f"Error: File not found: {input_file}")
        sys.exit(1)

    success = align_hex_file(input_file, output_file, align=alignment, fill_byte=0x00)
    pass
    

# def __zxdoc_on_exit__():
    # pass
