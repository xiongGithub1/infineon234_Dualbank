/**********************************************************************************************************************
 * \file    uds.c
 * \brief
 * \version V1.0.0
 * \date    2021??11??26??
 * \author  Administrator
 *********************************************************************************************************************/
#include "uds_app.h"
#include "App_bootloader_cfg.h"
#include "Can_session.h"
#include "fls_app.h"
#include "Flash.h"
#include "Flash.h"
#include "App_bootloader.h"
#include "Boot_DualBank.h"

uint32 pageData1[128];



const  uint8 gs_aEraseMemoryRoutineControlId[4u] = { 0x31u, 0x01u, 0xFFu, 0x00u };


const  uint8 gs_aCheckSumRoutineControlId[4u] = { 0x31u, 0x01u, 0x02u, 0x02u };


const  uint8 gs_aCheckProgrammingDependencyId[4u] = { 0x31u, 0x01u, 0xFFu, 0x01u };





uint8 data_vin_f190[] = VIN_F190;
uint8 data_bsidid_f180[] = BSID_F180;

tUDSCommCtrlMode g_CanMsgCommCtrlMode = UDS_CC_MODE_RX_TX;

uint32 p_rw_finger_data = 0;




tUDSRwDataTable g_rwDataTable[] =
{
		{F15A,UDS_RWDATA_RDWR,UDS_RWDATA_RAM,UDS_RWDATA_HEX,(uint32) &p_rw_finger_data,4,4},
		{F190,UDS_RWDATA_RDWR,UDS_RWDATA_RAM,UDS_RWDATA_ASCII,(uint32) &data_vin_f190[0],16,16}
};

#define IsWriteFingerprintRight(x) ((x == gs_aWriteFingerprintId)?TRUE:FALSE)



static const tUdsTimeInfo gs_stUdsAppCfg =
{
	1u,
	3u,
	10000u,
	5000u
};

static tUdsInfo gs_stUdsInfo =
{
	DEFALUT_SESSION,
	ERRO_REQUEST_ID,
	NONE_SECURITY,
	0u,
	0u,
};

static const tUDSService gs_astUDSService[] =
{
	{
			0x10u,
			DEFALUT_SESSION | PROGRAM_SESSION | EXTEND_SESSION,
			SUPPORT_PHYSICAL_ADDR | SUPPORT_FUNCTION_ADDR,
			NONE_SECURITY,
			DigSession0x10
	},
	{
			0x11u,
	#ifdef DIAGNOSTIC_MODE_FOR_APP
			DEFALUT_SESSION | EXTEND_SESSION,
	#endif
	#ifdef DIAGNOSTIC_MODE_FOR_BOOTLOADER
			DEFALUT_SESSION | PROGRAM_SESSION | EXTEND_SESSION,
	#endif
			SUPPORT_PHYSICAL_ADDR | SUPPORT_FUNCTION_ADDR,
			NONE_SECURITY,
			DoResetMCU0x11
	},
	{
			0x27u,
	#ifdef DIAGNOSTIC_MODE_FOR_APP
			EXTEND_SESSION | EXTEND_SESSION,
	#endif
	#ifdef DIAGNOSTIC_MODE_FOR_BOOTLOADER
			PROGRAM_SESSION | EXTEND_SESSION,
	#endif
			SUPPORT_PHYSICAL_ADDR,
			NONE_SECURITY,
			SecurityAccess0x27
	},
	{
			0x28u,
			DEFALUT_SESSION | PROGRAM_SESSION | EXTEND_SESSION,
			SUPPORT_PHYSICAL_ADDR | SUPPORT_FUNCTION_ADDR,
			SECURITY_LEVEL_1,
			CommunicationControl0x28
	},
	{
			0x22u,
			DEFALUT_SESSION | PROGRAM_SESSION | EXTEND_SESSION,
			SUPPORT_PHYSICAL_ADDR | SUPPORT_FUNCTION_ADDR,
			NONE_SECURITY,
			ReadDataByIdentifier0x22
	},
	{
			0x23u,
			DEFALUT_SESSION | EXTEND_SESSION,
			SUPPORT_PHYSICAL_ADDR | SUPPORT_FUNCTION_ADDR,
			NONE_SECURITY,
			ReadDataByAddress0x23
	},
	{
			0x2Eu,
			EXTEND_SESSION | PROGRAM_SESSION,
			SUPPORT_PHYSICAL_ADDR,
			SECURITY_LEVEL_1,
			WriteDataByIdentifier0x2E
	},
	{
			0x31u,
			DEFALUT_SESSION | PROGRAM_SESSION | EXTEND_SESSION,
			SUPPORT_PHYSICAL_ADDR,
			SECURITY_LEVEL_1,
			RoutineControl0x31
	},

	{
			0x34u,
			PROGRAM_SESSION ,
			SUPPORT_PHYSICAL_ADDR | SUPPORT_FUNCTION_ADDR,
			SECURITY_LEVEL_2,
			RequestDownload0x34
	},

	{
			0x36u,
			PROGRAM_SESSION,
			SUPPORT_PHYSICAL_ADDR | SUPPORT_FUNCTION_ADDR,
			SECURITY_LEVEL_2,
			TransferData0x36
	},
	{
			0x37u,
			PROGRAM_SESSION,
			SUPPORT_PHYSICAL_ADDR | SUPPORT_FUNCTION_ADDR,
			SECURITY_LEVEL_2,
			RequestTransferExit0x37
	},
	{
			0x85u,
			DEFALUT_SESSION | PROGRAM_SESSION | EXTEND_SESSION,
			SUPPORT_PHYSICAL_ADDR | SUPPORT_FUNCTION_ADDR,
			SECURITY_LEVEL_1,
			ControlDTCSetting0x85
	},
	/* Tester present service */
	{
			0x3Eu,
			DEFALUT_SESSION | PROGRAM_SESSION | EXTEND_SESSION,
			SUPPORT_PHYSICAL_ADDR | SUPPORT_FUNCTION_ADDR,
			NONE_SECURITY,
			TesterPresent0x3E
	},
};


uint8 IsCheckRoutineControlRight(tCheckRoutineCtlInfo i_eCheckRoutineCtlId,
	tUdsAppMsgInfo* m_pstPDUMsg)
{
	uint8 Index = 0u;
	uint8 FindCnt = 0u;
	uint8* pDestRoutineCltId = NULL_PTR;

	ASSERT(NULL_PTR == m_pstPDUMsg);

	switch (i_eCheckRoutineCtlId)
	{
		case ERASE_MEMORY_ROUTINE_CONTROL:
			pDestRoutineCltId = (uint8*) &gs_aEraseMemoryRoutineControlId[0u];

			FindCnt = sizeof(gs_aEraseMemoryRoutineControlId);

			break;

		case CHECK_SUM_ROUTINE_CONTROL:
			pDestRoutineCltId = (uint8*) &gs_aCheckSumRoutineControlId[0u];

			FindCnt = sizeof(gs_aCheckSumRoutineControlId);

			break;

		case CHECK_DEPENDENCY_ROUTINE_CONTROL:
			pDestRoutineCltId = (uint8*) &gs_aCheckProgrammingDependencyId[0u];

			FindCnt = sizeof(gs_aCheckProgrammingDependencyId);

			break;

		default:

			return FALSE;


	}

	if ((NULL_PTR == pDestRoutineCltId) || (m_pstPDUMsg->xDataLen < FindCnt))
	{
		return FALSE;
	}

	while (Index < FindCnt)
	{
		if (m_pstPDUMsg->aDataBuf[Index] != pDestRoutineCltId[Index])
		{
			return FALSE;
		}

		Index++;
	}

	return TRUE;
}


uint8 IsEraseMemoryRoutineControl(tUdsAppMsgInfo* m_pstPDUMsg)
{


	return IsCheckRoutineControlRight(ERASE_MEMORY_ROUTINE_CONTROL, m_pstPDUMsg);
}


uint8 IsCheckSumRoutineControl(tUdsAppMsgInfo* m_pstPDUMsg)
{
	return IsCheckRoutineControlRight(CHECK_SUM_ROUTINE_CONTROL, m_pstPDUMsg);
}


uint8 IsCheckProgrammingDependency(tUdsAppMsgInfo* m_pstPDUMsg)
{
	return IsCheckRoutineControlRight(CHECK_DEPENDENCY_ROUTINE_CONTROL, m_pstPDUMsg);
}

uint16 GetUdsS3ServerTime(void)
{
	return (gs_stUdsInfo.xUdsS3ServerTime);
}

void SubUdsS3ServerTime(uint16 i_SubTime)
{
	gs_stUdsInfo.xUdsS3ServerTime -= i_SubTime;
}

