with open(r'e:\workFiles\IEBS\tc234bootloader\BootLoader_dualBank\AppSw\Tricore\App_bootloader\Boot_DualBank.c', 'r', encoding='utf-8') as f:
    lines = f.readlines()

# Replace lines 392-407 (1-based indexing) with new logic
# Line 392: /* Check if bank was ever marked valid (non-zero CRC stored) */
# Line 393: if (validFlag == 0u)
# ...
# Line 407: return BANK_STATUS_INVALID;
start_line = 392
end_line = 407

new_lines = lines[:start_line-1]  # keep lines before 392

new_block = [
    "    /* Check if bank was ever marked valid (non-zero value stored).\n",
    "     * The CRC written here is the one verified during programming (0x31 01 DFFF),\n",
    "     * not recalculated at boot, to avoid Data Cache staleness issues after\n",
    "     * Flash erase/write operations on TC234. */\n",
    "    if (validFlag == 0u)\n",
    "    {\n",
    "        return BANK_STATUS_INVALID;\n",
    "    }\n",
    "\n",
    "    return BANK_STATUS_VALID;\n",
]
new_lines.extend(new_block)
new_lines.extend(lines[end_line:])  # keep lines after 407

with open(r'e:\workFiles\IEBS\tc234bootloader\BootLoader_dualBank\AppSw\Tricore\App_bootloader\Boot_DualBank.c', 'w', encoding='utf-8') as f:
    f.writelines(new_lines)

print(f"Replaced lines {start_line}-{end_line}")
print(f"Total lines: {len(lines)} -> {len(new_lines)}")
