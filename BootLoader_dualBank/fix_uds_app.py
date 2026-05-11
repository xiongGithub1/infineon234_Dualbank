with open(r'e:\workFiles\IEBS\tc234bootloader\BootLoader_dualBank\AppSw\Tricore\App_UDS\uds_app.c', 'r', encoding='utf-8') as f:
    lines = f.readlines()

# Lines to remove (1-based indexing from grep output):
# 1759: uint32 startAddr;
# 1760: uint32 bankSize;
# 1769-1778: if (targetBank == BANK_A) { ... } else { ... }
lines_to_remove = {1759, 1760, 1770, 1771, 1772, 1773, 1774, 1775, 1776, 1777, 1778}
# Note: line 1769 is "if (targetBank == BANK_A)" which we keep as the marker,
# but actually we need to remove the whole if-else block including 1769.
# Let's adjust: remove 1759, 1760, and 1769-1778.
lines_to_remove = set(range(1769, 1779)) | {1759, 1760}

new_lines = []
for idx, line in enumerate(lines, start=1):
    if idx not in lines_to_remove:
        new_lines.append(line)

with open(r'e:\workFiles\IEBS\tc234bootloader\BootLoader_dualBank\AppSw\Tricore\App_UDS\uds_app.c', 'w', encoding='utf-8') as f:
    f.writelines(new_lines)

print(f"Removed lines: {sorted(lines_to_remove)}")
print(f"Total lines: {len(lines)} -> {len(new_lines)}")
