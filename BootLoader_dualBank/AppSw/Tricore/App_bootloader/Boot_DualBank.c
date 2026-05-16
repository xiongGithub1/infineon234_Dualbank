
/**********************************************************************************************************************
 * \file    Boot_DualBank.c
 * \brief   A/B Dual Bank Boot Management Implementation for TC234
 * \version V2.0.0
 * \note    TC234 has single PFlash bank (PF0), no hardware AB-swap.
 *          Bank switching is done purely in software via DFlash flags.
 *          RWW constraint: PFlash erase/write must execute from PSRAM (PSPR).
 *********************************************************************************************************************/

#include "Boot_DualBank.h"
#include "Flash.h"
#include <string.h>
#include "IfxCpu_reg.h"
#include "IfxCpu.h"
#include "Bsp.h"              /* for now(), TimeConst_1ms */
#include "custom_delay.h"     /* for delay_ms() */
#include "Can.h"              /* for CAN_deinit() */
#include "Tmr.h"              /* for TMR_deinit() */
#include "crypto/sha256.h"
#include "crypto/rsa2048.h"
#include "crypto/public_key.h"
 /* SW_Reset() is defined in App_bootloader.c */
extern void SW_Reset(void);

/*********************************************************************************************************************/
/*------------------------------------Erase Time Measurement (Debug Only)-------------------------------------------*/
/*********************************************************************************************************************/

/* Bank A sector range: S8 ~ S22 (15 sectors total) */
#define BANKA_SECTOR_START      8
#define BANKA_SECTOR_END        22
#define BANKA_SECTOR_COUNT      (BANKA_SECTOR_END - BANKA_SECTOR_START + 1)

EraseTimeLog_t g_eraseTimeLog[BANKA_SECTOR_COUNT];
uint32 g_eraseTotalTimeMs = 0;
uint32 g_eraseTestDone = 0;

/**
 * @brief Measure erase time for each sector in Bank A (S8~S22)
 * @note  Call this from debugger or after system init for testing.
 *        Results are stored in g_eraseTimeLog[] and g_eraseTotalTimeMs.
 */
void MeasureEraseBankA_Time(void)
{
    Ifx_TickTime tStart, tEnd;
    uint32 i, logIdx = 0;

    g_eraseTotalTimeMs = 0;
    g_eraseTestDone = 0;
    memset(g_eraseTimeLog, 0, sizeof(g_eraseTimeLog));

    for (i = BANKA_SECTOR_START; i <= BANKA_SECTOR_END; i++)
    {
        uint32 addr = IfxFlash_pFlashTableLog[i].start;
        uint32 size = IfxFlash_pFlashTableLog[i].end - addr + 1;
        int    result;
        uint32 elapsedMs;

        tStart = now();
        result = Flash_erasePFlash_port(addr);
        tEnd = now();

        /* Convert ticks to milliseconds */
        elapsedMs = (uint32) ((tEnd - tStart) / TimeConst_1ms);

        if (logIdx < BANKA_SECTOR_COUNT)
        {
            g_eraseTimeLog[logIdx].sectorIndex = i;
            g_eraseTimeLog[logIdx].sectorSize = size;
            g_eraseTimeLog[logIdx].eraseTimeMs = elapsedMs;
            g_eraseTimeLog[logIdx].result = (result == 0) ? 0u : 1u;
            logIdx++;
        }

        g_eraseTotalTimeMs += elapsedMs;

        /* Small delay to let CAN/WDT tasks breathe between sectors */
        delay_ms(5);
    }

    g_eraseTestDone = 1;
}

/*********************************************************************************************************************/
/*---------------------------------------------Private Variables-----------------------------------------------------*/
/*********************************************************************************************************************/

static uint32 g_activeBank = BANK_A;

/* Debug buffers for signature verification troubleshooting */
/* uint8 g_sigDebugHash[SIG_DEBUG_HASH_LEN]; */
/* uint8 g_sigDebugEM[SIG_DEBUG_EM_LEN]; */
/* uint8 g_sigDebugVerifyResult = 0; */

/* Global boot phase identifier for OEM traceability */
volatile BootPhase_t g_bootPhase = BOOT_PHASE_STARTUP;

/*********************************************************************************************************************/
/*---------------------------------------------Private CRC32 Table---------------------------------------------------*/
/*********************************************************************************************************************/

