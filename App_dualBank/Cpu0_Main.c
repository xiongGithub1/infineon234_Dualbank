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

#include "Cpu0_Main.h"
#include "MultiCAN.h"
#include "Boot_DualBank.h"



Deta_Time	DetaTime;
uint32		g_LoopFlag;		// 0.5ms循环标志
uint32		g_BaseTime;
uint32 		g_count2ms;		// 2ms计数器
uint32 		g_count1ms;		// 1ms计数器
tRxTxCanMsg txcanmsg;
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

uint8 num=10;
void MainProgram(void) 
{
	static uint32 m_DetaTime = 0;
	static uint8 i=0;
	//  测量多长时间执行一次while主循环标志20210129
    DetaTime.MainWhileTime.time1 = Stm_GetSystemClock();
    DetaTime.MainWhileTime.detatime = DetaTime.MainWhileTime.time1 - DetaTime.MainWhileTime.time2;
    DetaTime.MainWhileTime.time2 = DetaTime.MainWhileTime.time1;

    m_DetaTime = DetaTime.MainWhileTime.time1 - g_BaseTime;

    if(m_DetaTime >= C_TIME_05MS)
    {
    	g_BaseTime += C_TIME_05MS;
    	g_LoopFlag ++;
        AppUds_main();

        /* brd */

    	if((g_LoopFlag % 1000) == 0)	// 500ms
    	{
    		txcanmsg.usRxTxDataId = 0x123;
    		txcanmsg.aucDataBuf[0] = 1;
    		txcanmsg.aucDataBuf[1] = 1;
    		txcanmsg.aucDataBuf[2] = 1;
    		txcanmsg.aucDataBuf[3] = 1;
    		txcanmsg.aucDataBuf[4] = 1;
    		drv_can1_send(&txcanmsg);
    	}


	}
}
uint32 resetReason;
boolean isPowerOnReset;

/* APP phase identifiers for OEM traceability */
typedef enum
{
    APP_PHASE_INIT    = 0xA0u,  /* APP early initialization */
    APP_PHASE_PERIPH  = 0xA1u,  /* APP peripheral initialization */
    APP_PHASE_RUN     = 0xA2u,  /* APP main loop running */
    APP_PHASE_ERROR   = 0xAFu   /* APP unrecoverable error */
} AppPhase_t;

volatile AppPhase_t g_appPhase = APP_PHASE_INIT;

//int main( int argc, char *argv[] )
void core0_main(void)
{
    //	uint32 *p = (uint32 *)0x70000000;
    //	*p = 0;

    /* === APP Phase: Early Initialization === */
    g_appPhase = APP_PHASE_INIT;

    /*
     * !!WATCHDOG0 AND SAFETY WATCHDOG ARE DISABLED HERE!!
     * Enable the watchdog in the demo if it is required and also service the watchdog periodically
     * */
    IfxScuWdt_disableCpuWatchdog(IfxScuWdt_getCpuWatchdogPassword());
    IfxScuWdt_disableSafetyWatchdog(IfxScuWdt_getSafetyWatchdogPassword());

    /* OEM: Record reset reason for diagnostic and fault analysis */
    resetReason = ResetStatus_Previous();
    isPowerOnReset = (resetReason & 0x01) != 0;

    IfxCpu_disableInterrupts();

    IfxScuClock_init();

    /* OEM: Clear boot attempts as early as possible after stable clock.
     * If APP crashes before this, Bootloader will increment bootAttempts
     * and eventually roll back after MAX_BOOT_ATTEMPTS failures.
     * Must be called BEFORE any complex/risky peripheral initialization. */
    Boot_DualBank_ClearBootAttempts();

    delay_init();

    IfxStm_init();    /* Initialize STM timer before using Stm_GetSystemClock */

    /* === APP Phase: Peripheral Initialization === */
    g_appPhase = APP_PHASE_PERIPH;

    Multican_init();  /* Initialize CAN module before accessing CAN registers */

    BrdLed_init();
	UdsInit(UDS_FUN_ADDR_ID,UDS_PHY_ADDR_ID,UDS_RESP_ADDR_ID);
    //

    g_BaseTime = Stm_GetSystemClock();
    IfxCpu_enableInterrupts();

    /* === APP Phase: Main Loop Running === */
    g_appPhase = APP_PHASE_RUN;

    while (TRUE)
    {
        MainProgram();
        BrdLed_main();
    }
}
