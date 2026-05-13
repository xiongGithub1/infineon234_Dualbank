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

/* OEM: startup phase timeout protection (max 100ms from reset to jump decision) */
#define BOOT_STARTUP_TIMEOUT_MS     100u

//int main( int argc, char *argv[] )
void core0_main(void)
{
    //	uint32 *p = (uint32 *)0x70000000;
    //	*p = 0;

    /* === Phase 1: System Startup (OEM Standard) === */
    g_bootPhase = BOOT_PHASE_STARTUP;

    /*
     * Enable CPU Watchdog to prevent runaway after Trap.
     * Explicitly set reload=0x7FFF (~5.3s @ 100MHz/16384) to safely cover
     * DFlash/PFlash operations. Default UCB reload may be too short.
     * Safety WDT is disabled to simplify bootloader handling.
     * */
    {
        IfxScuWdt_Config wdtConfig;
        IfxScuWdt_initConfig(&wdtConfig);
        wdtConfig.password = IfxScuWdt_getCpuWatchdogPassword();
        wdtConfig.reload   = 0x7FFF;  /* ~5.3s, safe for all flash operations */
        IfxScuWdt_initCpuWatchdog(&MODULE_SCU.WDTCPU[0], &wdtConfig);
    }
    IfxScuWdt_disableSafetyWatchdog(IfxScuWdt_getSafetyWatchdogPassword());

    /* OEM: Record reset reason for diagnostic and fault analysis */
    resetReason = ResetStatus_Previous();
    isPowerOnReset = ((resetReason & 0x01u) != 0u) ? TRUE : FALSE;

    //    IfxCpu_disableInterrupts();
	/* app init*/

    IfxScuClock_init();

    delay_init();

    BrdLed_init();
    AppBL_init();
    /* Dual Bank: initialize flag system and attempt to jump to active bank */
    Boot_DualBank_Init();

    /* === OEM Phase: Check if APP requested bootloader mode via RAM flag === */
    {
        uint16 ramBootMode = *(uint16 *)RAM_BOOT_MODE_Addr;
        if (ramBootMode == RAM_BOOT_MODE_APP)
        {
            /* APP requested bootloader: clear flag and stay in bootloader for flashing */
            *(uint16 *)RAM_BOOT_MODE_Addr = RAM_BOOT_MODE_NORMAL;
            g_bootPhase = BOOT_PHASE_BL_ENTRY;
            /* Fall through to AppBL_init() without attempting jump */
        }
        else
        {
            /* Normal boot: attempt to jump to active bank */
            Boot_DualBank_SelectAndJump();
            /* If SelectAndJump() returns, both banks are invalid -> stay in bootloader */
        }
    }


    // measureEraseTime
//    MeasureEraseBankA_Time();
    IfxCpu_enableInterrupts();
    while (TRUE)
    {
        /* app main */
        AppBL_main();

        /* brd */
        BrdLed_main();

        /* Service CPU watchdog: if main loop stops (e.g. Trap deadlock), WDT resets system */
        IfxScuWdt_serviceCpuWatchdog(IfxScuWdt_getCpuWatchdogPassword());
    }
}
