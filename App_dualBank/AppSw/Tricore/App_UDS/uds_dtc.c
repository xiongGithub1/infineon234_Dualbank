/**********************************************************************************************************************
 * \file    obd_dtc.c
 * \brief
 * \version V1.0.0
 * \date    2021年12月1日
 * \author  Administrator
 *********************************************************************************************************************/
#include <uds_dtc.h>
#define TEST_SANP
#ifdef TEST_SANP
// 测试用故障快照数据
snap_data_t s_data[] = {{B001,0x1111},{B002,0x2222},{B003,0x3333},{B004,0x4444},{B005,0x5555},{B006,0x6666},{B007,0x7777},{B008,0x8888}};
#endif

// 故障代码定义
static dtc_data_table dtcDataTableList[DTC_DID_MAX_NUM] =
{
		/* 补充故障代码,诊断函数,诊断周期 */
		{P150019,{0x01u},{SANP_EEPROM_BASE_ADDR,0},0,0,0,nullptr,0},
		{P150101,{0x02u},{SANP_EEPROM_BASE_ADDR,0},0,0,0,nullptr,0},
		{P150201,{0x02u},{SANP_EEPROM_BASE_ADDR,0},0,0,0,nullptr,0},
		{P150417,{0x02u},{SANP_EEPROM_BASE_ADDR,0},0,0,0,nullptr,0},
};

uint8 isDtcStatuCanUpdate = ON;//DTC状态位是否允许在发生故障时更新


//保存快照信息到EEPROM
void dtcSaveSanpData(uint16 eepromAddr)
{
	// TODO
	// 添加快照保存代码
	// 1.获取需要保存的变量信息
	// 2.写入EEPROM中
	// tl_write_to_eeprom();
}


dtc_status_t getStatusByDtcCode(uint32 dtc)
{
	dtc_status_t status = {0};
	for(uint16 i = 0; i < sizeof(dtcDataTableList) / sizeof(dtc_data_table); i++)
	{
		if(dtcDataTableList[i].dtc_code == dtc)
		{
			return dtcDataTableList[i].dtc_st;
		}
	}

	return status;
}

uint8 IsFaultConfirmed(DTCStatusType status)
{
	if(status.DTCbit.ConfirmedDTC)
	{
		return TRUE;
	}

	return FALSE;
}
/* 根据DTC掩码报告DTC数量 */
uint16 getDTCCountByStatusMask(uint8 status_mask)
{
	uint16 dtc_count = 0;
	uint8 record_count;
	for(record_count = 0; record_count < sizeof(dtcDataTableList) / sizeof(dtc_data_table); record_count++)
	{
		if(dtcDataTableList[record_count].dtc_code == 0)
		{
			continue;
		}
		if((dtcDataTableList[record_count].dtc_st.byteAll & status_mask))
		{
			dtc_count++;
		}
	}
	return dtc_count;
}
/*按照状态掩码统计ECU中与之匹配的DTC，返回该DTC信息*/
uint16 getDTCByStatusMask(uint8 *p_dtc, uint8 status_mask)
{
    uint16 dtc_count = 0;
    uint8 record_count;

    for(record_count = 0; record_count < sizeof(dtcDataTableList) / sizeof(dtc_data_table); record_count++)
    {
    	if(dtcDataTableList[record_count].dtc_code == 0)
		{
			continue;
		}
    	if((dtcDataTableList[record_count].dtc_st.byteAll & status_mask))
        {
            *p_dtc++ = GET_DTC_HIGH_BYTE(dtcDataTableList[record_count].dtc_code);
            *p_dtc++ = GET_DTC_MID_BYTE(dtcDataTableList[record_count].dtc_code);
            *p_dtc++ = GET_DTC_LOW_BYTE(dtcDataTableList[record_count].dtc_code);

            *p_dtc++ = dtcDataTableList[record_count].dtc_st.byteAll;
            dtc_count++;
        }
    }
    return dtc_count;
}