uint16 GetUdsSecurityReqLockTime(void)
{
	return (gs_stUdsInfo.xSecurityReqLockTime);
}

void SubUdsSecurityReqLockTime(uint16 i_SubTime)
{
	gs_stUdsInfo.xSecurityReqLockTime -= i_SubTime;
}


uint8 IsS3ServerTimeout(void)
{
	uint8 TimeoutStatus = FALSE;

	if (0u == gs_stUdsInfo.xUdsS3ServerTime)
	{
		TimeoutStatus = TRUE;
	}
	else
	{
		TimeoutStatus = FALSE;
	}

	return TimeoutStatus;
}

uint8 IsCurDefaultSession(void)
{
	uint8 isCurDefaultSessionStatus = FALSE;

	if (DEFALUT_SESSION == gs_stUdsInfo.CurSessionMode)
	{
		isCurDefaultSessionStatus = TRUE;
	}
	else
	{
		isCurDefaultSessionStatus = FALSE;
	}

	return isCurDefaultSessionStatus;
}

uint8 IsCurSeesionCanRequest(uint8 i_SerSessionMode)
{
	uint8 status = FALSE;

	if ((i_SerSessionMode & gs_stUdsInfo.CurSessionMode)
		== gs_stUdsInfo.CurSessionMode)
	{
		status = TRUE;
	}
	else
	{
		status = FALSE;
	}

	return status;
}

uint8 IsCurSecurityLevelRequet(uint8 i_SerSecurityLevel)
{
	uint8 status = 0u;

	if ((gs_stUdsInfo.SecurityLevel & i_SerSecurityLevel) == i_SerSecurityLevel)
	{
		status = TRUE;
	}
	else
	{
		status = FALSE;
	}

	return status;
}

void SetCurrentSession(const uint8 i_SerSessionMode)
{
	gs_stUdsInfo.CurSessionMode = i_SerSessionMode;
}

void SetSecurityLevel(const uint8 i_SerSecurityLevel)
{
	gs_stUdsInfo.SecurityLevel = i_SerSecurityLevel;
}

#define SetRequestIdType(xRequestIDType) (gs_stUdsInfo.RequsetIdMode = (xRequestIDType))


#define UdsAppTimeToCount(xTime) ((xTime) / gs_stUdsAppCfg.CalledPeriod)


void RestartS3Server(void)
{
	gs_stUdsInfo.xUdsS3ServerTime = UdsAppTimeToCount(gs_stUdsAppCfg.xS3Server);

}


void SaveRequestIdType(const uint32 i_SerRequestID)
{
	if (i_SerRequestID == TP_GetConfigRxMsgPHYID())
	{
		SetRequestIdType(SUPPORT_PHYSICAL_ADDR);
	}
	else if (i_SerRequestID == TP_GetConfigRxMsgFUNID())
	{
		SetRequestIdType(SUPPORT_FUNCTION_ADDR);
	}
	else
	{
		SetRequestIdType(ERRO_REQUEST_ID);
	}
}


void UDS_SystemTickCtl(void)
{
	if (GetUdsS3ServerTime())
	{
		SubUdsS3ServerTime(1u);
	}

	if (GetUdsSecurityReqLockTime())
	{
		SubUdsSecurityReqLockTime(1u);
	}

	/* S3 timeout: automatically return to default session and reset security level */
	if ((0u == GetUdsS3ServerTime()) && (TRUE != IsCurDefaultSession()))
	{
		SetCurrentSession(DEFALUT_SESSION);
		SetSecurityLevel(NONE_SECURITY);
		Flash_InitDowloadInfo();
		Flash_SetNextDownloadStep(FL_REQUEST_STEP);
	}
}

uint8 IsCurRxIdCanRequest(uint8 i_SerRequestIdMode)
{
	uint8 status = 0u;

	if ((i_SerRequestIdMode & gs_stUdsInfo.RequsetIdMode)
		== gs_stUdsInfo.RequsetIdMode)
	{
		status = TRUE;
	}
	else
	{
		status = FALSE;
	}

	return status;
}

/* Check received Key against computed Key for Security Access
 * i_SecurityLevel: 1 for Level 1, 2 for Level 2
 */
static uint8 IsReceivedKeyRight(const uint8* i_pReceivedKey, const uint8* i_pTxSeed,
	const uint8 i_SecurityLevel)
{
	uint8 index = 0u;
	uint8 aComputedKey[SA_ALGORITHM_SEED_LEN] = { 0u };

	if (1u == i_SecurityLevel)
	{
		UDS_ALG_HAL_ComputeKey_Level1(i_pTxSeed, aComputedKey);
	}
	else if (2u == i_SecurityLevel)
	{
		UDS_ALG_HAL_ComputeKey_Level2(i_pTxSeed, aComputedKey);
	}
	else
	{
		return FALSE;
	}

	for (index = 0u; index < SA_ALGORITHM_SEED_LEN; index++)
	{
		if (i_pReceivedKey[index] != aComputedKey[index])
		{
			return FALSE;
		}
	}

	return TRUE;
}



void SetNegativeErroCode(const uint8 i_UDSServiceNum, const uint8 i_ErroCode,
	tUdsAppMsgInfo* m_pstPDUMsg)
{

	m_pstPDUMsg->aDataBuf[0u] = NEGTIVE_RESPONSE_ID;
	m_pstPDUMsg->aDataBuf[1u] = i_UDSServiceNum;
	m_pstPDUMsg->aDataBuf[2u] = i_ErroCode;
	m_pstPDUMsg->xDataLen = 3u;
}


tUDSService* GetUDSServiceInfo(uint8* m_pSupServItem)
{

	*m_pSupServItem = sizeof(gs_astUDSService) / sizeof(gs_astUDSService[0u]);
	return (tUDSService*) &gs_astUDSService[0u];
}

/* Tester present service  */
static void TesterPresent0x3E(struct UDSServiceInfo* i_pstUDSServiceInfo, tUdsAppMsgInfo* m_pstPDUMsg)
{
	uint8 RequestSubfunction = 0u;

	/* Check message length: must be SID + subfunction = 2 bytes */
	if (m_pstPDUMsg->xDataLen != 2u)
	{
		SetNegativeErroCode(i_pstUDSServiceInfo->SerNum, NRC_INVALID_MESSAGE_LENGTH_OR_FORMAT, m_pstPDUMsg);
		return;
	}

	RequestSubfunction = m_pstPDUMsg->aDataBuf[1u];

	/* Sub function */
	switch (RequestSubfunction)
	{
		case 0x00u:  /* Zero sub-function - send positive response */
			m_pstPDUMsg->aDataBuf[0u] = i_pstUDSServiceInfo->SerNum + 0x40u;
			m_pstPDUMsg->aDataBuf[1u] = RequestSubfunction;
			m_pstPDUMsg->xDataLen = 2u;
			RestartS3Server();
			break;

		case 0x80u:  /* Suppress positive response */
			m_pstPDUMsg->xDataLen = 0u;
			RestartS3Server();
			break;

		default:
			SetNegativeErroCode(i_pstUDSServiceInfo->SerNum, NRC_SUBFUNCTION_NOT_SUPPORTED, m_pstPDUMsg);
			break;
	}
}
void UDS_MainFun(void)
{
	uint8 UDSSerIndex = 0u;
	uint8 UDSSerNum = 0u;
	tUdsAppMsgInfo stUdsAppMsg = { 0u, 0u, {0u}, NULL_PTR };

	uint8 isFindService = FALSE;
	uint8 SupSerItem = 0u;
	tUDSService* pstUDSService = nullptr;

#if defined (EN_AES_SA_ALGORITHM_SW) || defined (EN_ZLG_SA_ALGORITHM)
	UDS_ALG_HAL_AddSWTimerTickCnt();
#endif
	if (TRUE == TP_ReadAFrameDataFromTP(&stUdsAppMsg.xUdsId, &stUdsAppMsg.xDataLen, stUdsAppMsg.aDataBuf))
	{

		if (TRUE != IsCurDefaultSession())
		{


			RestartS3Server();
		}


		SaveRequestIdType(stUdsAppMsg.xUdsId);
	}
	else
	{
		return;
	}

	pstUDSService = GetUDSServiceInfo(&SupSerItem);


	UDSSerNum = stUdsAppMsg.aDataBuf[0u];
	while ((UDSSerIndex < SupSerItem) && (nullptr != pstUDSService))
	{
		if (UDSSerNum == pstUDSService[UDSSerIndex].SerNum)
		{
			isFindService = TRUE;

			if (TRUE != IsCurRxIdCanRequest(pstUDSService[UDSSerIndex].SupReqMode))
			{

				SetNegativeErroCode(stUdsAppMsg.aDataBuf[0u], NRC_SERVICE_NOT_SUPPORTED, &stUdsAppMsg);

				break;
			}

			if (TRUE != IsCurSeesionCanRequest(pstUDSService[UDSSerIndex].SessionMode))
			{

				SetNegativeErroCode(stUdsAppMsg.aDataBuf[0u], NRC_SERVICE_NOT_SUPPORTED_IN_ACTIVE_SESSION, &stUdsAppMsg);
				break;
			}

			if (TRUE != IsCurSecurityLevelRequet(pstUDSService[UDSSerIndex].ReqLevel))
			{

				SetNegativeErroCode(stUdsAppMsg.aDataBuf[0u], NRC_SECURITY_ACCESS_DENIED, &stUdsAppMsg);

				break;
			}

			stUdsAppMsg.pfUDSTxMsgServiceCallBack = nullptr;


			if (nullptr != pstUDSService[UDSSerIndex].pfSerNameFun)
			{
				pstUDSService[UDSSerIndex].pfSerNameFun((tUDSService*) &pstUDSService[UDSSerIndex], &stUdsAppMsg);
			}
			else
			{

				SetNegativeErroCode(stUdsAppMsg.aDataBuf[0u], 4, &stUdsAppMsg);
			}

			break;
		}
		UDSSerIndex++;
	}

	if (TRUE != isFindService)
	{

		SetNegativeErroCode(stUdsAppMsg.aDataBuf[0u], NRC_SERVICE_NOT_SUPPORTED, &stUdsAppMsg);
	}

	if (0u != stUdsAppMsg.xDataLen)
	{
		stUdsAppMsg.xUdsId = TP_GetConfigTxMsgID();
		(void) TP_WriteAFrameDataInTP(stUdsAppMsg.xUdsId, stUdsAppMsg.pfUDSTxMsgServiceCallBack, stUdsAppMsg.xDataLen, stUdsAppMsg.aDataBuf);
	}
}

