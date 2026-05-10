#include "fls_app.h"





/*flash download info*/
static tFlsDownloadStateType gs_stFlashDownloadInfo;

/*application flash status*/
static tAppFlashStatus gs_stAppFlashStatus;


/*flash driver config*/
const BlockInfo_t gs_astFlashDriverBlock[] = {
    {0x1FFF8010u, 0x1FFF8810u},
};




/*Is flash driver download?*/
#define IsFlashDriverDownload() (gs_stFlashDownloadInfo.isFlashDrvDownloaded)
#define SetFlashDriverDowload() (gs_stFlashDownloadInfo.isFlashDrvDownloaded = TRUE)
#define SetFlashDriverNotDonwload() (gs_stFlashDownloadInfo.isFlashDrvDownloaded = FALSE)


/*Is dowload data len valid?*/
uint8 IsDownloadDataLenValid(const uint32 i_DataLen)
{
    return TRUE;
}



/*Is download data address valid?*/
uint8 IsDownloadDataAddrValid(const uint32 i_DataAddr)
{
	//因为地址是800开头，并不是a00开头，此判断会一直false
//	i_DataAddr在PFLASH地址范围内-合理
	if((i_DataAddr >= FL_PFLASH_PF_ADDR_Start) && (i_DataAddr < FL_PFLASH_PF_ADDR_End ))//在PFLASH地址范围内
	{
		 return TRUE;
	}
	else
	{
		return FALSE;
	}
}



/*get current donwload step*/
tFlDownloadStepType Flash_GetCurDownloadStep(void)
{
	return gs_stFlashDownloadInfo.eDownloadStep;
}


/*set operate flash active job.*/
void Flash_SetOperateFlashActiveJob(const tFlshJobModle i_activeJob,
									const tpfResponse i_pfActiveFinshedCallBack,
									const uint8 i_requestUDSSerID,
									const tpfReuestMoreTime i_pfRequestMoreTimeCallback)
{
	gs_stFlashDownloadInfo.eActiveJob = i_activeJob;
	gs_stFlashDownloadInfo.requestActiveJobUDSSerID = i_requestUDSSerID;
	gs_stFlashDownloadInfo.pfRequestMoreTime = i_pfRequestMoreTimeCallback;
	gs_stFlashDownloadInfo.pfActiveJobFinshedCallBack = i_pfActiveFinshedCallBack;
}


/*set next downlaod step.*/
void Flash_SetNextDownloadStep(const tFlDownloadStepType i_donwloadStep)
{
	gs_stFlashDownloadInfo.eDownloadStep = i_donwloadStep;
}



uint8 Flash_ProgramRegion(uint32 i_addr,uint8 *i_pDataBuf,uint32 i_dataLen)
{

    uint8 result = TRUE;

    // 检查当前是否为传输阶段
    if (FL_TRANSFER_STEP != Flash_GetCurDownloadStep())
    {
        result = FALSE;
    }

    // 检查输入指针是否有效
    if (NULL_PTR == i_pDataBuf)
    {
        result = FALSE;
    }

	result = (uint8)Flash_writePFlash_portex(i_addr, i_pDataBuf, i_dataLen); // 直接调用写入函数

	if (TRUE == result)
	{
		Flash_SetOperateFlashActiveJob(FLASH_PROGRAMMING, NULL_PTR, INVALID_UDS_SERVICES_ID, NULL_PTR);
		gs_stFlashDownloadInfo.errorCode = TRUE;
	}
	else
	{
		gs_stFlashDownloadInfo.errorCode = FALSE; // 标记写入失败
	}
    return result;
}



/*get flash driver start and length*/
boolean FLASH_HAL_GetFlashDriverInfo(uint32 *o_pFlashDriverAddrStart, uint32 *o_pFlashDriverEndAddr)
{
//	ASSERT(NULL_PTR == o_pFlashDriverAddrStart);
//	ASSERT(NULL_PTR == o_pFlashDriverEndAddr);

	*o_pFlashDriverAddrStart = gs_astFlashDriverBlock[0u].xBlockStartLogicalAddr;
	*o_pFlashDriverEndAddr = gs_astFlashDriverBlock[0u].xBlockEndLogicalAddr;

	return TRUE;
}



/*erase flash driver in RAM*/
void Flash_EraseFlashDriverInRAM(void)
{
	uint32 flashDriverStartAddr = 0u;
	uint32 flashDriverEndAddr = 0u;
	boolean result = FALSE;

	result = FLASH_HAL_GetFlashDriverInfo(&flashDriverStartAddr, &flashDriverEndAddr);

	if(TRUE == result)
	{
		fsl_memset((void *)flashDriverStartAddr, 0x0u, flashDriverEndAddr - flashDriverStartAddr);
	}
}




/*Init flash download*/
void Flash_InitDowloadInfo(void)
{
	gs_stFlashDownloadInfo.isFingerPrintWritten = FALSE;

	if(TRUE == IsFlashDriverDownload())
	{
		Flash_EraseFlashDriverInRAM();

		SetFlashDriverNotDonwload();
	}

	Flash_SetNextDownloadStep(FL_REQUEST_STEP);

	Flash_SetOperateFlashActiveJob(FLASH_IDLE, NULL_PTR, INVALID_UDS_SERVICES_ID, NULL_PTR);

	gs_stFlashDownloadInfo.pstAppFlashStatus = &gs_stAppFlashStatus;

	fsl_memset(&gs_stFlashDownloadInfo.stFlashOperateAPI, 0x0u, sizeof(tFlashOperateAPI));

	fsl_memset(&gs_stAppFlashStatus, 0xFFu, sizeof(tAppFlashStatus));
}



/*save download data information, the API called by UDS request download service*/
void Flash_SaveDownloadDataInfo(const uint32 i_dataStartAddr, const uint32 i_dataLen)
{
	/*program data info*/
	gs_stFlashDownloadInfo.startAddr = i_dataStartAddr;
	gs_stFlashDownloadInfo.length = i_dataLen;

	/*calculate data CRC info*/
	gs_stFlashDownloadInfo.receivedDataStartAddr = i_dataStartAddr;
	gs_stFlashDownloadInfo.receivedDataLength = i_dataLen;
}

