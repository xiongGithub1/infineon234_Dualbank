/*
 * can_tp.c
 *
 *  Created on: 2021年11月26日
 *      Author: Administrator
 */
#include "uds_tp.h"
#include "custom_delay.h"

static tCanTpWorkStatus gs_eCanTpWorkStatus = IDLE;
static tNetTime gs_xCanTPTxSTmin = 0u;       /* TX STmin */
static tCanTpInfo gs_stCanTPTxDataInfo;      /* CAN TP TX data */
static tCanTpInfo gs_stCanTPRxDataInfo;      /* CAN TP RX data */
static uint32 gs_CANTPTxMsgMaxWaitTime = 0u; /* TX message max wait time, RX / TX frame both used waiting status */
static tpfUDSTxMsgCallBack gs_pfUDSTxMsgCallBack = nullptr; /* TX message callback */
static tpfAbortTxMsg gs_pfCANTPAbortTxMsg = nullptr;
static tpfNetTxCallBack gs_pfTxMsgSuccessfulCallBack = nullptr;
static tCanTPTxMsgStatus gs_eCANTPTxMsStatus = CANTP_TX_MSG_IDLE;
static tpfNetTxCallBack gs_pfCANTPTxMsgCallBack = nullptr;


/* Get cur CAN TP status */
#define GetCurCANTPStatus() (gs_eCanTpWorkStatus)
/* Get cur CAN TP status PTR */
#define GetCurCANTPStatusPtr() (&gs_eCanTpWorkStatus)
/* Set cur CAN TP status */
#define SetCurCANTPSatus(status)\
    do{\
        gs_eCanTpWorkStatus = status;\
    }while(0u)
/* Clear CAN TP RX msg buffer */
#define ClearCanTpRxMsgBuf(pMsgInfo)\
    do{\
        (pMsgInfo)->isFree = TRUE;\
        (pMsgInfo)->msgLen = 0u;\
        (pMsgInfo)->xMsgId = 0u;\
    }while(0u)

#define IsSF(xNetWorkFrameType) ((((xNetWorkFrameType) >> 4u) == SF) ? TRUE : FALSE)
#define IsFF(xNetWorkFrameType) ((((xNetWorkFrameType) >> 4u) == FF) ? TRUE : FALSE)
#define IsCF(xNetWorkFrameType) ((((xNetWorkFrameType) >> 4u) == CF) ? TRUE : FALSE)
#define IsFC(xNetWorkFrameType) ((((xNetWorkFrameType)>> 4u) == FC) ? TRUE : FALSE)
#define IsRxSNValid(xSN) ((gs_stCanTPRxDataInfo.ucSN == ((xSN) & 0x0Fu)) ? TRUE : FALSE)
/* Is received consecutive frame all. */
#define IsReceiveCFAll(xCFDataLen) (((gs_stCanTPRxDataInfo.stCanTpDataInfo.xPduDataLen + (uint8)(xCFDataLen))\
                                    >= gs_stCanTPRxDataInfo.stCanTpDataInfo.xFFDataLen) ? TRUE : FALSE)
/* Is transmitted data len overflow max SF? */
#define IsTxDataLenOverflowSF() ((gs_stCanTPTxDataInfo.stCanTpDataInfo.xFFDataLen > TX_SF_DATA_MAX_LEN) ? TRUE : FALSE)
/* Is transmitted data less than min? */
#define IsTxDataLenLessSF() ((0u == gs_stCanTPTxDataInfo.stCanTpDataInfo.xFFDataLen) ? TRUE : FALSE)
/* Get config CAN TP TX Response Address ID */
#define CANTP_GetConfigTxMsgID() (g_stCANUdsNetLayerCfgInfo.xTxId)
/* Get config CAN TP RX Function Address ID */
#define CANTP_GetConfigRxMsgFUNID() (g_stCANUdsNetLayerCfgInfo.xRxFunId)
/* Get config CAN TP RX Physical Address ID */
#define CANTP_GetConfigRxMsgPHYID() (g_stCANUdsNetLayerCfgInfo.xRxPhyId)

/* Check received message length valid or not? */
#define IsRxMsgLenValid(address_type, frameLen, RXCANMsgLen) ((address_type == NORMAL_ADDRESSING) ? (frameLen <= RXCANMsgLen - 1) : (frameLen <= RXCANMsgLen - 2))

#define CanTpTimeToCount(xTime) ((xTime) / g_stCANUdsNetLayerCfgInfo.ucCalledPeriod)
/* Set TX wait frame time */
#define SetTxWaitFrameTime(xWaitTime)\
    do{\
        (gs_stCanTPTxDataInfo.xMaxWatiTimeout = CanTpTimeToCount(xWaitTime));\
        gs_CANTPTxMsgMaxWaitTime = gs_stCanTPTxDataInfo.xMaxWatiTimeout;\
    }while(0u);

/* Set transmitted SF data len */
#define SetTxSFDataLen(pucSFDataLenBuf, xTxSFDataLen)\
    do{\
        *(pucSFDataLenBuf) &= 0xF0u;\
        (*(pucSFDataLenBuf) |= (xTxSFDataLen));\
    }while(0u)

/* Set transmitted FF data len */
#define SetTxFFDataLen(pucTxFFDataLenBuf, xTxFFDataLen)\
    do{\
        *(pucTxFFDataLenBuf + 0u) &= 0xF0u;\
        *(pucTxFFDataLenBuf + 0u) |= (uint8)((xTxFFDataLen) >> 8u);\
        *(pucTxFFDataLenBuf + 1u) |= (uint8)(xTxFFDataLen);\
    }while(0u)
/* Add TX SN */
#define AddTxSN()\
    do{\
        gs_stCanTPTxDataInfo.ucSN++;\
        if(gs_stCanTPTxDataInfo.ucSN > 0x0Fu)\
        {\
            gs_stCanTPTxDataInfo.ucSN = 0u;\
        }\
    }while(0u)
#define AddWaitSN()\
    do{\
        gs_stCanTPRxDataInfo.ucSN++;\
        if(gs_stCanTPRxDataInfo.ucSN > 0x0Fu)\
        {\
            gs_stCanTPRxDataInfo.ucSN = 0u;\
        }\
    }while(0u)
/* Set STmin */
#define SetSTmin(pucSTminBuf, xSTmin) (*(pucSTminBuf) = (uint8)(xSTmin))
/* Save TX STmin */
#define SaveTxSTmin(xTxSTmin) (gs_xCanTPTxSTmin = xTxSTmin)
/* Set wait frame time */
#define SetRxWaitFrameTime(xWaitTimeout)\
    do{\
        (gs_stCanTPRxDataInfo.xMaxWatiTimeout = CanTpTimeToCount(xWaitTimeout));\
        gs_CANTPTxMsgMaxWaitTime = gs_stCanTPRxDataInfo.xMaxWatiTimeout;\
    }while(0u);
/* Add TX data len */
#define AddTxDataLen(xTxDataLen) (gs_stCanTPTxDataInfo.stCanTpDataInfo.xPduDataLen += (xTxDataLen))
/* Add received data len */
#define AddRxDataLen(xRxDataLen) (gs_stCanTPRxDataInfo.stCanTpDataInfo.xPduDataLen += (xRxDataLen))
/* TX frame set TX message wait time */
#define TXFrame_SetTxMsgWaitTime(xWaitTime) SetTxWaitFrameTime(xWaitTime)
/* TX frame set TX message wait time */
#define TXFrame_SetRxMsgWaitTime(xWaitTime) SetTxWaitFrameTime(xWaitTime)
/* Save received message ID */
#define SaveRxMsgId(xMsgId) (gs_stCanTPRxDataInfo.stCanTpDataInfo.xCanTpId = (xMsgId))
/* Save FF data len */
#define SaveFFDataLen(i_xRxFFDataLen) (gs_stCanTPRxDataInfo.stCanTpDataInfo.xFFDataLen = i_xRxFFDataLen)
/* RX frame set TX msg wait time */
#define RXFrame_SetTxMsgWaitTime(xWaitTimeout) SetRxWaitFrameTime(xWaitTimeout)
/* RX frame set RX msg wait time */
#define RXFrame_SetRxMsgWaitTime(xWaitTimeout) SetRxWaitFrameTime(xWaitTimeout)
/* Is wait consecutive frame timeout? */
#define IsWaitCFTimeout() ((0u == gs_stCanTPRxDataInfo.xMaxWatiTimeout) ? TRUE : FALSE)
/* Is block size overflow */
#define IsRxBlockSizeOverflow() (((0u != g_stCANUdsNetLayerCfgInfo.xBlockSize) &&\
                                  (gs_stCanTPRxDataInfo.ucBlockSize >= g_stCANUdsNetLayerCfgInfo.xBlockSize))\
                                 ? TRUE : FALSE)
/* Is wait Flow control timeout? */
#define IsWaitFCTimeout()  ((0u == gs_stCanTPRxDataInfo.xMaxWatiTimeout) ? TRUE : FALSE)

