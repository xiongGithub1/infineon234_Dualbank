/**********************************************************************************************************************
 * \file    uds_main.c
 * \brief
 * \version V1.0.0
 * \date    2022年3月10日
 * \author  Administrator
 *********************************************************************************************************************/
#include "uds_main.h"

void UdsRxCANMsgMainFun(tRxTxCanMsg *recvMsg)
{
    /* 将总线接收到的消息放入总线FIFO中 */
    CANTP_DriverWriteDataInCANTP(recvMsg->usRxTxDataId, 8, recvMsg->aucDataBuf);
}

void UdsInit(tUdsId xRxFunId,tUdsId xRxPhyId,tUdsId xTxId)
{
	// 注册CAN接收报文的UDS处理函数
	CanRegisterUdsRxMsgHandler(UdsRxCANMsgMainFun);
	CANTP_Init(xRxFunId,xRxPhyId,xTxId);	// CAN Transport Protocol Layer，CAN传输协议层
}

void UdsMainProcess(void)
{
	if (TRUE == uds_timer_Is1msTickTimeout())   //1ms超时
	{
		TP_SystemTickCtl();   //TP层的时间控制
		UDS_SystemTickCtl();   //应用层的时间控制
	}

	CANTP_MainFun();   	//传输层处理函数，传输协议（TP, Transport Protocol）
	UDS_MainFun();   	//应用层处理函数，执行服务程序，如0x10服务

	SendMsgMainFun();   //发送处理函数
}
