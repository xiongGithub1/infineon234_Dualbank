
/**********************************************************************************************************************
 * \file    App_bootloader.c
 * \brief
 * \version V1.0.0
 *********************************************************************************************************************/



/*********************************************************************************************************************/
/*-----------------------------------------------------Includes------------------------------------------------------*/
/*********************************************************************************************************************/
#include "App_bootloader.h"
#include <App_bootloader_cfg.h>
#include "bsp.h"
#include "Can.h"
#include "IfxFlash.h"
#include "IfxFlash_cfg.h"
#include "Flash.h"
#include "Cpu0_Main.h"
#include "Tmr.h"
#include "Can_Session.h"
#include "custom_delay.h"
#include "CANRxTxInterface.h"
#include "uds_main.h"
#include "MultiCAN.h"
#include "uds_main.h"
#include "Boot_DualBank.h"


/*********************************************************************************************************************/
/*-------------------------------------------------Global variables--------------------------------------------------*/
/*********************************************************************************************************************/
#define bootTrapFlag	0x70000500;


/*********************************************************************************************************************/
/*--------------------------------------------Private Variables/Constants--------------------------------------------*/
/*********************************************************************************************************************/
#if 0
const uint16 cFlashFlag __at(FL_APP_FLAG_Addr) = 0xa55au;
#endif



//extern IFX_CONST IfxFlash_flashSector IfxFlash_dFlashTableEepLog[IFXFLASH_DFLASH_NUM_LOG_SECTORS];

/*
 * test
 */
//const   uint16 test1 __at(0xa001c000+0xf0) = 0x0;
//const   uint16 testflag __at(0xa001c000+0x100) = 0xffff;




backToBeforeCodeS g_backToBeforeCodeS;


/*********************************************************************************************************************/
/*------------------------------------------------Function Prototypes------------------------------------------------*/
/*********************************************************************************************************************/
void (*pAppSwEnter)(void);
void AppBL_GotoAppSW(void);


/*********************************************************************************************************************/
/*---------------------------------------------Function Implementations----------------------------------------------*/
/*********************************************************************************************************************/



/*********************************************************************************************
 * @brief Software reset
 ********************************************************************************************/
void SW_Reset(void)
{
	/* Ensure all Flash operations (PFlash/DFlash) are complete before reset
	 * to avoid bus error on reboot due to busy Flash controller */
	IfxFlash_waitUnbusy(FLASH_MODULE, IfxFlash_FlashType_P0);
	IfxFlash_waitUnbusy(FLASH_MODULE, IfxFlash_FlashType_D0);

	IfxScuWdt_clearSafetyEndinit(IfxScuWdt_getSafetyWatchdogPassword());
	IfxCpu_triggerSwReset();
	IfxScuWdt_setSafetyEndinit(IfxScuWdt_getSafetyWatchdogPassword());
}

/*
** ============================================================================
** @Function    Mcu_getCpuFreq
** @Description Get CPU/PLL/System/STM frequencies
** @Parameters  cpu : pointer to cpu_freq_stt structure
** @Returns     0 on success
** @Date
** ============================================================================
*/
int Mcu_getCpuFreq(cpu_freq_stt * cpu)
{
    /* Initialise the application state */
    cpu->pllFreq = IfxScuCcu_getPllFrequency();
    cpu->cpuFreq = IfxScuCcu_getCpuFrequency(IfxCpu_getCoreIndex());
    cpu->sysFreq = IfxScuCcu_getSpbFrequency();
    cpu->stmFreq = IfxStm_getFrequency(&MODULE_STM0);

    return 0;
}

/*
** ============================================================================
** @Function    AppBL_cpyMem
** @Description Memory copy
** @Parameters  dest   : destination buffer
**              source : source buffer
**              length : copy length in bytes
** @Returns     None
** @Date
** ============================================================================
*/
void AppBL_cpyMem(uint8 * dest, const uint8 *source, uint32 length)
{
    while (length > 0)
    {
        if ((dest != NULLPTR) && (source != NULLPTR))
        {
            *dest = *source;
            dest ++;
            source ++;
        }
        else
        {
            break;
        }

        length --;
    }
    return;
}



#define APP_StartADDR 					0x80020000
#define APP_StartWriteADDR 				0xa0020000
#define BACKUP_StartADDR  				0x80100000
#define check_SIZE                 		20

/* FlashBlock : start address + size */
typedef struct {
    uint32 startAddr;
    uint32 size;
} FlashBlock;