/* Is TX STmin timeout? */
#define IsTxSTminTimeout() ((0u == gs_stCanTPTxDataInfo.xSTmin) ? TRUE : FALSE)
/* Is TX message wait frame timeout? */
#define IsTxMsgWaitingFrameTimeout() ((0u == gs_CANTPTxMsgMaxWaitTime) ? TRUE : FALSE)
/* Is TX all */
#define IsTxAll() ((gs_stCanTPTxDataInfo.stCanTpDataInfo.xPduDataLen >= \
                    gs_stCanTPTxDataInfo.stCanTpDataInfo.xFFDataLen) ? TRUE : FALSE)

/* Is TX wait frame timeout? */
#define IsTxWaitFrameTimeout() ((0u == gs_stCanTPTxDataInfo.xMaxWatiTimeout) ? TRUE : FALSE)
/* Set FS */
#define SetFS(pucFsBuf, xFlowStatus) (*(pucFsBuf) = (*(pucFsBuf) & 0xF0u) | (uint8)(xFlowStatus))
/* Get FS */
#define GetFS(ucFlowStaus, pxFlowStatusBuf) (*(pxFlowStatusBuf) = (ucFlowStaus) & 0x0Fu)
/* Set TX SN */
#define SetTxSN(pucSNBuf) (*(pucSNBuf) = gs_stCanTPTxDataInfo.ucSN | (*(pucSNBuf) & 0xF0u))
/* Set TX STmin */
#define SetTxSTmin() (gs_stCanTPTxDataInfo.xSTmin = CanTpTimeToCount(gs_xCanTPTxSTmin))
/* Set BS */
#define SetBlockSize(pucBSBuf, xBlockSize) (*(pucBSBuf) = (uint8)(xBlockSize))

/* Add block size */
#define AddBlockSize()\
    do{\
        if(0u != g_stCANUdsNetLayerCfgInfo.xBlockSize)\
        {\
            gs_stCanTPRxDataInfo.ucBlockSize++;\
        }\
    }while(0u)

static tCanTpFunInfo gs_astCanTpFunInfo[] =
{
    {IDLE, CANTP_DoCanTpIdle},					// 空闲
    {RX_SF, CANTP_DoReceiveSF},					// 单帧
    {RX_FF, CANTP_DoReceiveFF},					// 首帧
    {TX_FC, CANTP_DoTransmitFC},				// 流控帧
    {RX_CF, CANTP_DoReceiveCF},					// 连续帧

    {TX_SF, CANTP_DoTransmitSF},
    {TX_FF, CANTP_DoTransmitFF},
    {RX_FC, CANTP_DoReceiveFC},
    {TX_CF, CANTP_DoTransmitCF},
    {WAITING_TX, CANTP_DoWaitingTxMsg}			// 等待发送消息
};

/* UDS Network layer config info */
static tUdsCANNetLayerCfg g_stCANUdsNetLayerCfgInfo =
{
    1u,                 /* Called CAN TP main function period */
	0u,     			/* RX FUN ID */
	0u,     			/* RX PHY ID */
	0u,    				/* TX RESP ID */
    0u,                 /* BS = block size */
    1u,                 /* STmin */
    25u,                /* N_As */
    25u,                /* N_Ar */
    75u,                /* N_Bs */
    0u,                 /* N_Br */
    25u,               /* N_Cs < 0.9 N_Cr */
    150u,               /* N_Cr */
    50u,                /* TX Max blocking time(ms). > 0 mean timeout for TX. equal 0 is not waiting. */
    CANTP_TxMsg,        /* CAN TP TX */
    CANTP_RxMsg,        /* CAN TP RX */
    CANTP_AbortTxMsg,   /* Abort TX message */
};

/* 判断ID是否是UDS的ID */
uint8 CANTP_IsReceivedMsgIDValid(const uint16 i_receiveMsgID)
{
    uint8 result = FALSE;

    if ((i_receiveMsgID == CANTP_GetConfigRxMsgFUNID())|| (i_receiveMsgID == CANTP_GetConfigRxMsgPHYID()))
    {
        result = TRUE;
    }

    return result;
}

/* Do register TX message callback */
static void CANTP_DoRegisterTxMsgCallBack(void)
{
    tCanTPTxMsgStatus CANTPTxMsgStatus = CANTP_TX_MSG_IDLE;
    CANTPTxMsgStatus = gs_eCANTPTxMsStatus;
    if (CANTP_TX_MSG_SUCC == CANTPTxMsgStatus)
    {
        if (nullptr != gs_pfCANTPTxMsgCallBack)
        {
            (gs_pfCANTPTxMsgCallBack)();
            gs_pfCANTPTxMsgCallBack = nullptr;
        }
    }
    else if (CANTP_TX_MSG_FAIL == CANTPTxMsgStatus)
    {
        //TPDebugPrintf("\n TX msg failed callback=%X, status=%d\n", gs_pfCANTPTxMsgCallBack, gs_eCANTPTxMsStatus);
        gs_eCANTPTxMsStatus = CANTP_TX_MSG_IDLE;
        /* If TX message failed, clear TX message callback */
        gs_pfCANTPTxMsgCallBack = nullptr;
    }
    else
    {
        /* do nothing */
    }
}
/* Register TX message successful callback */
static void CANTP_RegisterTxMsgCallBack(const tpfNetTxCallBack i_pfNetTxCallBack)
{
    gs_pfCANTPTxMsgCallBack = i_pfNetTxCallBack;
}

/* Register transmit a frame message callback */
void TP_RegisterTransmittedAFrmaeMsgCallBack(const tpfUDSTxMsgCallBack i_pfTxMsgCallBack)
{
    gs_pfUDSTxMsgCallBack = (tpfUDSTxMsgCallBack)i_pfTxMsgCallBack;
}


/* Read a frame from TP RX FIFO. If no data can read return FALSE, else return TRUE */
uint8 TP_ReadAFrameDataFromTP(tUdsId *o_pRxMsgID,tUdsLen *o_pxRxDataLen,uint8 *o_pDataBuf)//读取从上位机发下来的缓存数据
{
    tErroCode eStatus;
    tLen xReadDataLen = 0u;
    tUDSAndTPExchangeMsgInfo exchangeMsgInfo;

    /* CAN read data from buffer */
    GetCanReadLen(RX_TP_QUEUE_ID, &xReadDataLen, &eStatus);

    if (ERRO_NONE != eStatus || (xReadDataLen < sizeof(tUDSAndTPExchangeMsgInfo)))
    {
        return FALSE;
    }

    /* Read receive ID and data len */
    ReadDataFromFifo(RX_TP_QUEUE_ID,
                     sizeof(exchangeMsgInfo),
                     (uint8 *)&exchangeMsgInfo,
                     &xReadDataLen,
                     &eStatus);

    if (ERRO_NONE != eStatus || sizeof(exchangeMsgInfo) != xReadDataLen)
    {
        //TPDebugPrintf("Read data len error!\n");
        return FALSE;
    }

    /* Read data from FIFO */
    ReadDataFromFifo(RX_TP_QUEUE_ID,
                     (tLen)exchangeMsgInfo.dataLen,
                     o_pDataBuf,
                     &xReadDataLen,
                     &eStatus);

    if (ERRO_NONE != eStatus || (exchangeMsgInfo.dataLen != xReadDataLen))
    {
        //TPDebugPrintf("Read data error!\n");
        return FALSE;
    }

    *o_pRxMsgID = exchangeMsgInfo.msgID;
    *o_pxRxDataLen = exchangeMsgInfo.dataLen;
    return TRUE;
}

/* Write a frame data to TP TX FIFO */
uint8 TP_WriteAFrameDataInTP(const tUdsId i_TxMsgID,const tpfUDSTxMsgCallBack i_pfUDSTxMsgCallBack,const tUdsLen i_xTxDataLen,const uint8 *i_pDataBuf)
{
    tErroCode eStatus;
    tLen xCanWriteLen = 0u;
    tLen xWritDataLen = i_xTxDataLen;
    tUDSAndTPExchangeMsgInfo exchangeMsgInfo;
    uint32 totalWriteDataLen = i_xTxDataLen + sizeof(tUDSAndTPExchangeMsgInfo);
    exchangeMsgInfo.msgID = i_TxMsgID;
    exchangeMsgInfo.dataLen = i_xTxDataLen;
    exchangeMsgInfo.pfCallBack = (tpfUDSTxMsgCallBack)i_pfUDSTxMsgCallBack;
    //ASSERT(nullptr == i_pDataBuf);

    /* Check transmit ID */
    if (i_TxMsgID != TP_GetConfigTxMsgID())
    {
        return FALSE;
    }

    if (0u == xWritDataLen)
    {
        return FALSE;
    }

    /* Check can write data len */
    GetCanWriteLen(TX_TP_QUEUE_ID, &xCanWriteLen, &eStatus);

    if (ERRO_NONE != eStatus || xCanWriteLen < totalWriteDataLen)
    {
        return FALSE;
    }

    /* Write UDS transmit ID */
    WriteDataInFifo(TX_TP_QUEUE_ID, (uint8 *)&exchangeMsgInfo, sizeof(tUDSAndTPExchangeMsgInfo), &eStatus);

    if (ERRO_NONE != eStatus)
    {
        return FALSE;
    }

    /* Write data in FIFO */
    WriteDataInFifo(TX_TP_QUEUE_ID, (uint8 *)i_pDataBuf, xWritDataLen, &eStatus);

    if (ERRO_NONE != eStatus)
    {
        return FALSE;
    }

    return TRUE;
}


