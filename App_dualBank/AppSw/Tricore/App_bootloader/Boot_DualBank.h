
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

/* Boot stability stage flags (stored in BootFlagMain_t.flags) */
#define BOOT_FLAG_STAGE1_PASS           (1u << 0u)  /* App completed peripheral init */
#define BOOT_FLAG_STAGE2_PASS           (1u << 1u)  /* App main loop stable for N seconds */

/* Bank physical address */
#define BANK_A_START_ADDR               0x80020000u
#define BANK_A_END_ADDR                 0x80100000u
#define BANK_B_START_ADDR               0x80100000u
#define BANK_B_END_ADDR                 0x80200000u
#define BANK_APP_SIZE                   (896u * 1024u)  /* S8~S22 = 896KB */

/* APP vector table offsets relative to bank base (must match APP LSL/linker config)
 * Verified against App_dualBank_a.lsl / App_dualbank_b.lsl:
 *   Bank A: INTTAB=0x80090000, TRAPTAB=0x80098000 (base=0x80020000)
 *   Bank B: INTTAB=0x80170000, TRAPTAB=0x80178000 (base=0x80100000)
 */
#define APP_INTTAB_OFFSET               0x00070000u
#define APP_TRAPTAB_OFFSET              0x00078000u

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

/* Boot phase identifiers for OEM traceability and fault analysis */
typedef enum
{
    BOOT_PHASE_STARTUP          = 0x00u,   /* Phase 1: System startup (watchdog/clock) */
    BOOT_PHASE_FLAG_INIT        = 0x01u,   /* Phase 2: Dual-bank flag initialization */
    BOOT_PHASE_BANK_VERIFY      = 0x02u,   /* Phase 3: Bank validity verification */
    BOOT_PHASE_JUMP_DECISION    = 0x03u,   /* Phase 4A: Jump decision */
    BOOT_PHASE_JUMP_EXEC        = 0x04u,   /* Phase 4A-Jump: Bare jump to APP */
    BOOT_PHASE_ROLLBACK         = 0x05u,   /* Phase 4B: Rollback handling */
    BOOT_PHASE_BL_ENTRY         = 0x10u,   /* Phase 5: Bootloader mode entry */
    BOOT_PHASE_BL_MAIN          = 0x11u,   /* Phase 6: Bootloader main loop */
    BOOT_PHASE_PROG_SESSION     = 0x20u,   /* Phase 7: Programming session */
    BOOT_PHASE_PROG_VERIFY      = 0x21u,   /* Phase 7.6: Post-programming CRC verify */
    BOOT_PHASE_ERROR            = 0xFFu    /* Unrecoverable error / trap */
} BootPhase_t;

extern volatile BootPhase_t g_bootPhase;

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
    uint32 sequence;        // +0x1C  (4 bytes)  д�����кţ�������/���ٲã�
    uint32 crc32;           // +0x20  (4 bytes)  ���ṹ��� CRC32���������ֶΣ�
    uint32 targetWriteBank; // +0x24  (4 bytes)  ��λ��Ŀ��ˢдBank (BANK_A / BANK_B)
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
} BootFlagShadow_t;                // = 40 bytes �ܼ�



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
uint32 Boot_CRC32_Update(uint32 crc, const uint8 *data, uint32 length);
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
void Boot_DualBank_MarkStage1Pass(void);
void Boot_DualBank_MarkStage2Pass(void);

/* Debug: measure erase time for Bank A (S8~S22) */
void MeasureEraseBankA_Time(void);

/* OEM helper: set BIV/BTV/SP to APP vector table before jump */
void Boot_DualBank_SetAppVectors(uint32 bankStartAddr);

#endif
