/******************************************************************************

                  版权所有 (C), 2020-2030, 重庆和天电子科技有限公司

 ******************************************************************************
  文 件 名  : MultiCAN.c
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



/******************************************************************************/
/*-----------------------------------Macros-----------------------------------*/
/******************************************************************************/

/******************************************************************************/
/*--------------------------------Enumerations--------------------------------*/
/******************************************************************************/

/******************************************************************************/
/*-----------------------------Data Structures--------------------------------*/
/******************************************************************************/

/******************************************************************************/
/*------------------------------Global variables------------------------------*/
/******************************************************************************/
App_MulticanBasic   g_MulticanBasic;
IfxMultican_Message Node0Readmsg;	//9252
IfxMultican_Message Node1Readmsg;	//9251



/******************************************************************************/
/*-------------------------Function Prototypes--------------------------------*/
/******************************************************************************/

/******************************************************************************/
/*------------------------Private Variables/Constants-------------------------*/
/******************************************************************************/

/******************************************************************************/
/*-------------------------Function Implementations---------------------------*/
/******************************************************************************/


/***************************************************************
 函数名称： void Multican_init(void)
 输入变量：
 输出变量：
 函数注释： CAN初始化的步骤是：曾军20201228
 * 1、CAN模块配置
 * 2、CAN模块初始化
 * 3、CAN节点配置
 * 4、CAN节点初始化
 * 5、CAN消息对象配置
 * 6、CAN消息对象初始化
 * CAN1采用查询方式，因此，此处暂时不用设置中断优先级及服务
 **************************************************************/
void Multican_init(void)
{
    /* create module config */                                                 // 创建CAN模块
    IfxMultican_Can_Config canConfig;
    IfxMultican_Can_initModuleConfig(&canConfig, &MODULE_CAN);                 // 初始化CAN模块配置

    /* Set up the service request node,less than 16*/                          // 设置中断服务请求节点，须少于16个，下面共设置了4个节点指针
    canConfig.nodePointer[IfxMultican_SrcId_1].priority = ISR_PRIORITY_CAN0_RX;// CAN0 RX interrupt priority
    canConfig.nodePointer[IfxMultican_SrcId_1].typeOfService = IfxSrc_Tos_cpu0;// Interrupt service type: CPU0
    canConfig.nodePointer[IfxMultican_SrcId_2].priority = ISR_PRIORITY_CAN0_ER;// CAN0 node0错误中断   busoff 中断优先级
    canConfig.nodePointer[IfxMultican_SrcId_2].typeOfService = IfxSrc_Tos_cpu0;// 中断服务类型， CPU中断响应
    // CAN1采用查询方式，因此，此处暂时不用设置中断优先级及服务
    canConfig.nodePointer[IfxMultican_SrcId_3].priority = ISR_PRIORITY_CAN1_RX;// CAN1 RX interrupt priority
    canConfig.nodePointer[IfxMultican_SrcId_3].typeOfService = IfxSrc_Tos_cpu0;
	canConfig.nodePointer[IfxMultican_SrcId_4].priority = ISR_PRIORITY_CAN1_ER;// CAN1错误中断   busoff
	canConfig.nodePointer[IfxMultican_SrcId_4].typeOfService = IfxSrc_Tos_cpu0;

    /* initialize module */
    IfxMultican_Can_initModule(&g_MulticanBasic.drivers.can, &canConfig);       // 初始化CAN模块

    CAN_TLE9251_config_node1();                                                 // 节点1配置为普通CAN

	CAN_TLE9252_config_node0();

//	CANSend_flag.CANSendAllow = 1;
//	CANSend_flag.CAN1SendAllow = 1;

	tl_queue_init(&gl_rxDataQueue);
	tl_queue_init(&gl_txDataQueue);
}

/***************************************************************
 函数名称： void CAN_TLE9251_config_node1(void)
 输入变量：
 输出变量：
 函数注释： TLE9251 节点1配置及初始化，消息对象配置及初始化
 **************************************************************/
