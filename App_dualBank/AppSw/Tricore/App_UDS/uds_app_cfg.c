#include "uds_app_cfg.h"
#include "uds_tp.h"
#include "fls_app.h"
#include "App_bootloader_cfg.h"

#include "uds_app.h"


/**********************UDS Information Static function************************/

/***********************UDS Information Static Global value************************/

///*Is security request lock timeout?*/
//static uint8 IsSecurityRequestLockTimeout(void)
//{
//    uint8 status = 0u;
//
//    if(gs_stUdsInfo.xSecurityReqLockTime)
//    {
//        status = TRUE;
//    }
//    else
//    {
//        status = FALSE;
//    }
//
//    return status;
//}

/***********************UDS Information Global function************************/
/*set current request id  SUPPORT_PHYSICAL_ADDR/SUPPORT_FUNCTION_ADDR */
//#define SetRequestIdType(xRequestIDType) (gs_stUdsInfo.RequsetIdMode = (xRequestIDType))






/*define security access info*/
typedef struct
{
    uint8 SubfunctionNumber;    /*subfunction number*/
    uint8 RequestSession;       /*request session*/
    uint8 RequestIDMode;        /*request id mode*/
    uint8 RequestSecurityLevel; /*request security level*/
    void (*pfRoutine)(void);    /*routine*/
} tSecurityAccessInfo;

/*define write data subfunction*/
typedef struct
{
    uint8 Subfunction;      /*subfunction*/
    uint8 RequestSession;   /*request session*/
    uint8 RequestIdMode;    /*request id mode*/
    uint8 RequestLevel;     /*request level*/
    void (*pfRoutine)(void);/*routine*/
} tWriteDataByIdentifierInfo;

#define DOWLOAD_DATA_ADDR_LEN (4u)      /*dowload data addr len*/
#define DOWLOAD_DATA_LEN (4u)           /*dowload data len*/

/*********************************************************/




#ifdef EN_DELAY_TIME
typedef struct
{
    boolean isReceiveUDSMsg;
    uint32 jumpToAPPDelayTime;
} tJumpAppDelayTimeInfo;

static tJumpAppDelayTimeInfo gs_stJumpAPPDelayTimeInfo = {FALSE, 0u};
#endif

/*********************************************************/
/***********************UDS Information************************/

/**********************UDS service correlation subfunction realizing************************/
/*app memcopy*/
//static void AppMemcopy(const void *i_pvSource, const uint8 i_CopyLen, void *o_pvDest)
//{
//    fsl_memcpy(o_pvDest, i_pvSource, i_CopyLen);
//}

///*app memset*/
//static void AppMemset(const uint8 i_SetValue, const uint16 i_Len, void *m_pvSource)
//{
//
//    fsl_memset(m_pvSource, i_SetValue, i_Len);
//}


typedef void (*tpfFlashOperateMoreTimecallback)(uint8);

/* For erasing or programming flash were timeout callback */
tpfFlashOperateMoreTimecallback gs_pfFlashOperateMoreTimecallback = NULL_PTR;

void RequestMoreTimeCallback(uint8 i_TxStatus)
{
    if(TX_MSG_SUCCESSFUL == i_TxStatus)
    {
        RestartS3Server();
    }

    if(NULL_PTR != gs_pfFlashOperateMoreTimecallback)
    {
        gs_pfFlashOperateMoreTimecallback(i_TxStatus);
        gs_pfFlashOperateMoreTimecallback = NULL_PTR;
    }
}

void RequestMoreTime(const uint8 UDSServiceID, void (*pcallback)(uint8))
{
    tUdsAppMsgInfo stMsgBuf = {0};

    ASSERT(NULL_PTR == pcallback);

    stMsgBuf.xUdsId = TP_GetConfigTxMsgID();
    SetNegativeErroCode(UDSServiceID, NRC_SERVICE_BUSY, &stMsgBuf);
    stMsgBuf.pfUDSTxMsgServiceCallBack = &RequestMoreTimeCallback;
    gs_pfFlashOperateMoreTimecallback = pcallback;

    (void)TP_WriteAFrameDataInTP(stMsgBuf.xUdsId, stMsgBuf.pfUDSTxMsgServiceCallBack,\
                                 stMsgBuf.xDataLen, stMsgBuf.aDataBuf);
}
/*do response checksum*/
void DoResponseChecksum(uint8 i_Status)
{
    uint8 Index = 0u;
    uint8 aResponseBuf[8u] = {0u};
    uint8 TxDataLen = 0u;
    tUdsId UdsTxId = 0u;

    TxDataLen = sizeof(gs_aCheckSumRoutineControlId) / sizeof(gs_aCheckSumRoutineControlId[0u]);
    aResponseBuf[0u] = gs_aCheckSumRoutineControlId[0u] + 0x40u;

    for(Index = 0u; Index < TxDataLen - 1u; Index++)
    {
        aResponseBuf[Index + 1u] = gs_aCheckSumRoutineControlId[Index + 1u];
    }

    if(TRUE == i_Status)
    {
        aResponseBuf[TxDataLen] = 0u;
    }
    else
    {
        aResponseBuf[TxDataLen] = 1u;
    }

    TxDataLen++;

    UdsTxId = TP_GetConfigTxMsgID();

    (void)TP_WriteAFrameDataInTP(UdsTxId, NULL_PTR, TxDataLen, aResponseBuf);
}

