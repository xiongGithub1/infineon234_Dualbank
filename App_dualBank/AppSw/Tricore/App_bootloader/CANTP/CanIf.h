
/**********************************************************************************************************************
 * \file    CanIf.h
 * \brief
 * \version V1.0.0
 * \date
 *********************************************************************************************************************/


#ifndef CANTP_CANIF_H_
#define CANTP_CANIF_H_

/*********************************************************************************************************************/
/*-----------------------------------------------------Includes------------------------------------------------------*/
/*********************************************************************************************************************/
#include "Std_Types.h"
#include <App_bootloader_cfg.h>
#include "IfxMultican.h"
#include "IfxMultican_Can.h"
#include "Cpu0_Main.h"



/*********************************************************************************************************************/
/*------------------------------------------------------Macros-------------------------------------------------------*/
/*********************************************************************************************************************/
#define UDS_FRAME_TYPE_

typedef enum
{
    /* Not specified by ISO15765 - used as error return type when decoding frame. */
    SINGLE_FRAME =0u,       /* Consecutive Frame */
    FIRST_FRAME =1u,        /* Consecutive Frame */
    CONSECUTIVE_FRAME =2,      /* Consecutive Frame */
    /* FC with CTS:Continus to send */
    FLOW_CONTROL_CTS_FRAME  = 3     /* Consecutive Frame */
}cantp_iso15765_frame_enum;
/*********************************************************************************************************************/
/*-------------------------------------------------Global variables--------------------------------------------------*/
/*********************************************************************************************************************/

/*********************************************************************************************************************/
/*-------------------------------------------------Data Structures---------------------------------------------------*/
/*********************************************************************************************************************/
typedef struct
{
	/* N_AI */
//	uint8   idMsgType;      /* message type */
//	uint8   idTarType;      /* network target address type */
//	uint8   idTarAddr;      /* network target address */
//	uint8   idSrcAddr;      /* network source address   */
	// 上面4个是扩展帧ID的定义
	uint8	idStandardFrameh;	// 标准帧ID的高字节，占3位，前面补0，因标准帧是11位
	uint8	idStandardFramel;	// 标准帧ID的低字节，占8位
	/* N_PCI */
//	uint8 ubProtocol_AE ;//[network address extension]
	cantp_iso15765_frame_enum  protocolType;// network protocol control information type
	volatile uint8   protocolFrameCnt;   /* continue Frame */
	uint16  dataLength ;     //ubResult+ubData_Array
	/* N_Data */
//	uint8 ubResult ;    //[]
	uint16  dataCnt ;
	uint8   data[CAN_DATA_BUFF_Length] ;
}diag_can_stt;


typedef struct
{
    float32 sysFreq;                /**< \brief Actual SPB frequency */
    float32 cpuFreq;                /**< \brief Actual CPU frequency */
    float32 pllFreq;                /**< \brief Actual PLL frequency */
    float32 stmFreq;                /**< \brief Actual STM frequency */
} cpu_freq_stt;


typedef struct
{
	/* cpu info */
	cpu_freq_stt  cpuFreq;

	/* flag */
	uint16  ramFlag;
	uint16  *pRamFlag;
	uint16  flashFlag;

	/* can */
	diag_can_stt tx;
	diag_can_stt rx;

	/* tmr */
	Ifx_TickTime bspTime;
	Ifx_TickTime ledTime;
}app_data_stt;


typedef union
{
	uint32 b16;
	uint8  b8[2];
}un4byte_union;


/*********************************************************************************************************************/
/*--------------------------------------------Private Variables/Constants--------------------------------------------*/
/*********************************************************************************************************************/
extern app_data_stt gAppData;




/*********************************************************************************************************************/
/*------------------------------------------------Function Prototypes------------------------------------------------*/
/*********************************************************************************************************************/
uint8 CanIf_fillBuffer(IfxMultican_Message msg,  diag_can_stt *canbuff);
int  CanIf_TransmitBuffer(IfxMultican_Can_MsgObj *msgObj, diag_can_stt *canbuff);
void CAN_CleanTxBuffer(diag_can_stt *canbuff);
#endif /* CANTP_CANIF_H_ */