/* Blocks to check for backup/restore (PS8-PS15 + PS16-PS19) */
const FlashBlock blocksToCheck[] = {
    {0x80020000, 32 * 1024 / 32},   // PS8
    {0x80028000, 32 * 1024 / 32},   // PS9
    {0x80030000, 32 * 1024 / 32},   // PS10
    {0x80038000, 32 * 1024 / 32},   // PS11
    {0x80040000, 32 * 1024 / 32},   // PS12

    {0x80048000, 32 * 1024 / 32},   // PS13
	{0x80050000, 32 * 1024 / 32},   // PS14
	{0x80058000, 32 * 1024 / 32},   // PS15
	{0x80060000, 64 * 1024 / 32},   // PS16
	{0x80070000, 64 * 1024 / 32},   // PS17

	{0x80080000, 64 * 1024 / 32},   // PS18
	{0x80090000, 64 * 1024 / 32},   // PS19
//	{0x800A0000, 128 * 1024 / 32},   // PS20
};


const uint32 flash_block_size = sizeof(blocksToCheck) / sizeof(FlashBlock);


/* Backup APP blocks to backup area (PS23) */
void Flash_BackupAppBlocks(void)
{
	Flash_erasePFlash_port(IfxFlash_pFlashTableLog[23].start);
	Flash_erasePFlash_port(IfxFlash_pFlashTableLog[24].start);
	delay_ms(2);

	uint32 cycle_index = 0;
	for (int block = 0; block < flash_block_size; block++)
	{
		uint32 mainAddr = blocksToCheck[block].startAddr;
		/* Copy main APP data to backup area */
		 for(uint32 page = 0; page<blocksToCheck[block].size; page++)
		 {
			 uint32 *code_data = ( uint32 *)(mainAddr + (page*32)) ;
			 Flash_writePFlash_port((IfxFlash_pFlashTableLog[23].start + cycle_index*32), code_data, PFLASH_PAGE_LENGTH);
			 cycle_index++;
		 }
	}

	uint32 *backupAdress;
	uint32 *targetAdress;
	uint32 *finishAdress;
	uint32 *interruptAdress[500]={0};
	uint16 i = 0;
	uint32 DflashData[2];

	backupAdress = (uint32*)IfxFlash_pFlashTableLog[23].start;
	targetAdress = (uint32*)IfxFlash_pFlashTableLog[8].start;
	finishAdress = (uint32*)(IfxFlash_pFlashTableLog[19].end - 4);
	do{
		if(*backupAdress == *targetAdress)
		{
			backupAdress++;
			targetAdress++;
		}
		else
		{
			i++;
			if(i>=499)
			{
				i=499;
			}
			interruptAdress[i] = targetAdress;
			backupAdress++;
			targetAdress++;
		}
	}while(targetAdress <= finishAdress);

	if(interruptAdress[499] == 0)
	{
		DflashData[0] = BACKUP_DATE;
		DflashData[1] = 0;
		Flash_eraseDFlash_port(FL_APP_Update_FLAG_Addr);
		Flash_writePFlash_port( FL_APP_Update_FLAG_Addr, DflashData,  DFLASH_PAGE_LENGTH);
	}
}


/* Restore deleted blocks from backup */
void Flash_RestoreDeletedBlocks(void)
{
	for (int block = 0; block < sizeof(blocksToCheck) / sizeof(FlashBlock); block++)
	{
		Flash_erasePFlash_port(blocksToCheck[block].startAddr);
	}
	delay_ms(2);
	for (int block = 0; block < sizeof(blocksToCheck) / sizeof(FlashBlock); block++)
    {
        uint32 mainAddr = blocksToCheck[block].startAddr;
        uint32 backupAddr = BACKUP_StartADDR + (mainAddr - APP_StartADDR);

		/* Write backup data to main area */
		 for(uint32 page = 0; page<blocksToCheck[block].size; page++)
		 {
			 uint32 *code_data = ( uint32 *)(backupAddr + (page*32)) ;
			 Flash_writePFlash_port((((mainAddr& 0x00FFFFFF) | 0xA0000000)+page*32), code_data, PFLASH_PAGE_LENGTH);
		 }
    }

	/* Verify restored data matches backup */
	uint32 *backupAdress;
	uint32 *targetAdress;
	uint32 *finishAdress;
	uint32 *interruptAdress[500]={0};
	uint16 j = 0;
	uint32 data[8];

	backupAdress = (uint32*)IfxFlash_pFlashTableLog[23].start;
	targetAdress = (uint32*)IfxFlash_pFlashTableLog[8].start;
	finishAdress = (uint32*)(IfxFlash_pFlashTableLog[19].end - 4);
	do{
		if(*backupAdress == *targetAdress)
		{
			backupAdress++;
			targetAdress++;
		}
		else
		{
			j++;
			if(j>=499)
			{
				j=499;
			}
			interruptAdress[j] = targetAdress;
			backupAdress++;
			targetAdress++;
		}
	}while(targetAdress <= finishAdress);

	if(interruptAdress[499] == 0)
	{
		/* 1. erase sector */
		Flash_erasePFlash_port( FL_APP_FLAG_Addr );
		/* 2  write page */
		for(uint8 i=0;i<8;i++)
		{
			data[i] = 0;
		}
		data[0] = FL_APP_FLAG;
		Flash_writePFlash_port( FL_APP_FLAG_Addr, data,  PFLASH_PAGE_LENGTH);    /* min = 1page = 32byte */
	}
}


