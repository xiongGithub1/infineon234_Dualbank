
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
 ** @Function    ：
 ** @Description ：      擦除一个扇区      在PSPR中运行
 ** @Parameters  ：
 ** @Returns     ：
 ** @Date        ：
 ** ============================================================================
 */
#if defined(__TASKING__)
#pragma section code "psram_cpu0"
#endif
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
 ** @Function    ：
 ** @Description ：
 ** @Parameters  ：
 ** @Returns     ：
 ** @Date        ：
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

#if defined(__TASKING__)
#pragma section code restore
#endif

/*
 ** ============================================================================
 ** @Function    ：
 ** @Description ：       example
 ** @Parameters  ：
 ** @Returns     ：
 ** @Date        ：
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
 ** @Function    ：
 ** @Description ：
 ** @Parameters  ：          refresh bl
 ** @Returns     ：
 ** @Date        ：
 ** ============================================================================
 */

#if defined(__TASKING__)
#pragma section code "psram_cpu0"
#endif
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

#if defined(__TASKING__)
#pragma section code restore
#endif

/*
 ** ============================================================================
 ** @Function    ：
 ** @Description ：
 *              D-Flash
 *              LOGIC sector=8kb=0x2000b
 *              1 page = 8 bytes
 ** @Parameters  ：
 **             addr
 **
 ** @Returns     ：
 ** @Date        ：
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
 ** @Function    锟斤拷
 ** @Description 锟斤拷
 ** @Parameters  锟斤拷      length : unit :bytes
 ** @Returns     锟斤拷
 ** @Date        锟斤拷
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
 ** @Function    锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷统锟斤拷锟斤拷锟斤拷谈锟斤拷频锟紺PU0锟斤拷program scratche - pad SRAM (PSPR)锟叫ｏ拷锟斤拷锟斤拷锟斤拷锟斤拷指锟诫赋锟斤拷锟斤拷锟角★拷
 ** @Description 锟斤拷
 **          This function copies the erase and program routines to the Program Scratch-Pad SRAM (PSPR) of the CPU0 and assigns
 * function pointers to them.
 ** @Parameters  锟斤拷
 ** @Returns     锟斤拷
 ** @Date        锟斤拷
 ** ============================================================================
 */
// 这段函数的功能是把相关函数的程序数据拷贝到CPU0的program scratche-pad SRAM (PSPR)中，临时存储程序，以便后期可以快速的调用运行，曾军20220725
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

    /* Flash_erasePFLASH and Flash_writePFlashPage are now linked into .text.psram_cpu0,
       no need for runtime memcpy. Directly use their addresses. */
    g_commandFromPSPR.eraseFlash = (void *)Flash_erasePFLASH;
    g_commandFromPSPR.writeFlash = (void *)Flash_writePFlashPage;

    return;
}


/*
 ** ============================================================================
 ** @Function    ：初始化Flash
 ** @Description ：
 ** @Parameters  ：
 ** @Returns     ：
 ** @Date        ：
 ** ============================================================================
 */
void Flash_init(void)
{
    Flash_copyFunctionsToPSPR();

    return;
}

/*
 ** ============================================================================
 ** @Function    ：
 ** @Description ：
 ** @Parameters  ：
 ** @Returns     ：
 ** @Date        ：
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
    if (FLASH0_FSR.B.EVER || FLASH0_FSR.B.OPER || FLASH0_FSR.B.SQER || FLASH0_FSR.B.PROER)
    {
        /* Clear sticky error flags (W1C) so they do not poison subsequent operations */
        FLASH0_FSR.U = (1u << 26) | (1u << 13) | (1u << 12) | (1u << 11);
        return -1;  /* Erase error detected */
    }
    return 0;
}