/* Software CRC32 lookup table (IEEE 802.3 polynomial) */
static const uint32 s_crc32Table[256] = {
    0x00000000,0x77073096,0xEE0E612C,0x990951BA,0x076DC419,0x706AF48F,0xE963A535,0x9E6495A3,
    0x0EDB8832,0x79DCB8A4,0xE0D5E91E,0x97D2D988,0x09B64C2B,0x7EB17CBD,0xE7B82D07,0x90BF1D91,
    0x1DB71064,0x6AB020F2,0xF3B97148,0x84BE41DE,0x1ADAD47D,0x6DDDE4EB,0xF4D4B551,0x83D385C7,
    0x136C9856,0x646BA8C0,0xFD62F97A,0x8A65C9EC,0x14015C4F,0x63066CD9,0xFA0F3D63,0x8D080DF5,
    0x3B6E20C8,0x4C69105E,0xD56041E4,0xA2677172,0x3C03E4D1,0x4B04D447,0xD20D85FD,0xA50AB56B,
    0x35B5A8FA,0x42B2986C,0xDBBBC9D6,0xACBCF940,0x32D86CE3,0x45DF5C75,0xDCD60DCF,0xABD13D59,
    0x26D930AC,0x51DE003A,0xC8D75180,0xBFD06116,0x21B4F4B5,0x56B3C423,0xCFBA9599,0xB8BDA50F,
    0x2802B89E,0x5F058808,0xC60CD9B2,0xB10BE924,0x2F6F7C87,0x58684C11,0xC1611DAB,0xB6662D3D,
    0x76DC4190,0x01DB7106,0x98D220BC,0xEFD5102A,0x71B18589,0x06B6B51F,0x9FBFE4A5,0xE8B8D433,
    0x7807C9A2,0x0F00F934,0x9609A88E,0xE10E9818,0x7F6A0DBB,0x086D3D2D,0x91646C97,0xE6635C01,
    0x6B6B51F4,0x1C6C6162,0x856530D8,0xF262004E,0x6C0695ED,0x1B01A57B,0x8208F4C1,0xF50FC457,
    0x65B0D9C6,0x12B7E950,0x8BBEB8EA,0xFCB9887C,0x62DD1DDF,0x15DA2D49,0x8CD37CF3,0xFBD44C65,
    0x4DB26158,0x3AB551CE,0xA3BC0074,0xD4BB30E2,0x4ADFA541,0x3DD895D7,0xA4D1C46D,0xD3D6F4FB,
    0x4369E96A,0x346ED9FC,0xAD678846,0xDA60B8D0,0x44042D73,0x33031DE5,0xAA0A4C5F,0xDD0D7CC9,
    0x5005713C,0x270241AA,0xBE0B1010,0xC90C2086,0x5768B525,0x206F85B3,0xB966D409,0xCE61E49F,
    0x5EDEF90E,0x29D9C998,0xB0D09822,0xC7D7A8B4,0x59B33D17,0x2EB40D81,0xB7BD5C3B,0xC0BA6CAD,
    0xEDB88320,0x9ABFB3B6,0x03B6E20C,0x74B1D29A,0xEAD54739,0x9DD277AF,0x04DB2615,0x73DC1683,
    0xE3630B12,0x94643B84,0x0D6D6A3E,0x7A6A5AA8,0xE40ECF0B,0x9309FF9D,0x0A00AE27,0x7D079EB1,
    0xF00F9344,0x8708A3D2,0x1E01F268,0x6906C2FE,0xF762575D,0x806567CB,0x196C3671,0x6E6B06E7,
    0xFED41B76,0x89D32BE0,0x10DA7A5A,0x67DD4ACC,0xF9B9DF6F,0x8EBEEFF9,0x17B7BE43,0x60B08ED5,
    0xD6D6A3E8,0xA1D1937E,0x38D8C2C4,0x4FDFF252,0xD1BB67F1,0xA6BC5767,0x3FB506DD,0x48B2364B,
    0xD80D2BDA,0xAF0A1B4C,0x36034AF6,0x41047A60,0xDF60EFC3,0xA867DF55,0x316E8EEF,0x4669BE79,
    0xCB61B38C,0xBC66831A,0x256FD2A0,0x5268E236,0xCC0C7795,0xBB0B4703,0x220216B9,0x5505262F,
    0xC5BA3BBE,0xB2BD0B28,0x2BB45A92,0x5CB36A04,0xC2D7FFA7,0xB5D0CF31,0x2CD99E8B,0x5BDEAE1D,
    0x9B64C2B0,0xEC63F226,0x756AA39C,0x026D930A,0x9C0906A9,0xEB0E363F,0x72076785,0x05005713,
    0x95BF4A82,0xE2B87A14,0x7BB12BAE,0x0CB61B38,0x92D28E9B,0xE5D5BE0D,0x7CDCEFB7,0x0BDBDF21,
    0x86D3D2D4,0xF1D4E242,0x68DDB3F8,0x1FDA836E,0x81BE16CD,0xF6B9265B,0x6FB077E1,0x18B74777,
    0x88085AE6,0xFF0F6A70,0x66063BCA,0x11010B5C,0x8F659EFF,0xF862AE69,0x616BFFD3,0x166CCF45,
    0xA00AE278,0xD70DD2EE,0x4E048354,0x3903B3C2,0xA7672661,0xD06016F7,0x4969474D,0x3E6E77DB,
    0xAED16A4A,0xD9D65ADC,0x40DF0B66,0x37D83BF0,0xA9BCAE53,0xDEBB9EC5,0x47B2CF7F,0x30B5FFE9,
    0xBDBDF21C,0xCABAC28A,0x53B39330,0x24B4A3A6,0xBAD03605,0xCDD70693,0x54DE5729,0x23D967BF,
    0xB3667A2E,0xC4614AB8,0x5D681B02,0x2A6F2B94,0xB40BBE37,0xC30C8EA1,0x5A05DF1B,0x2D02EF8D
};

