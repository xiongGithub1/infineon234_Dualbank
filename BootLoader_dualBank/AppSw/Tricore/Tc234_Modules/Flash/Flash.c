
/**********************************************************************************************************************
 * \file    Flash.c
 * \brief
 * \version V1.0.0
 *********************************************************************************************************************/



/*********************************************************************************************************************/
/*-----------------------------------------------------Includes------------------------------------------------------*/
/*********************************************************************************************************************/
#include <string.h>
#include "Ifx_Types.h"
#include "Flash.h"
#include "IfxFlash.h"
#include "IfxCpu.h"
#include "Cpu0_Main.h"



/*********************************************************************************************************************/
/*-------------------------------------------------Global variables--------------------------------------------------*/
/*********************************************************************************************************************/
Function g_commandFromPSPR;

uint32 gPageData[8];


/*********************************************************************************************************************/
/*--------------------------------------------Private Variables/Constants--------------------------------------------*/
/*********************************************************************************************************************/

/*********************************************************************************************************************/
/*------------------------------------------------Function Prototypes------------------------------------------------*/
/*********************************************************************************************************************/


/*********************************************************************************************************************/
/*---------------------------------------------Function Implementations----------------------------------------------*/
/*********************************************************************************************************************/



/*
 ** ============================================================================
 ** @Function    锛�
 ** @Description 锛�      鎿﹂櫎涓�涓墖鍖�      鍦≒SPR涓繍琛�
 ** @Parameters  锛�
 ** @Returns     锛�
 ** @Date        锛�
 ** ============================================================================
 */

void Flash_erasePFLASH(uint32 sectorAddr)
{
    /* Get the current password of the Safety WatchDog module */
    uint16 endInitSafetyPassword = IfxScuWdt_getSafetyWatchdogPasswordInline();

    /* Erase the sector */
    IfxScuWdt_clearSafetyEndinitInline(endInitSafetyPassword);      /* Disable EndInit protection                   */
    g_commandFromPSPR.eraseSectors(sectorAddr, 1);                  /* Erase the given sector                      */
    IfxScuWdt_setSafetyEndinitInline(endInitSafetyPassword);        /* Enable EndInit protection                    */

    /* Wait until the sector is erased */
    g_commandFromPSPR.waitUnbusy(FLASH_MODULE, PROGRAM_FLASH_0);

    return ;
}
/*
 ** ============================================================================
 ** @Function    锛�
 ** @Description 锛�
 ** @Parameters  锛�
 ** @Returns     锛�
 ** @Date        锛�
 ** ============================================================================
 */
void Flash_writePFlashPage(uint32 startingAddr, uint32 *data, uint32 byteLength)
{
    uint32 page,pageTotal;                                                /* Variable to cycle over all the pages             */
    uint32 offset;                                                      /* Variable to cycle over all the words in a page   */

    /* Get the current password of the Safety WatchDog module */
    uint16 endInitSafetyPassword = IfxScuWdt_getSafetyWatchdogPasswordInline();

    /* Write all the pages */
    pageTotal = byteLength / PFLASH_PAGE_LENGTH;
    for(page = 0; page < pageTotal; page++)              /* Loop over all the pages                  */
    {
        uint32 pageAddr = startingAddr + (page * PFLASH_PAGE_LENGTH);   /* Get the address of the page              */

        /* Enter in page mode */
        g_commandFromPSPR.enterPageMode(pageAddr);

        /* Wait until page mode is entered */
        g_commandFromPSPR.waitUnbusy(FLASH_MODULE, PROGRAM_FLASH_0);

        /* Write 32 bytes (8 double words) into the assembly buffer */
        for(offset = 0; offset < PFLASH_PAGE_LENGTH; offset += 0x8)     /* Loop over the page length                */
        {
            g_commandFromPSPR.load2X32bits(pageAddr, data[(page*8)+(offset/4)], data[(page*8)+(offset/4)+1] );    /* Load 2 words of 32 bits each */
        }

        /* Write the page */
        IfxScuWdt_clearSafetyEndinitInline(endInitSafetyPassword);      /* Disable EndInit protection               */
        g_commandFromPSPR.writePage(pageAddr);                          /* Write the page                           */
        IfxScuWdt_setSafetyEndinitInline(endInitSafetyPassword);        /* Enable EndInit protection                */

        /* Wait until the page is written in the Program Flash memory */
        g_commandFromPSPR.waitUnbusy(FLASH_MODULE, PROGRAM_FLASH_0);
    }


    return ;
}


