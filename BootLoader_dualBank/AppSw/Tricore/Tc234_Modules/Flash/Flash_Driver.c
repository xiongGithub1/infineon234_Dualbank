/**********************************************************************************************************************
 * \file    Flash_Driver.c
 * \brief   Core Flash Driver - OEM Standard Hardware Abstraction Layer
 * \version V1.0.0
 * \date    2026-05-13
 *
 * Architecture:
 *  - This layer is state-free: no download state, no alignment buffer.
 *  - Upper layer (fls_app.c) is responsible for:
 *      * Address / length alignment
 *      * Cross-page remainder buffering
 *      * UDS state machine and NRC mapping
 *  - All Flash operations run from PSPR to avoid RWW conflicts.
 *********************************************************************************************************************/

/*********************************************************************************************************************/
/*-----------------------------------------------------Includes------------------------------------------------------*/
/*********************************************************************************************************************/
#include <string.h>
#include "Flash_Driver.h"
#include "IfxFlash.h"
#include "IfxCpu.h"
#include "IfxScuWdt.h"

/*********************************************************************************************************************/
/*------------------------------------------------------Macros-------------------------------------------------------*/
/*********************************************************************************************************************/

#define PFLASH_PAGE_SIZE            IFXFLASH_PFLASH_PAGE_LENGTH   /* 32 bytes */
#define DFLASH_PAGE_SIZE            IFXFLASH_DFLASH_PAGE_LENGTH   /* 8 bytes  */

/* PSPR layout for relocated routines */
#define FDRV_ERASESECTOR_LEN        (0x100U)
#define FDRV_WAITUNBUSY_LEN         (0x100U)
#define FDRV_ENTERPAGEMODE_LEN      (0x100U)
#define FDRV_LOADPAGE2X32_LEN       (0x100U)
#define FDRV_WRITEPAGE_LEN          (0x100U)

#define FDRV_ERASESECTOR_ADDR       (FLASH_DRV_PSPR_BASE)
#define FDRV_WAITUNBUSY_ADDR        (FDRV_ERASESECTOR_ADDR  + FDRV_ERASESECTOR_LEN)
#define FDRV_ENTERPAGEMODE_ADDR     (FDRV_WAITUNBUSY_ADDR   + FDRV_WAITUNBUSY_LEN)
#define FDRV_LOADPAGE2X32_ADDR      (FDRV_ENTERPAGEMODE_ADDR + FDRV_ENTERPAGEMODE_LEN)
#define FDRV_WRITEPAGE_ADDR         (FDRV_LOADPAGE2X32_ADDR + FDRV_LOADPAGE2X32_LEN)

/* Address range checks.
 * TC234 PFlash has two views:
 *   Cached:   0x80000000 ~ 0x801FFFFF
 *   Uncached: 0xA0000000 ~ 0xA01FFFFF  (used by IfxFlash_pFlashTableLog)
 * DFlash:
 *   0xAF000000 ~ 0xAF01FFFF
 */
#define PFLASH_CACHED_BASE          (0x80000000U)
#define PFLASH_CACHED_TOP           (0x80200000U)
#define PFLASH_UNCACHED_BASE        (0xA0000000U)
#define PFLASH_UNCACHED_TOP         (0xA0200000U)
#define DFLASH_BASE                 (0xAF000000U)
#define DFLASH_TOP                  (0xAF020000U)

#define IS_PFLASH_ADDR(addr)        (((addr) >= PFLASH_CACHED_BASE && (addr) < PFLASH_CACHED_TOP) || \
                                     ((addr) >= PFLASH_UNCACHED_BASE && (addr) < PFLASH_UNCACHED_TOP))

/*********************************************************************************************************************/
/*-------------------------------------------------Data Structures---------------------------------------------------*/
/*********************************************************************************************************************/

typedef struct
{
    void     (*eraseSectors)(uint32 sectorAddr, uint32 numSectors);
    uint8    (*waitUnbusy)(uint32 flash, IfxFlash_FlashType flashType);
    uint8    (*enterPageMode)(uint32 pageAddr);
    void     (*load2X32bits)(uint32 pageAddr, uint32 wordL, uint32 wordU);
    void     (*writePage)(uint32 pageAddr);
} Flash_Drv_PSPR_Funcs_t;

/*********************************************************************************************************************/
/*-------------------------------------------------Static variables--------------------------------------------------*/
/*********************************************************************************************************************/

static Flash_Drv_PSPR_Funcs_t s_pspr;

/*********************************************************************************************************************/
/*---------------------------------------------Static Helper Functions-----------------------------------------------*/
/*********************************************************************************************************************/