/*********************************************************************************************************************/
/*---------------------------------------------Private Functions-----------------------------------------------------*/
/*********************************************************************************************************************/

static uint32 Boot_CRC32(const uint8* data, uint32 length)
{
    uint32 crc = 0xFFFFFFFFu;
    uint32 i;
    for (i = 0; i < length; i++)
    {
        crc = (crc >> 8u) ^ s_crc32Table[(crc ^ (uint32) data[i]) & 0xFFu];
    }
    return crc ^ 0xFFFFFFFFu;
}

/* Incremental CRC32 update (used during TransferData streaming) */
uint32 Boot_CRC32_Update(uint32 crc, const uint8* data, uint32 length)
{
    uint32 i;
    for (i = 0; i < length; i++)
    {
        crc = (crc >> 8u) ^ s_crc32Table[(crc ^ (uint32) data[i]) & 0xFFu];
    }
    return crc;
}

/* Calculate CRC over main flag structure (excluding crc32 field itself) */
static uint32 Boot_CalcFlagCRC(const BootFlagMain_t* flag)
{
    BootFlagMain_t tmp;
    memcpy(&tmp, flag, sizeof(BootFlagMain_t));
    tmp.crc32 = 0u;  /* zero out crc32 field before calculating CRC over the whole struct */
    return Boot_CRC32((const uint8*) &tmp, sizeof(BootFlagMain_t));
}

/* Copy main flag fields to shadow structure */
static void Boot_CopyMainToShadow(const BootFlagMain_t* main, BootFlagShadow_t* shadow)
{
    shadow->shadow_magic = main->magic;
    shadow->shadow_activeBank = main->activeBank;
    shadow->shadow_bankA_valid = main->bankA_valid;
    shadow->shadow_bankB_valid = main->bankB_valid;
    shadow->shadow_bankA_version = main->bankA_version;
    shadow->shadow_bankB_version = main->bankB_version;
    shadow->shadow_bootAttempts = main->bootAttempts;
    shadow->shadow_flags = main->flags;
    shadow->shadow_sequence = main->sequence;
    shadow->shadow_crc32 = main->crc32;
    shadow->shadow_targetWriteBank = main->targetWriteBank;
    shadow->shadow_bankA_codeSize = main->bankA_codeSize;
    shadow->shadow_bankB_codeSize = main->bankB_codeSize;
}

/* Copy shadow flag fields back to main structure */
static void Boot_CopyShadowToMain(const BootFlagShadow_t* shadow, BootFlagMain_t* main)
{
    main->magic = shadow->shadow_magic;
    main->activeBank = shadow->shadow_activeBank;
    main->bankA_valid = shadow->shadow_bankA_valid;
    main->bankB_valid = shadow->shadow_bankB_valid;
    main->bankA_version = shadow->shadow_bankA_version;
    main->bankB_version = shadow->shadow_bankB_version;
    main->bootAttempts = shadow->shadow_bootAttempts;
    main->flags = shadow->shadow_flags;
    main->sequence = shadow->shadow_sequence;
    main->crc32 = shadow->shadow_crc32;
    main->targetWriteBank = shadow->shadow_targetWriteBank;
    main->bankA_codeSize = shadow->shadow_bankA_codeSize;
    main->bankB_codeSize = shadow->shadow_bankB_codeSize;
}
/* Write flags to DFlash with dual-backup strategy:
 * 1. Erase main sector  2. Write main  3. Erase shadow sector  4. Write shadow
 */
static boolean Boot_WriteFlagsToDFlash(const DualBankFlags_t* flags)
{
    DualBankFlags_t writeBuf;
    uint32 i;
    uint32 flagSize;
    uint32 pageCnt;
    uint16 wdtPwd = IfxScuWdt_getCpuWatchdogPassword();

    memcpy(&writeBuf, flags, sizeof(DualBankFlags_t));

    /* Recalculate CRC for main flag */
    writeBuf.main.crc32 = Boot_CalcFlagCRC(&writeBuf.main);
    /* Update shadow to match */
    Boot_CopyMainToShadow(&writeBuf.main, &writeBuf.shadow);

    flagSize = sizeof(DualBankFlags_t);
    pageCnt = (flagSize + DFLASH_PAGE_LENGTH - 1u) / DFLASH_PAGE_LENGTH;

    /* Step 1: Erase DFlash sector (both main and shadow are within same 8KB sector) */
    Flash_eraseDFlash_port(DFLASH_FLAG_ADDR);

    /* Wait for erase complete */
    IfxFlash_waitUnbusy(FLASH_MODULE, IfxFlash_FlashType_D0);
    IfxScuWdt_serviceCpuWatchdog(wdtPwd);

    /* Step 2: Write main flag area */
    Flash_writeDFlash_port(DFLASH_FLAG_ADDR, (uint32*) &writeBuf.main, sizeof(BootFlagMain_t));

    /* Step 3: Write shadow flag area */
    Flash_writeDFlash_port(DFLASH_FLAG_ADDR + DFLASH_FLAG_SHADOW_OFFSET,
        (uint32*) &writeBuf.shadow,
        sizeof(BootFlagShadow_t));

    /* Wait for all DFlash writes to complete before reset */
    IfxFlash_waitUnbusy(FLASH_MODULE, IfxFlash_FlashType_D0);
    IfxScuWdt_serviceCpuWatchdog(wdtPwd);

    /* Step 4: Verify by reading back */
    {
        DualBankFlags_t verify;
        if (Boot_DualBank_ReadFlags(&verify) == TRUE)
        {
            if (memcmp(&verify.main, &writeBuf.main, sizeof(BootFlagMain_t)) == 0)
            {
                return TRUE;
            }
        }
    }
    return FALSE;
}