/*
 ** ============================================================================
 ** @Function    锛�
 ** @Description 锛�       example
 ** @Parameters  锛�
 ** @Returns     锛�
 ** @Date        锛�
 ** ============================================================================
 */
#if 0
//void writePFlashPage(uint32 startingAddr)
void writePFlashPage(uint32 startingAddr, uint32 *data, uint32 byteLength)
//void Flash_writePFlashPage(uint32 pageAddr)
{
    uint32 page;                                                /* Variable to cycle over all the pages             */
    uint32 offset;                                              /* Variable to cycle over all the words in a page   */

    /* Get the current password of the Safety WatchDog module */
    uint16 endInitSafetyPassword = IfxScuWdt_getSafetyWatchdogPasswordInline();

    /* Write all the pages */
    for(page = 0; page < PFLASH_NUM_PAGE_TO_FLASH; page++)              /* Loop over all the pages                  */
    {
        uint32 pageAddr = startingAddr + (page * PFLASH_PAGE_LENGTH);   /* Get the address of the page              */

        /* Enter in page mode */
        g_commandFromPSPR.enterPageMode(pageAddr);

        /* Wait until page mode is entered */
        g_commandFromPSPR.waitUnbusy(FLASH_MODULE, PROGRAM_FLASH_0);

        /* Write 32 bytes (8 double words) into the assembly buffer */
        for(offset = 0; offset < PFLASH_PAGE_LENGTH; offset += 0x8)     /* Loop over the page length                */
        {
            g_commandFromPSPR.load2X32bits(pageAddr, FL_APP_FLAG, FL_APP_FLAG); /* Load 2 words of 32 bits each */
        }

        /* Write the page */
        IfxScuWdt_clearSafetyEndinitInline(endInitSafetyPassword);      /* Disable EndInit protection               */
        g_commandFromPSPR.writePage(pageAddr);                          /* Write the page                           */
        IfxScuWdt_setSafetyEndinitInline(endInitSafetyPassword);        /* Enable EndInit protection                */

        /* Wait until the page is written in the Program Flash memory */
        g_commandFromPSPR.waitUnbusy(FLASH_MODULE, PROGRAM_FLASH_0);
    }

    return ;
}

/*
 ** ============================================================================
 ** @Function    锛�
 ** @Description 锛�
 ** @Parameters  锛�          refresh bl
 ** @Returns     锛�
 ** @Date        锛�
 ** ============================================================================
 */

int     Flash_writePFlashPage_main(uint32 addr,  uint32 length,  uint32 *data)
{
    boolean interruptState = IfxCpu_disableInterrupts(); /* Get the current state of the interrupts and disable them*/

    /* Copy all the needed functions to the PSPR memory to avoid overwriting them during the flash execution */
    copyFunctionsToPSPR();

    /* Erase the Program Flash sector before writing */
    g_commandFromPSPR.eraseFlash(addr);

    /* Write the Program Flash */
    g_commandFromPSPR.writeFlash(addr);

    /* Restore the interrupts state                            */
    IfxCpu_restoreInterrupts(interruptState);

    return 0;
}




/*
 ** ============================================================================
 ** @Function    锛�
 ** @Description 锛�
 *              D-Flash
 *              LOGIC sector=8kb=0x2000b
 *              1 page = 8 bytes
 ** @Parameters  锛�
 **             addr
 **
 ** @Returns     锛�
 ** @Date        锛�
 ** ============================================================================
 */
