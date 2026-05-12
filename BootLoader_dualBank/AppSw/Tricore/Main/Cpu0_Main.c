/**
 * \file Cpu0_Main.c
 * \brief System initialisation and main program implementation.
 *
 * \version iLLD_Demos_1_0_1_7_0
 * \copyright Copyright (c) 2014 Infineon Technologies AG. All rights reserved.
 *
 *
 *                                 IMPORTANT NOTICE
 *
 *
 * Infineon Technologies AG (Infineon) is supplying this file for use
 * exclusively with Infineon's microcontroller products. This file can be freely
 * distributed within development tools that are supporting such microcontroller
 * products.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
 * OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
 * INFINEON SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL,
 * OR CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 */

 /******************************************************************************/
 /*----------------------------------Includes----------------------------------*/
 /******************************************************************************/
#include "Brd_led.h"
#include "ScuClock.h"
#include <Tc234_Modules/Tmr/Tmr.h>
#include "Cpu0_Main.h"
#include "SysSe/Bsp/Bsp.h"
#include "_PinMap/IfxPort_PinMap.h"
#include  "_Reg/IfxScu_reg.h"
#include "App_bootloader.h"
#include "IfxMultican.h"
#include    "Can.h"
#include "custom_delay.h"
#include <Tc234_Modules/Flash/Flash.h>
#include "Boot_DualBank.h"





/******************************************************************************/
/*-------------------------Function Implementations---------------------------*/
/******************************************************************************/

/** \brief Main entry point after CPU boot-up.
 *
 *  It initialise the system and enter the endless loop that handles the demo
 */

uint32 ResetStatus_Previous(void)
{
    uint32 rststat;

    volatile Ifx_SCU_RSTSTAT* scu_rststat = &SCU_RSTSTAT;
    rststat = scu_rststat->U;

    IfxScuWdt_clearSafetyEndinit(IfxScuWdt_getSafetyWatchdogPassword());
    volatile Ifx_SCU_RSTCON2* scu_rstcon2 = &SCU_RSTCON2;
    scu_rstcon2->B.CLRC = 1;
    IfxScuWdt_setSafetyEndinit(IfxScuWdt_getSafetyWatchdogPassword());

    return rststat;
}

uint32 resetReason;
boolean isPowerOnReset;
//int main( int argc, char *argv[] )
void core0_main(void)
{
    //	uint32 *p = (uint32 *)0x70000000;
    //	*p = 0;
        /*
         * !!WATCHDOG0 AND SAFETY WATCHDOG ARE DISABLED HERE!!
         * Enable the watchdog in the demo if it is required and also service the watchdog periodically
         * */
    IfxScuWdt_disableCpuWatchdog(IfxScuWdt_getCpuWatchdogPassword());
    IfxScuWdt_disableSafetyWatchdog(IfxScuWdt_getSafetyWatchdogPassword());

    //    IfxCpu_disableInterrupts();
	/* app init*/

    IfxScuClock_init();

    delay_init();

    BrdLed_init();

    /* Dual Bank: initialize flag system and attempt to jump to active bank */
    Boot_DualBank_Init();

    /* ===== 检查 RAM 启动标志（只在非上电复位时） ===== */
    /* 读取复位原因：bit0=1 表示上电复位 */
    //
   /* Check if APP requested bootloader mode via RAM flag */
//   {
//       uint16 ramBootMode = *(uint16 *)RAM_BOOT_MODE_Addr;
//       if (ramBootMode == RAM_BOOT_MODE_APP)
//       {
//           /* APP requested bootloader: clear flag and stay in bootloader for flashing */
//           *(uint16 *)RAM_BOOT_MODE_Addr = RAM_BOOT_MODE_NORMAL;
//       }
//       else
//       {
           /* Normal boot: attempt to jump to active bank */
            Boot_DualBank_SelectAndJump();
           /* If SelectAndJump() returns, both banks are invalid -> stay in bootloader */
//       }
//   }

    AppBL_init();
    // measureEraseTime
//    MeasureEraseBankA_Time();
    IfxCpu_enableInterrupts();
    while (TRUE)
    {
        /* app main */
        AppBL_main();

        /* brd */
        BrdLed_main();


    }
}

