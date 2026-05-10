/******************************************************************************

                  版权所有 (C), 2020-2030, 重庆和天电子科技有限公司

 ******************************************************************************
  文 件 名  : CANRxTxInterface.c
  版 本 号  : 初稿
  作    者    :
  生成日期 : 2021年10月15日
  最近修改 :
  功能描述 : CAN通讯初始化相关设置
  函数列表 :
  修改历史 :
  1.日    期  :
  2.作    者  :
    修改内容   :20221210 增加了CANFD节点初始化的函数，CANFD接收中断的函数，CANFD数据发送初始
    化函数，在模块初始化函数里增加了CAN和CANFD的选择项，增加了CANFD发送和接收的ID定义。曾军
  20221212如果要在CANFD中只发送8个字节的数据，那么就需要在CANFD初始化函数void
  CAN_TLE9252_config_node0_CANFD(void)中，把发送字节长度canMsgObjConfig.control.messageLen
  = IfxMultican_DataLengthCode_32;改为IfxMultican_DataLengthCode_8;然后再改其它的。
******************************************************************************/


/*****************************************************************************/
/*----------------------------------包含头文件--------------------------------*/
/*****************************************************************************/
#include "MultiCAN.h"
#include "CANRxTxInterface.h"
#include "tool_class.h"
#include "uds_app.h"
#include "ConfigurationIsr.h"


/******************************************************************************/
/*-----------------------------------Macros-----------------------------------*/
/******************************************************************************/
/*
// 用于地址分配，但应该没有用上，参考20221121赵工发的CANFD驱动程序
#pragma align 32
Ifx_CAN_MO   g_MoBuff;   //ram ,dma to mo
#pragma align 32
*/
// 报文解析函数入口
static tpfDataParser gl_appRxCanMsgMainFuction = ((void *) 0);
static tpfDataParser gl_udsRxCanMsgMainFuction = ((void *) 0);


// 收发缓冲区
DataQueue gl_rxDataQueue;// 接收缓冲区
DataQueue gl_txDataQueue;// 发送缓冲区


/******************************************************************************/
/*--------------------------------Enumerations--------------------------------*/
/******************************************************************************/


/******************************************************************************/
/*-----------------------------Data Structures--------------------------------*/
/******************************************************************************/


/******************************************************************************/
/*------------------------------Global variables------------------------------*/
/******************************************************************************/
CanRX_MsgObjInit CanRxFrm_9252_InitTab[]=
{//消息对象ID  CAN消息ID
	{ 20,     UDS_FUN_ADDR_ID,		 Enable_Rx_Enable_Tx, ((void*)0)},
	{ 21,     UDS_PHY_ADDR_ID,		 Enable_Rx_Enable_Tx, ((void*)0)},
};


CanTX_MsgObjInit CanTxFrm_9252_InitTab[]=
{//           CAN消息ID
    { 10,     UDS_RESP_ADDR_ID,   	Enable_Rx_Enable_Tx,   ((void*)0)},// UDS_RESP_ADDR_ID
};


const uint16  CanRxFrm9252Num = (sizeof(CanRxFrm_9252_InitTab)/sizeof(CanRX_MsgObjInit));
const uint16  CanTxFrm9252Num = (sizeof(CanTxFrm_9252_InitTab)/sizeof(CanTX_MsgObjInit));




/******************************************************************************/
/*-------------------------Function Prototypes--------------------------------*/
/******************************************************************************/

/******************************************************************************/
/*------------------------Private Variables/Constants-------------------------*/
/******************************************************************************/

/******************************************************************************/
/*-------------------------Function Implementations---------------------------*/
/******************************************************************************/
IFX_INTERRUPT(isrCAN0_RX, 0, ISR_PRIORITY_CAN0_RX);

void isrCAN0_RX(void)
{
    uint32 status = 0;
	QueueMsgObject qmsg;

	for(uint8 j=0; j<CanRxFrm9252Num;j++)
	{
		if(TRUE == IfxMultican_Can_MsgObj_isRxPending(&g_MulticanBasic.drivers.canNode0MsgRx2[j]))
		{
			status = IfxMultican_Status_ok;
			status = IfxMultican_Can_MsgObj_readMessage(&g_MulticanBasic.drivers.canNode0MsgRx2[j],&Node0Readmsg);
			if(status != IfxMultican_Status_receiveEmpty)
			{
				if((Node0Readmsg.id==UDS_FUN_ADDR_ID)||(Node0Readmsg.id==UDS_PHY_ADDR_ID))
				{
					qmsg.id = (uint16)Node0Readmsg.id;
					tl_memcpy(qmsg.data,Node0Readmsg.data,8);
					tl_queue_add_item(&gl_rxDataQueue,&qmsg);
				}
			}
		}
	}
}

// Original polling function replaced by interrupt isrCAN0_RX. No need to call in main loop.
void Can9252RxLookup(void)
{
    // Interrupt mode: isrCAN0_RX handles CAN0 reception
}



void CANtle9252Send(uint16 messageId, uint32 dataLow, uint32 dataHigh )
{
	IfxMultican_Message msg;

	IfxMultican_Message_init(&msg, (uint32)messageId, dataLow, dataHigh, IfxMultican_DataLengthCode_8);
	while(IfxMultican_Can_MsgObj_sendMessage(&g_MulticanBasic.drivers.canNode0MsgTx2[0], &msg) == IfxMultican_Status_notSentBusy );
}


// 设置CAN的工作模式
void drv_can9252_set_mode(TLE925x_CAN_MODE TLE9252mode)
{
	switch(TLE9252mode)
	{
	case Normal:
	    Tle9252EnterNormalMode();
		break;
	case ReceiveOnly:	// 配置CAN1进入只收模式
		Tle9252EnterReceiveOnlyMode();
		break;
	case GoToSleep:
		Tle9252EnterSleepMode();
		break;
	case StandBy:
		Tle9252EnterStandbyMode();
		break;
	default:
		break;
	}
}