/* ??EEPROM?��??????
  *  ?????:??????????byteToRead
 *  	?????????0
 * */
uint8 readDataFromEEPROM(uint32 entry, uint8* pData, uint8 byteToRead)
{

	uint8 ret = tl_read_from_eeprom(entry, pData, byteToRead);
	if (ret == 0)
	{
		for (int i = 0; i < byteToRead; i++)
		{
			pData[i] = 0;
		}
	}
	return ret;
}
/* ??FLASH?��??????
  *  ?????:??????????byteToRead
 *  	?????????0
 * */
uint8 readDataFromFLASH(uint32 entry, uint8* pData, uint8 byteToRead)
{

	uint8 ret = tl_read_from_flash(entry, pData, byteToRead);
	if (ret == 0)
	{
		for (int i = 0; i < byteToRead; i++)
		{
			pData[i] = 0;
		}
	}
	return ret;
}
/* ??EEPROM??��??????
  *  ?????:��????????byteToWrite
 *  	��????????0
 * */
uint8 writeDataToEEPROM(uint32 entry, uint8* pData, uint8 byteToWrite)
{

	uint8 ret = tl_write_to_eeprom(entry, pData, byteToWrite);
	if (ret == 0)
	{
		for (int i = 0; i < byteToWrite; i++)
		{
			pData[i] = 0;
		}
	}
	return ret;
}
/* ??FLASH??��??????
  *  ?????:��????????byteToWrite
 *  	��????????0
 * */
uint8 writeDataToFLASH(uint32 entry, uint8* pData, uint8 byteToWrite)
{

	uint8 ret = tl_write_to_flash(entry, pData, byteToWrite);
	if (ret == 0)
	{
		for (int i = 0; i < byteToWrite; i++)
		{
			pData[i] = 0;
		}
	}
	return ret;
}


void SendMsgMainFun(void)
{
	uint8 aucMsgBuf[8u] = { 0xAA };
	tUdsId msgId = 0u;
	tUdsLen msgLength = 0u;
	tRxTxCanMsg txMsg;

	if (TRUE == TP_DriverReadDataFromTP(8u, &aucMsgBuf[0u], &msgId, &msgLength))
	{
		txMsg.usRxTxDataId = msgId;
		tl_memcpy(txMsg.aucDataBuf, aucMsgBuf, 8);
		DrvCanSendMessage(&txMsg);
		CANTP_DoTxMsgSuccessfulCallBack();




	}
}


/**
 * @brief ????????? App ???��??Sector 6??
 * @note ??? Flash ?????????????��???��?? 0xA55A??
 */


void readFlagS6(void)
{
	uint16* p16;
	uint8* p;
	uint8 flag[2] = { 0x5a,0xa5 };
	uint8 r = 0;
	uint32 i;

	if ((*(uint32*) FL_APP_Update_FLAG_Addr) == 0)
	{
		Flash_BackupAppBlocks();
	}

	p16 = (uint16*) FL_APP_FLAG_Addr;
	if (*p16 == 0)
	{
		flag[0] = (uint8) FL_APP_FLAG;
		flag[1] = (uint8) (FL_APP_FLAG >> 8);
		uint32 data[8];
		data[0] = *(uint32*) FL_APP_FLAG_Addr;
		if (FL_APP_FLAG == data[0])
		{
			r = 0;
		}
		else
		{

			Flash_erasePFlash_port(FL_APP_FLAG_Addr);

			for (i = 0;i < 8;i++)
			{
				data[i] = 0;
			}
			data[0] = FL_APP_FLAG;

			Flash_writePFlash_port(FL_APP_FLAG_Addr, data, PFLASH_PAGE_LENGTH);


			if (FL_APP_FLAG == *(uint32*) FL_APP_FLAG_Addr)
			{
				r = 0;
			}
			else
			{
				r = 3;
			}
		}
	}
	else
	{
		r = 2;
	}
	if (r == 0)
	{

		p = (uint8*) RAM_BOOT_MODE_Addr;
	}
	else
	{

	}
}


void readFlag(void)
{
	uint32 flashFlagAddr = 0xa0020020;
	uint16 currentFlag = *(uint16*) flashFlagAddr;


	if (currentFlag == FL_APP_FLAG)
	{
		return;
	}
	else
	{

		Flash_erasePFlash_port(flashFlagAddr);


		uint32 pageData1[8] = { 0 };

		pageData1[0] = 0xF8004091;
		pageData1[1] = 0x3422FFD9;
		pageData1[2] = 0x90000FDC;

		Flash_writePFlash_port(flashFlagAddr, pageData1, PFLASH_PAGE_LENGTH);


		currentFlag = *(uint16*) flashFlagAddr;
	}
}

/**
 * @brief UDS Tx callback: jump to active APP bank after positive response sent.
 * @note  Called when 0x31 02 jumpToApp response is successfully transmitted.
 */
static void DoJumpToActiveBank(uint8 status)
{
	if (TX_MSG_SUCCESSFUL == status)
	{
		Boot_DualBank_JumpToBank(Boot_DualBank_GetActiveBank());
	}
}

#ifdef DIAGNOSTIC_MODE_FOR_APP
/**
 * @brief UDS Tx callback: trigger soft reset to enter bootloader after positive response sent.
 * @note  Called when 0x10 02 session switch response is successfully transmitted.
 */
static void DoResetToBootloader(uint8 status)
{
	if (TX_MSG_SUCCESSFUL == status)
	{
		SW_Reset();
	}
}
#endif

