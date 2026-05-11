with open(r'e:\workFiles\IEBS\tc234bootloader\BootLoader_dualBank\AppSw\Tricore\App_bootloader\Boot_DualBank.c', 'r', encoding='utf-8') as f:
    content = f.read()

old_sig = 'void Boot_DualBank_MarkBankValid(uint32 bank, uint32 version)'
new_sig = 'void Boot_DualBank_MarkBankValid(uint32 bank, uint32 version, uint32 crc)'

if old_sig in content:
    content = content.replace(old_sig, new_sig)
    print("Signature replaced")
else:
    print("Signature NOT found")

# Now replace the body: delete the CRC calculation and use passed-in crc
old_body = '''    DualBankFlags_t flags;
    uint32 startAddr;
    uint32 crc;

    if (bank == BANK_A)
    {
        startAddr = BANK_A_START_ADDR;
    }
    else
    {
        startAddr = BANK_B_START_ADDR;
    }

    /* Calculate CRC over entire bank */
    {
        uint32 bankSize = (bank == BANK_A) ? BANK_APP_A_SIZE : BANK_APP_B_SIZE;
        crc = Boot_DualBank_CalculateCRC(startAddr, bankSize);
    }

    if (Boot_DualBank_ReadFlags(&flags) == TRUE)'''

new_body = '''    DualBankFlags_t flags;

    /* The 'crc' parameter is the one verified during 0x31 01 DFFF
     * (transmission-stream CRC), not recalculated here, to avoid
     * Data Cache staleness on TC234 after erase/write. */

    if (Boot_DualBank_ReadFlags(&flags) == TRUE)'''

if old_body in content:
    content = content.replace(old_body, new_body)
    print("Body replaced")
else:
    print("Body NOT found")

with open(r'e:\workFiles\IEBS\tc234bootloader\BootLoader_dualBank\AppSw\Tricore\App_bootloader\Boot_DualBank.c', 'w', encoding='utf-8') as f:
    f.write(content)
