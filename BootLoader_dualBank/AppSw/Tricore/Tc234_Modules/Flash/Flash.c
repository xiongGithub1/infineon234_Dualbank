/**********************************************************************************************************************
 * \file    Flash.c
 * \brief   OEM Standard Flash Port Layer for TC234
 * \version V3.0.0
 * \date    2026-05-13
 *
 * Architecture:
 *  - Flash_Driver.c/h : Core hardware abstraction (PSPR-safe, FSR management)
 *  - Flash.c/h        : Port layer + legacy PSPR relocation + alignment buffer
 *  - fls_app.c/h      : UDS download state machine
 *
 * Design rules:
 *  1. Alignment buffer is static to Flash.c but MUST be reset via
 *     Flash_ResetAlignBuffer() on every new 0x34 RequestDownload.
 *  2. Pad bytes for incomplete pages are 0xFF (erased state), never 0x00.
 *  3. All actual Flash programming goes through Flash_Drv_ProgramPages.
 *********************************************************************************************************************/

/*********************************************************************************************************************/
/*-----------------------------------------------------Includes------------------------------------------------------*/
/*********************************************************************************************************************/
#include <string.h>
#include "Ifx_Types.h"
#include "Flash.h"
#include "Flash_Driver.h"
#include "IfxFlash.h"
#include "IfxCpu.h"
#include "Cpu0_Main.h"

/*********************************************************************************************************************/
/*-------------------------------------------------Global variables--------------------------------------------------*/
/*********************************************************************************************************************/
Function g_commandFromPSPR;
uint32 gPageData[8];

/* Alignment buffer for cross-page data (< 32 bytes).
 * IMPORTANT: Must be explicitly reset via Flash_ResetAlignBuffer() on every
 * new 0x34 RequestDownload to prevent stale data from poisoning new sessions. */
static uint8  s_alignBuf[PFLASH_PAGE_LENGTH];
static uint32 s_alignAddr = 0;
static uint32 s_alignLen  = 0;

/*********************************************************************************************************************/
/*---------------------------------------------Function Implementations----------------------------------------------*/
/*********************************************************************************************************************/

#if defined(__TASKING__)
#pragma section code "psram_cpu0"
#endif

void Flash_erasePFLASH(uint32 sectorAddr)
{
    uint16 endInitSafetyPassword = IfxScuWdt_getSafetyWatchdogPasswordInline();
    IfxScuWdt_clearSafetyEndinitInline(endInitSafetyPassword);
    g_commandFromPSPR.eraseSectors(sectorAddr, 1);
    IfxScuWdt_setSafetyEndinitInline(endInitSafetyPassword);
    g_commandFromPSPR.waitUnbusy(FLASH_MODULE, PROGRAM_FLASH_0);
}

void Flash_writePFlashPage(uint32 startingAddr, uint32 *data, uint32 byteLength)
{
    uint32 page, pageTotal;
    uint32 offset;
    uint16 endInitSafetyPassword = IfxScuWdt_getSafetyWatchdogPasswordInline();

    pageTotal = byteLength / PFLASH_PAGE_LENGTH;
    for (page = 0; page < pageTotal; page++)
    {
        uint32 pageAddr = startingAddr + (page * PFLASH_PAGE_LENGTH);

        g_commandFromPSPR.enterPageMode(pageAddr);
        g_commandFromPSPR.waitUnbusy(FLASH_MODULE, PROGRAM_FLASH_0);

        for (offset = 0; offset < PFLASH_PAGE_LENGTH; offset += 0x8)
        {
            g_commandFromPSPR.load2X32bits(pageAddr,
                data[(page * 8) + (offset / 4)],
                data[(page * 8) + (offset / 4) + 1]);
        }

        IfxScuWdt_clearSafetyEndinitInline(endInitSafetyPassword);
        g_commandFromPSPR.writePage(pageAddr);
        IfxScuWdt_setSafetyEndinitInline(endInitSafetyPassword);

        g_commandFromPSPR.waitUnbusy(FLASH_MODULE, PROGRAM_FLASH_0);
    }
}

#if defined(__TASKING__)
#pragma section code restore
#endif