/* Driver read data from TP for TX message to BUS */
uint8 TP_DriverReadDataFromTP(const tUdsLen i_readDataLen, uint8 *o_pReadDatabuf, tUdsId *o_pTxMsgID, tUdsLen *o_pTxMsgLength)
{
    uint8 result = FALSE;
    tTPTxMsgHeader TPTxMsgHeader;
    //ASSERT(0u == i_readDataLen);
    //ASSERT(nullptr == o_pReadDatabuf);
    //ASSERT(nullptr == o_pTxMsgID);
    //ASSERT(nullptr == o_pTxMsgLength);
#ifdef EN_CAN_TP
    result = CANTP_DriverReadDataFromCANTP(i_readDataLen, o_pReadDatabuf, &TPTxMsgHeader);
#endif

    if (TRUE == result)
    {
        *o_pTxMsgID = TPTxMsgHeader.TxMsgID;
        *o_pTxMsgLength = TPTxMsgHeader.TxMsgLength;
    }

    return result;
}
/* CANP TP set TX message status */
static void CANTP_SetTxMsgStatus(const tCanTPTxMsgStatus i_eTxMsgStatus)
{
    gs_eCANTPTxMsStatus = i_eTxMsgStatus;
}

/* Set transmit frame type */
static uint8 CANTP_SetFrameType(const tNetWorkFrameType i_eFrameType,uint8 *o_pucFrameType)
{
    //ASSERT(nullptr == o_pucFrameType);

    if (SF == i_eFrameType ||FF == i_eFrameType ||FC == i_eFrameType ||CF == i_eFrameType)
    {
        *o_pucFrameType &= 0x0Fu;
        *o_pucFrameType |= ((uint8)i_eFrameType << 4u);
        return TRUE;
    }

    return FALSE;
}
/* UDS transmitted a application frame data, copy these data in TX FIFO. */
static uint8 CANTP_CopyAFrameFromFifoToBuf(tUdsId *o_pxTxCanID,uint8 *o_pTxDataLen,uint8 *o_pDataBuf)
{
    tErroCode eStatus;
    tLen xRealReadLen = 0u;
    tUDSAndTPExchangeMsgInfo exchangeMsgInfo;
    //ASSERT(nullptr == o_pxTxCanID);
    //ASSERT(nullptr == o_pTxDataLen);
    //ASSERT(nullptr == o_pDataBuf);
    /* Can read data from buffer */
    GetCanReadLen(TX_TP_QUEUE_ID, &xRealReadLen, &eStatus);

    if ((ERRO_NONE != eStatus) || (0u == xRealReadLen) || (xRealReadLen < sizeof(tUDSAndTPExchangeMsgInfo)))
    {
        return FALSE;
    }

    /* Read receive ID */
    ReadDataFromFifo(TX_TP_QUEUE_ID,sizeof(tUDSAndTPExchangeMsgInfo),(uint8 *)&exchangeMsgInfo,&xRealReadLen,&eStatus);

    if (ERRO_NONE != eStatus || sizeof(tUDSAndTPExchangeMsgInfo) != xRealReadLen)
    {
        return FALSE;
    }

    /* Read data from FIFO */
    ReadDataFromFifo(TX_TP_QUEUE_ID,(tLen)exchangeMsgInfo.dataLen,o_pDataBuf,&xRealReadLen,&eStatus);

    if (ERRO_NONE != eStatus || exchangeMsgInfo.dataLen != xRealReadLen)
    {
        return FALSE;
    }

    *o_pxTxCanID = exchangeMsgInfo.msgID;
    *o_pTxDataLen = (uint8)exchangeMsgInfo.dataLen;
    TP_RegisterTransmittedAFrmaeMsgCallBack(exchangeMsgInfo.pfCallBack);
    return TRUE;
}
/* Received a CAN TP frame, copy these data in UDS RX FIFO. */
static uint8 CANTP_CopyAFrameDataInRxFifo(const tUdsId i_xRxCanID,
                                          const tLen i_xRxDataLen,
                                          const uint8 *i_pDataBuf)
{
    tErroCode eStatus;
    tLen xCanWriteLen = 0u;
    tUDSAndTPExchangeMsgInfo exchangeMsgInfo;
    //ASSERT(nullptr == i_pDataBuf);

    if (0u == i_xRxDataLen)
    {
        return FALSE;
    }

    /* Check can write data len */
    GetCanWriteLen(RX_TP_QUEUE_ID, &xCanWriteLen, &eStatus);

    if ((ERRO_NONE != eStatus) || (xCanWriteLen < (i_xRxDataLen + sizeof(tUDSAndTPExchangeMsgInfo))))
    {
        return FALSE;
    }

    exchangeMsgInfo.msgID = i_xRxCanID;
    exchangeMsgInfo.dataLen = i_xRxDataLen;
    exchangeMsgInfo.pfCallBack = nullptr;
    /* Write data UDS transmit ID and data len */
    WriteDataInFifo(RX_TP_QUEUE_ID, (uint8 *)&exchangeMsgInfo, sizeof(tUDSAndTPExchangeMsgInfo), &eStatus);

    if (ERRO_NONE != eStatus)
    {
        return FALSE;
    }

    /* Write data in FIFO */
    WriteDataInFifo(RX_TP_QUEUE_ID, (uint8 *)i_pDataBuf, i_xRxDataLen, &eStatus);

    if (ERRO_NONE != eStatus)
    {
        return FALSE;
    }

    return TRUE;
}
/* Get RX SF frame message length */
static uint8 GetRXSFFrameMsgLength(const uint32 i_RxMsgLen, const uint8 *i_pMsgBuf, uint16 *o_pFrameLen)
{
    uint8 result = FALSE;
    uint16 frameLen = 0u;
    //ASSERT(nullptr == i_pMsgBuf);
    //ASSERT(nullptr == o_pFrameLen);

    if ((i_RxMsgLen <= 1u) || (TRUE != IsSF(i_pMsgBuf[0u])))
    {
        return FALSE;
    }

    /* Check received single message length based on ISO15765-2 2016 */
    if (i_RxMsgLen <= 8u)   //长度小于等于8
    {
        frameLen = i_pMsgBuf[0u] & 0x0Fu;//取低4位

        if ((frameLen <= SF_CAN_DATA_MAX_LEN) && (frameLen > 0u))
        {
            result = IsRxMsgLenValid(NORMAL_ADDRESSING, frameLen, i_RxMsgLen);
        }
    }
    else
    {
        frameLen = i_pMsgBuf[0u] & 0x0Fu;

        if (0u == frameLen)
        {
            frameLen = i_pMsgBuf[1u];

            if ((frameLen <= SF_CANFD_DATA_MAX_LEN) && (frameLen > 0u))
            {
                result = IsRxMsgLenValid(NORMAL_ADDRESSING, frameLen, i_RxMsgLen);
            }
        }
    }

    if (TRUE == result)
    {
        *o_pFrameLen = frameLen;
    }

    return result;
}
/* Get RX FF frame message length */
static uint8 GetRXFFFrameMsgLength(const uint32 i_RxMsgLen, const uint8 *i_pMsgBuf, uint32 *o_pFrameLen)
{
    uint8 result = FALSE;
    uint32 frameLen = 0u;
    uint8 index = 0u;
    //ASSERT(nullptr == i_pMsgBuf);
    //ASSERT(nullptr == o_pFrameLen);

    if ((i_RxMsgLen < 8u) || (TRUE != IsFF(i_pMsgBuf[0u])))
    {
        return FALSE;
    }

    /* Check received single message length based on ISO15765-2 2016 */
    /* Calculate FF message length */
    //首帧指示数据的长度占12位
    frameLen = (uint32)((i_pMsgBuf[0u] & 0x0Fu) << 8u) | i_pMsgBuf[1u];

    if (0u == frameLen)
    {
        /* FF message length is over 4095 Bytes */
        for (index = 0u; index < 4; index++)
        {
            frameLen <<= 8u;
            frameLen |= i_pMsgBuf[index + 2u];
        }
    }

    if (frameLen < FF_DATA_MIN_LEN)
    {
        result = FALSE;
    }
    else
    {
        result = TRUE;
    }

    if (TRUE == result)
    {
        *o_pFrameLen = frameLen;
    }

    return result;
}
/* CAN TP TX message callback */
static void CANTP_TxMsgSuccessfulCallBack(void)
{
    gs_eCANTPTxMsStatus = CANTP_TX_MSG_SUCC;
}
/* Do transmitted a frame message callback */
void TP_DoTransmittedAFrameMsgCallBack(const uint8 i_result)
{
    if (nullptr != gs_pfUDSTxMsgCallBack)
    {
        (gs_pfUDSTxMsgCallBack)(i_result);
        gs_pfUDSTxMsgCallBack = nullptr;
    }
}
/* Do TX message successful callback */
void CANTP_DoTxMsgSuccessfulCallBack(void)
{
    if (nullptr != gs_pfTxMsgSuccessfulCallBack)
    {
        (gs_pfTxMsgSuccessfulCallBack)();
        gs_pfTxMsgSuccessfulCallBack = nullptr;
    }
}