static void DigSession0x10(struct UDSServiceInfo* i_pstUDSServiceInfo,
	tUdsAppMsgInfo* m_pstPDUMsg)
{
	uint8 RequestSubfunction = 0u;
	RequestSubfunction = m_pstPDUMsg->aDataBuf[1u];





	switch (RequestSubfunction)
	{
		case 0x01u:
			m_pstPDUMsg->aDataBuf[0u] = i_pstUDSServiceInfo->SerNum + 0x40u;
			m_pstPDUMsg->aDataBuf[1u] = RequestSubfunction;
			m_pstPDUMsg->xDataLen = 2u;
			SetCurrentSession(DEFALUT_SESSION);
			SetSecurityLevel(NONE_SECURITY);
			break;
		case 0x81u:
			SetCurrentSession(DEFALUT_SESSION);
			SetSecurityLevel(NONE_SECURITY);
			if (0x81u == RequestSubfunction)
			{
				m_pstPDUMsg->xDataLen = 0u;
			}

			break;
		case 0x02u:
			if (TRUE != IsCurSecurityLevelRequet(SECURITY_LEVEL_1))
			{
				SetNegativeErroCode(i_pstUDSServiceInfo->SerNum, NRC_SECURITY_ACCESS_DENIED, m_pstPDUMsg);
				break;
			}
			else
			{
				m_pstPDUMsg->aDataBuf[0u] = i_pstUDSServiceInfo->SerNum + 0x40u;
				m_pstPDUMsg->aDataBuf[1u] = RequestSubfunction;
				m_pstPDUMsg->xDataLen = 2u;
				SetCurrentSession(PROGRAM_SESSION);
#ifdef DIAGNOSTIC_MODE_FOR_APP
				/* APP mode: set bootloader flag and reset after positive response is sent */
				* (uint16*) RAM_BOOT_MODE_Addr = RAM_BOOT_MODE_APP;
				m_pstPDUMsg->pfUDSTxMsgServiceCallBack = &DoResetToBootloader;
#endif
			}
			break;
		case 0x82u:
			SetCurrentSession(PROGRAM_SESSION);

			if (0x82u == RequestSubfunction)
			{
				m_pstPDUMsg->xDataLen = 0u;
			}


			break;
		case 0x03u:
			m_pstPDUMsg->aDataBuf[0u] = i_pstUDSServiceInfo->SerNum + 0x40u;
			m_pstPDUMsg->aDataBuf[1u] = RequestSubfunction;
			m_pstPDUMsg->xDataLen = 2u;
			SetCurrentSession(EXTEND_SESSION);
			break;
		case 0x83u:
			SetCurrentSession(EXTEND_SESSION);

			if (0x83u == RequestSubfunction)
			{
				m_pstPDUMsg->xDataLen = 0u;
			}




			break;

		default:
			SetNegativeErroCode(i_pstUDSServiceInfo->SerNum, NRC_SUBFUNCTION_NOT_SUPPORTED, m_pstPDUMsg);
			break;
	}
}



/**
 * @brief UDS Tx callback: perform hard reset after positive response sent.
 */
static void DoHardReset(uint8 status)
{
	if (TX_MSG_SUCCESSFUL == status)
	{
		HardReset();
	}
}

/**
 * @brief UDS Tx callback: perform soft reset after positive response sent.
 */
static void DoSoftReset(uint8 status)
{
	if (TX_MSG_SUCCESSFUL == status)
	{
		SW_Reset();
	}
}

static void DoResetMCU0x11(struct UDSServiceInfo* i_pstUDSServiceInfo,
	tUdsAppMsgInfo* m_pstPDUMsg)
{

	uint8 RequestSubfunction = m_pstPDUMsg->aDataBuf[1u];

	m_pstPDUMsg->aDataBuf[0u] = i_pstUDSServiceInfo->SerNum + 0x40u;
	m_pstPDUMsg->aDataBuf[1u] = RequestSubfunction;



	switch (RequestSubfunction)
	{
		case RESET_NONE:
			break;
		case HARD_RESET:
			m_pstPDUMsg->pfUDSTxMsgServiceCallBack = &DoHardReset;
			break;
		case SOFT_RESET:
			m_pstPDUMsg->pfUDSTxMsgServiceCallBack = &DoSoftReset;
			break;
		default:
			SetNegativeErroCode(i_pstUDSServiceInfo->SerNum, NRC_SUBFUNCTION_NOT_SUPPORTED, m_pstPDUMsg);
			break;
	}


}


static void SecurityAccess0x27(struct UDSServiceInfo* i_pstUDSServiceInfo,
	tUdsAppMsgInfo* m_pstPDUMsg)
{

	uint8 RequestSubfunction = m_pstPDUMsg->aDataBuf[1u];
	static uint8 s_aSeedBuf[SA_ALGORITHM_SEED_LEN] = { 0u };
	static uint8 s_securityAttemptCnt = 0u;
	static const uint8 MAX_SECURITY_ATTEMPTS = 3u;
	uint8 ret = FALSE;

	/* Check if security access is currently locked */
	if (GetUdsSecurityReqLockTime() > 0)
	{
		SetNegativeErroCode(i_pstUDSServiceInfo->SerNum, NRC_REQUIRED_TIME_DELAY_NOT_EXPIRED, m_pstPDUMsg);
		return;
	}



	switch (RequestSubfunction)
	{
		case 1:
			m_pstPDUMsg->aDataBuf[0u] = i_pstUDSServiceInfo->SerNum + 0x40u;

			ret = UDS_ALG_HAL_GetRandom(SA_ALGORITHM_SEED_LEN, s_aSeedBuf);

			if (TRUE == ret)
			{
				fsl_memcpy(&m_pstPDUMsg->aDataBuf[2u], s_aSeedBuf, SA_ALGORITHM_SEED_LEN);
				m_pstPDUMsg->xDataLen = 2u + SA_ALGORITHM_SEED_LEN;
			}
			else
			{
				SetNegativeErroCode(i_pstUDSServiceInfo->SerNum, NRC_INVALID_KEY, m_pstPDUMsg);
			}

			break;

		case 0x02u:

			if (TRUE == IsReceivedKeyRight(&m_pstPDUMsg->aDataBuf[2u], s_aSeedBuf, 1u))
			{
				m_pstPDUMsg->aDataBuf[0u] = i_pstUDSServiceInfo->SerNum + 0x40u;
				m_pstPDUMsg->xDataLen = 2u;
				fsl_memset(s_aSeedBuf, 0x1u, sizeof(s_aSeedBuf));
				s_securityAttemptCnt = 0u;
				SetSecurityLevel(SECURITY_LEVEL_1);
			}
			else
			{
				s_securityAttemptCnt++;
				if (s_securityAttemptCnt >= MAX_SECURITY_ATTEMPTS)
				{
					gs_stUdsInfo.xSecurityReqLockTime = UdsAppTimeToCount(10000u);
					s_securityAttemptCnt = 0u;
					SetNegativeErroCode(i_pstUDSServiceInfo->SerNum, NRC_EXCEEDED_NUMBER_OF_ATTEMPTS, m_pstPDUMsg);
				}
				else
				{
					SetNegativeErroCode(i_pstUDSServiceInfo->SerNum, NRC_INVALID_KEY, m_pstPDUMsg);
				}
			}

			break;
		case 0x03u:
			m_pstPDUMsg->aDataBuf[0u] = i_pstUDSServiceInfo->SerNum + 0x40u;

			ret = UDS_ALG_HAL_GetRandom(SA_ALGORITHM_SEED_LEN, s_aSeedBuf);

			if (TRUE == ret)
			{
				fsl_memcpy(&m_pstPDUMsg->aDataBuf[2u], s_aSeedBuf, SA_ALGORITHM_SEED_LEN);
				m_pstPDUMsg->xDataLen = 2u + SA_ALGORITHM_SEED_LEN;
			}
			else
			{
				SetNegativeErroCode(i_pstUDSServiceInfo->SerNum, NRC_INVALID_KEY, m_pstPDUMsg);
			}

			break;

		case 0x04u:

			if (TRUE == IsReceivedKeyRight(&m_pstPDUMsg->aDataBuf[2u], s_aSeedBuf, 2u))
			{
				m_pstPDUMsg->aDataBuf[0u] = i_pstUDSServiceInfo->SerNum + 0x40u;
				m_pstPDUMsg->xDataLen = 2u;
				fsl_memset(s_aSeedBuf, 0x1u, sizeof(s_aSeedBuf));
				s_securityAttemptCnt = 0u;
				SetSecurityLevel(SECURITY_LEVEL_2);
			}
			else
			{
				s_securityAttemptCnt++;
				if (s_securityAttemptCnt >= MAX_SECURITY_ATTEMPTS)
				{
					gs_stUdsInfo.xSecurityReqLockTime = UdsAppTimeToCount(10000u);
					s_securityAttemptCnt = 0u;
					SetNegativeErroCode(i_pstUDSServiceInfo->SerNum, NRC_EXCEEDED_NUMBER_OF_ATTEMPTS, m_pstPDUMsg);
				}
				else
				{
					SetNegativeErroCode(i_pstUDSServiceInfo->SerNum, NRC_INVALID_KEY, m_pstPDUMsg);
				}
			}

			break;
		default:
			SetNegativeErroCode(i_pstUDSServiceInfo->SerNum, NRC_SUBFUNCTION_NOT_SUPPORTED, m_pstPDUMsg);
			break;
	}
}

