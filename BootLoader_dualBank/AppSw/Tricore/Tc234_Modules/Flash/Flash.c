
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
 ** @Function    ��
 ** @Description ��      ����һ������      ��PSPR������
 ** @Parameters  ��
 ** @Returns     ��
 ** @Date        ��
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
 ** @Function    ��
 ** @Description ��
 ** @Parameters  ��
 ** @Returns     ��
 ** @Date        ��
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
 ** @Function    ��
 ** @Description ��       example
 ** @Parameters  ��
 ** @Returns     ��
 ** @Date        ��
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
 ** @Function    ��
 ** @Description ��
 ** @Parameters  ��          refresh bl
 ** @Returns     ��
 ** @Date        ��
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
 ** @Function    ��
 ** @Description ��
 *              D-Flash
 *              LOGIC sector=8kb=0x2000b
 *              1 page = 8 bytes
 ** @Parameters  ��
 **             addr
 **
 ** @Returns     ��
 ** @Date        ��
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
 ** @Function    ��
 ** @Description ��
 ** @Parameters  ��      length : unit :bytes
 ** @Returns     ��
 ** @Date        ��
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
 ** @Function    ����������������ͳ������̸��Ƶ�CPU0��program scratche - pad SRAM (PSPR)�У���������ָ�븳�����ǡ�
 ** @Description ��
 **          This function copies the erase and program routines to the Program Scratch-Pad SRAM (PSPR) of the CPU0 and assigns
 * function pointers to them.
 ** @Parameters  ��
 ** @Returns     ��
 ** @Date        ��
 ** ============================================================================
 */
// ��κ����Ĺ����ǰ���غ����ĳ������ݿ�����CPU0��program scratche-pad SRAM (PSPR)�У���ʱ�洢�����Ա���ڿ��Կ��ٵĵ������У�����20220725
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
 ** @Function    ����ʼ��Flash
 ** @Description ��
 ** @Parameters  ��
 ** @Returns     ��
 ** @Date        ��
 ** ============================================================================
 */
void Flash_init(void)
{
    Flash_copyFunctionsToPSPR();

    return;
}

/*
 ** ============================================================================
 ** @Function    ��
 ** @Description ��
 ** @Parameters  ��
 ** @Returns     ��
 ** @Date        ��
 ** ============================================================================
 */
int Flash_erasePFlash_port(uint32 flashAddr)
{
    boolean interruptState = IfxCpu_disableInterrupts(); /* Get the current state of the interrupts and disable them*/
    Flash_copyFunctionsToPSPR();
    g_commandFromPSPR.eraseFlash(flashAddr);
    IfxCpu_restoreInterrupts(interruptState);            /* Restore the interrupts state                            */

    /* Check PFlash erase error flags: EVER (erase verify), OPER (operation), SQER (sequence), PROER (protection) */
    if (FLASH0_FSR.B.EVER || FLASH0_FSR.B.OPER || FLASH0_FSR.B.SQER || FLASH0_FSR.B.PROER)
    {
        return -1;  /* Erase error detected */
    }
    return 0;
}

/*
 ** ============================================================================
 ** @Function    ����PFLASH��������д��
 ** @Description ��
 **                 ÿ��д1page=32bytes
 ** @Parameters  ��
 ** @Returns     ��
 ** @Date        ��
 ** ============================================================================
 */
