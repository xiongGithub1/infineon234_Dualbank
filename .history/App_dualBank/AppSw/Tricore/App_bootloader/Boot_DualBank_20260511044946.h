
/**********************************************************************************************************************
 * \file    Boot_DualBank.h
 * \brief   A/B Dual Bank Boot Management for TC234
 * \version V2.0.0
 *********************************************************************************************************************/

#ifndef BOOT_DUALBANK_H_
#define BOOT_DUALBANK_H_

#include "Std_Types.h"
#include "IfxFlash.h"

 /* DFlash flag area */
#define DFLASH_FLAG_ADDR                0xAF000000u
#define DFLASH_FLAG_SHADOW_OFFSET       0x100u
#define DFLASH_FLAG_SIZE                80u

#define FLAG_MAGIC                      0x5A5AA5A5u
#define BANK_VALID_MAGIC                0x55AA55AAu
#define FLAG_SEQUENCE_INIT              0x00000001u

#define BANK_A                          0u
#define BANK_B                          1u
#define MAX_BOOT_ATTEMPTS               3u

/* Bank physical address */
#define BANK_A_START_ADDR               0x80020000u
#define BANK_A_END_ADDR                 0x80100000u
#define BANK_B_START_ADDR               0x80100000u
#define BANK_B_END_ADDR                 0x80200000u
#define BANK_APP_SIZE                   (896u * 1024u)  /* S8~S22 = 896KB */

/* Bank logical sector range (matches IfxFlash_pFlashTableLog index) */
#define BANK_A_SECTOR_START             8u
#define BANK_A_SECTOR_END               22u
#define BANK_B_SECTOR_START             23u
#define BANK_B_SECTOR_END               26u
#define BOOTLOADER_SECTOR_START         0u
#define BOOTLOADER_SECTOR_END           7u
#define BOOTLOADER_SECTOR_MAX           7u

/* Erase time measurement structure (debug/test only) */
typedef struct
{
    uint32 sectorIndex;   /* Logical sector number (8~22) */
    uint32 sectorSize;    /* Sector size in bytes */
    uint32 eraseTimeMs;   /* Erase time in milliseconds */
    uint32 result;        /* 0=success, 1=fail */
} EraseTimeLog_t;

extern EraseTimeLog_t g_eraseTimeLog[];
extern uint32 g_eraseTotalTimeMs;
extern uint32 g_eraseTestDone;

/* Rollback reason */
#define ROLLBACK_REASON_NONE            0x00u
#define ROLLBACK_REASON_VERIFY_FAIL     0x01u
#define ROLLBACK_REASON_BOOT_TIMEOUT    0x02u
#define ROLLBACK_REASON_TRAP            0x03u
#define ROLLBACK_REASON_WATCHDOG        0x04u

typedef struct
{
    uint32 magic;
    uint32 activeBank;
    uint32 bankA_valid;
    uint32 bankB_valid;
    uint32 bankA_version;
    uint32 bankB_version;
    uint16 bootAttempts;
    uint16 flags;
    uint32 sequence;        // +0x1C  (4 bytes)  写入序列号（用于主/备仲裁）
    uint32 crc32;           // +0x20  (4 bytes)  本结构体的 CRC32（不含此字段）
    uint32 targetWriteBank; // +0x24  (4 bytes)  上位机目标刷写Bank (BANK_A / BANK_B)
} BootFlagMain_t;

typedef struct
{
    uint32 shadow_magic;
    uint32 shadow_activeBank;
    uint32 shadow_bankA_valid;
    uint32 shadow_bankB_valid;
    uint32 shadow_bankA_version;
    uint32 shadow_bankB_version;
    uint16 shadow_bootAttempts;
    uint16 shadow_flags;
    uint32 shadow_sequence;        // 4 bytes
    uint32 shadow_crc32;           // 4 bytes
    uint32 shadow_targetWriteBank; // 4 bytes
} BootFlagShadow_t;                // = 40 bytes 总计



typedef struct
{
    BootFlagMain_t  main;
    BootFlagShadow_t shadow;
} DualBankFlags_t;

typedef enum
{
    BANK_STATUS_INVALID = 0,
    BANK_STATUS_VALID,
    BANK_STATUS_UNKNOWN
} BankStatus_t;




void Boot_DualBank_Init(void);
boolean Boot_DualBank_ReadFlags(DualBankFlags_t* flags);
boolean Boot_DualBank_WriteFlags(const DualBankFlags_t* flags);
void Boot_DualBank_SelectAndJump(void);
BankStatus_t Boot_DualBank_VerifyBank(uint32 bank);
BankStatus_t Boot_DualBank_VerifyBankWithCrc(uint32 bank, uint32 expectedCrc);
uint32 Boot_DualBank_CalculateCRC(uint32 startAddr, uint32 size);
void Boot_DualBank_InvalidateBank(uint32 bank);
void Boot_DualBank_MarkBankValid(uint32 bank, uint32 version);
void Boot_DualBank_SwitchBank(uint32 targetBank);
void Boot_DualBank_SetActiveBank(uint32 targetBank);
uint32 Boot_DualBank_GetActiveBank(void);
void Boot_DualBank_JumpToBank(uint32 bank);
uint32 Boot_DualBank_GetTargetWriteBank(void);
void   Boot_DualBank_SetTargetWriteBank(uint32 bank);
;
/**
 * @brief Clear boot attempt counter after successful APP startup.
 * @note  Must be called by APP in its early initialization phase.
 *        If APP crashes (trap/WDT) before calling this, the counter
 *        remains non-zero and Bootloader will increment it on next boot.
 *        After MAX_BOOT_ATTEMPTS consecutive failures, Bootloader rolls
 *        back to the fallback bank.
 */
void Boot_DualBank_ClearBootAttempts(void);

/* Debug: measure erase time for Bank A (S8~S22) */
void MeasureEraseBankA_Time(void);

#endif