static void CommunicationControl0x28(struct UDSServiceInfo* i_pstUDSServiceInfo,
	tUdsAppMsgInfo* m_pstPDUMsg)
{
	uint8 RequestSubfunction = 0u;
	uint8 communicationType = 0u;
	RequestSubfunction = m_pstPDUMsg->aDataBuf[1u];
	communicationType = m_pstPDUMsg->aDataBuf[2u];

	if (communicationType != 0x03u)
	{
		SetNegativeErroCode(i_pstUDSServiceInfo->SerNum, NRC_REQUEST_OUT_OF_RANGE, m_pstPDUMsg);
		return;
	}

	m_pstPDUMsg->aDataBuf[0u] = i_pstUDSServiceInfo->SerNum + 0x40u;
	m_pstPDUMsg->aDataBuf[1u] = RequestSubfunction;
	m_pstPDUMsg->xDataLen = 2u;








	switch (RequestSubfunction)
	{
		case UDS_CC_MODE_RX_TX:
			g_CanMsgCommCtrlMode = UDS_CC_MODE_RX_TX;
			break;
		case UDS_CC_MODE_RX_NO:
			g_CanMsgCommCtrlMode = UDS_CC_MODE_RX_NO;
			break;
		case UDS_CC_MODE_NO_TX:
			g_CanMsgCommCtrlMode = UDS_CC_MODE_NO_TX;
			break;
		case UDS_CC_MODE_NO_NO:
			g_CanMsgCommCtrlMode = UDS_CC_MODE_NO_NO;
			break;
		default:
			SetNegativeErroCode(i_pstUDSServiceInfo->SerNum, NRC_SUBFUNCTION_NOT_SUPPORTED, m_pstPDUMsg);
			break;
	}
	DrvCanRxTxModeSet(g_CanMsgCommCtrlMode);

}
static void ReadDataByAddress0x23(struct UDSServiceInfo* i_pstUDSServiceInfo,
	tUdsAppMsgInfo* m_pstPDUMsg)
{
	uint32 u32Addr = 0;
	uint32 u32DataLength = 0;
	uint8 u8AddrBytes, u8DataBytes;
	uint8 Index = 0u;

	if (m_pstPDUMsg->xDataLen < 2)
	{
		SetNegativeErroCode(i_pstUDSServiceInfo->SerNum, NRC_INVALID_MESSAGE_LENGTH_OR_FORMAT, m_pstPDUMsg);
		return;
	}

	u8AddrBytes = m_pstPDUMsg->aDataBuf[1u] & 0x0Fu;
	u8DataBytes = (m_pstPDUMsg->aDataBuf[1u] & 0xF0u) >> 4u;

	if ((u8AddrBytes == 0u) || (u8AddrBytes > 4u) || (u8DataBytes == 0u) || (u8DataBytes > 4u))
	{
		SetNegativeErroCode(i_pstUDSServiceInfo->SerNum, NRC_REQUEST_OUT_OF_RANGE, m_pstPDUMsg);
		return;
	}

	if (m_pstPDUMsg->xDataLen < (2u + u8AddrBytes + u8DataBytes))
	{
		SetNegativeErroCode(i_pstUDSServiceInfo->SerNum, NRC_INVALID_MESSAGE_LENGTH_OR_FORMAT, m_pstPDUMsg);
		return;
	}

	for (Index = 0u; Index < u8AddrBytes; Index++)
	{
		u32Addr <<= 8u;
		u32Addr |= m_pstPDUMsg->aDataBuf[Index + 2u];
	}

	for (Index = 0u; Index < u8DataBytes; Index++)
	{
		u32DataLength <<= 8u;
		u32DataLength |= m_pstPDUMsg->aDataBuf[Index + 2u + u8AddrBytes];
	}

	if (CAN_SSN_checkMemoryAddrAndSize(u32Addr, u32DataLength) != 0)
	{
		SetNegativeErroCode(i_pstUDSServiceInfo->SerNum, NRC_REQUEST_OUT_OF_RANGE, m_pstPDUMsg);
		return;
	}

	m_pstPDUMsg->aDataBuf[0] = i_pstUDSServiceInfo->SerNum + 0x40;
	uint8* pu8DataPtr = (uint8*) u32Addr;

	for (uint32 i = 0; i < u32DataLength; i++)
	{
		m_pstPDUMsg->aDataBuf[1 + i] = *pu8DataPtr;
		pu8DataPtr++;
	}

	m_pstPDUMsg->xDataLen = (uint16) u32DataLength + 1;
}



static void ReadDataByIdentifier0x22(struct UDSServiceInfo* i_pstUDSServiceInfo,
	tUdsAppMsgInfo* m_pstPDUMsg)
{
	uint16 did;
	uint8 not_find_did = 0;
	m_pstPDUMsg->aDataBuf[0u] = i_pstUDSServiceInfo->SerNum + 0x40u;
	did = (m_pstPDUMsg->aDataBuf[1u] << 8) | m_pstPDUMsg->aDataBuf[2u];

	if (m_pstPDUMsg->xDataLen < 3)
	{
		SetNegativeErroCode(i_pstUDSServiceInfo->SerNum, NRC_INVALID_MESSAGE_LENGTH_OR_FORMAT, m_pstPDUMsg);
		return;
	}

	for (int i = 0; i < sizeof(g_rwDataTable) / sizeof(g_rwDataTable[0]); i++)
	{
		if (g_rwDataTable[i].did == did)
		{
			uint8 dataLen = g_rwDataTable[i].dlc;
			if (dataLen > g_rwDataTable[i].dlc_max)
			{
				dataLen = g_rwDataTable[i].dlc_max;
			}
			m_pstPDUMsg->aDataBuf[1u] = (did & 0xFF00) >> 8;
			m_pstPDUMsg->aDataBuf[2u] = did & 0xFF;
			for (uint8 j = 0; j < dataLen; j++)
			{
				m_pstPDUMsg->aDataBuf[3u + j] = *((uint8*)g_rwDataTable[i].p_entry + j);
			}
			m_pstPDUMsg->xDataLen = 3u + dataLen;
			return;
		}
		not_find_did++;
	}

	if (not_find_did)
	{
		SetNegativeErroCode(i_pstUDSServiceInfo->SerNum, NRC_REQUEST_OUT_OF_RANGE, m_pstPDUMsg);
	}
	else
	{
		SetNegativeErroCode(i_pstUDSServiceInfo->SerNum, NRC_CONDITIONS_NOT_CORRECT, m_pstPDUMsg);
	}
}


static void WriteDataByIdentifier0x2E(struct UDSServiceInfo* i_pstUDSServiceInfo,
	tUdsAppMsgInfo* m_pstPDUMsg)
{
	uint16 did;
	uint8 not_find_did = 0;
	m_pstPDUMsg->aDataBuf[0u] = i_pstUDSServiceInfo->SerNum + 0x40u;
	did = (m_pstPDUMsg->aDataBuf[1u] << 8) | m_pstPDUMsg->aDataBuf[2u];


	if (m_pstPDUMsg->xDataLen < 4)
	{
		SetNegativeErroCode(i_pstUDSServiceInfo->SerNum, NRC_INVALID_MESSAGE_LENGTH_OR_FORMAT, m_pstPDUMsg);
		return;
	}


	for (int i = 0;i < sizeof(g_rwDataTable) / sizeof(g_rwDataTable[0]);i++)
	{
		if (g_rwDataTable[i].did == did)
		{
			if ((m_pstPDUMsg->xDataLen - 3) > g_rwDataTable[i].dlc_max)
			{
				SetNegativeErroCode(i_pstUDSServiceInfo->SerNum, NRC_INVALID_MESSAGE_LENGTH_OR_FORMAT, m_pstPDUMsg);
				return;
			}

			if (g_rwDataTable[i].rw_mode == UDS_RWDATA_RDONLY)
			{
				SetNegativeErroCode(i_pstUDSServiceInfo->SerNum, NRC_CONDITIONS_NOT_CORRECT, m_pstPDUMsg);
				return;
			}

			if (g_rwDataTable[i].rw_store == UDS_RWDATA_DFLASH)
			{

				if (writeDataToFLASH(g_rwDataTable[i].p_entry, &m_pstPDUMsg->aDataBuf[3], m_pstPDUMsg->xDataLen - 3) == 0)
				{
					SetNegativeErroCode(i_pstUDSServiceInfo->SerNum, NRC_GENERAL_PROGRAMMING_FAILURE, m_pstPDUMsg);
					return;
				}

				if (g_rwDataTable[i].rw_mode == UDS_RWDATA_RDWR_WRONCE)
				{
					g_rwDataTable[i].rw_mode = UDS_RWDATA_RDONLY;
				}

				g_rwDataTable[i].dlc = m_pstPDUMsg->xDataLen - 3;
			}
			else if (g_rwDataTable[i].rw_store == UDS_RWDATA_EEPROM)
			{

				if (writeDataToEEPROM(g_rwDataTable[i].p_entry, &m_pstPDUMsg->aDataBuf[3], m_pstPDUMsg->xDataLen - 3) == 0)
				{
					SetNegativeErroCode(i_pstUDSServiceInfo->SerNum, NRC_GENERAL_PROGRAMMING_FAILURE, m_pstPDUMsg);
					return;
				}

				if (g_rwDataTable[i].rw_mode == UDS_RWDATA_RDWR_WRONCE)
				{
					g_rwDataTable[i].rw_mode = UDS_RWDATA_RDONLY;
				}

				g_rwDataTable[i].dlc = m_pstPDUMsg->xDataLen - 3;
			}
			else
			{
				for (int j = 0; j < m_pstPDUMsg->xDataLen - 3; j++)
				{
					*((uint8*) g_rwDataTable[i].p_entry + j) = m_pstPDUMsg->aDataBuf[3 + j];
				}

				g_rwDataTable[i].dlc = m_pstPDUMsg->xDataLen - 3;
			}

			m_pstPDUMsg->aDataBuf[1u] = (did & 0xFF00) >> 8;
			m_pstPDUMsg->aDataBuf[2u] = did & 0xFF;
			m_pstPDUMsg->xDataLen = 3u;

			return;
		}
		not_find_did++;
	}

	if (not_find_did)
	{
		SetNegativeErroCode(i_pstUDSServiceInfo->SerNum, NRC_REQUEST_OUT_OF_RANGE, m_pstPDUMsg);
	}
	else
	{
		SetNegativeErroCode(i_pstUDSServiceInfo->SerNum, NRC_CONDITIONS_NOT_CORRECT, m_pstPDUMsg);
	}
}


