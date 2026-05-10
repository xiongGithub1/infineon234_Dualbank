/**
 * \file Configuration.h
 * \brief Global configuration
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
 * \defgroup IfxLld_Demo_Ccu6Timer_SrcDoc_Config Application configuration
 * \ingroup IfxLld_Demo_Ccu6Timer_SrcDoc
 *
 *
 */

#ifndef CONFIGURATION_H
#define CONFIGURATION_H

/******************************************************************************/
/*----------------------------------Includes----------------------------------*/
/******************************************************************************/

#include "Ifx_Cfg.h"
#include "ConfigurationIsr.h"

/******************************************************************************/
/*-----------------------------------Macros-----------------------------------*/
/******************************************************************************/

/** \addtogroup IfxLld_Demo_Ccu6Timer_SrcDoc_Config
 * \{ */

/*______________________________________________________________________________
** Help Macros
**____________________________________________________________________________*/

/**
 * \name Macros for Regression Runs
 * \{ */

#ifndef REGRESSION_RUN_STOP_PASS
#define REGRESSION_RUN_STOP_PASS
#endif

#ifndef REGRESSION_RUN_STOP_FAIL
#define REGRESSION_RUN_STOP_FAIL
#endif

/** \} */


#define LED1                    &MODULE_P00,7
#define LED2                    &MODULE_P00,8

//#define CAN2_RECIEVE_PIN		&IfxMultican_RXD1B_P14_1_IN
//#define CAN2_TRANSMIT_PIN		&IfxMultican_TXD1_P14_0_OUT
//#define CAN2_EN_PIN				&MODULE_P14,4

/* CAN通讯 */
#define	CAN1_RXD_IO_CONFIG		IfxMultican_RXD0A_P02_1_IN
#define	CAN1_TXD_IO_CONFIG		IfxMultican_TXD0_P02_0_OUT

#define CAN1_EN					&MODULE_P02,3	// V1.41板子20240430
#define CAN1_NSTB				&MODULE_P02,2	// V1.41板子20240430
#define CAN1_NERR				&MODULE_P02,4	// V1.41板子20240430

#define CAN2_EN					&MODULE_P14,4
#define	CAN2_RXD_IO_CONFIG		IfxMultican_RXD1B_P14_1_IN
#define	CAN2_TXD_IO_CONFIG		IfxMultican_TXD1_P14_0_OUT

/*
 * CAN CFG
 */
#define     CAN_DATA_BUFF_Length            0x200       // CAN数据缓存大小 1024u

#define     CAN_MO_DATA_Length              8u          // can数据长度

#define     CAN1_MO_TXPDU_ID                0x3AA
#define     CAN_MO_TXPDU_Msk                0x7FFFFFFF

#define     CAN1_MO_RXPDU_ID                0x2BB
#define     CAN_MO_RXPDU_Msk                0x7FFFFFFF

/** \} */

#endif
