#ifndef __FLS_APP_H__
#define __FLS_APP_H__


#include "uds_app.h"

/*define assert*/
#define EN_ASSERT


#if defined (EN_ASSERT) || defined (EN_DEBUG_TIMER) || defined (EN_DEBUG_PRINT)
//#include "bootloader_debug.h"
#endif

#ifdef EN_ASSERT
#define ASSERT(xValue)\
do{\
	if(xValue)\
	{\
		while(1){}\
	}\
}while(0)

#define ASSERT_Printf(pString, xValue)\
do{\
	if(FALSE != xValue)\
	{\
		DebugPrintf(pString);\
	}\
}while(0)

#define ASSERT_DebugPrintf(pString, xValue)\
do{\
	if(FALSE != xValue)\
	{\
		DebugPrintf((pString));\
		while(1u){}\
	}\
}while(0)
#else
#define ASSERT(xValue)
#define ASSERT_Printf(pString, xValue)
#define ASSERT_DebugPrintf(pString, xValue)
#endif



/*program data buf max length*/
#define MAX_FLASH_DATA_LEN (200u)

/*Flash finger print length*/
#define FL_FINGER_PRINT_LENGTH  (17u)

/*invalid UDS services ID*/
#define INVALID_UDS_SERVICES_ID (0xFFu)

/*input parameter : TRUE/FALSE. TRUE = operation successfull, else failled.*/
typedef void (*tpfResponse)(uint8);
typedef void (*tpfReuestMoreTime)(uint8, void (*)(uint8));

typedef boolean (*tpfFlashInit)(void);
typedef void (*tpfFlashDeInit)(void);
typedef boolean (*tpfEraseSecotr)(const uint32, const uint32);
typedef boolean (*tpfProgramData)(const uint32, const uint8 *, const uint32);
typedef boolean (*tpfReadFlashData)(const uint32, const uint32, uint8 *);


/** flashloader download step */
typedef enum
{
    FL_REQUEST_STEP,      /*flash request step*/
    FL_TRANSFER_STEP,     /*flash transfer data step*/
    FL_EXIT_TRANSFER_STEP,/*exit transfter data step*/
    FL_CHECKSUM_STEP      /*check sum step*/
}tFlDownloadStepType;


typedef enum
{
	FLASH_IDLE,           /*flash idle*/
	FLASH_ERASING,        /*erase flash */
	FLASH_PROGRAMMING,    /*program flash*/
	FLASH_CHECKING,       /*check flash*/
	FLASH_WAITTING       /*waitting transmitted message successful*/
}tFlshJobModle;


typedef struct
{
	/*flash programming successfull? If programming successfull, the value set TRUE, else set FALSE*/
	uint8 isFlashProgramSuccessfull;

	/*Is erase flash successfull? If erased flash successfull, set the TRUE, else set the FALSE.*/
	uint8 isFlashErasedSuccessfull;

	/*Is Flash struct data valid? If writen set the value is TRUE, else set the valid FALSE*/
	uint8 isFlashStructValid;

	/*indicate app Counter. Before download. */
	uint8 appCnt;

	/* flag if fingerprint buffer */
    uint8 aFingerPrint[FL_FINGER_PRINT_LENGTH];

	/*reset handler length*/
	uint32 appStartAddrLen;

	/*app Start address -- reset handler*/
	uint32 appStartAddr;

	/*count CRC*/
	uint32 crc;
}tAppFlashStatus;


typedef struct
{
	tpfFlashInit pfFlashInit;
	tpfEraseSecotr pfEraserSecotr;     /*erase sector*/
	tpfProgramData pfProgramData;      /*program data*/
	tpfReadFlashData pfReadFlashData;  /*read flash data*/
	tpfFlashDeInit pfFlashDeinit;
}tFlashOperateAPI;


typedef struct
{
    /* flag if fingerprint has written */
    uint8 isFingerPrintWritten;

    /* flag if flash driver has downloaded */
    uint8 isFlashDrvDownloaded;

    /* error code for flash active job */
    uint8 errorCode;

	/*request active job UDS service ID*/
	uint8 requestActiveJobUDSSerID;

	/*storage program data buff*/
    uint8 aProgramDataBuff[MAX_FLASH_DATA_LEN];

	/* current procees start address */
    uint32 startAddr;

    /* current procees length */
    uint32 length;

	/*recieve data start address*/
	uint32 receivedDataStartAddr;

	/*received data length*/
	uint32 receivedDataLength;

	/*received CRC value*/
	uint32 receivedCRC;

	/*received program data length*/
	uint32 receiveProgramDataLength;

    /* flashloader download step */
    tFlDownloadStepType eDownloadStep;

    /* current job status */
    tFlshJobModle eActiveJob;

	/*active job finshed callback*/
	tpfResponse pfActiveJobFinshedCallBack;

	/*request more time from host*/
	tpfReuestMoreTime pfRequestMoreTime;

	/*point app flash status*/
	tAppFlashStatus *pstAppFlashStatus;

    /*opeate flash API*/
	tFlashOperateAPI stFlashOperateAPI;

}tFlsDownloadStateType;


typedef uint32 tLogicalAddr;

typedef struct
{
	tLogicalAddr xBlockStartLogicalAddr; /*block start logical addr*/
	tLogicalAddr xBlockEndLogicalAddr;	 /*block end logical addr*/
}BlockInfo_t;






uint8 IsDownloadDataLenValid(const uint32 i_DataLen);
uint8 IsDownloadDataAddrValid(const uint32 i_DataAddr);
tFlDownloadStepType Flash_GetCurDownloadStep(void);
void Flash_SetOperateFlashActiveJob(const tFlshJobModle i_activeJob,
									const tpfResponse i_pfActiveFinshedCallBack,
									const uint8 i_requestUDSSerID,
									const tpfReuestMoreTime i_pfRequestMoreTimeCallback);
void Flash_SetNextDownloadStep(const tFlDownloadStepType i_donwloadStep);
uint8 Flash_ProgramRegion(uint32 i_addr,uint8 *i_pDataBuf,uint32 i_dataLen);
boolean FLASH_HAL_GetFlashDriverInfo(uint32 *o_pFlashDriverAddrStart, uint32 *o_pFlashDriverEndAddr);
void Flash_EraseFlashDriverInRAM(void);
void Flash_InitDowloadInfo(void);
void Flash_SaveDownloadDataInfo(const uint32 i_dataStartAddr, const uint32 i_dataLen);

#endif