/*
 ** ============================================================================
 ** @Function    AppBL_GotoAppSW
 ** @Description Jump from Bootloader to APP reset vector address
 ** @Parameters  None
 ** @Returns     None
 ** @Date
 ** ============================================================================
 */

void AppBL_GotoAppSW(void)
{
    /* Dual Bank: jump to currently active bank */
    uint32 activeBank = Boot_DualBank_GetActiveBank();
    Boot_DualBank_JumpToBank(activeBank);
}

/*
 ** ============================================================================
 ** @Function    AppBL_init
 ** @Description Bootloader initialization
 ** @Parameters  None
 ** @Returns     None
 ** @Date
 ** ============================================================================
 */

void AppBL_init(void)
{
    /* OEM: mark bootloader entry phase */
    g_bootPhase = BOOT_PHASE_BL_ENTRY;

    /* mcu star init */
    Mcu_getCpuFreq(&gAppData.cpuFreq);		// Get CPU frequency

    /*  read  RAM,FLASH  flag */
    gAppData.ramFlag = *(uint16 *)RAM_BOOT_MODE_Addr;	// Read RAM flag
    gAppData.pRamFlag = (uint16 *)RAM_BOOT_MODE_Addr;	// RAM flag pointer

    /*  Disable ECC Trap */
    IfxScuWdt_clearCpuEndinit(IfxScuWdt_getCpuWatchdogPassword());
    FLASH0_MARP.B.TRAPDIS = 1; // PFLSH Disable ECC TRAP
    FLASH0_MARD.B.TRAPDIS = 1; // DFLSH Disable ECC TRAP
    IfxScuWdt_setCpuEndinit(IfxScuWdt_getCpuWatchdogPassword());

    /* read addr */
    gAppData.flashFlag  = *(uint16 *)FL_APP_FLAG_Addr;		// Read flash flag

    /* Check RAM flag, write default at power-on */
    if( gAppData.ramFlag != RAM_BOOT_MODE_KEEP)
    {
        gAppData.ramFlag = RAM_BOOT_MODE_NORMAL;
        *gAppData.pRamFlag = RAM_BOOT_MODE_NORMAL;
    }

    /* init can module */
	Multican_init();
	UdsInit(UDS_FUN_ADDR_ID,UDS_PHY_ADDR_ID,UDS_RESP_ADDR_ID);

    initTime();

    /* flash */
    Flash_init();

    /* bsp  tmr */
    TMR_init();

    return;
}




/*
 ** ============================================================================
 ** @Function    AppBL_main
 ** @Description Main bootloader loop
 ** @Parameters  None
 ** @Returns     None
 ** @Date
 ** ============================================================================
 */
//uint8 WritePFlash =0;
uint8 can_node1_error=0;
void    AppBL_main(void)
{
    /* Dual Bank startup decision has been handled in Cpu0_Main.c via
     * Boot_DualBank_SelectAndJump(). If we reach here, both banks are
     * invalid or explicit bootloader mode is requested.
     * Skip legacy single-bank startup checks.
     */

    /* OEM: mark bootloader main loop phase */
    g_bootPhase = BOOT_PHASE_BL_MAIN;

	/* Bootloader main loop */

    // Can_RxIndicationMainFunc(); // Replaced by interrupt isrCAN1_RX

  	// Can9252RxLookup(); // Replaced by interrupt isrCAN0_RX
	UdsMainProcess();
	CanMainProcess();

	/* CAN bus error handling */
	if((CAN_NSR1.B.BOFF==1) && (CAN_NSR1.B.LEC==0x5)) // Bus-off
	{
		can_node1_error = 1;
	}
	else if((CAN_NSR1.B.EWRN==1) && (CAN_NSR1.B.LEC==0x3))  // Ack Error
	{
		can_node1_error = 2;
	}
	else if((CAN_NSR1.B.BOFF==0)&& (CAN_NSR1.B.LEC==0x0))
	{
		can_node1_error = 0;
	}

	if(((CAN_NSR1.B.BOFF==1) &&(CAN_NSR1.B.LEC==0x5))||
	   ((CAN_NSR1.B.EWRN==1) && (CAN_NSR1.B.LEC==0x3)))
	{
		CAN_NCR1.U &= ~((1<<6) | 1);   //reset CCE and INIT
	}



    return ;
}