void Flash_copyFunctionsToPSPR(void)
{
    memcpy((void *)ERASESECTOR_ADDR,  (const void *)IfxFlash_eraseMultipleSectors, ERASESECTOR_LEN);
    g_commandFromPSPR.eraseSectors = (void *)ERASESECTOR_ADDR;

    memcpy((void *)WAITUNBUSY_ADDR,   (const void *)IfxFlash_waitUnbusy,           WAITUNBUSY_LEN);
    g_commandFromPSPR.waitUnbusy = (void *)WAITUNBUSY_ADDR;

    memcpy((void *)ENTERPAGEMODE_ADDR,(const void *)IfxFlash_enterPageMode,        ENTERPAGEMODE_LEN);
    g_commandFromPSPR.enterPageMode = (void *)ENTERPAGEMODE_ADDR;

    memcpy((void *)LOAD2X32_ADDR,     (const void *)IfxFlash_loadPage2X32,         LOADPAGE2X32_LEN);
    g_commandFromPSPR.load2X32bits = (void *)LOAD2X32_ADDR;

    memcpy((void *)WRITEPAGE_ADDR,    (const void *)IfxFlash_writePage,            WRITEPAGE_LEN);
    g_commandFromPSPR.writePage = (void *)WRITEPAGE_ADDR;

    g_commandFromPSPR.eraseFlash = (void *)Flash_erasePFLASH;
    g_commandFromPSPR.writeFlash = (void *)Flash_writePFlashPage;
}

void Flash_init(void)
{
    Flash_Drv_Init();
    Flash_copyFunctionsToPSPR();
    Flash_ResetAlignBuffer();
}

void Flash_ResetAlignBuffer(void)
{
    memset(s_alignBuf, 0xFF, PFLASH_PAGE_LENGTH);
    s_alignAddr = 0;
    s_alignLen  = 0;
}

int Flash_erasePFlash_port(uint32 flashAddr)
{
    Flash_Drv_Result_t r = Flash_Drv_EraseSector(flashAddr);
    return (r == FLASH_DRV_OK) ? 0 : -1;
}

int Flash_writePFlash_port(uint32 flashAddr, uint32 *data, uint32 bytelength)
{
    Flash_Drv_Result_t r = Flash_Drv_ProgramPages(flashAddr, data, bytelength);
    return (r == FLASH_DRV_OK) ? 0 : -1;
}

/**
 * \brief  Write data to PFlash with automatic 32-byte alignment buffering.
 *         Handles non-aligned start addresses, cross-page remainders, and
 *         arbitrary lengths per OEM standard.
 * \return 1 on success, 0 on failure.
 */
