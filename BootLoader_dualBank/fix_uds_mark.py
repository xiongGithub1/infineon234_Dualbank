with open(r'e:\workFiles\IEBS\tc234bootloader\BootLoader_dualBank\AppSw\Tricore\App_UDS\uds_app.c', 'r', encoding='utf-8') as f:
    content = f.read()

old_call = 'Boot_DualBank_MarkBankValid(targetBank, 0x00010000u, expectedCRC);'
new_call = 'Boot_DualBank_MarkBankValid(targetBank, 0x00010000u);'

if old_call in content:
    content = content.replace(old_call, new_call)
    print("Replacement successful")
else:
    print("Replacement FAILED")

with open(r'e:\workFiles\IEBS\tc234bootloader\BootLoader_dualBank\AppSw\Tricore\App_UDS\uds_app.c', 'w', encoding='utf-8') as f:
    f.write(content)
