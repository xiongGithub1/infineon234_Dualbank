/*
 * can_tp.h
 *
 *  Created on: 2021年11月26日
 *      Author: Administrator
 */

#ifndef TP_CAN_TP_H_
#define TP_CAN_TP_H_

#include "uds_cfg.h"
#include "uds_common.h"
#include "uds_fifo.h"
#include "tool_class.h"
//#include "Platform_Types.h"

#define TPDebugPrintf(...)


typedef unsigned short tUdsId;
typedef unsigned short tUdsLen;
typedef unsigned short tNetTime;
typedef unsigned short tBlockSize;


#define RX_TP_QUEUE_ID ('R')   /* TP RX FIFO ID  ASCII码是82*/
#define TX_TP_QUEUE_ID ('T')   /* TP TX FIFO ID  ASCII码是84*/

/* Define FIFO length */
#define TX_TP_QUEUE_LEN (150u)  /* UDS send message to TP max length */
#define RX_TP_QUEUE_LEN (150u)  /* UDS read message from TP max length */

#define RX_BUS_FIFO_LEN     (300u)      /* RX BUS FIFO length */
#define RX_BUS_FIFO         ('r')       /* RX bus FIFO ASCII码是114*/
#define TX_BUS_FIFO         ('t')       /* RX bus FIFO ASCII码是116*/
#define TX_BUS_FIFO_LEN     (300)      /* RX BUS FIFO length */


/* Single message buffer len */
#define MAX_MESSAGE_LEN (16)

typedef struct
{
    uint16 rxDataLen;                   /* RX CAN hardware data len */
    uint16 rxDataId;                    /* RX data len */
    uint8 aucDataBuf[MAX_MESSAGE_LEN];  /* RX data buffer MAX_MESSAGE_LEN = 16*/
} tRxMsgInfo;

#define DATA_LEN                (8u)
#define SF_CANFD_DATA_MAX_LEN   (62u)   /* Max CAN FD Single Frame data len */
#define SF_CAN_DATA_MAX_LEN     (7u)    /* Max CAN2.0 Single Frame data len */

#define TX_SF_DATA_MAX_LEN      (7u)    /* RX support CAN FD, TX message is not support CAN FD */

#define FF_DATA_MIN_LEN         (8u)    /* Min First Frame data len*/

#define CF_DATA_MAX_LEN         (7u)    /* Single Consecutive Frame max data len */
#define MAX_CF_DATA_LEN         (150u)  /* Max Consecutive Frame data len */

#define NORMAL_ADDRESSING (0u) /* Normal addressing */
#define MIXED_ADDRESSING  (1u) /* Mixed addressing */

typedef enum
{
    IDLE,        /* CAN TP IDLE */
    RX_SF,       /* Wait single frame */
    RX_FF,       /* Wait first frame */
    RX_FC,       /* Wait flow control frame */
    RX_CF,       /* Wait consecutive frame */

    TX_SF,       /* TX single frame */
    TX_FF,       /* TX first frame */
    TX_FC,       /* TX flow control */
    TX_CF,       /* TX consecutive frame */

    WAITING_TX,  /* Waiting TX message */

    WAIT_CONFIRM /* Wait confirm */
} tCanTpWorkStatus;

typedef enum
{
    SF, /* Single frame value */
    FF, /* First frame value */
    CF, /* Consecutive frame value */
    FC  /* Flow control value */
} tNetWorkFrameType;

typedef enum
{
    N_OK = 0,       /* This value means that the service execution has completed successfully;
                       it can be issued to a service user on both the sender and receiver side */

    N_TIMEOUT_A,    /* This value is issued to the protocol user when the timer N_Ar/N_As has passed its time-out
                       value N_Asmax/N_Armax; it can be issued to service user on both the sender and receiver side. */

    N_TIMEOUT_Bs,   /* This value is issued to the service user when the timer N_Bs has passed its time-out value
                       N_Bsmax; it can be issued to the service user on the sender side only. */

    N_TIMEOUT_Cr,   /* This value is issued to the service user when the timer N_Cr has passed its time-out value
                       N_Crmax; it can be issued to the service user on the receiver side only. */

    N_WRONG_SN,     /* This value is issued to the service user upon reception of an unexpected sequence number
                       (PCI.SN) value; it can be issued to the service user on the receiver side only. */

    N_INVALID_FS,   /* This value is issued to the service user when an invalid or unknown FlowStatus value has
                       been received in a flow control (FC) N_PDU; it can be issued to the service user on the sender side only. */

    N_UNEXP_PDU,    /* This value is issued to the service user upon reception of an unexpected protocol data unit;
                       it can be issued to the service user on the receiver side only. */

    N_WTF_OVRN,     /* This value is issued to the service user upon reception of flow control WAIT frame that
                       exceeds the maximum counter N_WFTmax. */

    N_BUFFER_OVFLW, /* This value is issued to the service user upon reception of a flow control (FC) N_PDU with
                       FlowStatus = OVFLW. It indicates that the buffer on the receiver side of a segmented
                       message transmission cannot store the number of bytes specified by the FirstFrame
                       DataLength (FF_DL) parameter in the FirstFrame and therefore the transmission of the
                       segmented message was aborted. It can be issued to the service user on the sender side only. */

    N_ERROR         /* This is the general error value. It shall be issued to the service user when an error has been
                       detected by the network layer and no other parameter value can be used to better describe
                       the error. It can be issued to the service user on both the sender and receiver side. */
} tN_Result;


