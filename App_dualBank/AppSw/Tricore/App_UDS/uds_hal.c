/*
 * @Author: qinXiong
 * @Date: 2026-04-29 09:02:27
 * @LastEditors: xiong&&2307975018@qq.com
 * @LastEditTime: 2026-05-08 17:13:20
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
    /* Trigger application reset via Infineon iLLD API.
     * On TC234 this resets the CPU core and most peripherals,
     * equivalent to a power-on reset from the application perspective.
     */
    /* Ensure all Flash operations are complete before reset
     * to avoid bus error on reboot due to busy Flash controller */
//    IfxFlash_waitUnbusy(FLASH_MODULE, IfxFlash_FlashType_P0);
//    IfxFlash_waitUnbusy(FLASH_MODULE, IfxFlash_FlashType_D0);

    IfxCpu_triggerSwReset();
    while(1)
    {
        /* Ensure no further code execution after reset trigger */
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