void  Flash_writeDataFlashPage(uint32 pageaddr,   uint32 *data)
{
    /* --------------- ERASE PROCESS --------------- */
    /* Get the current password of the Safety WatchDog module */
    uint16 endInitSafetyPassword = IfxScuWdt_getSafetyWatchdogPassword();

    /* Erase the sector */
#if 0
    IfxScuWdt_clearSafetyEndinit(endInitSafetyPassword);        /* Disable EndInit protection                       */
    IfxFlash_eraseMultipleSectors(addr, sectorcnt);    /* Erase the given sector           */
    IfxScuWdt_setSafetyEndinit(endInitSafetyPassword);          /* Enable EndInit protection                        */
#endif

    /* --------------- WRITE PROCESS --------------- */
    /* Enter in page mode */
    IfxFlash_enterPageMode(pageaddr);

    /* Wait until page mode is entered */
    IfxFlash_waitUnbusy(FLASH_MODULE, DATA_FLASH_0);

    /* Load data to be written in the page ,8bytes */
    IfxFlash_loadPage2X32(pageaddr, data[0], data[1]);      /* Load two words of 32 bits each            */

    /* Write the loaded page */
    IfxScuWdt_clearSafetyEndinit(endInitSafetyPassword);    /* Disable EndInit protection                       */
    IfxFlash_writePage(pageaddr);                           /* Write the page                                   */
    IfxScuWdt_setSafetyEndinit(endInitSafetyPassword);      /* Enable EndInit protection                        */

    /* Wait until the data is written in the Data Flash memory */
    IfxFlash_waitUnbusy(FLASH_MODULE, DATA_FLASH_0);

    return ;
}

/*
 ** ============================================================================
 ** @Function    锛�
 ** @Description 锛�
 ** @Parameters  锛�      length : unit :bytes
 ** @Returns     锛�
 ** @Date        锛�
 ** ============================================================================
 */
void Flash_verifyFlashData(uint32 flashAddr,  uint32 *data ,  uint32 length)
{
    uint32 offset;        /* Variable to cycle over all the words in a page   */
    int  errors = 0;      /* Variable to keep record of the errors            */

    /* Verify the written data */


    for(offset = 0; offset < DFLASH_PAGE_LENGTH; offset += 0x4)   /* Loop over the page length    */
    {
        /* Check if the data in the Data Flash is correct */
        if(MEM(flashAddr + offset) != data[offset / 4 ])
        {
            /* If not, count the found errors */
            errors++;
        }
    }

    return ;
}
#endif


/*
 ** ============================================================================
 ** @Function    锛氳繖涓嚱鏁板皢鎿﹂櫎鍜岀▼搴忎緥绋嬪鍒跺埌CPU0鐨刾rogram scratche - pad SRAM (PSPR)涓紝骞跺皢鍑芥暟鎸囬拡璧嬬粰瀹冧滑銆�
 ** @Description 锛�
 **          This function copies the erase and program routines to the Program Scratch-Pad SRAM (PSPR) of the CPU0 and assigns
 * function pointers to them.
 ** @Parameters  锛�
 ** @Returns     锛�
 ** @Date        锛�
 ** ============================================================================
 */
// 杩欐鍑芥暟鐨勫姛鑳芥槸鎶婄浉鍏冲嚱鏁扮殑绋嬪簭鏁版嵁鎷疯礉鍒癈PU0鐨刾rogram scratche-pad SRAM (PSPR)涓紝涓存椂瀛樺偍绋嬪簭锛屼互渚垮悗鏈熷彲浠ュ揩閫熺殑璋冪敤杩愯锛屾浘鍐�20220725
void Flash_copyFunctionsToPSPR(void)
{
	/* Copy the IfxFlash_eraseMultipleSectors() routine and assign it to a function pointer */
    memcpy((void *)ERASESECTOR_ADDR, (const void *)IfxFlash_eraseMultipleSectors, ERASESECTOR_LEN);
    g_commandFromPSPR.eraseSectors = (void *)ERASESECTOR_ADDR;

    /* Copy the IfxFlash_waitUnbusy() routine and assign it to a function pointer */
    memcpy((void *)WAITUNBUSY_ADDR, (const void *)IfxFlash_waitUnbusy, WAITUNBUSY_LEN);
    g_commandFromPSPR.waitUnbusy = (void *)WAITUNBUSY_ADDR;

    /* Copy the IfxFlash_enterPageMode() routine and assign it to a function pointer */
    memcpy((void *)ENTERPAGEMODE_ADDR, (const void *)IfxFlash_enterPageMode, ENTERPAGEMODE_LEN);
    g_commandFromPSPR.enterPageMode = (void *)ENTERPAGEMODE_ADDR;

    /* Copy the IfxFlash_loadPage2X32() routine and assign it to a function pointer */
    memcpy((void *)LOAD2X32_ADDR, (const void *)IfxFlash_loadPage2X32, LOADPAGE2X32_LEN);
    g_commandFromPSPR.load2X32bits = (void *)LOAD2X32_ADDR;

    /* Copy the IfxFlash_writePage() routine and assign it to a function pointer */
    memcpy((void *)WRITEPAGE_ADDR, (const void *)IfxFlash_writePage, WRITEPAGE_LEN);
    g_commandFromPSPR.writePage = (void *)WRITEPAGE_ADDR;

    /* Copy the erasePFLASH() routine and assign it to a function pointer */
    memcpy((void *)ERASEPFLASH_ADDR, (const void *)Flash_erasePFLASH, ERASEPFLASH_LEN);
    g_commandFromPSPR.eraseFlash = (void *)ERASEPFLASH_ADDR;

    /* Copy the writeFlash() routine and assign it to a function pointer */
    memcpy((void *)WRITEPFLASH_ADDR, (const void *)Flash_writePFlashPage, WRITEPFLASH_LEN);
//    memcpy((void *)WRITEPFLASH_ADDR, (const void *)writePFlashPage, WRITEPFLASH_LEN);
    g_commandFromPSPR.writeFlash = (void *)WRITEPFLASH_ADDR;

    return;
}