void CAN_TLE9251_config_node1(void)   // tle9251配置为CAN节点1，曾军20201228
{
	/* create CAN node config */                                                    // 创建节点配置
    IfxMultican_Can_NodeConfig canNodeConfig;
    IfxMultican_Can_Node_initConfig(&canNodeConfig, &g_MulticanBasic.drivers.can);

    {  //node1
    	canNodeConfig.baudrate  = CAN2_TLE9251V_BAUDRATE;     // 500 KBaud          // 更改波特率为 500K
        canNodeConfig.nodeId    = (IfxMultican_NodeId)((int)IfxMultican_NodeId_1);  // 定义节点 ID
        canNodeConfig.rxPin     = &CAN2_RXD_IO_CONFIG;                      		// 指定接收对应的MCU针脚
        canNodeConfig.rxPinMode = IfxPort_InputMode_pullUp;                         // 指定针脚输入模式为接上拉电阻
        canNodeConfig.txPin     = &CAN2_TXD_IO_CONFIG;                      		// 指定发送对应的MCU针脚
        canNodeConfig.txPinMode = IfxPort_OutputMode_pushPull;                      // 指定输出模式为推挽式
        // tle9251接收不用中断形式，而用查询方式，因此这里禁止中断
//    	canNodeConfig.alertInterrupt.enabled = TRUE;                              	// 使能警告中断
//    	canNodeConfig.alertInterrupt.srcId   = IfxMultican_SrcId_4;               	// 指定错误中断的中断ID

        IfxMultican_Can_Node_init(&g_MulticanBasic.drivers.canNode1, &canNodeConfig); // 初始化CAN节点
    }

    // 节点1 CAN接收的消息对象配置
    {	//node1 msg1 of data receive
        /* create message object config */	//CAN Node 1
        IfxMultican_Can_MsgObjConfig canMsgObjConfig;                               // 创建消息对象配置
        IfxMultican_Can_MsgObj_initConfig(&canMsgObjConfig, &g_MulticanBasic.drivers.canNode1);
        canMsgObjConfig.msgObjId              = 1;
        canMsgObjConfig.messageId             = CAN1_MO_RXPDU_ID;
        canMsgObjConfig.acceptanceMask        = CAN_MO_RXPDU_Msk;
        canMsgObjConfig.frame                 = IfxMultican_Frame_receive;
        canMsgObjConfig.control.messageLen    = IfxMultican_DataLengthCode_8;
        canMsgObjConfig.control.extendedFrame = FALSE;//改为标准帧-20240816
        canMsgObjConfig.control.matchingId    = TRUE;
        canMsgObjConfig.control.fastBitRate   = FALSE;		//数据帧的快速波特率使能
        canMsgObjConfig.rxInterrupt.enabled   = TRUE;
        canMsgObjConfig.rxInterrupt.srcId     = IfxMultican_SrcId_3;
        /* initialize message object */
        IfxMultican_Can_MsgObj_init(&g_MulticanBasic.drivers.canNode1MsgObjRx, &canMsgObjConfig);
    }

    // 节点1 CAN发送的消息对象配置
    {	//node1 msg0 of data transmit
        /* create message object config */
        IfxMultican_Can_MsgObjConfig canMsgObjConfig;
        IfxMultican_Can_MsgObj_initConfig(&canMsgObjConfig, &g_MulticanBasic.drivers.canNode1);
        canMsgObjConfig.msgObjId              = 0;
        canMsgObjConfig.messageId             = CAN1_MO_TXPDU_ID;
        canMsgObjConfig.acceptanceMask        = CAN_MO_TXPDU_Msk;
        canMsgObjConfig.frame                 = IfxMultican_Frame_transmit;
        canMsgObjConfig.control.messageLen    = IfxMultican_DataLengthCode_8;
        canMsgObjConfig.control.extendedFrame = FALSE;//改为标准帧-20240816
        canMsgObjConfig.control.matchingId    = TRUE;
        canMsgObjConfig.control.fastBitRate   = FALSE;		//数据帧的快速波特率使能
        /* initialize message object */
        IfxMultican_Can_MsgObj_init(&g_MulticanBasic.drivers.canNode1MsgObjTx, &canMsgObjConfig);
    }

    Tle9251PortModeSet();
    drv_can9251_set_mode(Normal);
}