static void ControlDTCSetting0x85(struct UDSServiceInfo* i_pstUDSServiceInfo, tUdsAppMsgInfo* m_pstPDUMsg)
{
	uint8 flag = 0;
	m_pstPDUMsg->aDataBuf[0u] = i_pstUDSServiceInfo->SerNum + 0x40u;
	flag = m_pstPDUMsg->aDataBuf[1u];
	if (m_pstPDUMsg->xDataLen < 2)
	{
		SetNegativeErroCode(i_pstUDSServiceInfo->SerNum, NRC_INVALID_MESSAGE_LENGTH_OR_FORMAT, m_pstPDUMsg);
		return;
	}
	if (flag == 0x01)
	{

		m_pstPDUMsg->aDataBuf[1u] = 0x01;
		m_pstPDUMsg->xDataLen = 2;
		isDtcStatuCanUpdate = ON;
	}
	else if (flag == 0x02)
	{

		m_pstPDUMsg->aDataBuf[1u] = 0x02;
		m_pstPDUMsg->xDataLen = 2;
		isDtcStatuCanUpdate = OFF;
	}
	else
	{
		SetNegativeErroCode(i_pstUDSServiceInfo->SerNum, NRC_CONDITIONS_NOT_CORRECT, m_pstPDUMsg);
	}
}


/*
 * ?????app???????-250512
 * */
#define APP1_START  0xA0020000
#define APP1_END    0xA00FFFFF
#define APP2_START  0xa0100000
#define APP2_END    0xa01fffff




static tDowloadDataInfo gs_stDowloadDataInfo = { 0u, 0u };


static uint32 gs_RxBlockNum = 0u;


static void RequestDownload0x34(struct UDSServiceInfo* i_pstUDSServiceInfo,
	tUdsAppMsgInfo* m_pstPDUMsg)
{

	uint8 Index = 0u;
	uint8 Ret = TRUE;
	uint32  addrBytesLength, dataBytesLength;
	uint32 addrAndDataBytesLength;
	addrAndDataBytesLength = m_pstPDUMsg->aDataBuf[2u];
	addrBytesLength = addrAndDataBytesLength & 0x0f;
	dataBytesLength = (addrAndDataBytesLength & 0xf0) >> 4;


	if (m_pstPDUMsg->xDataLen < (1u + 2u + addrBytesLength + dataBytesLength))
	{
		Ret = FALSE;
		SetNegativeErroCode(i_pstUDSServiceInfo->SerNum, NRC_INVALID_MESSAGE_LENGTH_OR_FORMAT, m_pstPDUMsg);
	}

	if (TRUE == Ret)
	{



		gs_stDowloadDataInfo.StartAddr = 0u;
		for (Index = 0u; Index < addrBytesLength; Index++)

		{
			gs_stDowloadDataInfo.StartAddr <<= 8u;


			gs_stDowloadDataInfo.StartAddr |= m_pstPDUMsg->aDataBuf[Index + 3u];

		}
		gs_stDowloadDataInfo.StartAddr = (gs_stDowloadDataInfo.StartAddr & 0x00FFFFFF) | 0xA0000000;

		/* Dual Bank: determine target bank from download address
		 * StartAddr is uncached (0xA0...), convert back to cached for comparison */
		{
			uint32 cachedAddr = gs_stDowloadDataInfo.StartAddr - 0x20000000u;
			if ((cachedAddr >= BANK_B_START_ADDR) &&
				(cachedAddr < BANK_B_END_ADDR))
			{
				Boot_DualBank_SetTargetWriteBank(BANK_B);
			}
			else
			{
				Boot_DualBank_SetTargetWriteBank(BANK_A);
			}
		}


		if (Boot_DualBank_GetTargetWriteBank() == Boot_DualBank_GetActiveBank())
		{
			/* Never allow flashing the active bank, even if it appears invalid.
			 * The dual-bank design always writes to the INACTIVE bank first,
			 * then switches. This prevents overwriting the firmware we are
			 * currently running from. */
			SetNegativeErroCode(i_pstUDSServiceInfo->SerNum, NRC_CONDITIONS_NOT_CORRECT, m_pstPDUMsg);
			Ret = FALSE;
		}


		gs_stDowloadDataInfo.DataLen = 0u;
		for (Index = 0u; Index < dataBytesLength; Index++)
		{
			gs_stDowloadDataInfo.DataLen <<= 8u;
			gs_stDowloadDataInfo.DataLen |= m_pstPDUMsg->aDataBuf[Index + 3 + addrBytesLength];
		}
	}


	if (((TRUE != IsDownloadDataAddrValid(gs_stDowloadDataInfo.StartAddr)) ||

		(TRUE != IsDownloadDataLenValid(gs_stDowloadDataInfo.DataLen))) && (TRUE == Ret))
	{
		SetNegativeErroCode(i_pstUDSServiceInfo->SerNum, NRC_REQUEST_OUT_OF_RANGE, m_pstPDUMsg);

		Ret = FALSE;
	}

	if (TRUE == Ret)
	{

		Flash_SetNextDownloadStep(FL_TRANSFER_STEP);


		Flash_SaveDownloadDataInfo(gs_stDowloadDataInfo.StartAddr, gs_stDowloadDataInfo.DataLen);



		m_pstPDUMsg->aDataBuf[0u] = i_pstUDSServiceInfo->SerNum + 0x40u;
		m_pstPDUMsg->aDataBuf[1u] = 0x10u;
		m_pstPDUMsg->aDataBuf[2u] = 0x80u;
		m_pstPDUMsg->xDataLen = 3u;


		gs_RxBlockNum = 1;
	}
	else
	{
		Flash_InitDowloadInfo();


		Flash_SetNextDownloadStep(FL_REQUEST_STEP);
	}
}

