/*
 * @Author: qinXiong
 * @Date: 2026-05-10 23:01:27
 * @LastEditors: xiongGithub1&&qx20001119@163.com
 * @LastEditTime: 2026-05-11 03:10:50
 * @Description: 
 */
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
#include "CANRxTxInterface.h"
#include "Stm.h"
#include "App_bootloader.h"
#include    "Brd_led.h"
/******************************************************************************/
/*-----------------------------------Macros-----------------------------------*/
/******************************************************************************/
#define C_TIME_05MS   50000		// 系统时钟为200MHz，但是该应用是进行了2分频，因此周期为10ns，10*50000=500000ns=0.5ms

/******************************************************************************/
/*------------------------------Type Definitions------------------------------*/
/******************************************************************************/


/******************************************************************************/
/*------------------------------Global variables------------------------------*/
/******************************************************************************/
typedef struct
{
    uint32 time1;
    uint32 time2;
    uint32 detatime;
} sDeta_Time;

typedef struct
{
     sDeta_Time MainWhileTime;			// 主循环时间
     sDeta_Time TorqueCall;
     sDeta_Time STMTime;
     sDeta_Time FOCPWMTomTime;
     sDeta_Time CAN9252TransmitTime;
     sDeta_Time FOCPWMADCTime;
     sDeta_Time Tle5012SPITime;
     sDeta_Time VadcTime;
     sDeta_Time VadcScanTime;
     sDeta_Time SpeedTime;
     sDeta_Time CANhandle;
     sDeta_Time CANfalutHandle;
     sDeta_Time	CoastingTorqueCal;		// 滑行扭矩计算时间
     sDeta_Time DataCollectionTime;
} Deta_Time;

uint32 ResetStatus_Previous(void);

#endif