/*********************************************************************************************************************/
/*---------------------------------------------Public Functions------------------------------------------------------*/
/*********************************************************************************************************************/

/**
 * @brief Initialize dual bank flag system.
 * @note  If flags are corrupted or first boot, initialize with defaults.
 */
void Boot_DualBank_Init(void)
{
    DualBankFlags_t flags;
    g_bootPhase = BOOT_PHASE_FLAG_INIT;

    if (Boot_DualBank_ReadFlags(&flags) == FALSE)
    {
        /* First boot or flag area corrupted: initialize defaults */
        memset(&flags, 0, sizeof(DualBankFlags_t));
        flags.main.magic = FLAG_MAGIC;
        flags.main.activeBank = BANK_A;
        flags.main.bankA_valid = 0u;
        flags.main.bankB_valid = 0u;
        flags.main.bankA_version = 0u;
        flags.main.bankB_version = 0u;
        flags.main.bootAttempts = 0u;
        flags.main.flags = 0u;
        flags.main.sequence = FLAG_SEQUENCE_INIT;
        flags.main.crc32 = 0u;
        flags.main.targetWriteBank = BANK_B;  /* 默认写入 Bank B */

        Boot_CopyMainToShadow(&flags.main, &flags.shadow);
        Boot_WriteFlagsToDFlash(&flags);
    }

    g_activeBank = flags.main.activeBank;
}

/**
 * @brief Read dual bank flags from DFlash with redundancy check.
 * @return TRUE if valid flags read, FALSE if both main and shadow corrupted.
 */
boolean Boot_DualBank_ReadFlags(DualBankFlags_t* flags)
{
    const BootFlagMain_t* pMain = (const BootFlagMain_t*) DFLASH_FLAG_ADDR;
    const BootFlagShadow_t* pShadow = (const BootFlagShadow_t*) (DFLASH_FLAG_ADDR + DFLASH_FLAG_SHADOW_OFFSET);

    boolean mainOk = FALSE;
    boolean shadowOk = FALSE;
    uint32 mainCRC, shadowCRC;

    /* Check main flag */
    if (pMain->magic == FLAG_MAGIC)
    {
        mainCRC = Boot_CalcFlagCRC(pMain);
        if (mainCRC == pMain->crc32)
        {
            mainOk = TRUE;
        }
    }

    /* Check shadow flag */
    if (pShadow->shadow_magic == FLAG_MAGIC)
    {
        /* Reconstruct main from shadow to verify CRC */
        BootFlagMain_t recon;
        Boot_CopyShadowToMain(pShadow, &recon);
        mainCRC = Boot_CalcFlagCRC(&recon);
        if (mainCRC == recon.crc32)
        {
            shadowOk = TRUE;
        }
    }

    if (mainOk && shadowOk)
    {
        /* Both valid: use the one with larger sequence number */
        if (pMain->sequence >= pShadow->shadow_sequence)
        {
            memcpy(&flags->main, pMain, sizeof(BootFlagMain_t));
            memcpy(&flags->shadow, pShadow, sizeof(BootFlagShadow_t));
        }
        else
        {
            Boot_CopyShadowToMain(pShadow, &flags->main);
            memcpy(&flags->shadow, pShadow, sizeof(BootFlagShadow_t));
        }
        return TRUE;
    }
    else if (mainOk)
    {
        memcpy(&flags->main, pMain, sizeof(BootFlagMain_t));
        Boot_CopyMainToShadow(&flags->main, &flags->shadow);
        return TRUE;
    }
    else if (shadowOk)
    {
        Boot_CopyShadowToMain(pShadow, &flags->main);
        memcpy(&flags->shadow, pShadow, sizeof(BootFlagShadow_t));
        return TRUE;
    }

    return FALSE;
}

/**
 * @brief Write dual bank flags to DFlash.
 * @return TRUE if write successful, FALSE otherwise.
 */
boolean Boot_DualBank_WriteFlags(const DualBankFlags_t* flags)
{
    return Boot_WriteFlagsToDFlash(flags);
}

/**
 * @brief Calculate CRC32 over a memory region.
 * @note Uses uncached address and cache sync to avoid stale data after erase/write.
 */
