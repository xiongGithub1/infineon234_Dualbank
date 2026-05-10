/**********************************************************************************************************************
 * \file    obd_dtc.h
 * \brief
 * \version V1.0.0
 * \date    2021年12月1日
 * \author  Administrator
 *********************************************************************************************************************/
#ifndef UDSDIAGNOSTIC_UDS_DTC_H_
#define UDSDIAGNOSTIC_UDS_DTC_H_
#include "uds_cfg.h"
#include "uds_common.h"
#include "tool_class.h"

#define GET_DTC_HIGH_BYTE(x) ((uint8)((x & 0x00FF0000) >> 16))
#define GET_DTC_MID_BYTE(x) ((uint8)((x & 0x0000FF00) >> 8))
#define GET_DTC_LOW_BYTE(x) ((uint8)((x & 0x000000FF)))

typedef enum
{
	ISO_15031_6,
	ISO_14229_1,
	SAE_J1939_73,
	ISO_11992_4
}DTCFormat;

typedef enum{
    TEST_PASSED,//检测通过
    TEST_NORESULT,//无结果
    TEST_FAILED,//检测失败
}DTCTestResult;

typedef DTCTestResult (*DTCTestFunciton)(void);

typedef enum
{
	REPORT_DTCNUMBER_BY_MASK = 0x01,
	REPORT_DTCCODE_BY_MASK = 0x02,
	REPORT_DTCSNAPSHOT_BY_ID = 0x03,
	REPORT_DTCSNAPSHOT_BY_DTCNUMBER = 0x04,
	REPORT_DTCSNAPSHOT_BY_RECORDNUMBER = 0x05,
	REPORT_DTCEXTEND_DATA_BY_DTCNUMBER = 0x06,
	REPORT_DTCNUMBER_BY_SEVERITYMASK = 0x07,
	REPORT_DTC_BY_SEVERITYMASK = 0x08,
	REPORT_SEVERITYID_OF_DTC = 0x09,
	REPORT_SUPPORTED_DTC = 0x0A,
	REPORT_FIRST_FAILED_DTC = 0x0B,
	REPORT_FIRST_CONFIRMED_DTC = 0x0C,
	REPORT_MOST_FAILED_DTC = 0x0D,
	REPORT_MOST_CONFIRMED_DTC = 0x0E,
	REPORT_MIRRORDTC_BY_MASK = 0x0F,
	REPORT_MIRRORDTC_EXTENDED_BY_DTC_NUMBER = 0x10,
	REPORT_MIRRORDTC_NUMBER_BY_MASK = 0x11,
	REPORT_OBDDTC_NUMBER_BY_MASK = 0x12,
	REPORT_OBDDTC_BY_MASK = 0x13,
} DTCSubFunction;

typedef union
{
	uint8 DTCStatusByte;
	struct
	{
		uint8 TestFailed :1; //测试失败
		uint8 TestFailedThisMonitoringCycle :1; //测试未通过此监视循环
		uint8 PendingDTC :1;
		uint8 ConfirmedDTC :1; //确认故障代码
		uint8 TestNotCompleteSinceLastClear :1; //自上次清除后测试未完成
		uint8 TestFailedSinceLastClear :1;
		uint8 TestNotCompleteThisMonitoringCycle :1; //测试未完成此监视周期
		uint8 WarningIndicatorRequested :1;
	} DTCbit;
} DTCStatusType;


//typedef struct
//{
//	uint8 record_number;
//	uint8 OldCounter;//上一次计数
//	uint8 GoneCounter;//走完了的计数
//	uint8 TripCounter;//故障待定计数器
//	uint8 TripCounterThreshold;//故障计数器最大值
//	uint8 FaultOccurrences;//故障出现次数
//}DTCExtendedDataInfo;

#define FDT_MAX               (127)
#define FDT_MIN               (-128)
#define AGN_MAX               40



/* dtc 状态码联合体 */
typedef union
{
	uint8 byteAll;
	struct
	{
		uint8 TestFailed :1; //测试失败
		uint8 TestFailedThisMonitoringCycle :1; //测试未通过此监视循环
		uint8 PendingDTC :1;
		uint8 ConfirmedDTC :1; //确认故障代码
		uint8 TestNotCompleteSinceLastClear :1; //自上次清除后测试未完成
		uint8 TestFailedSinceLastClear :1;
		uint8 TestNotCompleteThisMonitoringCycle :1; //测试未完成此监视周期
		uint8 WarningIndicatorRequested :1;
	} bit;
} dtc_status_t;

typedef enum
{
	ON = 0x01u,
	OFF = 0x02u
}DTCStatuCanUpdate;



/* dtc 快照结构体 */
typedef struct
{
	uint16 record_did;	// 快照的DID
	uint16 record_data;	// 快照的数据
}snap_data_t;
//typedef struct
//{
//	uint16  record_num;		// 表示snap_data_t有多少个,即每条快照中记录的数据有多少
//	snap_data_t *data; 		// 具体的数据
//}dtc_snap_t;

typedef struct
{
	uint16 		base; //EEPROM 开始地址
	uint16 		current; 		  // 当前快照存储序号,最大值 = SANP_RECORD_MAX_NUM
	// 实际数据地址入口 = base + current  * SANP_DATA_PER_SIZE
}SnapDataTypedef;
/* dtc 数据表格 */
typedef struct
{
	dtc_did_name dtc_code;			//支持的did
	dtc_status_t dtc_st;			//状态码
	//dtc_snap_t dtc_snap[SANP_RECORD_MAX_NUM];	//快照数据入口
	SnapDataTypedef dtcSnapData;
	uint8      fec_cnt;
	sint16     fdt_cnt;        		/* FaultDetectionCount */
	uint8      agn_cnt;        		/* DTCAgingCounter */
	DTCTestFunciton testFunHandler;	//故障诊断函数入口
	uint32	   test_period;			// 诊断周期,单位ms
}dtc_data_table;

extern uint8 isDtcStatuCanUpdate;

uint16 getDTCCountByStatusMask(uint8 status_mask);
uint16 getDTCByStatusMask(uint8 *p_dtc, uint8 status_mask);
uint16 getDTCSupportedDtc(uint8 *p_dtc);

/* 根据DTC 获取相应的状态标识 */
dtc_status_t getStatusByDtcCode(uint32 dtc);
/* 故障是否确认 */
uint8 IsFaultConfirmed(DTCStatusType status);
/* 读取快照信息 */
uint8 getDTCSanpData(uint32 dtc_code,uint8 record_idx,uint8 *p_snap_data,uint8 *p_snap_len);

void clearDTCByGroup(uint32 group);
void dtcAddSnapData(uint32 dtc_code,snap_data_t *p_record,uint8 record_len);
void dtcInit(void);
void dtcTestMainProc(void);

#endif /* UDSDIAGNOSTIC_UDS_DTC_H_ */
