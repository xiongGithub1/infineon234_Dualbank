/**********************************************************************************************************************
 * \file    Flash_Driver.h
 * \brief   Core Flash Driver - OEM Standard Hardware Abstraction Layer
 * \version V1.0.0
 * \date    2026-05-13
 *
 * Design Principles:
 *  - State-free: No static download state, no alignment buffer (managed by upper layer)
 *  - PSPR-safe: All Flash operations execute from PSPR to avoid RWW conflicts
 *  - FSR discipline: Clear sticky bits before and after every operation
 *  - Fail-fast: Return detailed error codes, do not attempt recovery
 *********************************************************************************************************************/
#ifndef FLASH_DRIVER_H
#define FLASH_DRIVER_H

/*********************************************************************************************************************/
/*-----------------------------------------------------Includes------------------------------------------------------*/
/*********************************************************************************************************************/
#include "Ifx_Types.h"
#include "IfxFlash.h"

/*********************************************************************************************************************/
/*------------------------------------------------------Macros-------------------------------------------------------*/
/*********************************************************************************************************************/

/* Flash module base */
#define FLASH_MODULE                0U

/* FSR sticky error mask (W1C bits) — covers all reported error conditions */
#define FLASH0_FSR_ERROR_MASK       (0x4E7F7800U)

/* Operation timeout (arbitrary large count for polling loops) */
#define FLASH_DRV_TIMEOUT           (0x100000U)

/* PSPR relocation area for critical routines */
#define FLASH_DRV_PSPR_BASE         (0x70101800U)

/*********************************************************************************************************************/
/*-------------------------------------------------Data Types--------------------------------------------------------*/
/*********************************************************************************************************************/

typedef enum
{
    FLASH_DRV_OK                        = 0,  /* Success */
    FLASH_DRV_ERR_ADDR_ALIGN,                /* Address not on page boundary */
    FLASH_DRV_ERR_LEN_ALIGN,                 /* Length not a multiple of page size */
    FLASH_DRV_ERR_ERASE,                     /* Erase operation failed (FSR error) */
    FLASH_DRV_ERR_PROGRAM,                   /* Program operation failed (FSR error) */
    FLASH_DRV_ERR_VERIFY,                    /* Read-back verification failed */
    FLASH_DRV_ERR_TIMEOUT,                   /* Polling loop timeout */
    FLASH_DRV_ERR_INVALID_ADDR,              /* Address outside valid Flash range */
    FLASH_DRV_ERR_RWW,                       /* RWW conflict (attempt to execute from target bank) */
    FLASH_DRV_ERR_INTERNAL                   /* Internal / unexpected error */
} Flash_Drv_Result_t;

/*********************************************************************************************************************/
/*------------------------------------------------Function Prototypes------------------------------------------------*/
/*********************************************************************************************************************/

/* Initialize driver: copy critical routines to PSPR, clear FSR */
void Flash_Drv_Init(void);

/* Erase one PFlash sector (256 KB) starting at flashAddr. flashAddr must be sector-aligned. */
Flash_Drv_Result_t Flash_Drv_EraseSector(uint32 flashAddr);

/* Program 32-byte pages to PFlash. flashAddr and length must be 32-byte aligned. */
Flash_Drv_Result_t Flash_Drv_ProgramPages(uint32 flashAddr, const uint32 *data, uint32 length);

/* Erase one DFlash sector (4 KB / 256 B depending on config). flashAddr must be sector-aligned. */
Flash_Drv_Result_t Flash_Drv_EraseDFlashSector(uint32 flashAddr);

/* Program 8-byte pages to DFlash. flashAddr and length must be 8-byte aligned. */
Flash_Drv_Result_t Flash_Drv_ProgramDFlashPages(uint32 flashAddr, const uint32 *data, uint32 length);

/* Explicit FSR clear. Call before any Flash operation. */
IFX_INLINE void Flash_Drv_ClearFSR(void)
{
    FLASH0_FSR.U = FLASH0_FSR_ERROR_MASK;
}

/* Check whether any sticky error bits are set in FSR.
 * Returns 0 if clean, non-zero if error(s) present. */
uint32 Flash_Drv_CheckFSRError(void);

#endif /* FLASH_DRIVER_H */