static void TransferData0x36(struct UDSServiceInfo* i_pstUDSServiceInfo, tUdsAppMsgInfo* m_pstPDUMsg)
{

	uint8 Ret = TRUE;


	if ((FL_TRANSFER_STEP != Flash_GetCurDownloadStep()) && (TRUE == Ret))
	{
		Ret = FALSE;
		SetNegativeErroCode(i_pstUDSServiceInfo->SerNum, NRC_REQUEST_SEQUENCE_ERROR, m_pstPDUMsg);
	}

	/* Verify sequence number (SN) per UDS specification */
	{
		uint8 rxSN = m_pstPDUMsg->aDataBuf[1u];
		if (rxSN != gs_RxBlockNum)
		{
			Ret = FALSE;
			SetNegativeErroCode(i_pstUDSServiceInfo->SerNum, NRC_REQUEST_SEQUENCE_ERROR, m_pstPDUMsg);
			Flash_InitDowloadInfo();
			Flash_SetNextDownloadStep(FL_REQUEST_STEP);
			gs_RxBlockNum = 0u;
		}
		else
		{
			gs_RxBlockNum++;
			if (gs_RxBlockNum > 0xFFu)
			{
				gs_RxBlockNum = 0u;
			}
		}
	}

	uint8 actualDataLen = m_pstPDUMsg->xDataLen - 2;




	if (TRUE != Flash_ProgramRegion(gs_stDowloadDataInfo.StartAddr,
		&m_pstPDUMsg->aDataBuf[2],
		actualDataLen)
		&& (TRUE == Ret))
	{
		Ret = FALSE;

		SetNegativeErroCode(i_pstUDSServiceInfo->SerNum, NRC_CONDITIONS_NOT_CORRECT, m_pstPDUMsg);
	}
	else
	{






		gs_stDowloadDataInfo.StartAddr += actualDataLen;
		gs_stDowloadDataInfo.DataLen -= actualDataLen;
	}

	if ((0u == gs_stDowloadDataInfo.DataLen) && (TRUE == Ret))
	{

		gs_RxBlockNum = 0u;

		Flash_SetNextDownloadStep(FL_EXIT_TRANSFER_STEP);
	}

	if (TRUE == Ret)
	{


		m_pstPDUMsg->aDataBuf[0u] = i_pstUDSServiceInfo->SerNum + 0x40u;
		m_pstPDUMsg->xDataLen = 4u;

	}
	else
	{
		Flash_InitDowloadInfo();


		Flash_SetNextDownloadStep(FL_REQUEST_STEP);
		gs_RxBlockNum = 0u;
	}
}


static void RequestTransferExit0x37(struct UDSServiceInfo* i_pstUDSServiceInfo,
	tUdsAppMsgInfo* m_pstPDUMsg)
{

	uint8 Ret = TRUE;

	if (FL_EXIT_TRANSFER_STEP != Flash_GetCurDownloadStep())
	{
		Ret = FALSE;

		SetNegativeErroCode(i_pstUDSServiceInfo->SerNum, NRC_REQUEST_SEQUENCE_ERROR, m_pstPDUMsg);
	}

	if (TRUE == Ret)
	{
		Flash_SetNextDownloadStep(FL_CHECKSUM_STEP);



		m_pstPDUMsg->aDataBuf[0u] = i_pstUDSServiceInfo->SerNum + 0x40u;
		m_pstPDUMsg->xDataLen = 1u;
	}
	else
	{
		Flash_InitDowloadInfo();
	}
}


// ����������ľ���ʵ��?
// ����ֵ: ��8λ = canFlash (1=��, 0=����), ��8λ = targetBank ('A' �� 'B')
uint16 CheckProgrammingConditions(void) {
	uint8 canFlash = 1;
	uint8 targetBankChar;
	uint32 targetWriteBank = Boot_DualBank_GetTargetWriteBank();

	if (targetWriteBank == BANK_B)
	{
		targetBankChar = 0x0B;
	}
	else
	{
		targetBankChar = 0x0A;
	}
	return ((uint16) canFlash << 8) | (uint16) targetBankChar;
}

