/******************************************************************************

                  版权所有 (C), 2020-2030, 重庆和天电子科技有限公司

 ******************************************************************************
  文 件 名  : CANapp.h
  版 本 号  : 初稿
  作    者    :
  生成日期 : 2024年3月11日
  最近修改 :
  功能描述 :
  函数列表 :
  修改历史 :
  1.日    期  :
  2.作    者  :
    修改内容   :

******************************************************************************/

#ifndef _CAN_RXTX_INTERFACE_H_
#define _CAN_RXTX_INTERFACE_H_

/*****************************************************************************/
/*----------------------------------包含头文件--------------------------------*/
/*****************************************************************************/
#include "Platform_Types.h"
#include <Multican/Can/IfxMultican_Can.h>
#include <Multican/Std/IfxMultican.h>
#include "tool_class.h"

/******************************************************************************/
/*-----------------------------------Macros-----------------------------------*/
/******************************************************************************/

/******************************************************************************/
/*--------------------------------Enumerations--------------------------------*/
/******************************************************************************/

/******************************************************************************/
/*-----------------------------Data Structures--------------------------------*/
/******************************************************************************/

/* 报文ID配置 */
typedef enum
{
	/* Rx */
	UDS_FUN_ADDR_ID = 0x7DFu,
	UDS_PHY_ADDR_ID = 0x74Cu,
	/* Tx */
	UDS_RESP_ADDR_ID = 0x75Cu,
}UdsCanMsgId;


typedef enum
{
	Normal,
	ReceiveOnly,
	GoToSleep,
	Sleep,
	StandBy,
	PowerReset
}TLE925x_CAN_MODE;

typedef enum
{
	Enable_Rx_Enable_Tx,
	Enable_Rx_Disable_Tx,
	Enable_Tx_Disbale_Rx,
	Disbale_Rx_Disbale_Tx
}UdsComCtrlCanMsgRxTxMode; 		//用于UDS通信控制时，各报文的收发模式

typedef enum
{
	CAN1Port,
	CAN2Port
}CAN_Name;

typedef struct
{
//	uint32 ucRxDataLen;       /* RX CAN hardware data len */
    uint16 usRxTxDataId;        /* RX data ID */
    uint8 aucDataBuf[8];    /* RX data buffer */
} tRxTxCanMsg;


typedef struct
{
    uint16     msgObjId;      // 可理解为其它芯片的邮箱号
    uint16     messageId;
    UdsComCtrlCanMsgRxTxMode	RxTxStatus;
    void (*RxMsgDeal)(void ); // 对应该报文的处理函数
}CanRX_MsgObjInit;        // 曾军加 20201230


typedef struct
{
    uint16     msgObjId;
    uint16     messageId;
    UdsComCtrlCanMsgRxTxMode	RxTxStatus;
    void (*TxMsgDeal)(void );// 对应该报文的处理函数
}CanTX_MsgObjInit;       // 曾军加 20201230


/******************************************************************************/
/*------------------------------Global variables------------------------------*/
/******************************************************************************/
extern const uint16  CanRxFrm9252Num;
extern const uint16  CanTxFrm9252Num;

extern CanRX_MsgObjInit CanRxFrm_9252_InitTab[];
extern CanTX_MsgObjInit CanTxFrm_9252_InitTab[];

extern DataQueue gl_rxDataQueue;// 接收缓冲区
extern DataQueue gl_txDataQueue;// 发送缓冲区


/******************************************************************************/
/*-------------------------Function Prototypes--------------------------------*/
/******************************************************************************/
extern void Can9252RxLookup(void);
extern void CANtle9252Send(uint16 messageId, uint32 dataLow, uint32 dataHigh );

typedef void (*tpfDataParser)(tRxTxCanMsg *);

void drv_can9252_set_mode(TLE925x_CAN_MODE TLE9252mode);
void drv_can9251_set_mode(TLE925x_CAN_MODE TLE9251mode);

uint8 drv_can1_send(tRxTxCanMsg *txMsg);
void DrvCanSendMessage(tRxTxCanMsg *txMsg);
void CanRegisterUdsRxMsgHandler(tpfDataParser udsCallback);
void CanRegisterAppRxMsgHandler(tpfDataParser appCallback);
void DrvCanRxTxModeSet(uint8 mode);
void CanMainProcess(void);

#endif