/*
 ** ============================================================================
 ** @Function    ：往PFLASH进行数据写入
 ** @Description ：
 **                 每次写1page=32bytes
 ** @Parameters  ：
 ** @Returns     ：
 ** @Date        ：
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
    if (FLASH0_FSR.B.PVER || FLASH0_FSR.B.OPER || FLASH0_FSR.B.SQER || FLASH0_FSR.B.PROER)
    {
        /* Clear sticky error flags (W1C) so they do not poison subsequent operations */
        FLASH0_FSR.U = (1u << 25) | (1u << 13) | (1u << 12) | (1u << 11);
        return -1;  /* Write error detected */
    }
    return 0;
}

uint32 min(uint32 var1,uint32 var2)
{
	if(var1<var2)
		return var1;
	else
		return var2;
}
uint8 s_remainBuffer[32] = {0};  // 暂存不足32字节的数据
uint32 s_remainAddr = 0;   // 暂存数据的起始地址
uint32 s_remainSize = 0;   // 当前暂存的数据长度


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

int Flash_ForceWriteRemaining(void)//不足32位的直接写入，保存的数组的数据
{
//	static uint32 s_remainBuffer32[8] = {0};
    if (s_remainSize > 0)
    {
        // 填充剩余部分为0xFF
        memset(s_remainBuffer + s_remainSize, 0x00, 32 - s_remainSize);//不满32位的，没数据的都填写0，不然会乱入一些数据

        // 写入完整的32字节块
        if (Flash_writeAndVerifyPage(s_remainAddr, s_remainBuffer) == 0)
        {
            s_remainSize = 0;
            return 0;  /* Force write failed */
        }

        // 重置状态
        s_remainSize = 0;
    }
    return 1;
}