static void RoutineControl0x31(struct UDSServiceInfo* i_pstUDSServiceInfo, tUdsAppMsgInfo* m_pstPDUMsg)
{
	uint8 subFunc = m_pstPDUMsg->aDataBuf[1];
	uint16 routineIdentifier;
	static uint8 currentRoutine = 0;
	static uint16 routineResult = 0;
	uint8* p;

	if (m_pstPDUMsg->xDataLen < 2)
	{
		SetNegativeErroCode(i_pstUDSServiceInfo->SerNum, NRC_INVALID_MESSAGE_LENGTH_OR_FORMAT, m_pstPDUMsg);
		return;
	}

	switch (subFunc)
	{
		case 0x01:
			{
				Flash_ForceWriteRemaining();

				if (m_pstPDUMsg->xDataLen < 4)
				{
					SetNegativeErroCode(i_pstUDSServiceInfo->SerNum, NRC_INVALID_MESSAGE_LENGTH_OR_FORMAT, m_pstPDUMsg);
					break;
				}

				routineIdentifier = (m_pstPDUMsg->aDataBuf[2] << 8) | m_pstPDUMsg->aDataBuf[3];

				switch (routineIdentifier)
				{
					// #ifdef DIAGNOSTIC_MODE_FOR_APP
					case 0xFFFD:
						if (TRUE != IsCurSecurityLevelRequet(SECURITY_LEVEL_1))
						{
							SetNegativeErroCode(i_pstUDSServiceInfo->SerNum, NRC_SECURITY_ACCESS_DENIED, m_pstPDUMsg);
							break;
						}
						if (m_pstPDUMsg->xDataLen < 4)
						{
							SetNegativeErroCode(i_pstUDSServiceInfo->SerNum, NRC_INVALID_MESSAGE_LENGTH_OR_FORMAT, m_pstPDUMsg);
							break;
						}
						routineResult = CheckProgrammingConditions();
						m_pstPDUMsg->aDataBuf[0] = 0x71;
						m_pstPDUMsg->aDataBuf[1] = 0x01;
						m_pstPDUMsg->aDataBuf[2] = 0xFF;
						m_pstPDUMsg->aDataBuf[3] = 0xFD;
						m_pstPDUMsg->aDataBuf[4] = (uint8) (routineResult >> 8);   /* canFlash: 1=��, 0=���� */

						m_pstPDUMsg->aDataBuf[5] = (uint8) (routineResult & 0xFF); /* targetBank: 'A' �� 'B' */
						m_pstPDUMsg->xDataLen = 6;
						break;
						// #endif	
					// erase flash
										// erase flash
					case 0xFF00:
						if (TRUE != IsCurSecurityLevelRequet(SECURITY_LEVEL_2))
						{
							SetNegativeErroCode(i_pstUDSServiceInfo->SerNum, NRC_SECURITY_ACCESS_DENIED, m_pstPDUMsg);
							break;
						}

						if (m_pstPDUMsg->xDataLen < 6)
						{
							SetNegativeErroCode(i_pstUDSServiceInfo->SerNum, NRC_INVALID_MESSAGE_LENGTH_OR_FORMAT, m_pstPDUMsg);
							break;
						}

						/* Erase the requested sector only if it belongs to the currently
						 * selected target bank.  g_udsTargetBank is set during the
						 * preceding RequestDownload (0x34) service based on the download
						 * start address.  This prevents accidental erasure of:
						 *   - Bootloader sectors (S0 ~ S7)
						 *   - The inactive bank (e.g. erasing Bank A while updating Bank B)
						 */
						routineResult = EraseFlashSector(m_pstPDUMsg->aDataBuf[4], m_pstPDUMsg->aDataBuf[5]);

						if (routineResult == 0xFE)
						{
							SetNegativeErroCode(i_pstUDSServiceInfo->SerNum, NRC_REQUEST_OUT_OF_RANGE, m_pstPDUMsg);
							break;
						}
						if (routineResult == 0xFC)
						{
							/* Bootloader / reserved area protection */
							SetNegativeErroCode(i_pstUDSServiceInfo->SerNum, NRC_SECURITY_ACCESS_DENIED, m_pstPDUMsg);
							break;
						}
						if (routineResult == 0xFD)
						{
							/* Sector does not belong to the target bank selected by 0x34 */
							SetNegativeErroCode(i_pstUDSServiceInfo->SerNum, NRC_REQUEST_SEQUENCE_ERROR, m_pstPDUMsg);
							break;
						}
						if (routineResult == 0x00)
						{
							SetNegativeErroCode(i_pstUDSServiceInfo->SerNum, NRC_GENERAL_PROGRAMMING_FAILURE, m_pstPDUMsg);
							break;
						}

						/* Positive response: 71 01 FF 00 <sector> <result> */
						m_pstPDUMsg->aDataBuf[0] = 0x71;
						m_pstPDUMsg->aDataBuf[1] = 0x01;
						m_pstPDUMsg->aDataBuf[4] = m_pstPDUMsg->aDataBuf[4] << 8 | m_pstPDUMsg->aDataBuf[5];
						m_pstPDUMsg->aDataBuf[5] = (uint8) routineResult;
						m_pstPDUMsg->xDataLen = 6;
						break;

					case 0xFF01:
						/* CheckProgrammingDependency routine - not supported in this implementation */
						SetNegativeErroCode(i_pstUDSServiceInfo->SerNum, NRC_SUBFUNCTION_NOT_SUPPORTED, m_pstPDUMsg);
						break;

					case 0xDFFF:
						{
							if (TRUE != IsCurSecurityLevelRequet(SECURITY_LEVEL_2))
							{
								SetNegativeErroCode(i_pstUDSServiceInfo->SerNum, NRC_SECURITY_ACCESS_DENIED, m_pstPDUMsg);
								break;
							}

							/* Expect: SID(1) + subFunc(1) + RID(2) + CRC32(4) = 8 bytes */
							if (m_pstPDUMsg->xDataLen < 8)
							{
								SetNegativeErroCode(i_pstUDSServiceInfo->SerNum, NRC_INVALID_MESSAGE_LENGTH_OR_FORMAT, m_pstPDUMsg);
								break;
							}

							{
								uint32 expectedCRC;
								uint32 actualCRC;
								uint32 startAddr;
								uint32 bankSize;
								uint32 targetBank = Boot_DualBank_GetTargetWriteBank();

								/* Parse expected CRC32 from tester (big-endian) */
								expectedCRC = ((uint32) m_pstPDUMsg->aDataBuf[4] << 24) |
									((uint32) m_pstPDUMsg->aDataBuf[5] << 16) |
									((uint32) m_pstPDUMsg->aDataBuf[6] << 8) |
									((uint32) m_pstPDUMsg->aDataBuf[7]);

								if (targetBank == BANK_A)
								{
									startAddr = BANK_A_START_ADDR;
									bankSize = BANK_APP_A_SIZE;
								}
								else
								{
									startAddr = BANK_B_START_ADDR;
									bankSize = BANK_APP_B_SIZE;
								}

								/* Calculate CRC over actual Flash contents */
								actualCRC = Boot_DualBank_CalculateCRC(startAddr, bankSize);

								if (actualCRC != expectedCRC)
								{
									/* CRC mismatch: do not mark valid */
									routineResult = 0x00;
									m_pstPDUMsg->aDataBuf[0] = 0x71;
									m_pstPDUMsg->aDataBuf[1] = 0x01;
									m_pstPDUMsg->aDataBuf[2] = 0xDF;
									m_pstPDUMsg->aDataBuf[3] = 0xFF;
									m_pstPDUMsg->aDataBuf[4] = (uint8) routineResult;
									m_pstPDUMsg->xDataLen = 5;
									break;
								}

								/* CRC OK: mark valid and activate */
								Boot_DualBank_MarkBankValid(targetBank, 0x00010000u);
								Boot_DualBank_SetActiveBank(targetBank);
								routineResult = 0x01;
							}

							m_pstPDUMsg->aDataBuf[0] = 0x71;
							m_pstPDUMsg->aDataBuf[1] = 0x01;
							m_pstPDUMsg->aDataBuf[2] = 0xDF;
							m_pstPDUMsg->aDataBuf[3] = 0xFF;
							m_pstPDUMsg->aDataBuf[4] = (uint8) routineResult;
							m_pstPDUMsg->xDataLen = 5;
							break;
						}

					default:
						SetNegativeErroCode(i_pstUDSServiceInfo->SerNum, NRC_SUBFUNCTION_NOT_SUPPORTED, m_pstPDUMsg);
						break;
				}
				break;
			}

		case 0x02:
			currentRoutine = m_pstPDUMsg->aDataBuf[2];
			if (currentRoutine == 0)
			{
				SetNegativeErroCode(i_pstUDSServiceInfo->SerNum, NRC_CONDITIONS_NOT_CORRECT, m_pstPDUMsg);
				break;
			}
			if (m_pstPDUMsg->xDataLen < 3)
			{
				SetNegativeErroCode(i_pstUDSServiceInfo->SerNum, NRC_INVALID_MESSAGE_LENGTH_OR_FORMAT, m_pstPDUMsg);
				break;
			}

			switch (currentRoutine)
			{

				case jumpToApp:
					{
						if (TRUE != IsCurSecurityLevelRequet(SECURITY_LEVEL_2))
						{
							SetNegativeErroCode(i_pstUDSServiceInfo->SerNum, NRC_SECURITY_ACCESS_DENIED, m_pstPDUMsg);
							break;
						}
						m_pstPDUMsg->aDataBuf[0] = 0x71;
						m_pstPDUMsg->aDataBuf[1] = 0x02;
						m_pstPDUMsg->aDataBuf[2] = jumpToApp;
						m_pstPDUMsg->xDataLen = 3;


						m_pstPDUMsg->pfUDSTxMsgServiceCallBack = &DoJumpToActiveBank;
						break;
					}
#if 0  
					* (uint16*) RAM_BOOT_MODE_Addr = RAM_BOOT_MODE_NORMAL;
					p = (uint8*) RAM_BOOT_MODE_Addr;
					m_pstPDUMsg->aDataBuf[0] = 0x71;
					m_pstPDUMsg->aDataBuf[1] = 0x02;
					m_pstPDUMsg->aDataBuf[2] = jumpToApp;
					m_pstPDUMsg->aDataBuf[3] = p[1];
					m_pstPDUMsg->aDataBuf[4] = p[2];
					m_pstPDUMsg->xDataLen = 5;

					break;
#endif
				case jumpToBL:
					*(uint16*) RAM_BOOT_MODE_Addr = RAM_BOOT_MODE_APP;
					p = (uint8*) RAM_BOOT_MODE_Addr;
					m_pstPDUMsg->aDataBuf[0] = 0x71;
					m_pstPDUMsg->aDataBuf[1] = 0x02;
					m_pstPDUMsg->aDataBuf[2] = jumpToBL;
					m_pstPDUMsg->aDataBuf[3] = p[1];
					m_pstPDUMsg->aDataBuf[4] = p[2];
					m_pstPDUMsg->xDataLen = 5;
					break;
			}
			break;

		case 0x03:
			if (currentRoutine == 0)
			{
				SetNegativeErroCode(i_pstUDSServiceInfo->SerNum, NRC_CONDITIONS_NOT_CORRECT, m_pstPDUMsg);
				break;
			}

			m_pstPDUMsg->aDataBuf[0] = 0x71;
			m_pstPDUMsg->aDataBuf[1] = 0x03;
			m_pstPDUMsg->aDataBuf[2] = (routineResult >> 24) & 0xFF;
			m_pstPDUMsg->aDataBuf[3] = (routineResult >> 16) & 0xFF;
			m_pstPDUMsg->aDataBuf[4] = (routineResult >> 8) & 0xFF;
			m_pstPDUMsg->aDataBuf[5] = routineResult & 0xFF;
			m_pstPDUMsg->xDataLen = 6;
			break;

		default:
			SetNegativeErroCode(i_pstUDSServiceInfo->SerNum, NRC_SUBFUNCTION_NOT_SUPPORTED, m_pstPDUMsg);
			break;
	}
}



/**
 * @brief Check if a logical PFlash sector belongs to the current target bank.
 * @param sector Logical sector index (0 ~ IFXFLASH_PFLASH_NUM_LOG_SECTORS-1)
 * @return TRUE if the sector is within the target bank's allowed range
 */
static boolean IsSectorInTargetBank(uint16 sector)
{
	if (Boot_DualBank_GetTargetWriteBank() == BANK_B)
	{
		return (sector >= BANK_B_SECTOR_START) && (sector <= BANK_B_SECTOR_END);
	}
	else /* BANK_A */
	{
		return (sector >= BANK_A_SECTOR_START) && (sector <= BANK_A_SECTOR_END);
	}
}

/**
 * @brief Erase a single PFlash sector with target-bank and bootloader protection.
 * @param blockHigh High byte of sector number
 * @param blockLow  Low byte of sector number
 * @return 0x01 : success
 *         0x00 : flash erase error
 *         0xFE : sector number out of range
 *         0xFC : sector is in bootloader/reserved area (forbidden)
 *         0xFD : sector is not in the current target bank
 */
static uint16 EraseFlashSector(uint8 blockHigh, uint8 blockLow)
{
	uint16 blockNum = (blockHigh << 8) | blockLow;

	/* 1. Check valid sector range */
	if (blockNum >= IFXFLASH_PFLASH_NUM_LOG_SECTORS)
	{
		return 0xFE;
	}

	/* 2. Protect bootloader / reserved sectors (S0 ~ S7) */
	if (blockNum <= BOOTLOADER_SECTOR_MAX)
	{
		return 0xFC;
	}

	/* 3. Ensure the sector belongs to the currently selected target bank.
	 *    g_udsTargetBank is set during RequestDownload (0x34) based on
	 *    the start address provided by the tester. */
	if (!IsSectorInTargetBank(blockNum))
	{
		return 0xFD;
	}

	/* 4. Execute erase */
	if ((uint8) Flash_erasePFlash_port(IfxFlash_pFlashTableLog[blockNum].start) != 0)
	{
		return 0x00;
	}

	return 0x01;
}