/**
 * \brief  Copy a routine from Flash into PSRAM so it can be executed during RWW.
 */
static void fdrv_relocateRoutine(const void *src, void *dst, uint32 len)
{
    memcpy(dst, src, len);
}

/**
 * \brief  Poll FSR until flash is idle or timeout.
 * \return FLASH_DRV_OK or FLASH_DRV_ERR_TIMEOUT
 */
static Flash_Drv_Result_t fdrv_waitForIdle(IfxFlash_FlashType flashType)
{
    volatile uint32 timeout = FLASH_DRV_TIMEOUT;
    while (timeout > 0U)
    {
        if (s_pspr.waitUnbusy(FLASH_MODULE, flashType) == 0U)
        {
            return FLASH_DRV_OK;
        }
        timeout--;
    }
    return FLASH_DRV_ERR_TIMEOUT;
}

/**
 * \brief  Verify that written data matches expected data.
 * \return FLASH_DRV_OK or FLASH_DRV_ERR_VERIFY
 */
static Flash_Drv_Result_t fdrv_verifyPage(uint32 flashAddr, const uint32 *expected, uint32 numWords)
{
    const volatile uint32 *flashPtr = (const volatile uint32 *)flashAddr;
    uint32 i;
    for (i = 0U; i < numWords; i++)
    {
        if (flashPtr[i] != expected[i])
        {
            return FLASH_DRV_ERR_VERIFY;
        }
    }
    return FLASH_DRV_OK;
}

/*********************************************************************************************************************/
/*---------------------------------------------PSPR Routines (attribute)---------------------------------------------*/
/*********************************************************************************************************************/

#if defined(__TASKING__)
#pragma section code "psram_cpu0"
#endif

/**
 * \brief  Poll FSR until flash is idle, servicing CPU watchdog periodically.
 *         Runs from PSPR to avoid RWW stall while Flash is busy.
 */
static void fdrv_waitUnbusyAndFeedWatchdog(IfxFlash_FlashType flashType)
{
    Ifx_SCU_WDTCPU *cpuWdt = &(MODULE_SCU.WDTCPU[0]);
    uint16          cpuPwd = IfxScuWdt_getCpuWatchdogPasswordInline(cpuWdt);
    volatile uint32 pollCnt = 0;

    while (FLASH0_FSR.U & (1U << flashType))
    {
        /* Feed CPU watchdog every ~64K polling iterations (~1-2 ms @ 100MHz)
         * to prevent timeout during long sector erase (~200 ms). */
        if (++pollCnt >= 0x10000U)
        {
            pollCnt = 0;
            /* Unlock + service CPU0 WDT (inline logic from IfxScuWdt_setCpuEndinitInline) */
            if (cpuWdt->CON0.B.LCK)
            {
                cpuWdt->CON0.U = (1U << IFX_SCU_WDTCPU_CON0_ENDINIT_OFF) |
                                 (0U << IFX_SCU_WDTCPU_CON0_LCK_OFF) |
                                 (cpuPwd << IFX_SCU_WDTCPU_CON0_PW_OFF) |
                                 (cpuWdt->CON0.B.REL << IFX_SCU_WDTCPU_CON0_REL_OFF);
            }
            cpuWdt->CON0.U = (1U << IFX_SCU_WDTCPU_CON0_ENDINIT_OFF) |
                             (1U << IFX_SCU_WDTCPU_CON0_LCK_OFF) |
                             (cpuPwd << IFX_SCU_WDTCPU_CON0_PW_OFF) |
                             (cpuWdt->CON0.B.REL << IFX_SCU_WDTCPU_CON0_REL_OFF);
        }
    }
}

/**
 * \brief  Erase PFlash sector from PSPR.
 *         Disables interrupts to prevent ISR from executing in PFlash during
 *         Flash busy, waits for completion with watchdog service, then restores.
 */
static void fdrv_eraseSectorPSPR(uint32 sectorAddr, uint32 numSectors)
{
    boolean irqState = IfxCpu_disableInterrupts();
    uint16  pwd      = IfxScuWdt_getSafetyWatchdogPasswordInline();

    IfxScuWdt_clearSafetyEndinitInline(pwd);
    s_pspr.eraseSectors(sectorAddr, numSectors);
    IfxScuWdt_setSafetyEndinitInline(pwd);

    /* Wait from PSPR: safe from RWW, feeds watchdog to avoid timeout */
    fdrv_waitUnbusyAndFeedWatchdog(IfxFlash_FlashType_P0);

    IfxCpu_restoreInterrupts(irqState);
}