/*
 ** ============================================================================
 ** @Function    锟斤拷锟斤拷始锟斤拷Flash
 ** @Description 锟斤拷
 ** @Parameters  锟斤拷
 ** @Returns     锟斤拷
 ** @Date        锟斤拷
 ** ============================================================================
 */
void Flash_init(void)
{
    Flash_copyFunctionsToPSPR();

    return;
}

/*
 ** ============================================================================
 ** @Function    锛�
 ** @Description 锛�
 ** @Parameters  锛�
 ** @Returns     锛�
 ** @Date        锛�
 ** ============================================================================
 */
int Flash_erasePFlash_port(uint32 flashAddr)
{
    boolean interruptState = IfxCpu_disableInterrupts(); /* Get the current state of the interrupts and disable them*/
    uint16 wdtPassword = IfxScuWdt_getCpuWatchdogPassword();
    Flash_copyFunctionsToPSPR();
    g_commandFromPSPR.eraseFlash(flashAddr);
    IfxCpu_restoreInterrupts(interruptState);            /* Restore the interrupts state                            */

    /* Service watchdog after flash operation (interrupts were disabled, main loop could not feed) */
    IfxScuWdt_serviceCpuWatchdog(wdtPassword);

    /* Check PFlash erase error flags: EVER (erase verify), OPER (operation), SQER (sequence), PROER (protection) */
//    if (FLASH0_FSR.B.EVER || FLASH0_FSR.B.OPER || FLASH0_FSR.B.SQER || FLASH0_FSR.B.PROER)
//    {
//        return -1;  /* Erase error detected */
//    }
    return 0;
}

/*
 ** ============================================================================
 ** @Function    锛氬線PFLASH杩涜鏁版嵁鍐欏叆
 ** @Description 锛�
 **                 姣忔鍐�1page=32bytes
 ** @Parameters  锛�
 ** @Returns     锛�
 ** @Date        锛�
 ** ============================================================================
 */
int  Flash_writePFlash_port(uint32 flashAddr, uint32 *data, uint32 bytelength)
//int  Flash_writePFlash_port(uint32 flashAddr)
{
    IfxPort_setPinState(LED2, IfxPort_State_high);

    boolean interruptState = IfxCpu_disableInterrupts(); /* Get the current state of the interrupts and disable them*/
    uint16 wdtPassword = IfxScuWdt_getCpuWatchdogPassword();
    Flash_copyFunctionsToPSPR();
    g_commandFromPSPR.writeFlash(flashAddr, data, bytelength);
    IfxCpu_restoreInterrupts(interruptState);            /* Restore the interrupts state                            */

    /* Service watchdog after flash operation */
    IfxScuWdt_serviceCpuWatchdog(wdtPassword);

    IfxPort_setPinState(LED2, IfxPort_State_low);

    /* Check PFlash program error flags: PVER (program verify), OPER, SQER, PROER */
//    if (FLASH0_FSR.B.PVER || FLASH0_FSR.B.OPER || FLASH0_FSR.B.SQER || FLASH0_FSR.B.PROER)
//    {
//        return -1;  /* Write error detected */
//    }
    return 0;
}