uint16 getDTCSupportedDtc(uint8 *p_dtc)      /*返回所有支持的DTC信息*/
{
    uint8 record_count;
    uint16 dtc_count = 0;
    for(record_count = 0; record_count < DTC_DID_MAX_NUM; record_count++)
    {
    	uint32 did = dtcDataTableList[record_count].dtc_code;
    	if(did)
    	{
    		*p_dtc++ = GET_DTC_HIGH_BYTE(did);
			*p_dtc++ = GET_DTC_MID_BYTE(did);
			*p_dtc++ = GET_DTC_LOW_BYTE(did);
			*p_dtc++ = dtcDataTableList[record_count].dtc_st.byteAll;
			dtc_count++;
    	}

    }
    return dtc_count;
}

uint8 getDTCSanpData(uint32 dtc_code,uint8 record_idx,uint8 *p_data,uint8 *p_snap_len)
{
	if((record_idx != 0xFF) && (record_idx > SANP_RECORD_MAX_NUM))
	{
		return FALSE;
	}

	uint8 dataBuffer[SANP_DATA_PER_SIZE] = {0};
	for(int i = 0; i < sizeof(dtcDataTableList) / sizeof(dtc_data_table); i++)
	{
		if(dtcDataTableList[i].dtc_code == dtc_code)
		{
			if(record_idx != 0xFF)	// 指定的快照编号
			{
				// 获取地址
				uint16 actAddr = dtcDataTableList[i].dtcSnapData.base + record_idx * SANP_DATA_PER_SIZE;

				if(tl_read_from_eeprom(actAddr,dataBuffer,SANP_DATA_PER_SIZE))
				{
#ifdef TEST_SANP
					tl_memcpy(dataBuffer,&s_data,SANP_DATA_PER_SIZE);
#endif
					// 读取成功
					// 填充快照编号
					*p_data++ = record_idx;
					(*p_snap_len)++;
					// 填充快照中的成员数量
					*p_data++ = SANP_DATA_DID_NUM;
					(*p_snap_len)++;
					// 填充每个成员的ID信息和数据
					for(int j = 0; j < SANP_DATA_PER_SIZE; j+=4)
					{
						uint16 record_did = (uint16)((dataBuffer[j+1] << 8) | dataBuffer[j]);
						uint16 record_data = (uint16)((dataBuffer[j+3] << 8) | dataBuffer[j+2]);
						// 填充ID
						*p_data++ = (uint8)((record_did & 0xFF00) >> 8);
						(*p_snap_len)++;
						*p_data++ = (uint8)(record_did & 0x00FF);
						(*p_snap_len)++;
						// 填充数据
						*p_data++ = (uint8)((record_data & 0xFF00) >> 8);
						(*p_snap_len)++;
						*p_data++ = (uint8)(record_data & 0x00FF);
						(*p_snap_len)++;
					}
				}
				else
				{
					// 读取失败
					return FALSE;
				}
				break;// 提前返回，中断循环
			}
			else	// 全部快照
			{
				for(int j = 0; j < SANP_RECORD_MAX_NUM; j++)
				{
					// 获取地址
					uint16 actAddr = dtcDataTableList[i].dtcSnapData.base + j * SANP_DATA_PER_SIZE;
					if(tl_read_from_eeprom(actAddr,dataBuffer,SANP_DATA_PER_SIZE))
					{
#ifdef TEST_SANP
						tl_memcpy(dataBuffer,&s_data,SANP_DATA_PER_SIZE);
#endif
						// 填充快照编号
						*p_data++ = (uint8)j;
						(*p_snap_len)++;
						// 填充快照中的成员数量
						*p_data++ = SANP_DATA_DID_NUM;
						(*p_snap_len)++;
						// 填充每个成员的ID信息和数据
						for(int j = 0; j < SANP_DATA_PER_SIZE; j+=4)
						{
							uint16 record_did = (uint16)((dataBuffer[j+1] << 8) | dataBuffer[j]);
							uint16 record_data = (uint16)((dataBuffer[j+3] << 8) | dataBuffer[j+2]);
							// 填充ID
							*p_data++ = (uint8)((record_did & 0xFF00) >> 8);
							(*p_snap_len)++;
							*p_data++ = (uint8)(record_did & 0x00FF);
							(*p_snap_len)++;
							// 填充数据
							*p_data++ = (uint8)((record_data & 0xFF00) >> 8);
							(*p_snap_len)++;
							*p_data++ = (uint8)(record_data & 0x00FF);
							(*p_snap_len)++;
						}
					}
					else
					{
						// 读取失败
						return FALSE;
					}
				}
				break;
			}
		}
	}

	return TRUE;
}

