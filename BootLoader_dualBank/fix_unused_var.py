with open(r'e:\workFiles\IEBS\tc234bootloader\BootLoader_dualBank\AppSw\Tricore\App_bootloader\Boot_DualBank.c', 'r', encoding='utf-8') as f:
    lines = f.readlines()

# Find and remove unused startAddr variable and its assignment block in VerifyBank
new_lines = []
i = 0
while i < len(lines):
    line = lines[i]
    # Remove startAddr declaration
    if 'uint32 startAddr;' in line and i < 400:  # Only in VerifyBank context
        i += 1
        continue
    
    # Remove the bank->startAddr assignment block in VerifyBank
    if 'if (bank == BANK_A)' in line and i < 400:
        brace_count = 0
        started = False
        while i < len(lines):
            l = lines[i]
            if '{' in l:
                brace_count += l.count('{')
                started = True
            if '}' in l:
                brace_count -= l.count('}')
            i += 1
            if started and brace_count <= 0:
                break
        continue
    
    new_lines.append(line)
    i += 1

with open(r'e:\workFiles\IEBS\tc234bootloader\BootLoader_dualBank\AppSw\Tricore\App_bootloader\Boot_DualBank.c', 'w', encoding='utf-8') as f:
    f.writelines(new_lines)

print(f"Removed unused startAddr. Total lines: {len(lines)} -> {len(new_lines)}")