uint32 min(uint32 var1,uint32 var2)
{
	if(var1<var2)
		return var1;
	else
		return var2;
}
uint8 s_remainBuffer[32] = {0};  // 鏆傚瓨涓嶈冻32瀛楄妭鐨勬暟鎹�
uint32 s_remainAddr = 0;   // 鏆傚瓨鏁版嵁鐨勮捣濮嬪湴鍧�
uint32 s_remainSize = 0;   // 褰撳墠鏆傚瓨鐨勬暟鎹暱搴�


/**
 * @brief Write a 32-byte PFlash page and verify by reading back.
 * @param addr  Target address (must be PFlash uncached address, 32-byte aligned).
 * @param data  Pointer to 32 bytes of data to write.
 * @return 1 if write and verify succeeded, 0 if verify failed.
 */
static int Flash_writeAndVerifyPage(uint32 addr, uint8 *data)
{
    uint8 i;
    g_commandFromPSPR.writeFlash(addr, (uint32 *)data, 32);
    for (i = 0; i < 32; i++)
    {
        if (((volatile uint8 *)addr)[i] != data[i])
        {
            return 0;  /* Verify failed: written data does not match read-back */
        }
    }
    return 1;
}

void Flash_ForceWriteRemaining(void)//涓嶈冻32浣嶇殑鐩存帴鍐欏叆锛屼繚瀛樼殑鏁扮粍鐨勬暟鎹�
{
//	static uint32 s_remainBuffer32[8] = {0};
    if (s_remainSize > 0)
    {
        // 濉厖鍓╀綑閮ㄥ垎涓�0xFF
        memset(s_remainBuffer + s_remainSize, 0x00, 32 - s_remainSize);//涓嶆弧32浣嶇殑锛屾病鏁版嵁鐨勯兘濉啓0锛屼笉鐒朵細涔卞叆涓�浜涙暟鎹�

        // 鍐欏叆瀹屾暣鐨�32瀛楄妭鍧�
        g_commandFromPSPR.writeFlash(s_remainAddr, s_remainBuffer, 32);

        // 閲嶇疆鐘舵��
        s_remainSize = 0;
    }
}