uint32 Boot_DualBank_CalculateCRC(uint32 startAddr, uint32 size)
{
    uint32 uncachedAddr = (startAddr & 0x00FFFFFFu) | 0xA0000000u;
    __dsync();
    return Boot_CRC32((const uint8*) uncachedAddr, size);
}

/**
 * @brief Verify if a bank contains valid application code.
 * @return BANK_STATUS_VALID if CRC matches, BANK_STATUS_INVALID if mismatch or not marked, BANK_STATUS_UNKNOWN if flags unreadable.
 */
BankStatus_t Boot_DualBank_VerifyBank(uint32 bank)
{
    DualBankFlags_t flags;
    uint32 startAddr;
    uint32 validFlag;

    if (bank == BANK_A)
    {
        startAddr = BANK_A_START_ADDR;
    }
    else if (bank == BANK_B)
    {
        startAddr = BANK_B_START_ADDR;
    }
    else
    {
        return BANK_STATUS_UNKNOWN;
    }

    if (Boot_DualBank_ReadFlags(&flags) == FALSE)
    {
        return BANK_STATUS_UNKNOWN;
    }

    validFlag = (bank == BANK_A) ? flags.main.bankA_valid : flags.main.bankB_valid;

    /* Check if bank was ever marked valid (non-zero CRC stored) */
    if (validFlag == 0u)
    {
        return BANK_STATUS_INVALID;
    }

    /* Use actual code size from flags if available, otherwise fall back to full bank size */
    uint32 codeSize = (bank == BANK_A) ? flags.main.bankA_codeSize : flags.main.bankB_codeSize;
    if (codeSize == 0u)
    {
        codeSize = (bank == BANK_A) ? BANK_APP_A_SIZE : BANK_APP_B_SIZE;
    }

    /* Calculate actual CRC of bank content over actual code size */
    uint32 actualCRC = Boot_DualBank_CalculateCRC(startAddr, codeSize);
    if (actualCRC == validFlag)
    {
        return BANK_STATUS_VALID;
    }

    return BANK_STATUS_INVALID;
}

/**
 * @brief Verify bank by comparing externally supplied CRC32 with internally calculated one.
 * @param bank        Target bank (BANK_A or BANK_B).
 * @param expectedCrc CRC32 value calculated by the host (must match Boot_CRC32 result).
 * @return BANK_STATUS_VALID if CRC matches, BANK_STATUS_INVALID otherwise.
 */
BankStatus_t Boot_DualBank_VerifyBankWithCrc(uint32 bank, uint32 expectedCrc)
{
    uint32 startAddr;
    uint32 bankSize;

    if (bank == BANK_A)
    {
        startAddr = BANK_A_START_ADDR;
        bankSize = BANK_APP_A_SIZE;
    }
    else if (bank == BANK_B)
    {
        startAddr = BANK_B_START_ADDR;
        bankSize = BANK_APP_B_SIZE;
    }
    else
    {
        return BANK_STATUS_UNKNOWN;
    }

    uint32 actualCRC = Boot_DualBank_CalculateCRC(startAddr, bankSize);

    if (actualCRC == expectedCrc)
    {
        return BANK_STATUS_VALID;
    }

    return BANK_STATUS_INVALID;
}

/**
 * @brief Verify bank with RSA-2048 signature + SHA-256.
 * @param bank        Target bank (BANK_A or BANK_B).
 * @param expectedCrc CRC32 value calculated by the host.
 * @param signature   256-byte RSA signature (big-endian).
 * @param sigLen      Signature length (must be 256).
 * @return BANK_STATUS_VALID if both CRC and signature match.
 */
BankStatus_t Boot_DualBank_VerifyBankWithSignature(uint32 bank, uint32 expectedCrc,
                                                   uint32 codeSize, const uint8 *signature, uint32 sigLen)
{
    uint32 startAddr;
    uint8  hash[SHA256_HASH_SIZE];

    if (bank == BANK_A)
    {
        startAddr = BANK_A_START_ADDR;
    }
    else if (bank == BANK_B)
    {
        startAddr = BANK_B_START_ADDR;
    }
    else
    {
        return BANK_STATUS_UNKNOWN;
    }

    /* Step 1: CRC verification over actual downloaded payload */
    uint32 actualCRC = Boot_DualBank_CalculateCRC(startAddr, codeSize);
    if (actualCRC != expectedCrc)
    {
        return BANK_STATUS_INVALID;
    }

    /* Step 2: SHA-256 over actual downloaded payload (uncached to avoid DCache staleness) */
    uint32 uncachedAddr = (startAddr & 0x00FFFFFFu) | 0xA0000000u;
    __dsync();
    sha256((const uint8 *)uncachedAddr, codeSize, hash);

    /* Step 3: RSA-2048 signature verification */
    if (sigLen != RSA2048_SIG_LEN)
    {
        return BANK_STATUS_INVALID;
    }

    if (rsa2048_verify(signature, hash, rsa_public_modulus, rsa_public_exponent) != 1)
    {
        return BANK_STATUS_INVALID;
    }

    return BANK_STATUS_VALID;
}