static void fdrv_writePFlashPagePSPR(uint32 pageAddr, const uint32 *data)
{
    uint16 pwd = IfxScuWdt_getSafetyWatchdogPasswordInline();
    uint32 i;

    s_pspr.enterPageMode(pageAddr);
    fdrv_waitUnbusyAndFeedWatchdog(IfxFlash_FlashType_P0);

    for (i = 0U; i < (PFLASH_PAGE_SIZE / 8U); i++)
    {
        s_pspr.load2X32bits(pageAddr, data[i * 2U], data[i * 2U + 1U]);
    }

    IfxScuWdt_clearSafetyEndinitInline(pwd);
    s_pspr.writePage(pageAddr);
    IfxScuWdt_setSafetyEndinitInline(pwd);
    fdrv_waitUnbusyAndFeedWatchdog(IfxFlash_FlashType_P0);
}

#if defined(__TASKING__)
#pragma section code restore
#endif

/*********************************************************************************************************************/
/*---------------------------------------------Public API Implementations--------------------------------------------*/
/*********************************************************************************************************************/

void Flash_Drv_Init(void)
{
    /* 1. Clear FSR before anything else */
    FLASH0_FSR.U = FLASH0_FSR_ERROR_MASK;

    /* 2. Relocate iLLD routines into PSPR */
    fdrv_relocateRoutine((const void *)IfxFlash_eraseMultipleSectors,
                         (void *)FDRV_ERASESECTOR_ADDR, FDRV_ERASESECTOR_LEN);
    s_pspr.eraseSectors = (void *)FDRV_ERASESECTOR_ADDR;

    fdrv_relocateRoutine((const void *)IfxFlash_waitUnbusy,
                         (void *)FDRV_WAITUNBUSY_ADDR, FDRV_WAITUNBUSY_LEN);
    s_pspr.waitUnbusy = (void *)FDRV_WAITUNBUSY_ADDR;

    fdrv_relocateRoutine((const void *)IfxFlash_enterPageMode,
                         (void *)FDRV_ENTERPAGEMODE_ADDR, FDRV_ENTERPAGEMODE_LEN);
    s_pspr.enterPageMode = (void *)FDRV_ENTERPAGEMODE_ADDR;

    fdrv_relocateRoutine((const void *)IfxFlash_loadPage2X32,
                         (void *)FDRV_LOADPAGE2X32_ADDR, FDRV_LOADPAGE2X32_LEN);
    s_pspr.load2X32bits = (void *)FDRV_LOADPAGE2X32_ADDR;

    fdrv_relocateRoutine((const void *)IfxFlash_writePage,
                         (void *)FDRV_WRITEPAGE_ADDR, FDRV_WRITEPAGE_LEN);
    s_pspr.writePage = (void *)FDRV_WRITEPAGE_ADDR;
}

uint32 Flash_Drv_CheckFSRError(void)
{
    return (FLASH0_FSR.U & FLASH0_FSR_ERROR_MASK);
}

Flash_Drv_Result_t Flash_Drv_EraseSector(uint32 flashAddr)
{
    Flash_Drv_Result_t result;

    if (!IS_PFLASH_ADDR(flashAddr))
    {
        return FLASH_DRV_ERR_INVALID_ADDR;
    }

    /* Clear sticky bits before operation */
    FLASH0_FSR.U = FLASH0_FSR_ERROR_MASK;

    fdrv_eraseSectorPSPR(flashAddr, 1U);

    if (Flash_Drv_CheckFSRError() != 0U)
    {
        result = FLASH_DRV_ERR_ERASE;
    }
    else
    {
        result = FLASH_DRV_OK;
    }

    /* Always clear after operation */
    FLASH0_FSR.U = FLASH0_FSR_ERROR_MASK;
    return result;
}