int Flash_writePFlash_portex(uint32 flashAddr, uint8 *data, uint32 byteLength)//鎸�32浣嶈繘琛屽啓鍏�
{
    boolean interruptState = IfxCpu_disableInterrupts(); /* Disable interrupts during flash operation */
    uint16 wdtPassword = IfxScuWdt_getCpuWatchdogPassword();
    int result = 1;
    uint32 offset = 0;
    uint32 currentAddr = flashAddr;//0xa0020040+126=0xa00200be

    // 1. 妫�鏌ユ槸鍚﹂渶瑕佷笌鏆傚瓨鏁版嵁鎷兼帴
    if (s_remainSize > 0)
    {
    	//妫�鏌ュ湴鍧�鏄惁涓庣紦瀛樺潡杩炵画
		if (currentAddr == (s_remainAddr + s_remainSize))//杩炵画瓒�128闀垮害鐨勬暟鎹�->0xa00200be=0xa00200a0+30
    	{
            uint32 needed = 32 - s_remainSize;  // 闇�瑕佽ˉ鍏呯殑瀛楄妭鏁�
            uint32 copySize = min(needed, byteLength);

            // 缁勫悎鎴愬畬鏁寸殑32瀛楄妭鍧�
            memcpy(s_remainBuffer + s_remainSize, data, copySize);//(0xa00200be,鍓嶄袱涓暟鎹紝2)

            // 绔嬪嵆鍐欏叆瀹屾暣椤碉紙浣跨敤缂撳瓨鍦板潃锛�
			// g_commandFromPSPR.writeFlash(s_remainAddr, s_remainBuffer, 32);//(0xa00200a0,32涓暟鎹紝32)
            if(Flash_writeAndVerifyPage(s_remainAddr, s_remainBuffer) == 0)
            {
                result = 0;
//                goto flash_write_exit;
            }

            offset += copySize;//offset->0+2=2
			currentAddr += copySize;//0xa00200be+2=0xa00200c0
			s_remainSize = 0;
        }
		else//闈炶繛缁殑鏁版嵁
        {
			if((s_remainAddr + 32) < currentAddr)
			{
				// 鍦板潃涓嶈繛缁紝寮哄埗鍐欏叆鏃х紦瀛�
				Flash_ForceWriteRemaining();
			}
			else //if((s_remainAddr + 32) > currentAddr)
			{
				//鍋囪涔嬪墠涓嬭浇鐨勫湴鍧�涓猴細s_remainAddr锛�0x800451E0 + 0x20锛堥暱搴︼級 = 0x80045200 > 0x800451F4
				//涔嬪墠浣欎笅鐨勬暟鎹瓧鑺傛暟鍙湁锛歴_remainSize锛�10锛屽洜姝や笂涓�娆′笅杞藉畬鍚庣殑鍦板潃涓猴細0x800451E0 + 0xA = 0x800451EA
				//鑰屾柊鍦板潃涓�0x800451F4锛岄偅涔�0x80045200 - 0x800451F4 =  0xC锛屽嵆涓簄eedBetysLength
				//(s_remainBuffer + 32 - needBetysLength) = 0x800451E0 + 0x20 -0xC = 0x800451F4锛屾柊鎷疯礉鐨勬暟鎹粠0x800451F4寮�濮�
				//鍋囪鏂版嫹璐濇暟鎹彧鏈�4涓紝鍗砨yteLength锛岄偅涔堢┖浣欏瓧鑺傝繕搴斾綑8涓紝宸插崰鐢ㄧ殑瀛楄妭搴旀槸32-8=24,
				//閭ｄ箞32 - (needBetysLength - byteLength) = 24绗﹀悎涓婇潰鎺ㄦ柇锛屽簲鏄纭殑锛屾浘鍐�20250717
				uint32 needBetysLength = s_remainAddr + 32 - currentAddr;
				// 0x80045200 + 0x20 = 0x80045220 - 0x80045208 = 24
				uint32 copySize2 = min(needBetysLength, byteLength);	// 鍋囪 byteLength = 4锛岃�宯eedBetysLength = 12
				if((32 - needBetysLength) > s_remainSize)
				{
					memcpy((s_remainBuffer + 32 - needBetysLength), data, copySize2);
					//(s_remainBuffer + 32 - needBetysLength)
					if(needBetysLength <= byteLength)
                    {
                        // g_commandFromPSPR.writeFlash(s_remainAddr, s_remainBuffer, 32);//(0xa00200a0,32涓暟鎹紝32)
                        if (Flash_writeAndVerifyPage(s_remainAddr, s_remainBuffer) == 0)
							{
								result = 0;  /* Write-verify failed */
//								goto flash_write_exit;
							}//(0xa00200a0,32锟斤拷锟斤拷锟捷ｏ拷32)
						offset += copySize2;//offset->0+2=2
						currentAddr += copySize2;//0xa00200be+2=0xa00200c0
						s_remainSize = 0;
					}
					else // 锟斤拷为锟斤拷锟斤拷一页锟斤拷锟饺达拷锟斤拷一锟斤拷循锟斤拷
					{
						s_remainSize = 32 - (needBetysLength - byteLength);	//
					}
				}
				else
				{
					result = 0;
//					goto flash_write_exit;
				}
			}
    	}
    }

    // 2. 鍐欏叆瀹屾暣鐨�32瀛楄妭鍧�
    //鎷�168涓暱搴﹁繘琛屼妇渚嬶紝
    //绗竴娆�0x36鍙戦�佺殑鏁版嵁闀垮害鏄�126涓�
    //鈶燼0020040->a0020060(32)->offset=(0->32)
    //鈶0020060->a0020080(32)->offset=(32->64)
	//鈶0020080->a00200a0(32)->offset=(64->96)
    //鈶00200a0->a00200c0(32)->offset=96+32<126脳涓嶆墽琛岋紝offset=96->鍦ㄤ笂闈㈢殑鎷兼帴32鍐欏叆浜�
    //绗簩娆�0x36鍙戦�佺殑鏁版嵁闀垮害鏄�42涓�
    //鈶燼00200c0->a00200e0(32)->offset=(2->34)
    //鈶00200e0->a00200f0(8)->offset=34+32=66<42脳锛宱ffset=34
    while (offset + 32 <= byteLength) //126
    {
	    // g_commandFromPSPR.writeFlash(currentAddr, data + offset, 32);//鍐欏叆涓�椤�
        if (Flash_writeAndVerifyPage(currentAddr, data + offset) == 0)
        {
            s_remainSize = 0;  /* Clear buffer state on error */
            result = 0;          /* Write-verify failed */
            goto flash_write_exit;
        }
        offset += 32;
        currentAddr += 32; // 鍏抽敭锛氭瘡娆″啓鍏ュ悗閫掑32
    }

    // 3. 澶勭悊鍓╀綑涓嶈冻32瀛楄妭鐨勬暟鎹�
    if ((offset < byteLength)&&(s_remainSize == 0)) //鈶�96<126  鈶�66<42
    {
		// 璁＄畻瀵归綈鍦板潃锛�32瀛楄妭杈圭晫锛�
		s_remainAddr = currentAddr;//鈶�0xa00200a0  鈶00200e0
		// 瀛樺偍鍓╀綑鏁版嵁
		s_remainSize = byteLength - offset;//鈶�126-96=30  鈶�42-2-32=8
		for(uint8 i = 0;i<32;i++)
		{
			s_remainBuffer[i]=0;
		}
		memcpy(s_remainBuffer, data + offset, s_remainSize);//鈶�(s_remainBuffer,data+96,30)  鈶�(s_remainBuffer,data+34,8)
    }


flash_write_exit:
    IfxScuWdt_serviceCpuWatchdog(wdtPassword);
    IfxCpu_restoreInterrupts(interruptState);
    return result;
}