/**
 * @brief Invalidate a bank (clear its valid flag and CRC).
 */
void Boot_DualBank_InvalidateBank(uint32 bank)
{
    DualBankFlags_t flags;
    if (Boot_DualBank_ReadFlags(&flags) == TRUE)
    {
        if (bank == BANK_A)
        {
            flags.main.bankA_valid = 0u;
        }
        else
        {
            flags.main.bankB_valid = 0u;
        }
        flags.main.sequence++;
        Boot_DualBank_WriteFlags(&flags);
    }
}

/**
 * @brief Mark a bank as valid after successful programming.
 * @param bank    BANK_A or BANK_B
 * @param version Application version number
 * @param codeSize Actual downloaded code size in bytes (0 = use full bank size)
 */
void Boot_DualBank_MarkBankValid(uint32 bank, uint32 version, uint32 codeSize)
{
    DualBankFlags_t flags;
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

    /* Use provided code size if available, otherwise fall back to full bank size */
    if (codeSize == 0u)
    {
        codeSize = (bank == BANK_A) ? BANK_APP_A_SIZE : BANK_APP_B_SIZE;
    }

    crc = Boot_DualBank_CalculateCRC(startAddr, codeSize);

    if (Boot_DualBank_ReadFlags(&flags) == TRUE)
    {
        if (bank == BANK_A)
        {
            flags.main.bankA_valid = crc;
            flags.main.bankA_version = version;
            flags.main.bankA_codeSize = codeSize;
        }
        else
        {
            flags.main.bankB_valid = crc;
            flags.main.bankB_version = version;
            flags.main.bankB_codeSize = codeSize;
        }
        flags.main.sequence++;
        Boot_DualBank_WriteFlags(&flags);
    }
}

/**
 * @brief Jump to specified bank's application entry point.
 * @note  This function does NOT return if jump is successful.
 *        It reads the vector table at bank start, sets MSP, and jumps to Reset_Handler.
 */
 /**
  * @brief Set CPU vector tables (BIV/BTV) to APP's vector table base.
  * @note  OEM requirement: before jumping to APP, Bootloader must restore
  *        CPU vector table pointers so that interrupts/traps in APP land
  *        in APP's handlers, not Bootloader's.
  *        INTTAB/TRAPTAB offsets must match APP linker configuration.
  */
void Boot_DualBank_SetAppVectors(uint32 bankStartAddr)
{
    uint32 appBiv;
    uint32 appBtv;

    /* TriCore BIV for VSS=0 (32-byte vector spacing):
     * BIV = (INTTAB_base | 0x1FE0).
     * See IfxCpu_CStart0.c / CompilerTasking.h for project convention.
     */
    appBiv = (bankStartAddr + APP_INTTAB_OFFSET) | 0x1FE0u;
    appBtv = (bankStartAddr + APP_TRAPTAB_OFFSET);

    __mtcr(CPU_BIV, appBiv);
    __isync();
    __mtcr(CPU_BTV, appBtv);
    __isync();
}

void Boot_DualBank_JumpToBank(uint32 bank)
{
    uint32 startAddr;
    uint32 entryAddr;

    g_bootPhase = BOOT_PHASE_JUMP_EXEC;

    if (bank == BANK_A)
    {
        startAddr = BANK_A_START_ADDR;   /* 0x80020000 cached */
    }
    else
    {
        startAddr = BANK_B_START_ADDR;   /* 0x80100000 cached */
    }

    /* Basic sanity check: _START should not be 0xFFFFFFFF or 0x00000000 */
    if ((*(volatile uint32*) (startAddr + 0x20u) == 0xFFFFFFFFu) ||
        (*(volatile uint32*) (startAddr + 0x20u) == 0x00000000u))
    {
        g_bootPhase = BOOT_PHASE_BL_ENTRY;
        return; /* Invalid entry point, do not jump */
    }

    /* Disable interrupts before jumping */
    IfxCpu_disableInterrupts();
    __dsync();  /* Ensure interrupt disable completes before subsequent ops */

    /* Deinitialize peripherals to leave a clean state for APP */
    /* TODO: Add CAN/TMR de-init if needed. Keep commented until correct API is identified. */
    /* CAN_deInit(); */
    /* TMR_deInit(); */
//    CAN_deinit();

    /* ========== Disable ECC Trap to prevent spurious ECC traps on freshly written Flash ========== */
//    uint16 pwd = IfxScuWdt_getSafetyWatchdogPassword();
//    IfxScuWdt_clearSafetyEndinit(pwd);
//    FLASH0_MARP.B.TRAPDIS = 1;
//    FLASH0_MARD.B.TRAPDIS = 1;
//    IfxScuWdt_setSafetyEndinit(pwd);
    uint16 pwd = IfxScuWdt_getCpuWatchdogPassword();
    IfxScuWdt_clearCpuEndinit(pwd);
    FLASH0_MARP.B.TRAPDIS = 1;
    FLASH0_MARD.B.TRAPDIS = 1;
    IfxScuWdt_setCpuEndinit(pwd);

    /* ========== Disable Data Cache and Program Cache before jump ========== */
    IfxCpu_setDataCache(0);      /* Disable data cache */
    IfxCpu_setProgramCache(0);   /* Disable program cache (critical) */

    __dsync();                   /* Data synchronization barrier */
    __isync();                   /* Instruction synchronization barrier */

    /* TriCore: _START is at offset 0x20 from bank base (BMHD occupies 0x00~0x1F) */
    /* Use uncached address (0xA0xxxxxx) to avoid cache coherency issues after flash write */
    {
        uint32 uncachedStart = (startAddr & 0x00FFFFFFu) | 0xA0000000u;
        entryAddr = uncachedStart + 0x20u;
    }

    /* === OEM: Set APP vector tables (BIV/BTV) before jump === */
    Boot_DualBank_SetAppVectors(startAddr);

    /*
     * CRITICAL FIX: Do NOT use C function call (appEntry()).
     * TriCore 'call' instruction implicitly saves upper context into CSA,
     * updates PCXI, A11 (RA) and PSW.CDC. If Bootloader's context is left
     * behind, APP's function returns will corrupt the CSA list and access
     * invalid CSA addresses (e.g. 0x466 in PSPR), causing Bus Error Trap.
     *
     * Correct approach: perform a raw jump (ji) and manually cut the context
     * link chain by clearing PCXI, so APP's _start runs as if from reset.
     */
     /* Load entryAddr to A15 */
    __asm("mov   d15, %0" : : "d"(entryAddr) : "d15");
    __asm("mov.a a15, d15" : : : "a15");

    /* Clear PCXI: cut context chain from Bootloader */
    __mtcr(CPU_PCXI, 0);
    __isync();

    /* Clear Return Address (A11) */
    __asm("mov.a a11, #0" : : : "a11");

    /* Jump indirect: no context save, no link, no RA */
    __asm("ji    a15" : : : "a15");



    /* Should never reach here */
    while (1) {};
}


