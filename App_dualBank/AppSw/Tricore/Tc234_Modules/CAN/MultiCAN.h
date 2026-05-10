/******************************************************************************

                  版权所有 (C), 2020-2030, 重庆和天电子科技有限公司

 ******************************************************************************
  文 件 名  : MultiCAN.h
  版 本 号  : 初稿
  作    者    :
  生成日期 : 2021年10月15日
  最近修改 :
  功能描述 :
  函数列表 :
  修改历史 :
  1.日    期  :
  2.作    者  :
    修改内容   :

******************************************************************************/

#ifndef MULTICAN_H
#define MULTICAN_H 1

/*****************************************************************************/
/*----------------------------------包含头文件--------------------------------*/
/*****************************************************************************/
#include "Cpu0_Main.h"
#include <Multican/Can/IfxMultican_Can.h>
#include <Multican/Std/IfxMultican.h>
#include "Configuration.h"
#include "CANRxTxInterface.h"
#include "ConfigurationIsr.h"		// 配置中断优先级及中断服务提供者
#include "IfxMultican.h"
#include "IfxMultican_can.h"

/******************************************************************************/
/*-----------------------------------Macros-----------------------------------*/
/******************************************************************************/
// 波特率配置
#define		CAN1_TLE9252V_BAUDRATE		500000
#define		CAN2_TLE9251V_BAUDRATE		500000



/******************************************************************************/
/*--------------------------------Enumerations--------------------------------*/
/******************************************************************************/

/******************************************************************************/
/*-----------------------------Data Structures--------------------------------*/
/******************************************************************************/
typedef struct
{
    struct
    {
        IfxMultican_Can        can;          /**< \brief CAN driver handle */
        IfxMultican_Can_Node   canNode0;   /**< \brief CAN Node0 */
        IfxMultican_Can_Node   canNode1;   /**< \brief CAN Node1*/
        IfxMultican_Can_MsgObj canNode0MsgTx; /**< \brief CAN Destination Message object */
        IfxMultican_Can_MsgObj canNode0MsgRx; /**< \brief CAN Source Message object */
        IfxMultican_Can_MsgObj canNode0MsgTx2[128]; /**< \brief CAN Destination Message object */
        IfxMultican_Can_MsgObj canNode0MsgRx2[128]; /**< \brief CAN Source Message object */
        IfxMultican_Can_MsgObj canNode1MsgObjRx; /**< \brief CAN Destination Message object */
        IfxMultican_Can_MsgObj canNode1MsgObjTx; /**< \brief CAN Source Message object */
        IfxMultican_Can_MsgObj canNode1MsgObjRx1[128]; /**< \brief CAN Destination Message object */
        IfxMultican_Can_MsgObj canNode1MsgObjTx1[128]; /**< \brief CAN Source Message object */
    }drivers;
} App_MulticanBasic;



IFX_INLINE void Tle9251PortModeSet(void)
{
	IfxPort_setPinModeOutput(CAN2_EN,IfxPort_OutputMode_pushPull,IfxPort_OutputIdx_general);
}

IFX_INLINE void Tle9251EnterNormalMode()
{
	IfxPort_setPinLow(CAN2_EN);
}

IFX_INLINE void Tle9251EnterStandbyMode()
{
	IfxPort_setPinHigh(CAN2_EN);
}

IFX_INLINE void Tle9252PortModeSet(void)
{
	//EN
	IfxPort_setPinModeOutput(CAN1_EN,IfxPort_OutputMode_pushPull,IfxPort_OutputIdx_general);
	//NSTB
	IfxPort_setPinModeOutput(CAN1_NSTB,IfxPort_OutputMode_pushPull,IfxPort_OutputIdx_general);
	//NERR
	IfxPort_setPinModeInput(CAN1_NERR, IfxPort_InputMode_pullUp);
}

IFX_INLINE void Tle9252EnterNormalMode(void)
{
	//EN
	IfxPort_setPinHigh(CAN1_EN);
	//NSTB
	IfxPort_setPinHigh(CAN1_NSTB);
}

IFX_INLINE void Tle9252EnterReceiveOnlyMode(void)
{
	//EN
	IfxPort_setPinLow(CAN1_EN);
	//NSTB
	IfxPort_setPinHigh(CAN1_NSTB);
}

IFX_INLINE void Tle9252EnterSleepMode(void)
{
	//NSTB
	IfxPort_setPinLow(CAN1_NSTB);
}

IFX_INLINE void Tle9252EnterStandbyMode(void)
{
	//EN
	IfxPort_setPinLow(CAN1_EN);
	//NSTB
	IfxPort_setPinLow(CAN1_NSTB);
}

IFX_INLINE boolean Tle9252_getErrorState(void)
{
	if(IfxPort_getPinState(CAN1_NERR))
		return FALSE;
	else
		return TRUE;
}


/******************************************************************************/
/*------------------------------Global variables------------------------------*/
/******************************************************************************/
extern App_MulticanBasic 	g_MulticanBasic;
extern IfxMultican_Message  Node0Readmsg;   //9252,用于CCP程序
extern IfxMultican_Message  Node1Readmsg;	//9251



/******************************************************************************/
/*-------------------------Function Prototypes--------------------------------*/
/******************************************************************************/
extern void Multican_init(void);
extern void CAN_TLE9251_config_node1(void);

extern void CAN_TLE9252_config_node0(void);
extern void CanRX_MsgObj_initConfig(IfxMultican_Can_MsgObjConfig *config, CanRX_MsgObjInit *initStru);
extern void CanTX_MsgObj_initConfig(IfxMultican_Can_MsgObjConfig *config, CanTX_MsgObjInit *initStru);




#endif
