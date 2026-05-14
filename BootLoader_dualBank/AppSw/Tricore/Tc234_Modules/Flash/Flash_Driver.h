/**********************************************************************************************************************
 * \file    Flash_Driver.h
 * \brief   Low-level Flash driver primitives and constants for TC234 PSPR execution.
 * \version V1.0.0
 *********************************************************************************************************************/

#ifndef FLASH_DRIVER_H
#define FLASH_DRIVER_H

#include "Ifx_Types.h"

/*********************************************************************************************************************/
/* Flash page sizes                                                                                                  */
/*********************************************************************************************************************/
#define FLASH_DRV_PFLASH_PAGE_SIZE      32u
#define FLASH_DRV_DFLASH_PAGE_SIZE      8u

/*********************************************************************************************************************/
/* FSR Error bit mask for PFlash operations                                                                          */
/* bit 11: OPER  (Operation Error)                                                                                   */
/* bit 12: SQER  (Sequence Error)                                                                                    */
/* bit 13: PROER (Protection Error)                                                                                  */
/* bit 25: PVER  (Program Verify Error)                                                                              */
/* bit 26: EVER  (Erase Verify Error)                                                                                */
/*********************************************************************************************************************/
#define FLASH_DRV_FSR_ERROR_MASK        (0x02003800U)

/*********************************************************************************************************************/
/* Address helpers                                                                                                   */
/*********************************************************************************************************************/
#define FLASH_DRV_TO_UNCACHED(addr)     (((addr) & 0x00FFFFFFu) | 0xA0000000u)
#define FLASH_DRV_IS_PFLASH_ADDR(addr)  \
    ( ((((addr) & 0xFF000000u) >= 0x80000000u) && (((addr) & 0xFF000000u) < 0x90000000u)) || \
      ((((addr) & 0xFF000000u) >= 0xA0000000u) && (((addr) & 0xFF000000u) < 0xB0000000u)) )

/*********************************************************************************************************************/
/* Inline helpers (ensured to be inlined into PSPR routines)                                                         */
/*********************************************************************************************************************/

/** \brief Get FSR error bits relevant for PFlash program/erase */
static inline uint32 Flash_Drv_GetFSRError(void)
{
    return (FLASH0_FSR.U & FLASH_DRV_FSR_ERROR_MASK);
}

/** \brief Clear FSR error flags by W1C */
static inline void Flash_Drv_ClearFSR(void)
{
    FLASH0_FSR.U = FLASH_DRV_FSR_ERROR_MASK;
}

#endif /* FLASH_DRIVER_H */