/* CAN TP IDLE */
static tN_Result CANTP_DoCanTpIdle(tCanTpMsg *m_stMsgInfo, tCanTpWorkStatus *m_peNextStatus)
{
    uint8 txDataLen = (uint8)gs_stCanTPTxDataInfo.stCanTpDataInfo.xFFDataLen;
    //ASSERT(nullptr == m_peNextStatus);
    /* Clear CAN TP data */
    tl_memset((void *)&gs_stCanTPRxDataInfo, 0u, sizeof(tCanTpInfo));// sizeof(tCanTpInfo) = 164字节
    tl_memset((void *)&gs_stCanTPTxDataInfo, 0u, sizeof(tCanTpInfo));// 把gs_stCanTPTxDataInfo所有字节初始化为0
    /* Clear waiting time */
    gs_CANTPTxMsgMaxWaitTime = 0u;
    /* Set NULL to transmitted message callback */
    TP_RegisterTransmittedAFrmaeMsgCallBack(nullptr);

    /* If receive can TP message, judge type. Only received SF or FF message. Other frames ignore. */
    if (FALSE == m_stMsgInfo->isFree)//接收到消息
    {
        if (TRUE == IsSF(m_stMsgInfo->aMsgBuf[0u]))//是否单帧
        {
            *m_peNextStatus = RX_SF;//下一次循环时状态为接收单帧
        }
        else if (TRUE == IsFF(m_stMsgInfo->aMsgBuf[0u]))//是否首帧
        {
            *m_peNextStatus = RX_FF;//下一次循环时状态为接收首帧
        }
        else
        {
            //TPDebugPrintf("\n %s received invalid message!\n", __func__);
        }
    }
    else    //没有接收到消息
    {
        /* Judge have message can will TX. */   //有消息要发送,将数据拷贝到gs_stCanTPTxDataInfo中来
        if (TRUE == CANTP_CopyAFrameFromFifoToBuf(&gs_stCanTPTxDataInfo.stCanTpDataInfo.xCanTpId,&txDataLen,gs_stCanTPTxDataInfo.stCanTpDataInfo.aDataBuf))
        {
            gs_stCanTPTxDataInfo.stCanTpDataInfo.xFFDataLen = txDataLen;

            if (TRUE == IsTxDataLenOverflowSF())//判断长度是否大于单帧长度
            {
                *m_peNextStatus = TX_FF;//下一次循环时状态为发送首帧
            }
            else
            {
                *m_peNextStatus = TX_SF;//下一次循环时状态为发送单帧
            }
        }
    }

    return N_OK;
}
/* Do receive single frame 单帧*/
static tN_Result CANTP_DoReceiveSF(tCanTpMsg *m_stMsgInfo, tCanTpWorkStatus *m_peNextStatus)
{
    uint16 SFLen = 0u;
    //ASSERT(nullptr == m_peNextStatus);

    if ((0u == m_stMsgInfo->msgLen) || (TRUE == m_stMsgInfo->isFree))
    {
        return N_ERROR;
    }
    //判断是否是单帧
    if (TRUE != IsSF(m_stMsgInfo->aMsgBuf[0u]))
    {
        return N_ERROR;
    }

    /* Get RX frame: SF length */
    if (TRUE != GetRXSFFrameMsgLength(m_stMsgInfo->msgLen, m_stMsgInfo->aMsgBuf, &SFLen))
    {
        //TPDebugPrintf("SF:GetRXSFFrameMsgLength failed!\n");
        return N_ERROR;
    }

    /* Write data to UDS FIFO */
    //将数据写入网络层的接收FIFO中
    if (FALSE == CANTP_CopyAFrameDataInRxFifo(m_stMsgInfo->xMsgId,
                                              SFLen,
                                              &m_stMsgInfo->aMsgBuf[1u]))
    {
        //TPDebugPrintf("Copy data error!\n");
        return N_ERROR;
    }

    *m_peNextStatus = IDLE;//下一次状态为IDLE，表示单帧接收完成
    return N_OK;
}

/* Do receive first frame */
static tN_Result CANTP_DoReceiveFF(tCanTpMsg *m_stMsgInfo, tCanTpWorkStatus *m_peNextStatus)
{
    uint32 FFDataLen = 0u;
    //ASSERT(nullptr == m_peNextStatus);

    if ((0u == m_stMsgInfo->msgLen) || (TRUE == m_stMsgInfo->isFree))
    {
        return N_ERROR;
    }
    //判断是否首帧
    if (TRUE != IsFF(m_stMsgInfo->aMsgBuf[0u]))
    {
        //TPDebugPrintf("Received not FF\n");
        return N_ERROR;
    }

    /* Get FF Data len */
    if (TRUE != GetRXFFFrameMsgLength(m_stMsgInfo->msgLen, m_stMsgInfo->aMsgBuf, &FFDataLen))
    {
        //TPDebugPrintf("FF:GetRXFrameMsgLength failed!\n");
        return N_ERROR;
    }

    /* Save received msg ID */
    SaveRxMsgId(m_stMsgInfo->xMsgId);//保存接收的ID到全局变量gs_stCanTPRxDataInfo中
    /* Write data in global buffer. When receive all data, write these data in FIFO. */
    SaveFFDataLen((tLen)FFDataLen);//保存接收到的数据长度到全局变量gs_stCanTPRxDataInfo中
    /* Set wait flow control time */
    RXFrame_SetTxMsgWaitTime(g_stCANUdsNetLayerCfgInfo.xNBr);//设置流控时间参数,这里等于0
    /* Copy data in global buffer *///把数据保存到全局变量中
    fsl_memcpy(gs_stCanTPRxDataInfo.stCanTpDataInfo.aDataBuf, (const void *)&m_stMsgInfo->aMsgBuf[2u], m_stMsgInfo->msgLen - 2u);
    AddRxDataLen(m_stMsgInfo->msgLen - 2u);//设置数据长度 = 接收到的所有数据长度 - 2
    /* Jump to next status */
    *m_peNextStatus = TX_FC;//下一次状态为发送流控
    ClearCanTpRxMsgBuf(m_stMsgInfo);//清空缓冲
    return N_OK;
}