int Flash_writePFlash_portex(uint32 flashAddr, uint8 *data, uint32 byteLength)//按32位进行写入
{
    boolean interruptState = IfxCpu_disableInterrupts(); /* Disable interrupts during flash operation */
    uint16 wdtPassword = IfxScuWdt_getCpuWatchdogPassword();
    int result = 1;
    uint32 offset = 0;
    uint32 currentAddr = flashAddr;//0xa0020040+126=0xa00200be

    /* Ensure PSPR routines are valid before use */
    Flash_copyFunctionsToPSPR();
    // 1. 检查是否需要与暂存数据拼接
    if (s_remainSize > 0)
    {
    	//检查地址是否与缓存块连续
		if (currentAddr == (s_remainAddr + s_remainSize))//连续超128长度的数据->0xa00200be=0xa00200a0+30
    	{
            uint32 needed = 32 - s_remainSize;  // 需要补充的字节数
            uint32 copySize = min(needed, byteLength);

            // 组合成完整的32字节块
            memcpy(s_remainBuffer + s_remainSize, data, copySize);//(0xa00200be,前两个数据，2)

            // 立即写入完整页（使用缓存地址）
			if (Flash_writeAndVerifyPage(s_remainAddr, s_remainBuffer) == 0)
			{
				result = 0;  /* Write-verify failed */
				goto flash_write_exit;
			}

			offset += copySize;//offset->0+2=2
			currentAddr += copySize;//0xa00200be+2=0xa00200c0
			s_remainSize = 0;
        }
		else//非连续的数据
        {
			if((s_remainAddr + 32) < currentAddr)
			{
				// 地址不连续，强制写入旧缓存
				if (Flash_ForceWriteRemaining() == 0)
					{
						result = 0;  /* Force write-verify failed */
						goto flash_write_exit;
					}
			}
			else //if((s_remainAddr + 32) > currentAddr)
			{
				//假设之前下载的地址为：s_remainAddr：0x800451E0 + 0x20（长度） = 0x80045200 > 0x800451F4
				//之前余下的数据字节数只有：s_remainSize：10，因此上一次下载完后的地址为：0x800451E0 + 0xA = 0x800451EA
				//而新地址为0x800451F4，那么0x80045200 - 0x800451F4 =  0xC，即为needBetysLength
				//(s_remainBuffer + 32 - needBetysLength) = 0x800451E0 + 0x20 -0xC = 0x800451F4，新拷贝的数据从0x800451F4开始
				//假设新拷贝数据只有4个，即byteLength，那么空余字节还应余8个，已占用的字节应是32-8=24,
				//那么32 - (needBetysLength - byteLength) = 24符合上面推断，应是正确的，曾军20250717
				uint32 needBetysLength = s_remainAddr + 32 - currentAddr;
				// 0x80045200 + 0x20 = 0x80045220 - 0x80045208 = 24
				uint32 copySize2 = min(needBetysLength, byteLength);	// 假设 byteLength = 4，而needBetysLength = 12
				if((32 - needBetysLength) > s_remainSize)
				{
					memcpy((s_remainBuffer + 32 - needBetysLength), data, copySize2);
					//(s_remainBuffer + 32 - needBetysLength)
					if(needBetysLength <= byteLength)
					{
						if (Flash_writeAndVerifyPage(s_remainAddr, s_remainBuffer) == 0)
							{
								result = 0;  /* Write-verify failed */
								goto flash_write_exit;
							}
						offset += copySize2;//offset->0+2=2
						currentAddr += copySize2;//0xa00200be+2=0xa00200c0
						s_remainSize = 0;
					}
					else // 因为不够一页，等待下一个循环
					{
						s_remainSize = 32 - (needBetysLength - byteLength);	//
					}
				}
				else
				{
					result = 0;
					goto flash_write_exit;
				}
			}
    	}
    }

    // 2. 写入完整的32字节块
    //拿168个长度进行举例，
    //第一次0x36发送的数据长度是126个
    //①a0020040->a0020060(32)->offset=(0->32)
    //②a0020060->a0020080(32)->offset=(32->64)
	//③a0020080->a00200a0(32)->offset=(64->96)
    //④a00200a0->a00200c0(32)->offset=96+32<126×不执行，offset=96->在上面的拼接32写入了
    //第二次0x36发送的数据长度是42个
    //①a00200c0->a00200e0(32)->offset=(2->34)
    //②a00200e0->a00200f0(8)->offset=34+32=66<42×，offset=34
    while (offset + 32 <= byteLength) //126
    {
        if (Flash_writeAndVerifyPage(currentAddr, data + offset) == 0)
        {
            s_remainSize = 0;  /* Clear buffer state on error */
            result = 0;          /* Write-verify failed */
            goto flash_write_exit;
        }
        offset += 32;
        currentAddr += 32; // 关键：每次写入后递增32
    }

    // 3. 处理剩余不足32字节的数据
    if ((offset < byteLength)&&(s_remainSize == 0)) //①96<126  ②66<42
    {
		// 计算对齐地址（32字节边界）
		s_remainAddr = currentAddr;//①0xa00200a0  ②a00200e0
		// 存储剩余数据
		s_remainSize = byteLength - offset;//①126-96=30  ②42-2-32=8
		for(uint8 i = 0;i<32;i++)
		{
			s_remainBuffer[i]=0;
		}
		memcpy(s_remainBuffer, data + offset, s_remainSize);//①(s_remainBuffer,data+96,30)  ②(s_remainBuffer,data+34,8)
    }

    /* Check PFlash program error flags after all write operations */
    if (FLASH0_FSR.B.PVER || FLASH0_FSR.B.OPER || FLASH0_FSR.B.SQER || FLASH0_FSR.B.PROER)
    {
        s_remainSize = 0;  /* Clear buffer state on error */
        result = 0;          /* Write error detected */
        /* Clear sticky error flags (W1C) so they do not poison subsequent operations */
        FLASH0_FSR.U = (1u << 25) | (1u << 13) | (1u << 12) | (1u << 11);
    }

flash_write_exit:
    IfxScuWdt_serviceCpuWatchdog(wdtPassword);
    IfxCpu_restoreInterrupts(interruptState);
    return result;
}

/*
 ** ============================================================================
 ** @Function    ：
 ** @Description ：
 ** @Parameters  ：
 ** @Returns     ：
 ** @Date        ：
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
 ** @Function    ：
 ** @Description ：
 **                 每次写1page=8bytes
 ** @Parameters  ：
 ** @Returns     ：
 ** @Date        ：
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



