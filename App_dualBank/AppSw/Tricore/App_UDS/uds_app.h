/**********************************************************************************************************************
 * \file    uds.h
 * \brief
 * \version V1.0.0
 * \date    2021年11月26日
 * \author  Administrator
 *********************************************************************************************************************/
#ifndef UDSDIAGNOSTIC_UDS_APP_H_
#define UDSDIAGNOSTIC_UDS_APP_H_

#include "uds_alg.h"
#include "uds_dtc.h"
#include "uds_hal.h"
#include "uds_tp.h"
#include "uds_timer.h"
#include "uds_cfg.h"
//#include "drv_can.h"
#include "uds_common.h"
#include "tool_class.h"
#include "CANRxTxInterface.h"
#include "uds_app_cfg.h"
#include "Flash.h"

typedef uint16 tUdsTime;

/* Define session mode */
#define DEFALUT_SESSION (1u << 0u)       /* Default session */
#define PROGRAM_SESSION (1u << 1u)       /* Program session */
#define EXTEND_SESSION (1u << 2u)        /* Extend session */
/* Support function/physical ID request */
#define ERRO_REQUEST_ID (0u)             /* Received ID failed */
#define SUPPORT_PHYSICAL_ADDR (1u << 0u) /* Support physical ID request */
#define SUPPORT_FUNCTION_ADDR (1u << 1u) /* Support function ID request */
/* Security request */
#define NONE_SECURITY (1u << 0u)                          /* None security can request */
#define SECURITY_LEVEL_1 ((1 << 1u) | NONE_SECURITY)      /* Security level 1 request */
#define SECURITY_LEVEL_2 ((1u << 2u) | SECURITY_LEVEL_1)  /* Security level 2 request */

#define SA_ALGORITHM_SEED_LEN (4u) /* Seed Length */

#define MAX_NUM_OF_BLOCK_LENGTH		(0x100u)
#define DOWLOAD_DATA_ADDR_LEN (4u) /* Download data addr len */
#define DOWLOAD_DATA_LEN (4u)      /* Download data len */

/***********************UDS service Static Global value************************/

/*uds servie sub function config table*/
/*erase memory routine cotnrol ID*/
extern const  uint8 gs_aEraseMemoryRoutineControlId[4u];

/*check sum routine control ID*/
extern const  uint8 gs_aCheckSumRoutineControlId[4u];

/*check programming dependency*/
extern const  uint8 gs_aCheckProgrammingDependencyId[4u];

/*write fingerprint id*/
//const static uint8 gs_aWriteFingerprintId[] = {0x2Eu, 0xF1u, 0x5Au};



typedef struct
{
    uint32 StartAddr; /* Data start address */
    uint32 StartAddrBak;/* Data bake start address */
    uint32 DataLen;   /* Data len */
} tDowloadDataInfo;

/* UDS negative response code */
enum __UDS_NRC__
{
    NRC_GENERAL_REJECT                           = 0x10,
    NRC_SERVICE_NOT_SUPPORTED                    = 0x11,
    NRC_SUBFUNCTION_NOT_SUPPORTED                = 0x12,
    NRC_INVALID_MESSAGE_LENGTH_OR_FORMAT         = 0x13,
    NRC_BUSY_REPEAT_REQUEST                      = 0x21,
    NRC_CONDITIONS_NOT_CORRECT                   = 0x22,
    NRC_REQUEST_SEQUENCE_ERROR                   = 0x24,
    NRC_REQUEST_OUT_OF_RANGE                     = 0x31,
    NRC_SECURITY_ACCESS_DENIED                   = 0x33,
    NRC_INVALID_KEY                              = 0x35,
    NRC_EXCEEDED_NUMBER_OF_ATTEMPTS              = 0x36,
    NRC_REQUIRED_TIME_DELAY_NOT_EXPIRED          = 0x37,
    NRC_GENERAL_PROGRAMMING_FAILURE              = 0x72,
    NRC_SERVICE_BUSY                             = 0x78, /* Request correctly received and response pending */
    NRC_SERVICE_NOT_SUPPORTED_IN_ACTIVE_SESSION  = 0x7F,
};
#define NEGTIVE_RESPONSE_ID (0x7Fu)