/**
 * @brief Select active bank based on flags, verify, handle boot attempts and rollback.
 * @note  If a valid bank is found, this function jumps and does NOT return.
 *        If both banks are invalid, it returns so Bootloader can stay for re-flash.
 */
void Boot_DualBank_SelectAndJump(void)
{
    DualBankFlags_t flags;
    uint32 targetBank;
    uint32 fallbackBank;
    BankStatus_t targetStatus;
    BankStatus_t fallbackStatus;

    g_bootPhase = BOOT_PHASE_BANK_VERIFY;

    if (Boot_DualBank_ReadFlags(&flags) == FALSE)
    {
        /* Flags corrupted or DFlash erased: stay in bootloader */
        return;
    }

    targetBank = flags.main.activeBank;

    /* Sanity check */
    if ((targetBank != BANK_A) && (targetBank != BANK_B))
    {
        return;
    }

    fallbackBank = (targetBank == BANK_A) ? BANK_B : BANK_A;

    targetStatus = Boot_DualBank_VerifyBank(targetBank);

    if (targetStatus == BANK_STATUS_VALID)
    {
        g_bootPhase = BOOT_PHASE_JUMP_DECISION;

        /* Increment boot attempts before jumping. If APP crashes before
         * Boot_DualBank_ClearBootAttempts(), counter persists and on next
         * boot we roll back after MAX_BOOT_ATTEMPTS consecutive failures. */
        flags.main.bootAttempts++;
        flags.main.sequence++;
        Boot_DualBank_WriteFlags(&flags);

        if (flags.main.bootAttempts >= MAX_BOOT_ATTEMPTS)
        {
            g_bootPhase = BOOT_PHASE_ROLLBACK;

            /* Stage 2 pass check: if the Bank has previously proven stable,
             * treat this as a runtime fault rather than boot failure.
             * Clear bootAttempts and give the Bank another chance. */
            if ((flags.main.flags & BOOT_FLAG_STAGE2_PASS) != 0)
            {
                flags.main.bootAttempts = 0u;
                flags.main.sequence++;
                Boot_DualBank_WriteFlags(&flags);
                Boot_DualBank_JumpToBank(targetBank);
                return;
            }

            /* Too many consecutive boot failures without stage 2 pass:
             * invalidate target and try fallback */
            Boot_DualBank_InvalidateBank(targetBank);
            fallbackStatus = Boot_DualBank_VerifyBank(fallbackBank);
            if (fallbackStatus == BANK_STATUS_VALID)
            {
                flags.main.activeBank = fallbackBank;
                flags.main.bootAttempts = 0u;
                flags.main.flags = 0u;  /* clear stage flags for new Bank */
                flags.main.sequence++;
                Boot_DualBank_WriteFlags(&flags);
                SW_Reset();
            }
            else
            {
                /* Both invalid: stay in bootloader */
                g_bootPhase = BOOT_PHASE_BL_ENTRY;
                return;
            }
        }
        else
        {
            Boot_DualBank_JumpToBank(targetBank);
            return;
        }
    }
    else
    {
        g_bootPhase = BOOT_PHASE_ROLLBACK;

        fallbackStatus = Boot_DualBank_VerifyBank(fallbackBank);
        if (fallbackStatus == BANK_STATUS_VALID)
        {
            /* Switch to fallback */
            flags.main.activeBank = fallbackBank;
            flags.main.bootAttempts = 0u;
            flags.main.sequence++;
            Boot_DualBank_WriteFlags(&flags);
            SW_Reset();
        }
        else
        {
            /* Both invalid: stay in bootloader */
            g_bootPhase = BOOT_PHASE_BL_ENTRY;
            return;
        }
    }
}