// 设置CAN的工作模式
void drv_can9251_set_mode(TLE925x_CAN_MODE TLE9251mode)
{
	switch(TLE9251mode)
	{
	case Normal:
	    Tle9251EnterNormalMode();
		break;
	case StandBy:
		Tle9251EnterStandbyMode();
		break;
	default:
		break;
	}
}

static int getMessageObject(uint16 msgId)
{
	int result = -1;
	for(int i = 0; i < CanTxFrm9252Num; i++)
	{
		if((CanTxFrm_9252_InitTab)[i].messageId == msgId)
		{
			result = i;
		}
	}
	return result;
}



uint8 drv_can1_send(tRxTxCanMsg *txMsg)
{
	int idx = getMessageObject(txMsg->usRxTxDataId);
	if(idx >= 0)
	{
		if(CanTxFrm_9252_InitTab[idx].RxTxStatus == Disbale_Rx_Disbale_Tx ||
				CanTxFrm_9252_InitTab[idx].RxTxStatus == Enable_Rx_Disable_Tx)
		{
			//表示UDS已禁止该报文的发送，直接返回
			return TRUE;
		}
		else
		{
			//使用while检测状态，当bus错误或者发送失败时会进入死循环？
			//while (IfxMultican_Can_MsgObj_sendMessage(msgObj, &msg) == IfxMultican_Status_notSentBusy);

			if(IfxMultican_Can_MsgObj_isTransmitRequested(&g_MulticanBasic.drivers.canNode0MsgTx2[0]))
			{
				//前一个报文被挂起，表示发送失败
				return FALSE;
			}
			else
			{
				IfxMultican_Message msg;
				const unsigned dataLow = txMsg->aucDataBuf[0] | (txMsg->aucDataBuf[1] << 8) |
						(txMsg->aucDataBuf[2] << 16) | (txMsg->aucDataBuf[3] << 24);
				const unsigned dataHigh = txMsg->aucDataBuf[4] | (txMsg->aucDataBuf[5] << 8) |
						(txMsg->aucDataBuf[6] << 16) | (txMsg->aucDataBuf[7] << 24);
				IfxMultican_Message_init(&msg, txMsg->usRxTxDataId, dataLow, dataHigh, 8);
				while (IfxMultican_Can_MsgObj_sendMessage(&g_MulticanBasic.drivers.canNode0MsgTx2[0], &msg) ==
						IfxMultican_Status_notSentBusy);
			}
		}
	}
	return TRUE;
}


void DrvCanSendMessage(tRxTxCanMsg *txMsg)
{
	tl_queue_add_item(&gl_txDataQueue,(QueueMsgObject *)txMsg);
}


void CanRegisterUdsRxMsgHandler(tpfDataParser udsCallback)
{
	gl_udsRxCanMsgMainFuction = udsCallback;
}

void CanRegisterAppRxMsgHandler(tpfDataParser appCallback)
{
	gl_appRxCanMsgMainFuction = appCallback;
}

void DrvCanRxTxModeSet(uint8 mode)
{
	for(int i = 0; i < CanRxFrm9252Num; i++)
	{
		if((CanRxFrm_9252_InitTab)[i].messageId == UDS_FUN_ADDR_ID
				|| (CanRxFrm_9252_InitTab)[i].messageId == UDS_PHY_ADDR_ID)
		{
			//msgObj =  &(stCan1RxTxMessageConfigs[i]).canMsgObj;
			continue;
		}
	}

	for(int j = 0; j < CanTxFrm9252Num; j++)
	{
		if((CanTxFrm_9252_InitTab)[j].messageId == UDS_RESP_ADDR_ID)
		{
			continue;
		}
		else if(mode == UDS_CC_MODE_RX_TX)	// 使能接收
		{
			CanTxFrm_9252_InitTab[j].RxTxStatus = Enable_Rx_Enable_Tx;
		}
		else if(mode == UDS_CC_MODE_RX_NO)
		{
			CanTxFrm_9252_InitTab[j].RxTxStatus = Enable_Rx_Disable_Tx;
		}
		else if(mode == UDS_CC_MODE_NO_TX)
		{
			CanTxFrm_9252_InitTab[j].RxTxStatus = Enable_Tx_Disbale_Rx;
		}
		else if(mode == UDS_CC_MODE_NO_NO)
		{
			CanTxFrm_9252_InitTab[j].RxTxStatus = Disbale_Rx_Disbale_Tx;
		}
	}
}

// CAN收发报文处理函数
void CanMainProcess(void)
{
	// 处理接收报文
	QueueMsgObject rxMsgObj;
	if(tl_queue_take_item(&gl_rxDataQueue,&rxMsgObj))
	{
		// 执行报文解析
		if(rxMsgObj.id == UDS_FUN_ADDR_ID || rxMsgObj.id == UDS_PHY_ADDR_ID)
		{
			if(gl_udsRxCanMsgMainFuction != ((void *) 0))
			{
				(gl_udsRxCanMsgMainFuction)((tRxTxCanMsg *)&rxMsgObj);
			}
		}
		else
		{
			if(gl_appRxCanMsgMainFuction != ((void *) 0))
			{
				(gl_appRxCanMsgMainFuction)((tRxTxCanMsg *)&rxMsgObj);
			}
		}
	}

	// 处理发送报文
	QueueMsgObject txMsgObj;
	if(tl_queue_take_item(&gl_txDataQueue,&txMsgObj))
	{
		drv_can1_send((tRxTxCanMsg *)&txMsgObj);
	}
}