typedef enum{
    RESET_NONE = 0,
    HARD_RESET = 1,//硬件复位
    KEY_OFF_ON_RESET = 2,//开关复位
    SOFT_RESET = 3,//软件复位
    ENABLE_RAPID_POWER_SHUTDOWN = 4,//启用快速关机
    DISABLE_RAPID_POWER_SHUTDOWN = 5,//禁用快速关机
}tUdsEcuResetType;

typedef struct
{
    tUdsId xUdsId;
    tUdsLen xDataLen;//250627
    uint8 aDataBuf[150u];
//    uint8 aDataBuf[4096u];//250627

    void (*pfUDSTxMsgServiceCallBack)(uint8); /* TX message callback */
} tUdsAppMsgInfo;
extern tUdsAppMsgInfo gs_tUdsAppMsgInfo;
typedef struct UDSServiceInfo
{
    uint8 SerNum;      /* Service ID eg 0x3E/0x87... */
    uint8 SessionMode; /* Default session / program session / extend session */
    uint8 SupReqMode;  /* Support physical / function addr */
    uint8 ReqLevel;    /* Request level. Lock / unlock */
    void (*pfSerNameFun)(struct UDSServiceInfo *, tUdsAppMsgInfo *);
} tUDSService;
typedef struct
{
    uint8 CalledPeriod;         /* called UDS period */
    /* Security request count. If over this security request count, locked server some time */
    uint8 SecurityRequestCnt;
    tUdsTime xLockTime;         /* Lock time */
    tUdsTime xS3Server;         /* S3 Server time */
} tUdsTimeInfo;

typedef struct
{
	uint8 CurSessionMode; /* Current session mode. default/program/extend mode */
	uint8 RequsetIdMode; /* SUPPORT_PHYSICAL_ADDR/SUPPORT_FUNCTION_ADDR */
	uint8 SecurityLevel; /* Current security level */
	tUdsTime xUdsS3ServerTime; /* UDS s3 server time */
	tUdsTime xSecurityReqLockTime; /* Security request lock time */
} tUdsInfo;
/* uds Communication control type */
typedef enum
{
	UDS_CC_TYPE_NONE = 0,
	UDS_CC_TYPE_NORMAL,
	UDS_CC_TYPE_NM,
	UDS_CC_TYPE_NM_NOR
}tUDSCommCtrlType;

/* uds Communication control mode */
typedef enum
{
	UDS_CC_MODE_RX_TX = 0,
	UDS_CC_MODE_RX_NO,
	UDS_CC_MODE_NO_TX,
	UDS_CC_MODE_NO_NO
}tUDSCommCtrlMode;

/* uds read/write data read-write mode */
typedef enum
{
	UDS_RWDATA_RDONLY = 0,
	UDS_RWDATA_RDWR,
    UDS_RWDATA_RDWR_WRONCE,
    UDS_RWDATA_RDWR_INBOOT
}tUDSRwDataRWMode;

/* uds read/write data read-write mode */
typedef enum
{
	UDS_RWDATA_HEX = 0,
	UDS_RWDATA_ASCII,
	UDS_RWDATA_BCD
}tUDSRwDataType;

/* uds read/write data read-write mode */
typedef enum
{
	UDS_RWDATA_RAM = 0,
	UDS_RWDATA_DFLASH,
	UDS_RWDATA_EEPROM
}tUDSRwDataStoreMode;
/* uds read/write data typedef */
typedef struct
{
	rw_data_did did;
	tUDSRwDataRWMode rw_mode;
	tUDSRwDataStoreMode rw_store;
	tUDSRwDataType dataType;
    uint32 p_entry;//数据入口地址
    uint8 dlc;//字节数
    uint8 dlc_max;//最大可存储的字节数量

}tUDSRwDataTable;



typedef enum
{
    ERASE_MEMORY_ROUTINE_CONTROL,    /* Check erase memory routine control */
    CHECK_SUM_ROUTINE_CONTROL,       /* Check sum routine control */
    CHECK_DEPENDENCY_ROUTINE_CONTROL /* Check dependency routine control */
} tCheckRoutineCtlInfo;


typedef struct {
    uint8 upgrade_flag;     // 升级标志位
    uint32 app_size;        // 应用程序大小
    uint8 md5[16];          // MD5校验值
    uint32 retry_count;     // 重试计数器
    uint8 reserved[11];     // 保留区域
} BootloaderParams;


