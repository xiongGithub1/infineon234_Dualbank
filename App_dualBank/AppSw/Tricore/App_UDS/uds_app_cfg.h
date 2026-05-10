#ifndef __UDS_APP_CFG_H__
#define __UDS_APP_CFG_H__

/*********************Include headers************************/

#include "uds_tp.h"
#include "uds_app.h"
/**********************************************************/
/*********************************************************/

/*S3 timer watermark time percent*/
#ifndef S3_TIMER_WATERMARK_PERCENT
#define S3_TIMER_WATERMARK_PERCENT (90u)
#endif

#if (S3_TIMER_WATERMARK_PERCENT <= 0) || (S3_TIMER_WATERMARK_PERCENT >= 100)
#error "S3_TIMER_WATERMARK_PERCENT should config (0, 100]"
#endif

/*define session mode*/
#define DEFALUT_SESSION (1u << 0u)       /*default session*/
#define PROGRAM_SESSION (1u << 1u)       /*program session*/
#define EXTEND_SESSION (1u << 2u)        /*extend session*/

/*security request*/
#define NONE_SECURITY (1u << 0u)                          /*none security can request*/
#define SECURITY_LEVEL_1 ((1 << 1u) | NONE_SECURITY)      /*security level 1 request*/
#define SECURITY_LEVEL_2 ((1u << 2u) | SECURITY_LEVEL_1)  /*security level 2 request*/

/*********************************************************/

/*********************************************************/
/**********************UDS service configuration and function************************/

/**********************UDS service correlation subfunction define************************/
/*app memcopy*/
//extern void AppMemcopy(const void *i_pvSource, const uint8 i_CopyLen, void *o_pvDest);

/*app memset*/
//extern void AppMemset(const uint8 i_SetValue, const uint16 i_Len, void *m_pvSource);

extern uint8 DoCheckProgrammingDependency(void);
void TXConfrimMsgCallback(uint8 i_status);

/*Is download data address valid?*/
extern  uint8 IsDownloadDataAddrValid(const uint32 i_DataAddr);

/*Is dowload data len valid?*/
extern  uint8 IsDownloadDataLenValid(const uint32 i_DataLen);



/*set security level*/
extern void SetSecurityLevel(const uint8 i_SerSecurityLevel);


/* If Rx UDS msg, set g_ucIsRxUdsMsg TURE */
extern void SetIsRxUdsMsg(const uint8 i_SetValue);



/*uds time control*/
extern void UDS_SystemTickCtl(void);


/*do check sum. If check sum right return TRUE, else return FALSE.*/
extern void DoCheckSum(uint8 i_TxStatus);

/*do erase flash*/
extern void DoEraseFlash(uint8 i_TxStatus);

///*do check programming dependency*/
//static uint8 DoCheckProgrammingDependency(void);

/*do response checksum*/
void DoResponseChecksum(uint8 i_Status);

/*do erase flash response*/
void DoEraseFlashResponse(uint8 i_Status);

void RequestMoreTimeCallback(uint8 i_TxStatus);

/* When do uds service need more time, need call the funciton */
static void RequestMoreTime(const uint8 UDSServiceID, void (*pcallback)(uint8));

//static uint8 IsSecurityRequestLockTimeout(void);


extern uint16 GetUdsS3ServerTime(void);

extern void SubUdsS3ServerTime(uint16 i_SubTime);

extern uint16 GetUdsSecurityReqLockTime(void);

void SubUdsSecurityReqLockTime(uint16 i_SubTime);

/*write message to host basd on UDS for request enter bootloader mode*/
extern boolean UDS_TxMsgToHost(void);

#endif /*__UDS_APP_CFG_H__*/
/***************************End file********************************/