/* Do receive consecutive frame  连续帧*/
static tN_Result CANTP_DoReceiveCF(tCanTpMsg *m_stMsgInfo, tCanTpWorkStatus *m_peNextStatus)
{
	//ASSERT(nullptr == m_peNextStatus);

    /* Is timeout RX wait timeout? If wait timeout receive CF over. */
    if (TRUE == IsWaitCFTimeout())
    {
        //TPDebugPrintf("Wait consecutive frame timeout!\n");
        *m_peNextStatus = IDLE;//等待接收超时则回到IDLE状态
        return N_TIMEOUT_Cr;
    }

    if (0u == m_stMsgInfo->msgLen || TRUE == m_stMsgInfo->isFree)
    {
        /* Waiting CF message, It's normally for not received CAN message in the step. */
        return N_OK;
    }

    /* Check received message is SF or FF? If received SF or FF, start new receive progresses. */
    if ((TRUE == IsSF(m_stMsgInfo->aMsgBuf[0u])) || (TRUE == IsFF(m_stMsgInfo->aMsgBuf[0u])))
    {
        //本次接收到SF或者是FF，则重新开始，回到IDLE状态
        //TPDebugPrintf("In receive progresses: received SF\n");
        *m_peNextStatus = IDLE;
        return N_UNEXP_PDU;
    }

    if (gs_stCanTPRxDataInfo.stCanTpDataInfo.xCanTpId != m_stMsgInfo->xMsgId)
    {
        //本次收到的帧不是本次该收的数据
        return N_ERROR;
    }

    if (TRUE != IsCF(m_stMsgInfo->aMsgBuf[0u]))
    {
        //不是CF帧
        return N_ERROR;
    }

    /* Get received SN. If SN invalid, return FALSE. */
    if (TRUE != IsRxSNValid(m_stMsgInfo->aMsgBuf[0u]))
    {
        //本次收到CF帧的计数编号不对
        //TPDebugPrintf("Msg SN invalid in CF!\n");
        return N_WRONG_SN;
    }

    /* Check receive CF all? If receive all, copy data in FIFO and clear receive
    buffer information. Else count SN and add receive data len. */
    if (TRUE == IsReceiveCFAll(m_stMsgInfo->msgLen - 1u))
    {
        //全部接收完成
        /* Copy all data in FIFO and receive over. */
        //拷贝到全局变量中
        fsl_memcpy(&gs_stCanTPRxDataInfo.stCanTpDataInfo.aDataBuf[gs_stCanTPRxDataInfo.stCanTpDataInfo.xPduDataLen],
                   &m_stMsgInfo->aMsgBuf[1u],
                   gs_stCanTPRxDataInfo.stCanTpDataInfo.xFFDataLen - gs_stCanTPRxDataInfo.stCanTpDataInfo.xPduDataLen);
        /* Copy all data in FIFO *///写入网络层接收队列FIFO中
        (void)CANTP_CopyAFrameDataInRxFifo(gs_stCanTPRxDataInfo.stCanTpDataInfo.xCanTpId,
                                           gs_stCanTPRxDataInfo.stCanTpDataInfo.xFFDataLen,
                                           gs_stCanTPRxDataInfo.stCanTpDataInfo.aDataBuf);
        *m_peNextStatus = IDLE;//下一次状态为IDLE
    }
    else
    {
        //没有接收完
        /* If is block size overflow. */
        if (TRUE == IsRxBlockSizeOverflow())
        {
            //块大小溢出
            //设置NBr时间，距离发送流控帧的时间间隔
            RXFrame_SetTxMsgWaitTime(g_stCANUdsNetLayerCfgInfo.xNBr);
            //下次状态为发送FC
            *m_peNextStatus = TX_FC;
        }
        else
        {
            //块大小未溢出，表示要连续接收CF帧
            /* Count SN and set STmin, wait timeout time */
            AddWaitSN();//计数编号SN加1
            /* Set wait frame time */
            RXFrame_SetRxMsgWaitTime(g_stCANUdsNetLayerCfgInfo.xNCr);//设置NCr时间,下一个CF帧到来的时间间隔
        }

        /* Copy data in global FIFO */
        fsl_memcpy(&gs_stCanTPRxDataInfo.stCanTpDataInfo.aDataBuf[gs_stCanTPRxDataInfo.stCanTpDataInfo.xPduDataLen],
                   &m_stMsgInfo->aMsgBuf[1u],
                   m_stMsgInfo->msgLen - 1u);
        AddRxDataLen(m_stMsgInfo->msgLen - 1u);//增加本次接收到的数据长度
    }

    return N_OK;
}
/* Wait flow control frame */
static tN_Result CANTP_DoReceiveFC(tCanTpMsg *m_stMsgInfo, tCanTpWorkStatus *m_peNextStatus)
{
    tFlowStatus eFlowStatus;
    //ASSERT(nullptr == m_peNextStatus);

    /* If TX message wait FC timeout jump to IDLE. */
    if (TRUE == IsTxWaitFrameTimeout())
    {
        //TPDebugPrintf("Wait flow control timeout.\n");
        *m_peNextStatus = IDLE;
        return N_TIMEOUT_Cr;
    }

    if ((0u == m_stMsgInfo->msgLen) || (TRUE == m_stMsgInfo->isFree))
    {
        /* Waiting received FC. It's normally for waiting CAN message and return OK. */
        return N_OK;
    }

    if (TRUE != IsFC(m_stMsgInfo->aMsgBuf[0u]))
    {
        return N_ERROR;
    }

    /* Get flow status */
    GetFS(m_stMsgInfo->aMsgBuf[0u], &eFlowStatus);

    if (OVERFLOW_BUF == eFlowStatus)
    {
        *m_peNextStatus = IDLE;
        return N_BUFFER_OVFLW;
    }

    /* Wait flow control */
    if (WAIT_FC == eFlowStatus)
    {
        /* Set TX wait time */
        TXFrame_SetRxMsgWaitTime(g_stCANUdsNetLayerCfgInfo.xNBs);
        return N_OK;
    }

    /* Continue to send */
    if (CONTINUE_TO_SEND == eFlowStatus)
    {
        SetBlockSize(&gs_stCanTPTxDataInfo.ucBlockSize, m_stMsgInfo->aMsgBuf[1u]);
        SaveTxSTmin(m_stMsgInfo->aMsgBuf[2u]);
        TXFrame_SetTxMsgWaitTime(g_stCANUdsNetLayerCfgInfo.xNCs);
        /* Remove Add TX SN, because this SN is added in send First frame callback */
#if 0
        AddTxSN();
#endif
    }
    else
    {
        /* Received error Flow control */
        *m_peNextStatus = IDLE;
        return N_INVALID_FS;
    }

    *m_peNextStatus = TX_CF;
    return N_OK;
}
/* Transmit FC callback */
static void CANTP_DoTransmitFCCallBack(void)
{
    if (gs_stCanTPRxDataInfo.stCanTpDataInfo.xFFDataLen > MAX_CF_DATA_LEN)
    {
        SetCurCANTPSatus(IDLE);//设置网络层状态为IDLE
    }
    else
    {
        /* Set wait STmin */
        RXFrame_SetRxMsgWaitTime(g_stCANUdsNetLayerCfgInfo.xNCr);//设置下一个连续帧的到达时间NCr
        SetCurCANTPSatus(RX_CF);//设置网络层状态为接收CF
    }
}

/* Transmit flow control frame */
static tN_Result CANTP_DoTransmitFC(tCanTpMsg *m_stMsgInfo, tCanTpWorkStatus *m_peNextStatus)
{
    uint8 aucTransDataBuf[DATA_LEN] = {0u};

    /* Is wait FC timeout? */
    if (TRUE != IsWaitFCTimeout())
    {
        //TPDebugPrintf("\n Waiting transmit FC not timeout!\n");
        /* Waiting timeout for transmit FC */
        return N_OK;
    }

    /* Set frame type */
    (void)CANTP_SetFrameType(FC, &aucTransDataBuf[0u]);//将第一个字节的高4位设置为FC（0x03）

    /* Check current buffer. */
    //判断已接收的FF首帧数据长度是否已经大于最大CF连续帧可接收的长度
    if (gs_stCanTPRxDataInfo.stCanTpDataInfo.xFFDataLen > MAX_CF_DATA_LEN)
    {
        /* Set FS */
        SetFS(&aucTransDataBuf[1u], OVERFLOW_BUF);//设置FS位为缓冲区溢出
    }
    else
    {
        SetFS(&aucTransDataBuf[1u], CONTINUE_TO_SEND);//设置FS位为继续发送
    }

    /* Set BS */
    //设置块大小CF的大小
    SetBlockSize(&aucTransDataBuf[1u], g_stCANUdsNetLayerCfgInfo.xBlockSize);
    /* Add block size */
    AddBlockSize();//块计数加1
    /* Add wait SN */
    AddWaitSN();//编号SN加1
    /* Set STmin */
    SetSTmin(&aucTransDataBuf[2u], g_stCANUdsNetLayerCfgInfo.xSTmin);//设置STmin时间参数（CF帧PDU发送的最小间隔）
    /* Set wait next frame  max time */
    RXFrame_SetTxMsgWaitTime(g_stCANUdsNetLayerCfgInfo.xNAr);//设置接收下一帧的超时时间，可以用设置
    /* CAN TP set TX message status and register TX message successful callback. */
    CANTP_SetTxMsgStatus(CANTP_TX_MSG_WAITING);//设置发送状态为等待发送
    CANTP_RegisterTxMsgCallBack(CANTP_DoTransmitFCCallBack);//发送完成回调函数

    /* Transmit flow control */
    //发送FC帧
    if (TRUE == g_stCANUdsNetLayerCfgInfo.pfNetTxMsg(g_stCANUdsNetLayerCfgInfo.xTxId,
                                                     sizeof(aucTransDataBuf),
                                                     aucTransDataBuf,
                                                     CANTP_TxMsgSuccessfulCallBack,
                                                     g_stCANUdsNetLayerCfgInfo.txBlockingMaxTimeMs))
    {
        *m_peNextStatus = WAITING_TX;//下一个状态为等待发送
        return N_OK;
    }

    /* CAN TP set TX message status and register TX message successful callback. */
    CANTP_SetTxMsgStatus(CANTP_TX_MSG_FAIL);
    CANTP_RegisterTxMsgCallBack(nullptr);
    /* Transmit message failed and do idle */
    *m_peNextStatus = IDLE;
    return N_ERROR;
}

/* Transmit SF callback */
static void CANTP_DoTransmitSFCallBack(void)
{
    TP_DoTransmittedAFrameMsgCallBack(TX_MSG_SUCCESSFUL);
    SetCurCANTPSatus(IDLE);
    //gs_eCanTpWorkStatus = IDLE;
}

