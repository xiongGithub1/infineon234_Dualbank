
/**********************************************************************************************************************
 * \file    Flash.h
 * \brief
 * \version V1.0.0
 * \date
 *********************************************************************************************************************/


#ifndef MCU_PORT_FLASH_H_
#define MCU_PORT_FLASH_H_

/*********************************************************************************************************************/
/*-----------------------------------------------------Includes------------------------------------------------------*/
/*********************************************************************************************************************/
#include <string.h>
#include "Ifx_Types.h"
#include    <App_bootloader_cfg.h>

/*********************************************************************************************************************/
/*------------------------------------------------------Macros-------------------------------------------------------*/
/*********************************************************************************************************************/
#define MEM(address)                *((uint32 *)(address))      /* Macro to simplify the access to a memory address */


#define PFLASH_PAGE_LENGTH          IFXFLASH_PFLASH_PAGE_LENGTH /* 0x20 = 32 Bytes (smallest unit that can be
                                                                 * programmed in the Program Flash memory (PFLASH)) */
#define DFLASH_PAGE_LENGTH          IFXFLASH_DFLASH_PAGE_LENGTH /* 0x8 = 8 Bytes (smallest unit that can be
                                                                 * programmed in the Data Flash memory (DFLASH))    */
#define FLASH_MODULE                0                           /* Macro to select the flash (PMU) module           */
#define PROGRAM_FLASH_0             IfxFlash_FlashType_P0       /* Define the Program Flash Bank to be used         */
#define DATA_FLASH_0                IfxFlash_FlashType_D0       /* Define the Data Flash Bank to be used            */

#define DATA_TO_WRITE               0x07738135                  /* Dummy data to be written in the Flash memories   */

#define PFLASH_STARTING_ADDRESS				0xA0060000      //0xA00E0000 /* Address of the PFLASH where the data is written  */
#define PFLASH_appBackup_STARTING_ADDRESS	0xA00E0000		//app的备份起始地址
//#define DFLASH_STARTING_ADDRESS     0xAF000000                  /* Address of the DFLASH where the data is written  */

#define PFLASH_NUM_PAGE_TO_FLASH    9                           /* Number of pages to flash in the PFLASH           */
//#define PFLASH_NUM_SECTORS          1                           /* Number of PFLASH sectors to be erased            */
//#define DFLASH_NUM_PAGE_TO_FLASH    8                           /* Number of pages to flash in the DFLASH           */
//#define DFLASH_NUM_SECTORS          1                           /* Number of DFLASH sectors to be erased            */

/* Reserved space for erase and program routines in bytes */
#define ERASESECTOR_LEN             (0x100)		// 0x100=256 擦除扇区大小
#define WAITUNBUSY_LEN              (0x100)		// 等待空闲大小
#define ENTERPAGEMODE_LEN           (0x100)		// 进入页模式大小
#define LOADPAGE2X32_LEN            (0x100)		// 加载页2*0x32大小
#define WRITEPAGE_LEN               (0x100)		// 写页大小
#define ERASEPFLASH_LEN             (0x200)		// 擦除pFlash大小
#define WRITEPFLASH_LEN             (0x200)		// 写pFlash大小

/*
 * Ram PSPR
 */
/* Relocation address for the erase and program routines: Program Scratch-Pad SRAM (PSPR) of CPU0 */
#define RELOCATION_START_ADDR       (0x70100000U)		// 加载起始地址

/* Definition of the addresses where to relocate the erase and program routines, given their reserved space */
#define ERASESECTOR_ADDR            (RELOCATION_START_ADDR)					// 擦除扇区地址0x70100000U
#define WAITUNBUSY_ADDR             (ERASESECTOR_ADDR + ERASESECTOR_LEN)	// 等待空闲地址0x70100100U
#define ENTERPAGEMODE_ADDR          (WAITUNBUSY_ADDR + WAITUNBUSY_LEN)		// 进入页模式地址0x70100200U
#define LOAD2X32_ADDR               (ENTERPAGEMODE_ADDR + ENTERPAGEMODE_LEN)// 加载页2*32地址0x70100300U
#define WRITEPAGE_ADDR              (LOAD2X32_ADDR + LOADPAGE2X32_LEN)		// 写页地址0x70100400U
#define ERASEPFLASH_ADDR            (WRITEPAGE_ADDR + WRITEPAGE_LEN)		// 擦除pFlash地址0x70100500U
#define WRITEPFLASH_ADDR            (ERASEPFLASH_ADDR + ERASEPFLASH_LEN)	// 写pFlash地址0x70100700U
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
//    void (*writeFlash)(uint32 pageAddr, uint32 *data);
//    void (*writeFlash)(uint32 pageAddr);
    void (*writeFlash)(uint32 pageAddr, uint32 *data, uint32 byteLength);
} Function;

/*********************************************************************************************************************/
/*--------------------------------------------Private Variables/Constants--------------------------------------------*/
/*********************************************************************************************************************/

/*********************************************************************************************************************/
/*------------------------------------------------Function Prototypes------------------------------------------------*/
/*********************************************************************************************************************/
void Flash_erasePFLASH(uint32 sectorAddr);
void Flash_writePFlashPage(uint32 startingAddr, uint32 *data, uint32 byteLength);
void Flash_copyFunctionsToPSPR(void);
void Flash_init(void);
void Flash_ForceWriteRemaining(void) ;
int  Flash_erasePFlash_port(uint32 flashAddr);
int  Flash_writePFlash_port(uint32 flashAddr, uint32 *data, uint32 bytelength);
int  Flash_writePFlash_portex(uint32 flashAddr, uint8 *data, uint32 byteLength);//适配于uds的bootloader
int  Flash_eraseDFlash_port(uint32 flashAddr);
int  Flash_writeDFlash_port(uint32 flashAddr, uint32 *data, uint32 bytelength );
extern void Flash_erasePFLASH(uint32 sectorAddr);

#endif /* MCU_PORT_FLASH_H_ */