int  Flash_writePFlash_port(uint32 flashAddr, uint32 *data, uint32 bytelength)
//int  Flash_writePFlash_port(uint32 flashAddr)
{
    IfxPort_setPinState(LED2, IfxPort_State_high);

    boolean interruptState = IfxCpu_disableInterrupts(); /* Get the current state of the interrupts and disable them*/
    Flash_copyFunctionsToPSPR();
    g_commandFromPSPR.writeFlash(flashAddr, data, bytelength);
    IfxCpu_restoreInterrupts(interruptState);            /* Restore the interrupts state                            */

    IfxPort_setPinState(LED2, IfxPort_State_low);

    /* Check PFlash program error flags: PVER (program verify), OPER, SQER, PROER */
    if (FLASH0_FSR.B.PVER || FLASH0_FSR.B.OPER || FLASH0_FSR.B.SQER || FLASH0_FSR.B.PROER)
    {
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
uint8 s_remainBuffer[32] = {0};  // �ݴ治��32�ֽڵ�����
uint32 s_remainAddr = 0;   // �ݴ����ݵ���ʼ��ַ
uint32 s_remainSize = 0;   // ��ǰ�ݴ�����ݳ���

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

int Flash_ForceWriteRemaining(void)//����32λ��ֱ��д�룬��������������
{
//	static uint32 s_remainBuffer32[8] = {0};
    if (s_remainSize > 0)
    {
        // ���ʣ�ಿ��Ϊ0xFF
        memset(s_remainBuffer + s_remainSize, 0x00, 32 - s_remainSize);//����32λ�ģ�û���ݵĶ���д0����Ȼ������һЩ����

        // д��������32�ֽڿ顢����֤
        if (Flash_writeAndVerifyPage(s_remainAddr, s_remainBuffer) == 0)
        {
            s_remainSize = 0;
            return 0;  /* Force write failed */
        }

        // ����״̬
        s_remainSize = 0;
    }
    return 1;
}

int Flash_writePFlash_portex(uint32 flashAddr, uint8 *data, uint32 byteLength)//��32λ����д��
{
    uint32 offset = 0;
    uint32 currentAddr = flashAddr;//0xa0020040+126=0xa00200be

    // 1. ����Ƿ���Ҫ���ݴ�����ƴ��
    if (s_remainSize > 0)
    {
    	//����ַ�Ƿ��뻺�������
		if (currentAddr == (s_remainAddr + s_remainSize))//������128���ȵ�����->0xa00200be=0xa00200a0+30
    	{
            uint32 needed = 32 - s_remainSize;  // ��Ҫ������ֽ���
            uint32 copySize = min(needed, byteLength);

            // ��ϳ�������32�ֽڿ�
            memcpy(s_remainBuffer + s_remainSize, data, copySize);//(0xa00200be,ǰ�������ݣ�2)

            // ����д������ҳ��ʹ�û����ַ����֤
			if (Flash_writeAndVerifyPage(s_remainAddr, s_remainBuffer) == 0)
			{
				return 0;  /* Write-verify failed */
			}

			offset += copySize;//offset->0+2=2
			currentAddr += copySize;//0xa00200be+2=0xa00200c0
			s_remainSize = 0;
        }
		else//������������
        {
			if((s_remainAddr + 32) < currentAddr)
			{
				// ��ַ��������ǿ��д��ɻ���
				if (Flash_ForceWriteRemaining() == 0)
					{
						return 0;  /* Force write-verify failed */
					}
			}
			else //if((s_remainAddr + 32) > currentAddr)
			{
				//����֮ǰ���صĵ�ַΪ��s_remainAddr��0x800451E0 + 0x20�����ȣ� = 0x80045200 > 0x800451F4
				//֮ǰ���µ������ֽ���ֻ�У�s_remainSize��10�������һ���������ĵ�ַΪ��0x800451E0 + 0xA = 0x800451EA
				//���µ�ַΪ0x800451F4����ô0x80045200 - 0x800451F4 =  0xC����ΪneedBetysLength
				//(s_remainBuffer + 32 - needBetysLength) = 0x800451E0 + 0x20 -0xC = 0x800451F4���¿��������ݴ�0x800451F4��ʼ
				//�����¿�������ֻ��4������byteLength����ô�����ֽڻ�Ӧ��8������ռ�õ��ֽ�Ӧ��32-8=24,
				//��ô32 - (needBetysLength - byteLength) = 24���������ƶϣ�Ӧ����ȷ�ģ�����20250717
				uint32 needBetysLength = s_remainAddr + 32 - currentAddr;
				// 0x80045200 + 0x20 = 0x80045220 - 0x80045208 = 24
				uint32 copySize2 = min(needBetysLength, byteLength);	// ���� byteLength = 4����needBetysLength = 12
				if((32 - needBetysLength) > s_remainSize)
				{
					memcpy((s_remainBuffer + 32 - needBetysLength), data, copySize2);
					//(s_remainBuffer + 32 - needBetysLength)
					if(needBetysLength <= byteLength)
					{
						if (Flash_writeAndVerifyPage(s_remainAddr, s_remainBuffer) == 0)
							{
								return 0;  /* Write-verify failed */
							}//(0xa00200a0,32�����ݣ�32)
						offset += copySize2;//offset->0+2=2
						currentAddr += copySize2;//0xa00200be+2=0xa00200c0
						s_remainSize = 0;
					}
					else // ��Ϊ����һҳ���ȴ���һ��ѭ��
					{
						s_remainSize = 32 - (needBetysLength - byteLength);	//
					}
				}
				else
				{
					return 0;
				}
			}
    	}
    }

    // 2. д��������32�ֽڿ�
    //��168�����Ƚ��о�����
    //��һ��0x36���͵����ݳ�����126��
    //��a0020040->a0020060(32)->offset=(0->32)
    //��a0020060->a0020080(32)->offset=(32->64)
	//��a0020080->a00200a0(32)->offset=(64->96)
    //��a00200a0->a00200c0(32)->offset=96+32<126����ִ�У�offset=96->�������ƴ��32д����
    //�ڶ���0x36���͵����ݳ�����42��
    //��a00200c0->a00200e0(32)->offset=(2->34)
    //��a00200e0->a00200f0(8)->offset=34+32=66<42����offset=34
    while (offset + 32 <= byteLength) //126
    {
        if (Flash_writeAndVerifyPage(currentAddr, data + offset) == 0)
        {
            s_remainSize = 0;  /* Clear buffer state on error */
            return 0;          /* Write-verify failed */
        }
        offset += 32;
        currentAddr += 32; // �ؼ���ÿ��д������32
    }

    // 3. ����ʣ�಻��32�ֽڵ�����
    if ((offset < byteLength)&&(s_remainSize == 0)) //��96<126  ��66<42
    {
		// ��������ַ��32�ֽڱ߽磩
		s_remainAddr = currentAddr;//��0xa00200a0  ��a00200e0
		// �洢ʣ������
		s_remainSize = byteLength - offset;//��126-96=30  ��42-2-32=8
		for(uint8 i = 0;i<32;i++)
		{
			s_remainBuffer[i]=0;
		}
		memcpy(s_remainBuffer, data + offset, s_remainSize);//��(s_remainBuffer,data+96,30)  ��(s_remainBuffer,data+34,8)
    }

    /* Check PFlash program error flags after all write operations */
    if (FLASH0_FSR.B.PVER || FLASH0_FSR.B.OPER || FLASH0_FSR.B.SQER || FLASH0_FSR.B.PROER)
    {
        s_remainSize = 0;  /* Clear buffer state on error */
        return 0;          /* Write error detected */
    }
    return 1;
}

/*
 ** ============================================================================
 ** @Function    ��
 ** @Description ��
 ** @Parameters  ��
 ** @Returns     ��
 ** @Date        ��
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
 ** @Function    ��
 ** @Description ��
 **                 ÿ��д1page=8bytes
 ** @Parameters  ��
 ** @Returns     ��
 ** @Date        ��
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