void clearDTCByGroup(uint32 group)
{
	for (uint16 i = 0; i < DTC_CODE_MAX_NUM; i++)
	{
		if (dtcDataTableList[i].dtc_code & group)
		{
			dtcDataTableList[i].dtc_st.byteAll = 0x0u;
			//bit4 and bit6 must be setted to 1
			dtcDataTableList[i].dtc_st.bit.TestNotCompleteSinceLastClear = 1;
			dtcDataTableList[i].dtc_st.bit.TestNotCompleteThisMonitoringCycle = 1;
		}
	}
}
// 诊断周期是否超时,超时返回1，否则返回0
uint8 isTestPeroidTimeOut(uint32 time)
{
	// TODO
	// 添加硬件/软件定时器，定时时间检测代码
	return 0;
}

void dtcTestMainProc(void)
{
	DTCTestResult currentResult = TEST_PASSED;
	dtc_data_table *currentDTCObj = nullptr;
	for(int i = 0; i < sizeof(dtcDataTableList) / sizeof(dtcDataTableList[0]); i++)
	{
		currentDTCObj = &dtcDataTableList[i];
		if(isTestPeroidTimeOut(currentDTCObj->test_period))
		{
			continue;
		}

		if(currentDTCObj->testFunHandler != nullptr)
		{
			currentResult = currentDTCObj->testFunHandler();
			if(isDtcStatuCanUpdate == OFF)
			{
				// 由0x85服务控制为不更新状态位
				continue;
			}
			switch (currentResult)
			{
			case TEST_PASSED://测试通过
				if(currentDTCObj->fdt_cnt > FDT_MAX)
				{
					currentDTCObj->fdt_cnt--;
					if(currentDTCObj->fdt_cnt <= FDT_MIN)
					{
						currentDTCObj->dtc_st.bit.TestFailed = 0;
						currentDTCObj->dtc_st.bit.TestNotCompleteSinceLastClear = 0;
						currentDTCObj->dtc_st.bit.TestNotCompleteThisMonitoringCycle = 0;
					}
				}
				break;
			case TEST_FAILED://测试失败
				if(currentDTCObj->fdt_cnt < FDT_MAX)
				{
					if(currentDTCObj->fdt_cnt < 0)
					{
						currentDTCObj->fdt_cnt = 1;
					}
					else
					{
						currentDTCObj->fdt_cnt += 2;
					}

					if(currentDTCObj->fdt_cnt >= FDT_MAX)
					{
						currentDTCObj->dtc_st.bit.TestFailed = 1;
						currentDTCObj->dtc_st.bit.TestFailedThisMonitoringCycle = 1;
						currentDTCObj->dtc_st.bit.PendingDTC = 1;
						currentDTCObj->dtc_st.bit.ConfirmedDTC = 1;
						currentDTCObj->dtc_st.bit.TestNotCompleteSinceLastClear = 0;
						currentDTCObj->dtc_st.bit.TestFailedSinceLastClear = 1;
						currentDTCObj->dtc_st.bit.TestNotCompleteThisMonitoringCycle = 0;

						// 保存快照信息
						uint16 actAddr = currentDTCObj->dtcSnapData.base + currentDTCObj->dtcSnapData.current * SANP_DATA_PER_SIZE;
						dtcSaveSanpData(actAddr);
						currentDTCObj->dtcSnapData.current++;
						if(currentDTCObj->dtcSnapData.current > SANP_RECORD_MAX_NUM)
						{
							currentDTCObj->dtcSnapData.current = 0;
						}
					}
				}
				break;
			case TEST_NORESULT:
				break;
			default:
				break;
			}
		}
	}
}