/* Transmit single frame */
static tN_Result CANTP_DoTransmitSF(tCanTpMsg *m_stMsgInfo, tCanTpWorkStatus *m_peNextStatus)
{
    uint8 aDataBuf[DATA_LEN] = {0u};
    uint8 txLen = 0u;
    //ASSERT(nullptr == m_peNextStatus);

    /* Check transmit data len. If data len overflow Max SF, return FALSE. */
    if (TRUE == IsTxDataLenOverflowSF())
    {
        //发送SF的长度大于7
        *m_peNextStatus = TX_FF;
        return N_ERROR;
    }

    if (TRUE == IsTxDataLenLessSF())
    {
        //发送的SF的长度等于0
        *m_peNextStatus = IDLE;
        return N_ERROR;
    }

    /* Set transmitted frame type */
    (void)CANTP_SetFrameType(SF, &aDataBuf[0u]);
    /* Set transmitted data len */
    SetTxSFDataLen(&aDataBuf[0u], gs_stCanTPTxDataInfo.stCanTpDataInfo.xFFDataLen);
    txLen = aDataBuf[0u] + 1u;
    /* Copy data in TX buffer */
    fsl_memcpy(&aDataBuf[1u],
               gs_stCanTPTxDataInfo.stCanTpDataInfo.aDataBuf,
               gs_stCanTPTxDataInfo.stCanTpDataInfo.xFFDataLen);
    /* CAN TP set TX message status and register TX message successful callback. */
    CANTP_SetTxMsgStatus(CANTP_TX_MSG_WAITING);
    CANTP_RegisterTxMsgCallBack(CANTP_DoTransmitSFCallBack);

    /* Request transmitted application message. */
    if (TRUE != g_stCANUdsNetLayerCfgInfo.pfNetTxMsg(gs_stCanTPTxDataInfo.stCanTpDataInfo.xCanTpId,
                                                     txLen,
                                                     aDataBuf,
                                                     CANTP_TxMsgSuccessfulCallBack,
                                                     g_stCANUdsNetLayerCfgInfo.txBlockingMaxTimeMs))
    {
        /* CAN TP set TX message status and register TX message successful callback. */
        CANTP_SetTxMsgStatus(CANTP_TX_MSG_FAIL);
        CANTP_RegisterTxMsgCallBack(nullptr);
        /* Send message error */
        *m_peNextStatus = IDLE;
        /* Request transmitted application message failed. */
        return N_ERROR;
    }

    /* Set wait send frame successful max time */
    TXFrame_SetTxMsgWaitTime(g_stCANUdsNetLayerCfgInfo.xNAs);
    /* Jump to idle and clear transmitted message. */
    *m_peNextStatus = WAITING_TX;
    return N_OK;
}
/* Transmit FF callback */
static void CANTP_DoTransmitFFCallBack(void)
{
    /* Add TX data len */
    AddTxDataLen(FF_DATA_MIN_LEN - 2);
    /* Set TX wait time */
    TXFrame_SetRxMsgWaitTime(g_stCANUdsNetLayerCfgInfo.xNBs);
    /* Jump to idle and clear transmitted message. */
    AddTxSN();
    SetCurCANTPSatus(RX_FC);
}
/* Transmit first frame */
static tN_Result CANTP_DoTransmitFF(tCanTpMsg *m_stMsgInfo, tCanTpWorkStatus *m_peNextStatus)
{
    uint8 aDataBuf[DATA_LEN] = {0u};
    //ASSERT(nullptr == m_peNextStatus);

    /* Check transmit data len. If data len overflow less than SF, return FALSE. */
    if (TRUE != IsTxDataLenOverflowSF())
    {
        *m_peNextStatus = TX_SF;
        return N_BUFFER_OVFLW;
    }

    /* Set transmitted frame type */
    (void)CANTP_SetFrameType(FF, &aDataBuf[0u]);
    /* Set transmitted data len */
    SetTxFFDataLen(aDataBuf, gs_stCanTPTxDataInfo.stCanTpDataInfo.xFFDataLen);
    /* CAN TP set TX message status and register TX message successful callback. */
    CANTP_SetTxMsgStatus(CANTP_TX_MSG_WAITING);
    CANTP_RegisterTxMsgCallBack(CANTP_DoTransmitFFCallBack);
    /* Copy data in TX buffer */
    fsl_memcpy(&aDataBuf[2u], gs_stCanTPTxDataInfo.stCanTpDataInfo.aDataBuf, FF_DATA_MIN_LEN - 2);

    /* Request transmitted application message. */
    if (TRUE != g_stCANUdsNetLayerCfgInfo.pfNetTxMsg(gs_stCanTPTxDataInfo.stCanTpDataInfo.xCanTpId,
                                                     sizeof(aDataBuf),
                                                     aDataBuf,
                                                     CANTP_TxMsgSuccessfulCallBack,
                                                     g_stCANUdsNetLayerCfgInfo.txBlockingMaxTimeMs))
    {
        /* CAN TP set TX message status and register TX message successful callback. */
        CANTP_SetTxMsgStatus(CANTP_TX_MSG_FAIL);
        CANTP_RegisterTxMsgCallBack(nullptr);
        /* Send message error */
        *m_peNextStatus = IDLE;
        /* Request transmitted application message failed. */
        return N_ERROR;
    }

    /* Set wait send frame successful max time */
    TXFrame_SetTxMsgWaitTime(g_stCANUdsNetLayerCfgInfo.xNAs);
    /* Jump to idle and clear transmitted message. */
    *m_peNextStatus = WAITING_TX;
    return N_OK;
}
/* Transmit CF callback */
static void CANTP_DoTransmitCFCallBack(void)
{
    if (TRUE == IsTxAll())
    {
        TP_DoTransmittedAFrameMsgCallBack(TX_MSG_SUCCESSFUL);
        SetCurCANTPSatus(IDLE);
        return;
    }

    /* Set transmitted next frame min time. */
    SetTxSTmin();

    if (gs_stCanTPTxDataInfo.ucBlockSize)
    {
        gs_stCanTPTxDataInfo.ucBlockSize--;

        /* Block size is equal 0,  waiting  flow control message. if not equal 0, continual send CF message. */
        if (0u == gs_stCanTPTxDataInfo.ucBlockSize)
        {
            SetCurCANTPSatus(RX_FC);
            TXFrame_SetRxMsgWaitTime(g_stCANUdsNetLayerCfgInfo.xNBs);
            return;
        }
    }

    AddTxSN();
    /* Set TX next frame max time. */
    TXFrame_SetRxMsgWaitTime(g_stCANUdsNetLayerCfgInfo.xNCs);//这个时间不清楚多久为1周期，在这里先延时1秒，看读数结果，曾军20250718
    delay_ms(1);			//经测试，加在这里可以正常工作，曾军20250718
    SetCurCANTPSatus(TX_CF);
}
/* Transmit Consecutive Frame */
static tN_Result CANTP_DoTransmitCF(tCanTpMsg *m_stMsgInfo, tCanTpWorkStatus *m_peNextStatus)
{
    uint8 aTxDataBuf[DATA_LEN] = {0u};
    uint8 TxLen = 0u;
    uint8 aTxAllLen = 0u;
    //ASSERT(nullptr == m_peNextStatus);

    /* Is TX STmin timeout? */
    if (FALSE == IsTxSTminTimeout())
    {
        /* Waiting STmin timeout. It's normally in the step. */
        return N_OK;
    }

    /* Is transmitted timeout? */
    if (TRUE == IsTxWaitFrameTimeout())
    {
        *m_peNextStatus = IDLE;
        return N_TIMEOUT_Bs;
    }

    (void)CANTP_SetFrameType(CF, &aTxDataBuf[0u]);
    SetTxSN(&aTxDataBuf[0u]);
    TxLen = gs_stCanTPTxDataInfo.stCanTpDataInfo.xFFDataLen - gs_stCanTPTxDataInfo.stCanTpDataInfo.xPduDataLen;
    /* CAN TP set TX message status and register TX message successful callback. */
    CANTP_SetTxMsgStatus(CANTP_TX_MSG_WAITING);
    CANTP_RegisterTxMsgCallBack(CANTP_DoTransmitCFCallBack);

    if (TxLen >= CF_DATA_MAX_LEN)
    {
        fsl_memcpy(&aTxDataBuf[1u],
                   &gs_stCanTPTxDataInfo.stCanTpDataInfo.aDataBuf[gs_stCanTPTxDataInfo.stCanTpDataInfo.xPduDataLen],
                   CF_DATA_MAX_LEN);

        /* Request transmitted application message. */
        if (TRUE != g_stCANUdsNetLayerCfgInfo.pfNetTxMsg(gs_stCanTPTxDataInfo.stCanTpDataInfo.xCanTpId,
                                                         sizeof(aTxDataBuf),
                                                         aTxDataBuf,
                                                         CANTP_TxMsgSuccessfulCallBack,
                                                         g_stCANUdsNetLayerCfgInfo.txBlockingMaxTimeMs))
        {
            /* CAN TP set TX message status and register TX message successful callback. */
            CANTP_SetTxMsgStatus(CANTP_TX_MSG_FAIL);
            CANTP_RegisterTxMsgCallBack(nullptr);
            /* Send message error */
            *m_peNextStatus = IDLE;
            /* Request transmitted application message failed. */
            return N_ERROR;
        }

        AddTxDataLen(CF_DATA_MAX_LEN);
    }
    else
    {
        fsl_memcpy(&aTxDataBuf[1u],
                   &gs_stCanTPTxDataInfo.stCanTpDataInfo.aDataBuf[gs_stCanTPTxDataInfo.stCanTpDataInfo.xPduDataLen],
                   TxLen);
        aTxAllLen = TxLen + 1u;

        /* Request transmitted application message. */
        if (TRUE != g_stCANUdsNetLayerCfgInfo.pfNetTxMsg(gs_stCanTPTxDataInfo.stCanTpDataInfo.xCanTpId,
                                                         aTxAllLen,
                                                         aTxDataBuf,
                                                         CANTP_TxMsgSuccessfulCallBack,
                                                         g_stCANUdsNetLayerCfgInfo.txBlockingMaxTimeMs))
        {
            /* CAN TP set TX message status and register TX message successful callback. */
            CANTP_SetTxMsgStatus(CANTP_TX_MSG_FAIL);
            CANTP_RegisterTxMsgCallBack(nullptr);
            /* Send message error */
            *m_peNextStatus = IDLE;
            /* Request transmitted application message failed. */
            return N_ERROR;
        }

        AddTxDataLen(TxLen);
    }

    /* Set wait send frame successful max time */
    TXFrame_SetTxMsgWaitTime(g_stCANUdsNetLayerCfgInfo.xNAs);
    *m_peNextStatus = WAITING_TX;
    return N_OK;
}