void SetNegativeErroCode(const uint8 i_UDSServiceNum, const uint8 i_ErroCode,tUdsAppMsgInfo *m_pstPDUMsg);
//extern tUDSCommCtrlMode g_CanMsgCommCtrlMode;
static uint8 IsReceivedKeyRight(const uint8 *i_pReceivedKey, const uint8 *i_pTxSeed,const uint8 KeyLen);
static void DigSession0x10(struct UDSServiceInfo *i_pstUDSServiceInfo, tUdsAppMsgInfo *m_pstPDUMsg);
/* Do reset MCU */
static void DoResetMCU0x11(struct UDSServiceInfo *i_pstUDSServiceInfo, tUdsAppMsgInfo *m_pstPDUMsg);
static void SecurityAccess0x27(struct UDSServiceInfo *i_pstUDSServiceInfo, tUdsAppMsgInfo *m_pstPDUMsg);
static void TesterPresent0x3E(struct UDSServiceInfo *i_pstUDSServiceInfo, tUdsAppMsgInfo *m_pstPDUMsg);//
static void ReadDTCInformation0x19(struct UDSServiceInfo *i_pstUDSServiceInfo, tUdsAppMsgInfo *m_pstPDUMsg);
static void ClearDTCInformation0x14(struct UDSServiceInfo *i_pstUDSServiceInfo, tUdsAppMsgInfo *m_pstPDUMsg);
static void CommunicationControl0x28(struct UDSServiceInfo *i_pstUDSServiceInfo, tUdsAppMsgInfo *m_pstPDUMsg);
static void ReadDataByIdentifier0x22(struct UDSServiceInfo *i_pstUDSServiceInfo, tUdsAppMsgInfo *m_pstPDUMsg);
static void ReadDataByAddress0x23(struct UDSServiceInfo *i_pstUDSServiceInfo,tUdsAppMsgInfo *m_pstPDUMsg);//按地址读取数据，20250328
static void WriteDataByIdentifier0x2E(struct UDSServiceInfo *i_pstUDSServiceInfo, tUdsAppMsgInfo *m_pstPDUMsg);
static void RequestDownload0x34(struct UDSServiceInfo *i_pstUDSServiceInfo, tUdsAppMsgInfo *m_pstPDUMsg);
static void TransferData0x36(struct UDSServiceInfo *i_pstUDSServiceInfo, tUdsAppMsgInfo *m_pstPDUMsg);
static void RequestTransferExit0x37(struct UDSServiceInfo *i_pstUDSServiceInfo, tUdsAppMsgInfo *m_pstPDUMsg);
static void RoutineControl0x31(struct UDSServiceInfo *i_pstUDSServiceInfo, tUdsAppMsgInfo *m_pstPDUMsg);
static void ControlDTCSetting0x85(struct UDSServiceInfo* i_pstUDSServiceInfo, tUdsAppMsgInfo* m_pstPDUMsg);
static void TesterPresent0x3E(struct UDSServiceInfo* i_pstUDSServiceInfo, tUdsAppMsgInfo* m_pstPDUMsg);
void UDS_SystemTickCtl(void);
void UDS_MainFun(void);
void SendMsgMainFun(void);
void readFlagS6(void);
void rwDataInit(void);
void RestartS3Server(void);
void SetCurrentSession(const uint8 i_SerSessionMode);
static uint16 StartErasePFlash(uint8 blockHigh, uint8 blockLow);
static uint16 EraseFlashSector(uint8 blockHigh, uint8 blockLow);
static void DoResetToBootloader(uint8 status);
/*check routine control right?*/
uint8 IsCheckRoutineControlRight( tCheckRoutineCtlInfo i_eCheckRoutineCtlId,  tUdsAppMsgInfo *m_pstPDUMsg);

/*Is erase memory routine control?*/
uint8 IsEraseMemoryRoutineControl( tUdsAppMsgInfo *m_pstPDUMsg);

/*Is check sum routine control?*/
uint8 IsCheckSumRoutineControl( tUdsAppMsgInfo *m_pstPDUMsg);

/*Is check programming dependency?*/
uint8 IsCheckProgrammingDependency( tUdsAppMsgInfo *m_pstPDUMsg);

//tUDSCommCtrlMode udsGetCommCtrlMode();

#endif /* UDSDIAGNOSTIC_UDS_APP_H_ */
