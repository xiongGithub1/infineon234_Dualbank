/**********************************************************************************************************************
 * \file    hal_system.h
 * \brief
 * \version V1.0.0
 * \date    2021Äê11ÔÂ28ÈÕ
 * \author  Administrator
 *********************************************************************************************************************/
#ifndef UDS_HAL_H_
#define UDS_HAL_H_

#include "IfxCpu.h"


void HardReset(void);

void SoftwarewReset(void);

void KeyOffOnReset(void);

void EnableRapidPowerShutdown(void);

void DisableRapidPowerShutdown(void);

#endif /* UDS_HAL_H_ */