/* Waiting TX message */
static tN_Result CANTP_DoWaitingTxMsg(tCanTpMsg *m_stMsgInfo, tCanTpWorkStatus *m_peNextStatus)
{
    /* Check is waiting timeout? */
    if (TRUE == IsTxMsgWaitingFrameTimeout())
    {
        /* Abort CAN bus send message */
        if (nullptr != g_stCANUdsNetLayerCfgInfo.pfAbortTXMsg)
        {
            (g_stCANUdsNetLayerCfgInfo.pfAbortTXMsg) ();
        }

        /* Tell up layer, TX message timeout */
        TP_DoTransmittedAFrameMsgCallBack(TX_MSG_TIMEOUT);
        /* CAN TP set TX message status and register TX message successful callback. */
        CANTP_SetTxMsgStatus(CANTP_TX_MSG_FAIL);
        CANTP_RegisterTxMsgCallBack(nullptr);
        *m_peNextStatus = IDLE;
    }

    return N_OK;
}

/* Clear CAN TP TX BUS FIFO */
static uint8 CANTP_ClearTXBUSFIFO(void)
{
    uint8 result = FALSE;
    tErroCode eStatus = ERRO_NONE;
    ClearFIFO(TX_BUS_FIFO, &eStatus);

    if (ERRO_NONE == eStatus)
    {
        result = TRUE;
    }

    return result;
}



/* CAN TP TX message: there not use CAN driver TxFIFO, directly invoked CAN send function */
static uint8 CANTP_TxMsg(const tUdsId i_xTxId,const tUdsLen i_DataLen,const uint8 *i_pDataBuf,
                         const tpfNetTxCallBack i_pfNetTxCallBack,const uint32 txBlockingMaxtime)
{
    tLen xCanWriteDataLen = 0u;
    tErroCode eStatus;
    uint8 aMsgBuf[8] = {0};
    tTPTxMsgHeader TxMsgInfo;
    const uint32 msgInfoLen = sizeof(tTPTxMsgHeader) + sizeof(aMsgBuf);
    //ASSERT(nullptr == i_pDataBuf);

    if (i_DataLen > 8u)
    {
        return FALSE;
    }
    //获取总线FIFO可以写入的剩余空间大小
    GetCanWriteLen(TX_BUS_FIFO, &xCanWriteDataLen, &eStatus);

    if ((ERRO_NONE == eStatus) && (msgInfoLen <= xCanWriteDataLen))
    {
        TxMsgInfo.TxMsgID = i_xTxId;
        TxMsgInfo.TxMsgLength = sizeof(aMsgBuf);
        TxMsgInfo.TxMsgCallBack = (uint32)i_pfNetTxCallBack;
        fsl_memcpy(&aMsgBuf[0u], i_pDataBuf, i_DataLen);
        WriteDataInFifo(TX_BUS_FIFO, (uint8 *)&TxMsgInfo, sizeof(tTPTxMsgHeader), &eStatus);//写入ID，数据长度，回调函数指针

        if (ERRO_NONE != eStatus)
        {
            ClearFIFO(TX_BUS_FIFO, &eStatus);
            return FALSE;
        }

        WriteDataInFifo(TX_BUS_FIFO, (uint8 *)aMsgBuf, 8, &eStatus);//写入数据

        if (ERRO_NONE != eStatus)
        {
            ClearFIFO(TX_BUS_FIFO, &eStatus);
            return FALSE;
        }
    }

    return TRUE;
    //ret = TransmitCANMsg(i_xTxId, i_DataLen, i_pDataBuf, i_pfNetTxCallBack, txBlockingMaxtime);
}

static tRxMsgInfo stTpRxCanMsg = {0u};
/* CAN TP RX message: read RX msg from CAN driver RxFIFO */
static uint8 CANTP_RxMsg(tUdsId *o_pxRxId,uint8 *o_pRxDataLen,uint8 *o_pRxBuf)
{
    /*从数据接收总线FIFO中读取数据 */
    tLen xCanRxDataLen = 0u;
    tLen xReadDataLen = 0u;
    tErroCode eStatus;
    uint8 ucIndex = 0u;
    const uint16 headerLen = sizeof(stTpRxCanMsg.rxDataId) + sizeof(stTpRxCanMsg.rxDataLen);//4个字节
    //ASSERT(nullptr == o_pxRxId);
    //ASSERT(nullptr == o_pRxBuf);
    //ASSERT(nullptr == o_pRxDataLen);
    GetCanReadLen(RX_BUS_FIFO, &xCanRxDataLen, &eStatus);//获取当前fifo中的数据总长度
    if ((ERRO_NONE == eStatus) && (headerLen <= xCanRxDataLen))
    {
        /* 先读取ID和数据长度 */
        ReadDataFromFifo(RX_BUS_FIFO,headerLen,(uint8 *)&stTpRxCanMsg,&xReadDataLen,&eStatus);

        if ((ERRO_NONE == eStatus) && (headerLen <= xCanRxDataLen))
        {
            /* 再读取实际的数据 */
            ReadDataFromFifo(RX_BUS_FIFO,stTpRxCanMsg.rxDataLen,(uint8 *)&stTpRxCanMsg.aucDataBuf,&xCanRxDataLen,&eStatus);
            /* 判断ID是否是UDS相关的ID */
            if (TRUE != CANTP_IsReceivedMsgIDValid(stTpRxCanMsg.rxDataId))
            {
                return FALSE;
            }
            /* 以下填充数据 */
            *o_pxRxId = stTpRxCanMsg.rxDataId;
            *o_pRxDataLen = (uint8)stTpRxCanMsg.rxDataLen;

            for (ucIndex = 0u; ucIndex < stTpRxCanMsg.rxDataLen; ucIndex++)
            {
                o_pRxBuf[ucIndex] = stTpRxCanMsg.aucDataBuf[ucIndex];
            }
            return TRUE;
        }
    }
    else
    {
        if ((0u != xCanRxDataLen) || (ERRO_NONE != eStatus))
        {
            //TPDebugPrintf("\n %s write message in FIFO failed! status = %d, FIFO avaliable Length=%d\n", __func__, eStatus, xCanRxDataLen);
        }
    }

    return FALSE;
}

/* Abort CAN BUS TX message */
static void CANTP_AbortTxMsg(void)
{
    //TPDebugPrintf("CANTP_AbortTxMsg\n");

    if (nullptr != gs_pfCANTPAbortTxMsg)
    {
        (gs_pfCANTPAbortTxMsg)();
        gs_pfTxMsgSuccessfulCallBack = nullptr;
    }

    if (TRUE != CANTP_ClearTXBUSFIFO())
    {
        //TPDebugPrintf("CANTP_AbortTxMsg: Clear TX BUS FIFO failed!\n");
    }
}

void* fsl_memset (void *s, int c, unsigned int count)
{
    char *xs = (char*) s;

    while (count--)
        *xs++ = (char)c;

    return s;
}

void* fsl_memcpy (void *dst, const void *src, unsigned int count)
{
    char *tmp = (char*) dst, *s = (char*) src;
    unsigned int len;

    if (tmp <= s || tmp > (s + count))
    {
        while (count--)
            *tmp++ = *s++;
    }
    else
    {
        for (len = count; len > 0; len--)
            tmp[len - 1] = s[len - 1];
    }

    return dst;
}