void CAN_TLE9252_config_node0(void)         // tle9252配置为CAN节点0，曾军20201228
{
    /* create CAN node config */            // CAN节点配置
    IfxMultican_Can_NodeConfig canNodeConfig;
    IfxMultican_Can_Node_initConfig(&canNodeConfig, &g_MulticanBasic.drivers.can);

    {  //node0
    	canNodeConfig.baudrate = CAN1_TLE9252V_BAUDRATE;     // 500 KBaud
        canNodeConfig.nodeId    = (IfxMultican_NodeId)((int)IfxMultican_NodeId_0);
        canNodeConfig.rxPin     = &CAN1_RXD_IO_CONFIG;
        canNodeConfig.rxPinMode = IfxPort_InputMode_pullUp;
        canNodeConfig.txPin     = &CAN1_TXD_IO_CONFIG;
        canNodeConfig.txPinMode = IfxPort_OutputMode_pushPull;

        // 错误中断
//        canNodeConfig.alertInterrupt.enabled = TRUE;
//        canNodeConfig.alertInterrupt.srcId = IfxMultican_SrcId_2;

        IfxMultican_Can_Node_init(&g_MulticanBasic.drivers.canNode0, &canNodeConfig);
    }

    {
		/* Message object  */
		//node0 msg0 of data receive
		/* create message object config */  //CAN Node 0
		IfxMultican_Can_MsgObjConfig canMsgObjConfig;
		IfxMultican_Can_MsgObj_initConfig(&canMsgObjConfig, &g_MulticanBasic.drivers.canNode0);

		for(uint8 i = 0; i < CanTxFrm9252Num; i++)
		{
			canMsgObjConfig.msgObjId              = CanTxFrm_9252_InitTab[i].msgObjId;
			canMsgObjConfig.messageId             = CanTxFrm_9252_InitTab[i].messageId;
			canMsgObjConfig.acceptanceMask        = 0x7FFFFFFFUL;
			canMsgObjConfig.frame                 = IfxMultican_Frame_transmit;
			canMsgObjConfig.control.messageLen    = IfxMultican_DataLengthCode_8;
			canMsgObjConfig.control.extendedFrame = FALSE;
			canMsgObjConfig.control.matchingId    = TRUE;		//接收时，是否匹配IDE位(标准或者扩展帧) 如果为FALSE则都可接收
			canMsgObjConfig.control.fastBitRate   = FALSE;		//数据帧的快速波特率使能

			// initialize message object
			IfxMultican_Can_MsgObj_init(&g_MulticanBasic.drivers.canNode0MsgTx2[i], &canMsgObjConfig);
		}
    }

    // 节点0 CAN接收的消息对象配置
    {	//node0 msg1 of data receive
        /* create message object config */
		IfxMultican_Can_MsgObjConfig canMsgObjConfig;
		IfxMultican_Can_MsgObj_initConfig(&canMsgObjConfig, &g_MulticanBasic.drivers.canNode0);

		for(uint8 j = 0; j < CanRxFrm9252Num; j++)
		{
			canMsgObjConfig.msgObjId              = CanRxFrm_9252_InitTab[j].msgObjId; //空出两个MO作为上一个MO的CANFD的top与bottom数据空间
			canMsgObjConfig.messageId             = CanRxFrm_9252_InitTab[j].messageId;
			canMsgObjConfig.acceptanceMask        = 0x7FFFFFFFUL;
			canMsgObjConfig.frame                 = IfxMultican_Frame_receive;
			canMsgObjConfig.control.messageLen    = IfxMultican_DataLengthCode_8;
			canMsgObjConfig.control.extendedFrame = FALSE;
			canMsgObjConfig.control.matchingId    = TRUE;		//接收时，是否匹配IDE位(标准或者扩展帧) 如果为FALSE则都可接收
			canMsgObjConfig.control.fastBitRate   = FALSE;		//数据帧的快速波特率使能
			canMsgObjConfig.rxInterrupt.enabled     = TRUE;
			canMsgObjConfig.rxInterrupt.srcId       = IfxMultican_SrcId_1;

			/* initialize message object */
			IfxMultican_Can_MsgObj_init(&g_MulticanBasic.drivers.canNode0MsgRx2[j], &canMsgObjConfig);
		}
    }

    Tle9252PortModeSet();
    drv_can9252_set_mode(Normal);
}




