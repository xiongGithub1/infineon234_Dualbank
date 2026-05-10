
/**********************************************************************************************************************
 * \file    Can.c
 * \brief
 * \version V1.0.0
 *********************************************************************************************************************/



/*********************************************************************************************************************/
/*-----------------------------------------------------Includes------------------------------------------------------*/
/*********************************************************************************************************************/
#include    <stdio.h>
#include    "Can.h"
#include    "App_bootloader_cfg.h"
#include    "Can_session.h"
#include    "CanIf.h"
#include 	"MultiCAN.h"
#include    "IfxMultican.h"
#include    "ConfigurationIsr.h"
/*********************************************************************************************************************/
/*-------------------------------------------------Global variables--------------------------------------------------*/
/*********************************************************************************************************************/
//App_MulticanBasic g_MulticanBasic; /**< \brief Demo information */
IfxMultican_Message rxmsg;
IfxMultican_Status  readStatus;



/*********************************************************************************************************************/
/*--------------------------------------------Private Variables/Constants--------------------------------------------*/
/*********************************************************************************************************************/


/*********************************************************************************************************************/
/*------------------------------------------------Function Prototypes------------------------------------------------*/
/*********************************************************************************************************************/

/*********************************************************************************************************************/
/*---------------------------------------------Function Implementations----------------------------------------------*/
/*********************************************************************************************************************/


/*
 ** ============================================================================
 ** @Function    ：
 ** @Description ：Can_FindBuffer,find the buffer for rx or tx.
 ** @Parameters  ：
 ** @Returns     ：
 ** @Date        ：
 ** ============================================================================
 */
//uint32 Can_FindBuffer(uint32 buffer_mask)
//{
//    boolean exit_Flag       = FALSE;
//    uint32  compare_lastbit = 0x00000001uL;
//    uint8   count           = 0u;
//
//    while ((count < 32u) && (exit_Flag == FALSE))
//    {
//        if(((uint32)(buffer_mask >> count) & compare_lastbit) == 0u)
//        {
//         /* if this bit is zero, then count next */
//         count ++;
//        }
//        else
//        {
//         exit_Flag = TRUE;
//        }
//    }
//    return count;
//}


/*
 ** ============================================================================
 ** @Function    ：CAN接收消息处理
 ** @Description ：Handle the Can Message receive and indication to Cantp .
 ** @Parameters  ：
 ** @Returns     ：
 ** @Date        ：
 ** ============================================================================
 */
// 轮巡查询CAN接收状态，并把接收到的数据按TC234的数据格式进行缓存，当缓存满后，再决定是否下载等指令。曾军20220726理解
IFX_INTERRUPT(isrCAN1_RX, 0, ISR_PRIORITY_CAN1_RX);

void isrCAN1_RX(void)
{
	uint8 receiveFrameState;	// 接收帧状态，1：初始值，0：单帧或连续帧最后一帧，2：连续帧首帧或中间帐，4：流控帧

    if(TRUE == IfxMultican_Can_MsgObj_isRxPending(&g_MulticanBasic.drivers.canNode1MsgObjRx) )
    {
        IfxMultican_Message_init(&rxmsg, 0x0, 0x0, 0x0, IfxMultican_DataLengthCode_8);      /* start with invalid values */
        readStatus = IfxMultican_Can_MsgObj_readMessage(&g_MulticanBasic.drivers.canNode1MsgObjRx, &rxmsg);
        // 此函数可以读出CAN数据，并保存到rxmsg->data[0]和data[1]中，只是嵌套的比较深，曾军20220726

        if(CAN1_MO_RXPDU_ID == rxmsg.id)
        {
//        	IfxMultican_Can_MsgObj_cancelSend (&g_MulticanBasic.drivers.canTxMsgObj);
//        	IfxMultican_Can_MsgObj_clearTxPending(&g_MulticanBasic.drivers.canTxMsgObj);
        	receiveFrameState = CanIf_fillBuffer(rxmsg,  &gAppData.rx );	//gAppData.rx值是通过CanIf_fillBuffer函数运行赋值的，曾军20220726
            if(receiveFrameState == 0)		// 如果为单帧或连续帧长度小于7
            {
                CAN_SSN_ServiceWithId();//r=0意味着可以进入和上位机通信等待下一步操作
            }
        }
    }
}

// Original polling function, kept for compatibility. Reception is now handled by isrCAN1_RX interrupt.
void Can_RxIndicationMainFunc(void)
{
    // Interrupt mode: isrCAN1_RX handles CAN1 reception
}


/*
 ** ============================================================================
 ** @Function    ：CAN模块复位
 ** @Description ：
 ** @Parameters  ：
 ** @Returns     ：
 ** @Date        ：
 ** ============================================================================
 */
void CAN_deinit(void)
{
   IfxMultican_deinit(&MODULE_CAN);		// 在库文件IfxMultican_Can.c文件中有void IfxMultican_deinit(Ifx_CAN *mcan)
//   IfxMultican_deinit(&MODULE_CAN1);	// 这个没使用，因为不能复位，否则会进入陷阱程序，曾军20250718
    return;
}



void Multican_TLE9251_run(uint32 dataLow, uint32 dataHigh)
{
    /* Transmit Data */
	IfxMultican_Message msg;
	IfxMultican_Message_init(&msg, CAN1_MO_TXPDU_ID,dataLow, dataHigh, IfxMultican_DataLengthCode_8);
	while(IfxMultican_Can_MsgObj_sendMessage(&g_MulticanBasic.drivers.canNode1MsgObjTx, &msg) == IfxMultican_Status_notSentBusy );
}

