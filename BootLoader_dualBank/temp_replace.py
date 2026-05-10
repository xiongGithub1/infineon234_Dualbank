import sys

path = 'BootLoader20250714_UDS_tasking622/AppSw/Tricore/Tc234_Modules/CAN/Can.c'
with open(path, 'rb') as f:
    data = f.read()

# 查找函数位置
idx = data.find(b'void Can_RxIndicationMainFunc(void)')
print(f'Found function at index: {idx}')

# 找到函数开始和结束
start = data.find(b'//', data.rfind(b'\n', 0, idx))
end = data.find(b'}', idx)
end = data.find(b'}', end+1) + 1
print(f'Function block from {start} to {end}')
print('Content:')
print(data[start:end].decode('gbk', errors='replace'))
