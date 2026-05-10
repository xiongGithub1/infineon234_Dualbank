
/**********************************************************************************************************************
 * \file    Can.h
 * \brief
 * \version V1.0.0
 * \date
 *********************************************************************************************************************/


#ifndef APP_UDS_MCU_PORT_CAN_H_
#define APP_UDS_MCU_PORT_CAN_H_

/*********************************************************************************************************************/
/*-----------------------------------------------------Includes------------------------------------------------------*/
/*********************************************************************************************************************/
#include <App_bootloader_cfg.h>
#include "Std_Types.h"
#include "Cpu0_Main.h"
#include "IfxMultican_Can.h"
#include "IfxMultican.h"
#include "IfxPort.h"                                             /* For GPIO Port Pin Control                        */


/*********************************************************************************************************************/
/*------------------------------------------------------Macros-------------------------------------------------------*/
/*********************************************************************************************************************/
/* Message box status defines */
#define CAN_MB_TX_ONCE              0xcu
#define CAN_MB_INACTIVE             0x8u
#define CAN_MB_RX                   0x4u
#define CAN_MB_ABORT                0x9u
#define CAN_MB_RX_OVERRUN           0x6u

#define CAN_ERRINT                  0x00000002uL
#define CAN_BUSOFF                  0x00000004uL

/* mcr register */
#define CAN_MCR_MDIS        0x7FFFFFFFuL
#define CAN_MCR_FRZACK      0x01000000uL
#define CAN_MCR_MBFEN       0x00010000uL
#define CAN_MCR_MAXMB       31u
#define CAN_MCR_NOTRDY      0x08000000uL

#define CAN_MCR_SOFTREST    0x02000000uL

/* control register */
#define CAN_CR_CLKSRC       0xFFFFDFFFuL
#define CAN_CR_RJW          0x00C00000uL
#define CAN_CR_SMP          0xFFFFFF7FuL

/* Can Buffer */
#define CAN_IDE_STANDARD    0xFFDFFFFFuL
#define CAN_IDE_EXTENDED    0x00200000uL
/*********************************************************************************************************************/
/*-------------------------------------------------Global variables--------------------------------------------------*/
/*********************************************************************************************************************/

/*********************************************************************************************************************/
/*-------------------------------------------------Data Structures---------------------------------------------------*/
/*********************************************************************************************************************/
typedef uint16  PduIdType;

typedef uint16  PduLengthType;

typedef struct
{
    uint8 *sduDataPtr;
    PduLengthType sduLength;
} PduInfoType;


/** Peripheral Message Object*/
typedef struct
{
    uint32 contorlandstatus; /**< contorlandstatus :message code ,length and timestamp. */
    uint32 identifier;  /**< Identifier Field. */
    uint8 data[8u];     /**< Data Field0-Data Field7 (1 byte each). */
 } Can_Mbuffer_Type;




// typedef struct
// {
//     struct
//     {
//         IfxMultican_Can        can;          /**< \brief CAN driver handle */
//         IfxMultican_Can_Node   canNode;   /**< \brief CAN Source Node */
////       IfxMultican_Can_Node   canDstNode;   /**< \brief CAN Destination Node*/
//         IfxMultican_Can_MsgObj canRxMsgObj; /**< \brief CAN Destination Message object */
//         IfxMultican_Can_MsgObj canTxMsgObj; /**< \brief CAN Source Message object */
//     }drivers;
// } App_MulticanBasic;
//extern App_MulticanBasic g_MulticanBasic;

/*********************************************************************************************************************/
/*--------------------------------------------Private Variables/Constants--------------------------------------------*/
/*********************************************************************************************************************/


//IFX_INLINE void Tle9251PortModeSet(void)
//{
//	IfxPort_setPinModeOutput(CAN2_EN_PIN,IfxPort_OutputMode_pushPull,IfxPort_OutputIdx_general);
//}
//
//IFX_INLINE void Tle9251EnterNormalMode()
//{
//	IfxPort_setPinLow(CAN2_EN_PIN);
//}

/*********************************************************************************************************************/
/*------------------------------------------------Function Prototypes------------------------------------------------*/
/*********************************************************************************************************************/
//void CAN_init(void);
void CAN_deinit(void);
void CAN_main(void);
void Can_RxIndicationMainFunc(void);
//uint32 Can_FindBuffer(uint32 buffer_mask) ;
//void CAN_TLE9251_config_node1(void);

extern void Multican_TLE9251_run(uint32 dataLow, uint32 dataHigh);



#endif /* APP_UDS_MCU_PORT_CAN_H_ */
