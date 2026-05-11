/*
 * @Author: qinXiong
 * @Date: 2026-04-29 09:02:27
 * @LastEditors: Qxiong&&2307975018@qq.com
 * @LastEditTime: 2026-05-11 19:22:58
 * @Description: 
 */
/**********************************************************************************************************************
 * \file    hal_system.c
 * \brief
 * \version V1.0.0
 * \date    2021��11��28��
 * \author  Administrator
 *********************************************************************************************************************/
#include <uds_hal.h>
#include "IfxFlash.h"
#include "Flash.h"
void SoftwarewReset(void)
{
    /* Ensure all Flash operations are complete before reset */
//    IfxFlash_waitUnbusy(FLASH_MODULE, IfxFlash_FlashType_P0);
//    IfxFlash_waitUnbusy(FLASH_MODULE, IfxFlash_FlashType_D0);

    IfxCpu_triggerSwReset();
    while(1)
    {

    }
}

void HardReset(void)
{
    /* Hard Reset: trigger Application Reset via direct SCU register access.
     * On TC234, Application Reset resets CPU core, DMA, and most peripherals.
     * This is the strongest reset achievable by software and is equivalent
     * to ECU Reset (0x11 01) from the application perspective.
     */
    uint16 safetyWdtPassword;

    /* Ensure all Flash operations are complete before reset
     * to avoid bus error on reboot due to busy Flash controller */
    IfxFlash_waitUnbusy(FLASH_MODULE, IfxFlash_FlashType_P0);
    IfxFlash_waitUnbusy(FLASH_MODULE, IfxFlash_FlashType_D0);

    /* SWRSTCON is Safety-Endinit protected; must unlock before writing */
    safetyWdtPassword = IfxScuWdt_getSafetyWatchdogPassword();
    IfxScuWdt_clearSafetyEndinit(safetyWdtPassword);

    /* Direct register write: set SWRSTCON.SWRSTREQ to trigger Application Reset */
    // MODULE_SCU.SW![1778498572002](image/uds_hal/1778498572002.png)RSTCON.B.SWRSTREQ = 1;
    SCU_SWRSTCON.U = 0x01U;  /* SWRSTREQ = 1 */
    /* Restore Safety Endinit (won't actually reach here because reset is immediate) */
    IfxScuWdt_setSafetyEndinit(safetyWdtPassword);

    while(1)
    {
        /* Dead loop ensures no further code execution after reset trigger */
    }
}

void KeyOffOnReset(void)
{
	// TODO
    //Կ�׿��ظ�λ
}

void EnableRapidPowerShutdown(void)
{
	// TODO
    //���ÿ��ٵ�Դ�ض�
}

void DisableRapidPowerShutdown(void)
{
	// TODO
    //���ÿ��ٵ�Դ�ض�
}