int Flash_writePFlash_portex(uint32 flashAddr, uint8 *data, uint32 byteLength)
{
    uint32 offset = 0;
    uint32 currentAddr = flashAddr;

    /*-------------------------------------------------------------
     * Step 0: Handle non-aligned start address (no prior buffer)
     *-------------------------------------------------------------*/
    if ((s_alignLen == 0) && ((flashAddr & (PFLASH_PAGE_LENGTH - 1U)) != 0U))
    {
        uint32 pageBase = flashAddr & ~(PFLASH_PAGE_LENGTH - 1U);
        uint32 skip     = flashAddr - pageBase;
        uint32 avail    = PFLASH_PAGE_LENGTH - skip;
        uint32 copy     = (byteLength < avail) ? byteLength : avail;

        memset(s_alignBuf, 0xFF, PFLASH_PAGE_LENGTH);
        memcpy(s_alignBuf + skip, data, copy);
        s_alignAddr = pageBase;
        s_alignLen  = skip + copy;

        if (s_alignLen < PFLASH_PAGE_LENGTH)
        {
            /* Not enough data to fill the first page — wait for more */
            return 1;
        }
        else
        {
            /* Full page assembled — write it now */
            if (Flash_writePFlash_port(s_alignAddr, (uint32 *)s_alignBuf, PFLASH_PAGE_LENGTH) != 0)
            {
                Flash_ResetAlignBuffer();
                return 0;
            }
            offset      = copy;
            currentAddr = pageBase + PFLASH_PAGE_LENGTH;
            Flash_ResetAlignBuffer();
        }
    }

    /*-------------------------------------------------------------
     * Step 1: Handle previous alignment buffer
     *-------------------------------------------------------------*/
    if (s_alignLen > 0)
    {
        if (currentAddr == (s_alignAddr + s_alignLen))
        {
            /* Continuous — fill buffer and write if possible */
            uint32 needed = PFLASH_PAGE_LENGTH - s_alignLen;
            uint32 copy   = (byteLength < needed) ? byteLength : needed;

            memcpy(s_alignBuf + s_alignLen, data, copy);

            if (copy == needed)
            {
                if (Flash_writePFlash_port(s_alignAddr, (uint32 *)s_alignBuf, PFLASH_PAGE_LENGTH) != 0)
                {
                    Flash_ResetAlignBuffer();
                    return 0;
                }
                offset      += copy;
                currentAddr += copy;
                Flash_ResetAlignBuffer();
            }
            else
            {
                s_alignLen += copy;
                return 1;
            }
        }
        else if ((s_alignAddr + PFLASH_PAGE_LENGTH) > currentAddr)
        {
            /* Overlapping within same page */
            uint32 needBytes = s_alignAddr + PFLASH_PAGE_LENGTH - currentAddr;
            uint32 copy      = (byteLength < needBytes) ? byteLength : needBytes;

            if ((PFLASH_PAGE_LENGTH - needBytes) > s_alignLen)
            {
                memcpy(s_alignBuf + PFLASH_PAGE_LENGTH - needBytes, data, copy);
                if (needBytes <= byteLength)
                {
                    if (Flash_writePFlash_port(s_alignAddr, (uint32 *)s_alignBuf, PFLASH_PAGE_LENGTH) != 0)
                    {
                        Flash_ResetAlignBuffer();
                        return 0;
                    }
                    offset      += copy;
                    currentAddr += copy;
                    Flash_ResetAlignBuffer();
                }
                else
                {
                    s_alignLen = PFLASH_PAGE_LENGTH - (needBytes - byteLength);
                    return 1;
                }
            }
            else
            {
                Flash_ResetAlignBuffer();
                return 0;
            }
        }
        else
        {
            /* Discontinuous — force-write old buffer with 0xFF padding */
            if (Flash_ForceWriteRemaining() == 0)
            {
                Flash_ResetAlignBuffer();
                return 0;
            }
        }
    }

    /*-------------------------------------------------------------
     * Step 2: Write full 32-byte blocks
     *-------------------------------------------------------------*/
    while ((offset + PFLASH_PAGE_LENGTH) <= byteLength)
    {
        if (Flash_writePFlash_port(currentAddr, (uint32 *)(data + offset), PFLASH_PAGE_LENGTH) != 0)
        {
            Flash_ResetAlignBuffer();
            return 0;
        }
        offset      += PFLASH_PAGE_LENGTH;
        currentAddr += PFLASH_PAGE_LENGTH;
    }

    /*-------------------------------------------------------------
     * Step 3: Store remaining bytes (< 32)
     *-------------------------------------------------------------*/
    if (offset < byteLength)
    {
        s_alignAddr = currentAddr;
        s_alignLen  = byteLength - offset;
        memset(s_alignBuf, 0xFF, PFLASH_PAGE_LENGTH);
        memcpy(s_alignBuf, data + offset, s_alignLen);
    }

    return 1;
}

/**
 * \brief  Force-write any remaining alignment buffer data padded with 0xFF.
 * \return 1 on success, 0 on failure.
 */
int Flash_ForceWriteRemaining(void)
{
    if (s_alignLen == 0)
    {
        return 1;
    }

    while (s_alignLen < PFLASH_PAGE_LENGTH)
    {
        s_alignBuf[s_alignLen++] = 0xFF;
    }

    if (Flash_writePFlash_port(s_alignAddr, (uint32 *)s_alignBuf, PFLASH_PAGE_LENGTH) != 0)
    {
        return 0;
    }

    Flash_ResetAlignBuffer();
    return 1;
}

int Flash_eraseDFlash_port(uint32 flashAddr)
{
    Flash_Drv_Result_t r = Flash_Drv_EraseDFlashSector(flashAddr);
    return (r == FLASH_DRV_OK) ? 0 : -1;
}

int Flash_writeDFlash_port(uint32 flashAddr, uint32 *data, uint32 bytelength)
{
    Flash_Drv_Result_t r = Flash_Drv_ProgramDFlashPages(flashAddr, data, bytelength);
    return (r == FLASH_DRV_OK) ? 0 : -1;
}
