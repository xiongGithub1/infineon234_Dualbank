
/**********************************************************************************************************************
 * \file    App_bootloader_cfg.h
 * \brief
 * \version V1.0.0
 * \date
 *********************************************************************************************************************/


#ifndef APP_BOOTLOADER_CFG_H_
#define APP_BOOTLOADER_CFG_H_

/*********************************************************************************************************************/
/*-----------------------------------------------------Includes------------------------------------------------------*/
/*********************************************************************************************************************/
#include    "Std_Types.h"
/* ifx */
#include "IfxFlash_cfg.h"
/*********************************************************************************************************************/
/*------------------------------------------------------Macros-------------------------------------------------------*/
/*********************************************************************************************************************/

/*********************************************************************************************************************/
/*-------------------------------------------------Global variables--------------------------------------------------*/
/*********************************************************************************************************************/

/*********************************************************************************************************************/
/*-------------------------------------------------Data Structures---------------------------------------------------*/
/*********************************************************************************************************************/

/*********************************************************************************************************************/
/*--------------------------------------------Private Variables/Constants--------------------------------------------*/
/*********************************************************************************************************************/

/*********************************************************************************************************************/
/*------------------------------------------------Function Prototypes------------------------------------------------*/
/*********************************************************************************************************************/
/*
 * define   MCU
 */
#define     MCU_TC234

/*
 * Ram cfg
 */
/* Bootloader flag address in RAM (Local Address) */
#define     MCU_DSRAM0_Start                    (0x70000000uL)
#define     MCU_DSRAM0_End                   	(0x7002DfffuL)	// Local DRAM 184K
//#define   MCU_DSRAM0_End                    	(0x70003fffuL)	// 16k, app 168K
#define     MCU_DSRAM0_ALIGNMENT_Size           (4)
/* Ram flag define */
#define     RAM_BOOT_MODE_Addr          0x7002Dffc//(MCU_DSRAM0_Addr+MCU_DSRAM0_Length - 4)	// RAM flag address for app mode


#define     RAM_BOOT_MODE_KEEP              0xf0b1              /* Force stop boot */
#define     RAM_BOOT_MODE_NORMAL            0xb2b3              /* Normal power-on boot */
#define     RAM_BOOT_MODE_APP               0xa4a5              /* App to bootloader flag */


/*
 * flash cfg
 */
/* FL_NUM_LOGICAL_BLOCKS */
/* TC234 */
#ifdef      MCU_TC234
#define     FL_PFLASH_LOGICAL_SECTOR_Nr             IFXFLASH_PFLASH_NUM_LOG_SECTORS/IFXFLASH_PFLASH_BANKS
#define     FL_PFLASH_PF_ADDR_Start                (IfxFlash_pFlashTableLog[0].start) // PF0 S0 start
#define     FL_PFLASH_PF_ADDR_End                  (IfxFlash_pFlashTableLog[IFXFLASH_PFLASH_NUM_LOG_SECTORS-1].end) // PF0 S26 end
#define     FL_PFLASH_ALIGNMENT_Size                (4)


/*
 * Memory layout - 20240710
 * */
// Bootloader address -- S0-S3
#define		FL_PFLASH_PF_Bootloader_ADDR_Start 		(IfxFlash_pFlashTableLog[0].start)	// PF0 S0 start -- bootloader start (unused)
#define		FL_PFLASH_PF_Bootloader_ADDR_End  		(IfxFlash_pFlashTableLog[5].end) 	// PF0 S3 end -- bootloader end (unused)
// Reserved address -- S4-S7
#define		FL_PFLASH_PF_Reserved1_ADDR_Start 		(IfxFlash_pFlashTableLog[6].start) 	// PF0 S4 start -- Reserved1 start (unused)
#define		FL_PFLASH_PF_Reserved1_ADDR_End  		(IfxFlash_pFlashTableLog[7].end) 	// PF0 S7 end -- Reserved1 end (unused)
// APP address -- S8-s22
#define		FL_PFLASH_PF_APP_ADDR_Start0			(IfxFlash_pFlashTableLog[8].start) 	// PF0 S8 start -- APP start
#define		FL_PFLASH_PF_APP_ADDR_Start1			(IfxFlash_pFlashTableLog[16].start)	// Unused
#define		FL_PFLASH_PF_APP_ADDR_Start2			(IfxFlash_pFlashTableLog[20].start)	// Unused
#define		FL_PFLASH_PF_APP_ADDR_End  				(IfxFlash_pFlashTableLog[22].end) 	// PF0 S23 end -- APP end (unused)

// APP_Bak address -- S23-S26
#define		FL_PFLASH_PF_APP_Bak0_ADDR_Start 		(IfxFlash_pFlashTableLog[23].start) // PF0 S23 start -- APP_Bak start 0
#define		FL_PFLASH_PF_APP_Bak1_ADDR_Start 		(IfxFlash_pFlashTableLog[25].start) // PF0 S25 start -- APP_Bak start 1	Unused
#define		FL_PFLASH_PF_APP_Bak2_ADDR_Start 		(IfxFlash_pFlashTableLog[26].start) // PF0 S26 start -- APP_Bak start 2  	Unused
#define		FL_PFLASH_PF_APP_Bak_ADDR_End  			(IfxFlash_pFlashTableLog[26].end) 	// PF0 S26 end -- APP_Bak end	Unused