typedef struct
{
    uint8 isFree;            /* RX message status. TRUE = not received message. */
    uint16 xMsgId;           /* Received message ID */
    uint8 msgLen;            /* Received message len */
    uint8 aMsgBuf[DATA_LEN]; /* Message data buffer  DATA_LEN = 8*/
} tCanTpMsg;
typedef tN_Result (*tpfCanTpFun)(tCanTpMsg *, tCanTpWorkStatus *);

typedef struct
{
    tCanTpWorkStatus eCanTpStaus;
    tpfCanTpFun pfCanTpFun;
} tCanTpFunInfo;
typedef uint16 tCanTpDataLen;
typedef struct
{
    tUdsId xCanTpId;                 /* CAN TP message ID  2个字节*/
    tCanTpDataLen xPduDataLen;       /* PDU data len(RX/TX data len) 2个字节*/
    tCanTpDataLen xFFDataLen;        /* RX/TX FF data len 2个字节*/
    uint8 aDataBuf[MAX_CF_DATA_LEN]; /* RX/TX data buffer 150个字节*/
} tCanTpDataInfo;

typedef struct
{
    uint8 ucSN;               /* SN 1个字节*/
    uint8 ucBlockSize;        /* Block size 1个字节*/
    tNetTime xSTmin;          /* STmin 2个字节*/
    tNetTime xMaxWatiTimeout; /* Timeout time 2个字节*/
    tCanTpDataInfo stCanTpDataInfo;	//156个字节
    // 实际情况是因为考虑到内存对齐规则（通常编译器会对齐到其最大成员的大小），因此该结构体实际大小为164字节
} tCanTpInfo;

typedef struct
{
	tUdsId TxMsgID;       /* TX message ID */
	tUdsLen TxMsgLength;   /* TX message length */
    uint32 TxMsgCallBack; /* TX message callback */
} tTPTxMsgHeader;

typedef void (*tpfNetTxCallBack)(void);
typedef uint8 (*tNetTxMsg)(const tUdsId, const uint16, const uint8 *, const tpfNetTxCallBack, const uint32);
typedef uint8 (*tNetRx)(tUdsId *, uint8 *, uint8 *);

typedef void (*tpfAbortTxMsg)(void);
/* TX message callback */
typedef void (*tpfUDSTxMsgCallBack)(uint8);

typedef struct
{
    uint8 ucCalledPeriod;        /* Called CAN TP main function period */
    tUdsId xRxFunId;             /* RX FUN ID */
    tUdsId xRxPhyId;             /* RX PHY ID */
    tUdsId xTxId;                /* TX RESP ID */
    tBlockSize xBlockSize;       /* BS = block size */
    tNetTime xSTmin;             /* STmin */
    tNetTime xNAs;               /* N_As */
    tNetTime xNAr;               /* N_Ar */
    tNetTime xNBs;               /* N_Bs */
    tNetTime xNBr;               /* N_Br */
    tNetTime xNCs;               /* N_Cs < 0.9 N_Cr */
    tNetTime xNCr;               /* N_Cr */
    uint32 txBlockingMaxTimeMs;  /* TX Max blocking time(ms). > 0 mean timeout for TX. equal 0 is not waiting. */
    tNetTxMsg pfNetTxMsg;        /* Net TX message with non blocking */
    tNetRx pfNetRx;              /* Net RX */
    tpfAbortTxMsg pfAbortTXMsg;  /* Abort TX message */
} tUdsCANNetLayerCfg;

typedef struct
{
	tUdsId msgID;                   /* Message ID */
	tUdsLen dataLen;                 /* Data length */
    tpfUDSTxMsgCallBack pfCallBack; /* Callback */
} tUDSAndTPExchangeMsgInfo;