/*
 ** ============================================================================
 ** @Function    锛�
 ** @Description 锛�
 ** @Parameters  锛�
 ** @Returns     锛�
 ** @Date        锛�
 ** ============================================================================
 */
int Flash_eraseDFlash_port(uint32 flashAddr)
{
    uint16 endInitSafetyPassword = IfxScuWdt_getSafetyWatchdogPassword();

    boolean interruptState = IfxCpu_disableInterrupts(); /* Get the current state of the interrupts and disable them*/
    IfxScuWdt_clearSafetyEndinit(endInitSafetyPassword);        /* Disable EndInit protection                       */
    IfxFlash_eraseMultipleSectors(flashAddr, 1); /* Erase the given sector           */
//    IfxFlash_eraseSector(IfxFlash_dFlashTableEepLog[flashAddr].start);
    IfxScuWdt_setSafetyEndinit(endInitSafetyPassword);          /* Enable EndInit protection                        */
    IfxCpu_restoreInterrupts(interruptState);                  /* Restore the interrupts state                     */

    return 0;
}

/*
 ** ============================================================================
 ** @Function    锛�
 ** @Description 锛�
 **                 姣忔鍐�1page=8bytes
 ** @Parameters  锛�
 ** @Returns     锛�
 ** @Date        锛�
 ** ============================================================================
 */
int  Flash_writeDFlash_port(uint32 flashAddr, uint32 *data, uint32 bytelength )
{
    uint32 page,pageTotal;                                                /* Variable to cycle over all the pages             */

    /* Get the current password of the Safety WatchDog module */
    uint16 endInitSafetyPassword = IfxScuWdt_getSafetyWatchdogPasswordInline();

    IfxPort_setPinState(LED2, IfxPort_State_high);

    /* Wait until the sector is erased */
    IfxFlash_waitUnbusy(FLASH_MODULE, DATA_FLASH_0);

    /* --------------- WRITE PROCESS --------------- */
    pageTotal = bytelength / DFLASH_PAGE_LENGTH;
    for(page = 0; page < pageTotal; page++)      /* Loop over all the pages                          */
    {
        uint32 pageAddr = flashAddr + (page * DFLASH_PAGE_LENGTH); /* Get the address of the page     */

        /* Enter in page mode */
        IfxFlash_enterPageMode(pageAddr);

        /* Wait until page mode is entered */
        IfxFlash_waitUnbusy(FLASH_MODULE, DATA_FLASH_0);

        /* Load data to be written in the page */
        IfxFlash_loadPage2X32(pageAddr, data[page * 2 ], data[(page *2) +1] ); /* Load two words of 32 bits each            */

        /* Write the loaded page */
        IfxScuWdt_clearSafetyEndinit(endInitSafetyPassword);    /* Disable EndInit protection                       */
        IfxFlash_writePage(pageAddr);                           /* Write the page                                   */
        IfxScuWdt_setSafetyEndinit(endInitSafetyPassword);      /* Enable EndInit protection                        */

        /* Wait until the data is written in the Data Flash memory */
        IfxFlash_waitUnbusy(FLASH_MODULE, DATA_FLASH_0);
    }

    IfxPort_setPinState(LED2, IfxPort_State_low);
    return  0;
}