/* TP system tick control */
void TP_SystemTickCtl(void)
{
    if (gs_stCanTPRxDataInfo.xSTmin)
    {
        gs_stCanTPRxDataInfo.xSTmin--;
    }

    if (gs_stCanTPRxDataInfo.xMaxWatiTimeout)
    {
        gs_stCanTPRxDataInfo.xMaxWatiTimeout--;
    }

    if (gs_stCanTPTxDataInfo.xSTmin)
    {
        gs_stCanTPTxDataInfo.xSTmin--;
    }

    if (gs_stCanTPTxDataInfo.xMaxWatiTimeout)
    {
        gs_stCanTPTxDataInfo.xMaxWatiTimeout--;
    }

    if (gs_CANTPTxMsgMaxWaitTime)
    {
        gs_CANTPTxMsgMaxWaitTime--;
    }
}

tUdsId TP_GetConfigRxMsgPHYID(void)
{
    return CANTP_GetConfigRxMsgPHYID();
}

tUdsId TP_GetConfigRxMsgFUNID(void)
{
    return CANTP_GetConfigRxMsgFUNID();
}
tUdsId TP_GetConfigTxMsgID(void)
{
    return CANTP_GetConfigTxMsgID();
}

// 初始化时，通过ApplyFifo函数添加了4个链表，曾军20250221
void CANTP_Init(tUdsId xRxFunId,tUdsId xRxPhyId,tUdsId xTxId)
{
	g_stCANUdsNetLayerCfgInfo.xRxFunId = xRxFunId;
	g_stCANUdsNetLayerCfgInfo.xRxPhyId = xRxPhyId;
	g_stCANUdsNetLayerCfgInfo.xTxId = xTxId;

	tErroCode eStatus;
	ApplyFifo(RX_TP_QUEUE_LEN, RX_TP_QUEUE_ID, &eStatus);

	if (ERRO_NONE != eStatus)
	{
		//TPDebugPrintf("Apply RX_TP_QUEUE_ID failed!\n");

		while (1)
		{
		}
	}

	ApplyFifo(TX_TP_QUEUE_LEN, TX_TP_QUEUE_ID, &eStatus);

	if (ERRO_NONE != eStatus)
	{
		//TPDebugPrintf("Apply TX_TP_QUEUE_ID failed\n");

		while (1)
		{
		}
	}

	ApplyFifo(RX_BUS_FIFO_LEN, RX_BUS_FIFO, &eStatus);

	if (ERRO_NONE != eStatus)
	{
		//TPDebugPrintf("Apply RX_BUS_FIFO failed!\n");

		while (1)
		{
		}
	}

	ApplyFifo(TX_BUS_FIFO_LEN, TX_BUS_FIFO, &eStatus);

	if (ERRO_NONE != eStatus)
	{
		//TPDebugPrintf("Apply TX_BUS_FIFOfailed!\n");

		while (1)
		{

		}
	}
}


uint8 CANTP_DriverReadDataFromCANTP(const tUdsLen i_readDataLen, uint8 *o_pReadDataBuf, tTPTxMsgHeader *o_pstTxMsgHeader)
{
    uint8 result = FALSE;
    tLen xCanRxDataLen = 0u;
    tErroCode eStatus;
    tTPTxMsgHeader TxMsgInfo;
    const uint32 msgInfoLen = sizeof(tTPTxMsgHeader);// 8个字节，曾军20250220
    //ASSERT(nullptr == o_pReadDataBuf);
    //ASSERT(nullptr == o_pstTxMsgHeader);
    //ASSERT(0u == i_readDataLen);
    GetCanReadLen(TX_BUS_FIFO, &xCanRxDataLen, &eStatus);

    if ((ERRO_NONE == eStatus) && (xCanRxDataLen > msgInfoLen))
    {
        ReadDataFromFifo(TX_BUS_FIFO,sizeof(tTPTxMsgHeader),(uint8 *)&TxMsgInfo,&xCanRxDataLen,&eStatus);

        if ((ERRO_NONE == eStatus) && (xCanRxDataLen == sizeof(tTPTxMsgHeader)))
        {
            result = TRUE;
        }

        if (TRUE == result)
        {
            ReadDataFromFifo(TX_BUS_FIFO,i_readDataLen,o_pReadDataBuf,&xCanRxDataLen,&eStatus);

            if ((ERRO_NONE == eStatus) && (xCanRxDataLen == i_readDataLen) && (i_readDataLen >= TxMsgInfo.TxMsgLength))
            {
                result = TRUE;
                *o_pstTxMsgHeader = TxMsgInfo;

                /* Storage callback, if user want to TX message callback please call TP_DoTxMsgSuccesfulCallback or self call callback */
                gs_pfTxMsgSuccessfulCallBack = (tpfNetTxCallBack)TxMsgInfo.TxMsgCallBack;
            }
        }
    }

    return result;
}

/* Write data in CAN TP */
void CANTP_DriverWriteDataInCANTP(const tUdsId i_RxID, const tUdsLen i_dataLen, const uint8 *i_pDataBuf)
{
	tLen xCanWriteDataLen = 0u;
	tErroCode eStatus;
	tRxMsgInfo stRxCanMsg;
	const uint16 headerLen = sizeof(stRxCanMsg.rxDataId) + sizeof(stRxCanMsg.rxDataLen);//计算结果为4，之前为6，
	// 应该之前设定为扩展帧，如果为扩展帧，定义的ID，应改为uint32，曾军20250224

	if (i_dataLen > 8u) //收到的总线数据长度大于8,直接返回，此时总线可能出问题了
	{
		return ;
	}
	/* 获取当前接收数据总线FIFO剩余的空间 */
	GetCanWriteLen(RX_BUS_FIFO, &xCanWriteDataLen, &eStatus);
	/* 剩余空间可以保证本次写入数据 */
	if ((ERRO_NONE == eStatus) && ((i_dataLen + headerLen) <= xCanWriteDataLen)) //写入的大小=4字节ID+2字节长度+数据长度
	{
		stRxCanMsg.rxDataId = i_RxID;
		stRxCanMsg.rxDataLen = i_dataLen;
		/* 写入接收数据总线FIFO */
		WriteDataInFifo(RX_BUS_FIFO, (uint8 *)&stRxCanMsg, headerLen, &eStatus);//写入ID和数据长度

		if (ERRO_NONE != eStatus)
		{
			return ;//写入失败
		}
		/* 写入数据 */
		WriteDataInFifo(RX_BUS_FIFO, (uint8 *)i_pDataBuf, stRxCanMsg.rxDataLen, &eStatus);

		if (ERRO_NONE != eStatus)
		{
			return ;//写入失败
		}
	}
}

/* UDS network main function */
void CANTP_MainFun(void)
{
    uint8 index = 0u;
    const uint8 findCnt = sizeof(gs_astCanTpFunInfo) / sizeof(tCanTpFunInfo);//判断当前的CAN报文类型
    tCanTpMsg stRxCanTpMsg = {TRUE, 0u, 0u, {0u}};
    tN_Result result = N_OK;

//    stRxCanTpMsg.isFree = TRUE;
    /* In waiting TX message, cannot read message from FIFO. Because, In waiting message will lost read messages. */
    if (WAITING_TX != gs_eCanTpWorkStatus)//判断当前网络层状态为不等于等待发送状态
    {
        /* Read msg from CAN driver RxFIFO */ //调用CANTP_RxMsg,从数据接收总线FIFO中读取数据
        if (TRUE == g_stCANUdsNetLayerCfgInfo.pfNetRx(&stRxCanTpMsg.xMsgId,&stRxCanTpMsg.msgLen,stRxCanTpMsg.aMsgBuf))
        {	// 上面这个函数实际调用的是static uint8 CANTP_RxMsg(tUdsId *o_pxRxId,uint8 *o_pRxDataLen,uint8 *o_pRxBuf)，曾军20250224
            /* Check received message ID valid? */
            /* 这一段可以不要，在 CANTP_RxMsg中已经判断过了*/
            if (TRUE == CANTP_IsReceivedMsgIDValid(stRxCanTpMsg.xMsgId))
            {
                stRxCanTpMsg.isFree = FALSE;
            }
//            stRxCanTpMsg.isFree = FALSE;//表示有接收到数据
        }
    }

    while (index < findCnt)
    {
        if (gs_eCanTpWorkStatus == gs_astCanTpFunInfo[index].eCanTpStaus)//判断CAN的帧类型
        {
            /* 执行网络层服务函数 */
            if (nullptr != gs_astCanTpFunInfo[index].pfCanTpFun)
            {
                result = gs_astCanTpFunInfo[index].pfCanTpFun(&stRxCanTpMsg, &gs_eCanTpWorkStatus);
            }
        }

        /* If received unexpected PDU, then jump to IDLE and restart do progresses. */
        if (N_UNEXP_PDU != result)//6
        {
            if (N_OK != result)
            {
                //SetCurCANTPSatus(IDLE);
            	gs_eCanTpWorkStatus = IDLE;//服务函数执行不成功则将状态改为IDLE
            }

            index++;
        }
        else
        {
            index = 0u;
        }
    }

//    ClearCanTpRxMsgBuf(&stRxCanTpMsg);//清除读取到本函数中的缓存，可以不要
    /* Check CAN TP TX message successful? */
    CANTP_DoRegisterTxMsgCallBack();
}