typedef enum
{
    CANTP_TX_MSG_IDLE = 0, /* CAN TP TX message idle */
    CANTP_TX_MSG_SUCC,     /* CAN TP TX message successful */
    CANTP_TX_MSG_FAIL,     /* CAN TP TX message fail */
    CANTP_TX_MSG_WAITING   /* CAN TP waiting TX message */
} tCanTPTxMsgStatus;
typedef enum
{
    TX_MSG_SUCCESSFUL = 0u,
    TX_MSG_FAILD,
    TX_MSG_TIMEOUT
} tTxMsgStatus;
typedef enum
{
    CONTINUE_TO_SEND, /* Continue to send */
    WAIT_FC,          /* Wait flow control */
    OVERFLOW_BUF      /* Overflow buffer */
} tFlowStatus;

/* CAN TP IDLE */
static tN_Result CANTP_DoCanTpIdle(tCanTpMsg *m_stMsgInfo, tCanTpWorkStatus *m_peNextStatus);
/* Do receive single frame */
static tN_Result CANTP_DoReceiveSF(tCanTpMsg *m_stMsgInfo, tCanTpWorkStatus *m_peNextStatus);
/* Do receive first frame */
static tN_Result CANTP_DoReceiveFF(tCanTpMsg *m_stMsgInfo, tCanTpWorkStatus *m_peNextStatus);
/* Do receive consecutive frame */
static tN_Result CANTP_DoReceiveCF(tCanTpMsg *m_stMsgInfo, tCanTpWorkStatus *m_peNextStatus);
/* Transmit flow control frame */
static tN_Result CANTP_DoTransmitFC(tCanTpMsg *m_stMsgInfo, tCanTpWorkStatus *m_peNextStatus);
/* Transmit single frame */
static tN_Result CANTP_DoTransmitSF(tCanTpMsg *m_stMsgInfo, tCanTpWorkStatus *m_peNextStatus);
/* Transmit first frame */
static tN_Result CANTP_DoTransmitFF(tCanTpMsg *m_stMsgInfo, tCanTpWorkStatus *m_peNextStatus);
/* Wait flow control frame */
static tN_Result CANTP_DoReceiveFC(tCanTpMsg *m_stMsgInfo, tCanTpWorkStatus *m_peNextStatus);
/* Transmit consecutive frame */
static tN_Result CANTP_DoTransmitCF(tCanTpMsg *m_stMsgInfo, tCanTpWorkStatus *m_peNextStatus);
/* Waiting TX message */
static tN_Result CANTP_DoWaitingTxMsg(tCanTpMsg *m_stMsgInfo, tCanTpWorkStatus *m_peNextStatus);

static uint8 CANTP_TxMsg(const tUdsId i_xTxId,const tUdsLen i_DataLen,const uint8 *i_pDataBuf,
                         const tpfNetTxCallBack i_pfNetTxCallBack,const uint32 txBlockingMaxtime);
static uint8 CANTP_RxMsg(tUdsId *o_pxRxId,uint8 *o_pRxDataLen,uint8 *o_pRxBuf);
static void CANTP_AbortTxMsg(void);

uint8 TP_ReadAFrameDataFromTP(tUdsId *o_pRxMsgID,tUdsLen *o_pxRxDataLen,uint8 *o_pDataBuf);
uint8 TP_DriverReadDataFromTP(const tUdsLen i_readDataLen, uint8 *o_pReadDatabuf, tUdsId *o_pTxMsgID, tUdsLen *o_pTxMsgLength);
uint8 TP_WriteAFrameDataInTP(const tUdsId i_TxMsgID,const tpfUDSTxMsgCallBack i_pfUDSTxMsgCallBack,const tUdsLen i_xTxDataLen,const uint8 *i_pDataBuf);
void TP_SystemTickCtl(void);
tUdsId TP_GetConfigRxMsgPHYID(void);
tUdsId TP_GetConfigRxMsgFUNID(void);
tUdsId TP_GetConfigTxMsgID(void);
void CANTP_DoTxMsgSuccessfulCallBack(void);
uint8 CANTP_DriverReadDataFromCANTP(const tUdsLen i_readDataLen, uint8 *o_pReadDataBuf, tTPTxMsgHeader *o_pstTxMsgHeader);
void CANTP_DriverWriteDataInCANTP(const tUdsId i_RxID, const tUdsLen i_dataLen, const uint8 *i_pDataBuf);
void CANTP_Init(tUdsId xRxFunId,tUdsId xRxPhyId,tUdsId xTxId);
void CANTP_MainFun(void);
void* fsl_memset (void *s, int c, unsigned int count);
void* fsl_memcpy (void *dst, const void *src, unsigned int count);

#endif /* TP_CAN_TP_H_ */
