/**********************************************************************************************************************
 * \file    timer_hal.h
 * \brief
 * \version V1.0.0
 * \date    2021Äê11ÔÂ23ÈÕ
 * \author  Administrator
 *********************************************************************************************************************/
#ifndef _UDS_TIMER_H_
#define _UDS_TIMER_H_

#include "IfxStm.h"
#include "uds_common.h"


/* Timer 1ms period called */
void uds_timer_1msPeriod(void);
/*FUNCTION**********************************************************************
 *
 * Function Name : TIMER_HAL_Is1msTickTimeout
 * Description   : This function is check timeout. If timeout return TRUE, else return FALSE.
 *
 * Implements : Is1msTickTimeout_Activity
 *END**************************************************************************/
uint8 uds_timer_Is1msTickTimeout(void);
/*FUNCTION**********************************************************************
 *
 * Function Name : TIMER_HAL_Is10msTickTimeout
 * Description   : This function is check timeout or not. If timeout return TRUE, else return FALSE.
 *
 * Implements : Is10msTickTimeout_Activity
 *END**************************************************************************/
uint8 uds_timer_Is100msTickTimeout(void);
/* Get timer tick cnt for random seed. */
uint32 uds_timer_GetTimerTickCnt(void);
uint32 uds_timer_Get1msCnt(void);
/*FUNCTION**********************************************************************
 *
 * Function Name : TIMER_HAL_Deinit
 * Description   : This function initial this module.
 *
 * Implements : TIMER_HAL_Deinit_Activity
 *END**************************************************************************/
void uds_timer_deinit(void);

#endif /* _UDS_TIMER_H_ */
