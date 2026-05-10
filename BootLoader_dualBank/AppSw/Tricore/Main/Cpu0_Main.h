/**
 * \file Cpu0_Main.h
 * \brief System initialization and main program implementation.
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
 *
 * \defgroup IfxLld_Demo_Ccu6Timer_SrcDoc Source code documentation
 * \ingroup IfxLld_Demo_Ccu6Timer
 */

#ifndef CPU0_MAIN_H
#define CPU0_MAIN_H

/******************************************************************************/
/*----------------------------------Includes----------------------------------*/
/******************************************************************************/

#include "Configuration.h"
#include "Cpu/Std/Ifx_Types.h"
#include "IfxScuWdt.h"
#include "ConfigurationIsr.h"
#include "ScuClock.h"

/******************************************************************************/
/*-----------------------------------Macros-----------------------------------*/
/******************************************************************************/


/******************************************************************************/
/*------------------------------Type Definitions------------------------------*/
/******************************************************************************/


/******************************************************************************/
/*------------------------------Global variables------------------------------*/
/******************************************************************************/



uint32 ResetStatus_Previous(void);

#endif
