import re

with open(r'e:\workFiles\IEBS\tc234bootloader\BootLoader_dualBank\Recorder1_20260511103534.asc', 'r') as f:
    lines = f.readlines()

# Extract all frames for 74c (Tx) and 75c (Rx)
tx_frames = []
rx_frames = []
for line in lines:
    line = line.strip()
    if not line or line.startswith('//'):
        continue
    parts = line.split()
    if len(parts) < 6:
        continue
    try:
        time = float(parts[0])
        ch = parts[1]
        can_id = parts[2]
        dir = parts[3]
        dtype = parts[4]
        dlen = int(parts[5])
        data = parts[6:6+dlen]
        if can_id == '74c' and dir == 'Tx':
            tx_frames.append((time, data))
        elif can_id == '75c' and dir == 'Rx':
            rx_frames.append((time, data))
    except:
        continue

print(f'TX frames: {len(tx_frames)}, RX frames: {len(rx_frames)}')

# Reassemble multi-frame UDS messages
def reassemble_uds(frames):
    msgs = []
    i = 0
    while i < len(frames):
        time, data = frames[i]
        if len(data) == 0:
            i += 1
            continue
        first_byte = int(data[0], 16)
        if first_byte & 0xF0 == 0x00:  # Single frame
            payload_len = first_byte & 0x0F
            payload = [int(x,16) for x in data[1:1+payload_len]]
            msgs.append((time, payload))
            i += 1
        elif first_byte & 0xF0 == 0x10:  # First frame
            total_len = ((first_byte & 0x0F) << 8) | int(data[1], 16)
            payload = [int(x,16) for x in data[2:]]
            i += 1
            # Wait for FC
            while i < len(frames):
                time2, data2 = frames[i]
                if int(data2[0],16) & 0xF0 == 0x30:
                    i += 1
                    break
                i += 1
            # Read consecutive frames
            expected_sn = 1
            while len(payload) < total_len and i < len(frames):
                time2, data2 = frames[i]
                fb2 = int(data2[0], 16)
                if fb2 & 0xF0 == 0x20:
                    sn = fb2 & 0x0F
                    if sn == expected_sn:
                        need = total_len - len(payload)
                        cf_data = [int(x,16) for x in data2[1:1+min(need, 7)]]
                        payload.extend(cf_data)
                        expected_sn = (expected_sn + 1) & 0x0F
                    i += 1
                elif fb2 & 0xF0 == 0x30:
                    i += 1
                else:
                    break
            msgs.append((time, payload[:total_len]))
        else:
            i += 1
    return msgs

tx_msgs = reassemble_uds(tx_frames)
rx_msgs = reassemble_uds(rx_frames)

print(f'TX UDS msgs: {len(tx_msgs)}, RX UDS msgs: {len(rx_msgs)}')

# Print key messages
print('\n=== TX (Tester -> ECU) ===')
for time, payload in tx_msgs:
    sid = payload[0] if payload else 0
    sid_str = f'{sid:02X}'
    desc = ''
    if sid == 0x34:
        if len(payload) >= 11:
            addr = (payload[3]<<24)|(payload[4]<<16)|(payload[5]<<8)|payload[6]
            length = (payload[7]<<24)|(payload[8]<<16)|(payload[9]<<8)|payload[10]
            desc = f'ReqDownload addr=0x{addr:08X} len=0x{length:08X} ({length})'
    elif sid == 0x36:
        desc = f'TransferData SN={payload[1] if len(payload)>1 else "?"}'
    elif sid == 0x37:
        desc = 'ReqTransExit'
    elif sid == 0x31 and len(payload) >= 4:
        rid = (payload[2]<<8)|payload[3]
        if rid == 0xFF00:
            desc = f'Erase sector=0x{payload[4]:02X}{payload[5]:02X}' if len(payload)>=6 else 'Erase'
        elif rid == 0xDFFF:
            crc = (payload[4]<<24)|(payload[5]<<16)|(payload[6]<<8)|payload[7] if len(payload)>=8 else 0
            desc = f'CheckCRC RID=DFFF CRC=0x{crc:08X}'
        elif rid == 0xFFFD:
            desc = 'CheckProgConditions'
        else:
            desc = f'Routine RID=0x{rid:04X}'
    elif sid == 0x10:
        desc = f'Session=0x{payload[1]:02X}' if len(payload)>1 else ''
    elif sid == 0x11:
        desc = f'Reset=0x{payload[1]:02X}' if len(payload)>1 else ''
    elif sid == 0x27:
        desc = f'SecurityAccess=0x{payload[1]:02X}' if len(payload)>1 else ''
    elif sid == 0x2E:
        desc = f'WriteData=0x{payload[1]:02X}{payload[2]:02X}' if len(payload)>2 else ''
    
    if desc:
        print(f'{time:10.4f} {" ".join(f"{b:02X}" for b in payload):50s} {desc}')

print('\n=== RX (ECU -> Tester) ===')
for time, payload in rx_msgs:
    sid = payload[0] if payload else 0
    if sid == 0x7F and len(payload) >= 3:
        desc = f'NegRsp 0x{payload[1]:02X} NRC=0x{payload[2]:02X}'
    elif sid == 0x71 and len(payload) >= 4:
        rid = (payload[2]<<8)|payload[3]
        if rid == 0xDFFF and len(payload) >= 5:
            desc = f'Routine RID=DFFF result=0x{payload[4]:02X}'
        elif rid == 0xFF00 and len(payload) >= 5:
            desc = f'Erase result=0x{payload[4]:02X}{payload[5]:02X}' if len(payload)>=6 else f'Erase RID=FF00'
        else:
            desc = f'Routine RID=0x{rid:04X}'
    elif sid == 0x74 and len(payload) >= 3:
        desc = f'ReqDownload maxBlock=0x{payload[1]:02X}{payload[2]:02X}'
    elif sid == 0x76 and len(payload) >= 2:
        desc = f'TransferData SN=0x{payload[1]:02X}'
    elif sid == 0x77:
        desc = 'ReqTransExit OK'
    else:
        desc = ''
    
    if desc:
        print(f'{time:10.4f} {" ".join(f"{b:02X}" for b in payload):50s} {desc}')