#define     FL_DFLASH_LOGICAL_SECTOR_Nr             IFXFLASH_DFLASH_NUM_LOG_SECTORS
#define     FL_DFLASH_LOGICAL_SECTOR_Addr           0xaf000000u       /* 8k */
#define     FL_DFLASH_LOGICAL_SECTOR_Length         0x2000u       /* 8k */
#define     FL_DFLASH_ADDR_Start                (IfxFlash_dFlashTableEepLog[0].start)
#define     FL_DFLASH_ADDR_End                  (IfxFlash_dFlashTableEepLog[IFXFLASH_DFLASH_NUM_LOG_SECTORS-1].end)
#define     FL_DFLASH_ALIGNMENT_Size                (4)
#endif

typedef struct
{
        /* block start global address */
         uint32 address;
        /* block length */
         uint32 length;
}FL_sector_stt;

extern  FL_sector_stt   cPflashInfo[FL_PFLASH_LOGICAL_SECTOR_Nr];

/* App update flag */
#define FL_APP_Update_FLAG_Addr		    0xaf000000// One page data, 8k
/* Enter APP or backup flag */
#define FL_APP_Bak_Enter_FLAG_Addr		0xaf002000// One page data, 8k
/* Confirm enter APP or backup flag */
#define FL_APP_Second_FLAG_Addr		    0xaf004000// One page data, 8k

/* App flag address in FLASH (Local Address) */
//#define     FL_APP_FLAG_Addr           0xa001c000 /* PF0 S7 */
//#define     FL_APP_FLAG_Addr            0x80010000u  /* PF0 s4 */
#define     FL_APP_FLAG                 0xa55au
#define     FL_APP_FLAG_Addr            0xa0018000  /* PF0 s6 .not BMI */

 /* APP application address */
#define     FL_APP_PRG_PFLASH_Addr             0x80020000  /* PF0 S8 */
#define     FL_APP_PRG_PFLASH_Addr1            0x800f0000  /* PF0 S22 */	// Unused
/* APP backup address */
#define     FL_APP_PRG_PFLASH_Addr_Bak         0x80140000  /* PF0 S24 */

/* BL prg addr */
#define     FL_BL_PRG_PFLASH_Addr             0x80000000  /* PF0 S0 */		// Unused
/* flash map */
#define     FL_MAP_0_PFLASH_Addr              0x80030000  /* PF0 S10 */		// Unused

#define		BACKUP_DATE							0x00240625u		// Backup date flag
/*
 * Timer
 */
#define     TMR_PERIOD_TIME_MS      10		// Timer period 10MS
#define     TMR_WAIT_FOR_START      100		// Boot wait time
#define     TMR_LED1                100

/*
 * BL status
 */
typedef enum
{
    connected   =0x10,
    jumpToApp   =0xa0,
    writeFlag   =0xf0,
	jumpToBL	=0x90,
}bl_s2_cmd_enum;


/*********************************************************************************************************************/
/*------------------------------------------A/B Dual Bank Configuration----------------------------------------------*/
/*********************************************************************************************************************/
/*
 * Below definitions are for the new A/B Dual Bank boot scheme.
 * Legacy single-bank flag addresses (FL_APP_FLAG_Addr, etc.) are kept for
 * backward compatibility during migration.
 */

/* Bank cached (local) base addresses */
#define     DUALBANK_APP_A_CACHED_ADDR      0x80020000u     /* Bank A start (S8)  - Cached view */
#define     DUALBANK_APP_B_CACHED_ADDR      0x80100000u     /* Bank B start (S23) - Cached view */
#define     DUALBANK_APP_A_UNCACHED_ADDR    0xA0020000u     /* Bank A uncached view */
#define     DUALBANK_APP_B_UNCACHED_ADDR    0xA0100000u     /* Bank B uncached view */

/* Bank size (must match APP image size in LSL) */
#define     DUALBANK_APP_A_SIZE             (896u * 1024u)  /* S8~S22 = 896KB */
#define     DUALBANK_APP_B_SIZE             (1024u * 1024u) /* S23~S26 = 1MB */
#define     DUALBANK_APP_SIZE               DUALBANK_APP_A_SIZE

/* Backward-compatible macros for Boot_DualBank.c */
#define     BANK_APP_A_SIZE                 DUALBANK_APP_A_SIZE
#define     BANK_APP_B_SIZE                 DUALBANK_APP_B_SIZE

/* DFlash flag area for dual-bank management */
#define     DUALBANK_FLAG_ADDR              0xAF000000u
#define     DUALBANK_FLAG_SHADOW_OFFSET     0x100u          /* 256 bytes offset, different DFlash page */

/* Software reset helper (also declared in App_bootloader.c, extern here for Boot_DualBank.c) */
extern void SW_Reset(void);


#endif /* APP_BOOTLOADER_CFG_H_ */
