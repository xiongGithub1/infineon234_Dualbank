/**********************************************************************************************************************
 * \file    Flash.h
 * \brief   OEM Standard Flash Port Layer for TC234
 * \version V2.0.0
 * \date    2026-05-13
 *********************************************************************************************************************/

#ifndef MCU_PORT_FLASH_H_
#define MCU_PORT_FLASH_H_

/*********************************************************************************************************************/
/*-----------------------------------------------------Includes------------------------------------------------------*/
/*********************************************************************************************************************/
#include <string.h>
#include "Ifx_Types.h"
#include "Flash_Driver.h"
#include <App_bootloader_cfg.h>

/*********************************************************************************************************************/
/*------------------------------------------------------Macros-------------------------------------------------------*/
/*********************************************************************************************************************/
#define MEM(address)                *((uint32 *)(address))      /* Macro to simplify the access to a memory address */

#define PFLASH_PAGE_LENGTH          IFXFLASH_PFLASH_PAGE_LENGTH /* 0x20 = 32 Bytes (smallest unit that can be
                                                                 * programmed in the Program Flash memory (PFLASH)) */
#define DFLASH_PAGE_LENGTH          IFXFLASH_DFLASH_PAGE_LENGTH /* 0x8 = 8 Bytes (smallest unit that can be
                                                                 * programmed in the Data Flash memory (DFLASH))    */
/* FLASH_MODULE is now defined in Flash_Driver.h */
#define PROGRAM_FLASH_0             IfxFlash_FlashType_P0       /* Define the Program Flash Bank to be used         */
#define DATA_FLASH_0                IfxFlash_FlashType_D0       /* Define the Data Flash Bank to be used            */

#define DATA_TO_WRITE               0x07738135                  /* Dummy data to be written in the Flash memories   */

#define PFLASH_STARTING_ADDRESS             0xA0060000
#define PFLASH_appBackup_STARTING_ADDRESS   0xA00E0000

#define PFLASH_NUM_PAGE_TO_FLASH    9

/* Reserved space for erase and program routines in bytes */
#define ERASESECTOR_LEN             (0x100)
#define WAITUNBUSY_LEN              (0x100)
#define ENTERPAGEMODE_LEN           (0x100)
#define LOADPAGE2X32_LEN            (0x100)
#define WRITEPAGE_LEN               (0x100)
#define ERASEPFLASH_LEN             (0x200)
#define WRITEPFLASH_LEN             (0x200)

/* Relocation address for the erase and program routines: Program Scratch-Pad SRAM (PSPR) of CPU0 */
#define RELOCATION_START_ADDR       (0x70101800U)

#define ERASESECTOR_ADDR            (RELOCATION_START_ADDR)
#define WAITUNBUSY_ADDR             (ERASESECTOR_ADDR + ERASESECTOR_LEN)
#define ENTERPAGEMODE_ADDR          (WAITUNBUSY_ADDR + WAITUNBUSY_LEN)
#define LOAD2X32_ADDR               (ENTERPAGEMODE_ADDR + ENTERPAGEMODE_LEN)
#define WRITEPAGE_ADDR              (LOAD2X32_ADDR + LOADPAGE2X32_LEN)
#define ERASEPFLASH_ADDR            (WRITEPAGE_ADDR + WRITEPAGE_LEN)
#define WRITEPFLASH_ADDR            (ERASEPFLASH_ADDR + ERASEPFLASH_LEN)

/*********************************************************************************************************************/
/*-------------------------------------------------Global variables--------------------------------------------------*/
/*********************************************************************************************************************/
extern uint32 gPageData[8];

/*********************************************************************************************************************/
/*-------------------------------------------------Data Structures---------------------------------------------------*/
/*********************************************************************************************************************/
typedef struct
{
    void (*eraseSectors)(uint32 sectorAddr, uint32 numSector);
    uint8 (*waitUnbusy)(uint32 flash, IfxFlash_FlashType flashType);
    uint8 (*enterPageMode)(uint32 pageAddr);
    void (*load2X32bits)(uint32 pageAddr, uint32 wordL, uint32 wordU);
    void (*writePage)(uint32 pageAddr);
    void (*eraseFlash)(uint32 sectorAddr);
    void (*writeFlash)(uint32 pageAddr, uint32 *data, uint32 byteLength);
} Function;

/*********************************************************************************************************************/
/*------------------------------------------------Function Prototypes------------------------------------------------*/
/*********************************************************************************************************************/
void Flash_erasePFLASH(uint32 sectorAddr);
void Flash_writePFlashPage(uint32 startingAddr, uint32 *data, uint32 byteLength);
void Flash_copyFunctionsToPSPR(void);
void Flash_init(void);
void Flash_ResetAlignBuffer(void);
int  Flash_erasePFlash_port(uint32 flashAddr);
int  Flash_writePFlash_port(uint32 flashAddr, uint32 *data, uint32 bytelength);
int  Flash_writePFlash_portex(uint32 flashAddr, uint8 *data, uint32 byteLength);
int  Flash_ForceWriteRemaining(void);
int  Flash_eraseDFlash_port(uint32 flashAddr);
int  Flash_writeDFlash_port(uint32 flashAddr, uint32 *data, uint32 bytelength);

#endif /* MCU_PORT_FLASH_H_ */