/**
 * @brief Switch active bank to target and trigger software reset.
 * @note  Typically called after successful flash programming to new bank.
 */
void Boot_DualBank_SwitchBank(uint32 targetBank)
{
    DualBankFlags_t flags;
    if (Boot_DualBank_ReadFlags(&flags) == TRUE)
    {
        flags.main.activeBank = targetBank;
        flags.main.bootAttempts = 0u;
        flags.main.flags = 0u;  /* clear stage flags for fresh start */
        flags.main.sequence++;
        Boot_DualBank_WriteFlags(&flags);

        SW_Reset();
    }
}

/**
 * @brief Set active bank flag without triggering reset.
 * @note  Used in UDS CheckProgrammingDependency (0x31 01 0203) to prepare
 *        for subsequent ECU reset (0x11 03). The new bank takes effect
 *        after the next reset.
 */
void Boot_DualBank_SetActiveBank(uint32 targetBank)
{
    DualBankFlags_t flags;
    if (Boot_DualBank_ReadFlags(&flags) == TRUE)
    {
        flags.main.activeBank = targetBank;
        flags.main.bootAttempts = 0u;
        flags.main.flags = 0u;  /* clear stage flags for fresh start */
        flags.main.sequence++;
        Boot_DualBank_WriteFlags(&flags);
        g_activeBank = targetBank;
    }
}

/**
 * @brief Get currently active bank from cached value.
 */
uint32 Boot_DualBank_GetActiveBank(void)
{
    return g_activeBank;
}

/**
 * @brief Clear boot attempt counter after successful APP startup.
 * @note  Called by APP in its early initialization (before any risky operation).
 *        Resets the counter to 0 so Bootloader knows the last boot was successful.
 *        If APP crashes before calling this, the counter persists and Bootloader
 *        will increment it on next boot, eventually triggering rollback after
 *        MAX_BOOT_ATTEMPTS consecutive failures.
 */
void Boot_DualBank_ClearBootAttempts(void)
{
    DualBankFlags_t flags;
    if (Boot_DualBank_ReadFlags(&flags) == TRUE)
    {
        flags.main.bootAttempts = 0u;
        flags.main.sequence++;
        Boot_DualBank_WriteFlags(&flags);
    }
}

/**
 * @brief Mark that App has completed stage 1 (peripheral initialization).
 * @note  Called by APP after all risky peripheral init is done and interrupts
 *        are enabled. Sets BOOT_FLAG_STAGE1_PASS in flags.
 */
void Boot_DualBank_MarkStage1Pass(void)
{
    DualBankFlags_t flags;
    if (Boot_DualBank_ReadFlags(&flags) == TRUE)
    {
        flags.main.flags |= BOOT_FLAG_STAGE1_PASS;
        flags.main.sequence++;
        Boot_DualBank_WriteFlags(&flags);
    }
}

/**
 * @brief Mark that App has completed stage 2 (stable main loop running).
 * @note  Called by APP after main loop has been running for a configurable
 *        period (e.g. 5 seconds) without crashes. Sets BOOT_FLAG_STAGE2_PASS.
 *        Once stage 2 is passed, Bootloader treats subsequent resets as
 *        runtime faults rather than boot failures, giving the Bank extra
 *        chances before rollback.
 */
void Boot_DualBank_MarkStage2Pass(void)
{
    DualBankFlags_t flags;
    if (Boot_DualBank_ReadFlags(&flags) == TRUE)
    {
        flags.main.flags |= BOOT_FLAG_STAGE2_PASS;
        flags.main.sequence++;
        Boot_DualBank_WriteFlags(&flags);
    }
}

/**
 * @brief Get target write bank from DFlash flags.
 * @return BANK_A or BANK_B
 */
uint32 Boot_DualBank_GetTargetWriteBank(void)
{
    DualBankFlags_t flags;
    if (Boot_DualBank_ReadFlags(&flags) == TRUE)
    {
        return flags.main.targetWriteBank;
    }
    return BANK_B; /* default */
}

/**
 * @brief Set target write bank to DFlash flags.
 * @note  Typically called by App/UDS before flashing.
 */
void Boot_DualBank_SetTargetWriteBank(uint32 bank)
{
    DualBankFlags_t flags;
    if (Boot_DualBank_ReadFlags(&flags) == TRUE)
    {
        if ((bank == BANK_A) || (bank == BANK_B))
        {
            flags.main.targetWriteBank = bank;
            flags.main.sequence++;
            Boot_DualBank_WriteFlags(&flags);
        }
    }
}