Flash_Drv_Result_t Flash_Drv_ProgramPages(uint32 flashAddr, const uint32 *data, uint32 length)
{
    Flash_Drv_Result_t result = FLASH_DRV_OK;
    uint32 pageAddr;
    uint32 offset;

    /* Alignment checks */
    if ((flashAddr & (PFLASH_PAGE_SIZE - 1U)) != 0U)
    {
        return FLASH_DRV_ERR_ADDR_ALIGN;
    }
    if ((length & (PFLASH_PAGE_SIZE - 1U)) != 0U)
    {
        return FLASH_DRV_ERR_LEN_ALIGN;
    }
    if (!IS_PFLASH_ADDR(flashAddr) || !IS_PFLASH_ADDR((flashAddr) + (length)))
    {
        return FLASH_DRV_ERR_INVALID_ADDR;
    }

    /* Clear sticky bits before operation */
    FLASH0_FSR.U = FLASH0_FSR_ERROR_MASK;

    for (offset = 0U; offset < length; offset += PFLASH_PAGE_SIZE)
    {
        pageAddr = flashAddr + offset;

        fdrv_writePFlashPagePSPR(pageAddr, &data[offset / 4U]);

        if (Flash_Drv_CheckFSRError() != 0U)
        {
            result = FLASH_DRV_ERR_PROGRAM;
            break;
        }

        result = fdrv_verifyPage(pageAddr, &data[offset / 4U], PFLASH_PAGE_SIZE / 4U);
        if (result != FLASH_DRV_OK)
        {
            break;
        }
    }

    /* Always clear after operation */
    FLASH0_FSR.U = FLASH0_FSR_ERROR_MASK;
    return result;
}

Flash_Drv_Result_t Flash_Drv_EraseDFlashSector(uint32 flashAddr)
{
    /* DFlash erase: use iLLD directly (simpler, smaller sectors).
     * Still runs from PSPR via relocated waitUnbusy if needed.
     * For OEM compatibility, use the same pattern but with DFlash constants. */
    Flash_Drv_Result_t result;
    uint16 pwd;

    if ((flashAddr < DFLASH_BASE) || (flashAddr >= DFLASH_TOP))
    {
        return FLASH_DRV_ERR_INVALID_ADDR;
    }

    FLASH0_FSR.U = FLASH0_FSR_ERROR_MASK;

    pwd = IfxScuWdt_getSafetyWatchdogPasswordInline();
    IfxScuWdt_clearSafetyEndinitInline(pwd);
    IfxFlash_eraseMultipleSectors(flashAddr, 1U);
    IfxScuWdt_setSafetyEndinitInline(pwd);

    result = fdrv_waitForIdle(IfxFlash_FlashType_D0);
    if (result == FLASH_DRV_OK)
    {
        if (Flash_Drv_CheckFSRError() != 0U)
        {
            result = FLASH_DRV_ERR_ERASE;
        }
    }

    FLASH0_FSR.U = FLASH0_FSR_ERROR_MASK;
    return result;
}

Flash_Drv_Result_t Flash_Drv_ProgramDFlashPages(uint32 flashAddr, const uint32 *data, uint32 length)
{
    Flash_Drv_Result_t result = FLASH_DRV_OK;
    uint32 pageAddr;
    uint32 offset;
    uint16 pwd;

    if ((flashAddr & (DFLASH_PAGE_SIZE - 1U)) != 0U)
    {
        return FLASH_DRV_ERR_ADDR_ALIGN;
    }
    if ((length & (DFLASH_PAGE_SIZE - 1U)) != 0U)
    {
        return FLASH_DRV_ERR_LEN_ALIGN;
    }
    if ((flashAddr < DFLASH_BASE) || ((flashAddr + length) > DFLASH_TOP))
    {
        return FLASH_DRV_ERR_INVALID_ADDR;
    }

    FLASH0_FSR.U = FLASH0_FSR_ERROR_MASK;

    for (offset = 0U; offset < length; offset += DFLASH_PAGE_SIZE)
    {
        pageAddr = flashAddr + offset;

        /* DFlash page mode: enter -> load 8 bytes -> write */
        (void)s_pspr.enterPageMode(pageAddr);
        result = fdrv_waitForIdle(IfxFlash_FlashType_D0);
        if (result != FLASH_DRV_OK)
        {
            break;
        }

        s_pspr.load2X32bits(pageAddr, data[offset / 4U], data[offset / 4U + 1U]);

        pwd = IfxScuWdt_getSafetyWatchdogPasswordInline();
        IfxScuWdt_clearSafetyEndinitInline(pwd);
        s_pspr.writePage(pageAddr);
        IfxScuWdt_setSafetyEndinitInline(pwd);

        result = fdrv_waitForIdle(IfxFlash_FlashType_D0);
        if (result != FLASH_DRV_OK)
        {
            break;
        }

        if (Flash_Drv_CheckFSRError() != 0U)
        {
            result = FLASH_DRV_ERR_PROGRAM;
            break;
        }

        result = fdrv_verifyPage(pageAddr, &data[offset / 4U], DFLASH_PAGE_SIZE / 4U);
        if (result != FLASH_DRV_OK)
        {
            break;
        }
    }

    FLASH0_FSR.U = FLASH0_FSR_ERROR_MASK;
    return result;
}
