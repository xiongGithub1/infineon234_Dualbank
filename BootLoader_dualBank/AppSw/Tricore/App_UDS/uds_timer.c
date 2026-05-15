/**********************************************************************************************************************
 * \file    timer_hal.c
 * \brief
 * \version V1.0.0
 * \date    2021年11月23日
 * \author  Administrator
 *********************************************************************************************************************/
#include <uds_timer.h>
#include "Bsp.h"
#include "ConfigurationIsr.h"
#include "Stm.h"

//#define TIMER_STM_ISR_PRIORITY        12

//IfxStm_CompareConfig g_STMConf;                                 /* STM configuration structure                      */
//Ifx_TickTime g_ticksFor1ms;                                   /* Variable to store the number of ticks to wait    */

//Ifx_STM *stmSfr = &MODULE_STM0;	//定时器结构体句柄
//uint32 stmTimeInverval = 1;// 定时时间,单位ms
//IfxStm_CompareConfig stmConfig;// 定时器配置结构体

static int gs_1msCnt = 0;
static int gs_100msCnt = 0;

IFX_INTERRUPT(isrSTM, 0, ISR_PRIORITY_STM_INT0);

void isrSTM(void)
{
    /* Update the compare register value that will trigger the next interrupt and toggle the LED */
	uds_timer_1msPeriod();

    IfxStm_clearCompareFlag(g_Stm.stmSfr, g_Stm.stmConfig.comparator);
	IfxStm_increaseCompare(g_Stm.stmSfr, g_Stm.stmConfig.comparator, g_Stm.stmConfig.ticks);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : TIMER_HAL_Init
 * Description   : This function initial this module.
 *
 * Implements : TIMER_HAL_Init_Activity
 *END**************************************************************************/
//void uds_timer_init(void)
//{
//	IfxStm_initCompareConfig(&stmConfig);
//	sint32 ticks = IfxStm_getTicksFromMilliseconds(stmSfr,stmTimeInverval);
//	stmConfig.triggerPriority = TIMER_STM_ISR_PRIORITY;
//	stmConfig.typeOfService   = IfxSrc_Tos_cpu0;
//	stmConfig.ticks           = ticks;
//
//	IfxStm_initCompare(stmSfr, &stmConfig);
//}

/* Timer 1ms period called */
void uds_timer_1msPeriod(void)
{
    uint16 cntTmp = 0u;
    /* Just for check time overflow or not? */
    cntTmp = gs_1msCnt + 1u;

    if (0u != cntTmp)
    {
        gs_1msCnt++;
        if(gs_1msCnt >= 0xFFFFFFFE)
		{
			gs_1msCnt = 1;
		}
    }

    cntTmp = gs_100msCnt + 1u;

    if (0u != cntTmp)
    {
        gs_100msCnt++;
        if(gs_100msCnt >= 0xFFFFFFFE)
		{
			gs_1msCnt = 100;
		}
    }
}

/*FUNCTION**********************************************************************
 *
 * Function Name : TIMER_HAL_Is1msTickTimeout
 * Description   : This function is check timeout. If timeout return TRUE, else return FALSE.
 *
 * Implements : Is1msTickTimeout_Activity
 *END**************************************************************************/
uint8 uds_timer_Is1msTickTimeout(void)
{
    uint8 result = FALSE;

    if (gs_1msCnt)
    {
        result = TRUE;
        gs_1msCnt--;
    }

    return result;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : TIMER_HAL_Is10msTickTimeout
 * Description   : This function is check timeout or not. If timeout return TRUE, else return FALSE.
 *
 * Implements : Is10msTickTimeout_Activity
 *END**************************************************************************/
uint8 uds_timer_Is100msTickTimeout(void)
{
    uint8 result = FALSE;

    if (gs_100msCnt >= 100u)
    {
        result = TRUE;
        gs_100msCnt -= 100u;
    }

    return result;
}
/* Get timer tick cnt for random seed. 获取定时器值作为随机值种子*/
uint32 uds_timer_GetTimerTickCnt(void)
{
    /* This two variables not init before used, because it used for generate random */
    uint32 hardwareTimerTickCnt;
#if 1
    /* For S32K1xx get timer counter(LPTIMER), get timer count will trigger the period incorrect. */
    hardwareTimerTickCnt = (uint32)nowWithoutCriticalSection();
#endif
//#pragma GCC diagnostic ignored "-Wuninitialized"
    //timerTickCnt = ((hardwareTimerTickCnt & 0xFFFFu)) | (timerTickCnt << 16u);
    return hardwareTimerTickCnt;

    //IfxStm_getTicksFromMilliseconds();

    //可以获取系统tick
    //return getSystemTick();
}
/*FUNCTION**********************************************************************

/* Get 1ms counter for DTC test period management */
uint32 uds_timer_Get1msCnt(void)
{
    return (uint32)gs_1msCnt;
}
/*FUNCTION**********************************************************************
 *
 * Function Name : TIMER_HAL_Deinit
 * Description   : This function initial this module.
 *
 * Implements : TIMER_HAL_Deinit_Activity
 *END**************************************************************************/
void uds_timer_deinit(void)
{
}