/*do check sum. If check sum right return TRUE, else return FALSE.*/
void DoCheckSum(uint8 TxStatus)
{
    if(TX_MSG_SUCCESSFUL == TxStatus)
    {
        /*need request client delay time for flash checking flash data*/
        Flash_SetOperateFlashActiveJob(FLASH_CHECKING, &DoResponseChecksum, 0x31u, &RequestMoreTime);
    }
}



/*do erase flash response*/
void DoEraseFlashResponse(uint8 i_Status)
{
    uint8 Index = 0u;
    uint8 aResponseBuf[8u] = {0u};
    uint8 TxDataLen = 0u;
    tUdsId UdsTxId = 0u;

    TxDataLen = sizeof(gs_aEraseMemoryRoutineControlId) / sizeof(gs_aEraseMemoryRoutineControlId[0u]);
    aResponseBuf[0u] = gs_aEraseMemoryRoutineControlId[0u] + 0x40u;

    for(Index = 0u; Index < TxDataLen - 1u; Index++)
    {
        aResponseBuf[Index + 1u] = gs_aEraseMemoryRoutineControlId[Index + 1u];
    }

    if(TRUE == i_Status)
    {
        aResponseBuf[TxDataLen] = 0u;
    }
    else
    {
        aResponseBuf[TxDataLen] = 1u;
    }

    TxDataLen++;

    UdsTxId = TP_GetConfigTxMsgID();

    (void)TP_WriteAFrameDataInTP(UdsTxId, NULL_PTR, TxDataLen, aResponseBuf);
}

/*do erase flash*/
void DoEraseFlash(uint8 TxStatus)
{
    if(TX_MSG_SUCCESSFUL == TxStatus)
    {
        /*do erase flash need request client delay timeout*/
        Flash_SetOperateFlashActiveJob(FLASH_ERASING, &DoEraseFlashResponse, 0x31, &RequestMoreTime);
    }
}

//uint8 isInProgrammingState(void)
//{
//	if(gs_stUdsInfo.CurSessionMode==PROGRAM_SESSION || gs_stUdsInfo.CurSessionMode==EXTEND_SESSION)//默认/编程
//	{
//		return TRUE;
//	}
//
//	return FALSE;
//}

/*do check programming dependency*/
uint8 DoCheckProgrammingDependency(void)
{
//    uint8 ret = FALSE;
//
//    if(TRUE == isInProgrammingState())//判断会话状态
//    {
//        if(TRUE == Flash_IsAppInFlashValid())
//        {
//            ret = TRUE;
//        }
//        else
//        {
//            ret = FALSE;
//        }
//    }
//    else
//    {
//        ret = FALSE;
//    }

//    return ret;

	return TRUE;
}

/*********************************************************/
/**********************UDS service other module call function realizing************************/

/*transmitted confirm message callback*/
void TXConfrimMsgCallback(uint8 i_status)
{
    if(TX_MSG_SUCCESSFUL == i_status)
    {
        SetCurrentSession(PROGRAM_SESSION);
        SetSecurityLevel(NONE_SECURITY);

        /*restart s3server time*/
        RestartS3Server();
    }
}


/*write message to host basd on UDS for request enter bootloader mode*/
boolean UDS_TxMsgToHost(void)
{
    tUdsAppMsgInfo stUdsAppMsg = {0u, 0u, {0u}, NULL_PTR};
    boolean ret = FALSE;

    stUdsAppMsg.xUdsId = TP_GetConfigTxMsgID();
    stUdsAppMsg.xDataLen = 2;
    stUdsAppMsg.aDataBuf[0u] = 0x50u;
    stUdsAppMsg.aDataBuf[1u] = 0x02u;
    stUdsAppMsg.pfUDSTxMsgServiceCallBack = TXConfrimMsgCallback;

    ret = TP_WriteAFrameDataInTP(stUdsAppMsg.xUdsId, stUdsAppMsg.pfUDSTxMsgServiceCallBack,
                                 stUdsAppMsg.xDataLen, stUdsAppMsg.aDataBuf);

    return ret;
}

/***************************End file********************************/


