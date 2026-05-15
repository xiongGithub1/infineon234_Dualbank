/**********************************************************************************************************************
 * \file    obd_dtc.h
 * \brief
 * \version V1.0.0
 * \date    2021��12��1��
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
    TEST_PASSED,//���ͨ��
    TEST_NORESULT,//�޽��
    TEST_FAILED,//���ʧ��
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
		uint8 TestFailed :1; //����ʧ��
		uint8 TestFailedThisMonitoringCycle :1; //����δͨ���˼���ѭ��
		uint8 PendingDTC :1;
		uint8 ConfirmedDTC :1; //ȷ�Ϲ��ϴ���
		uint8 TestNotCompleteSinceLastClear :1; //���ϴ���������δ���
		uint8 TestFailedSinceLastClear :1;
		uint8 TestNotCompleteThisMonitoringCycle :1; //����δ��ɴ˼�������
		uint8 WarningIndicatorRequested :1;
	} DTCbit;
} DTCStatusType;


//typedef struct
//{
//	uint8 record_number;
//	uint8 OldCounter;//��һ�μ���
//	uint8 GoneCounter;//�����˵ļ���
//	uint8 TripCounter;//���ϴ���������
//	uint8 TripCounterThreshold;//���ϼ��������ֵ
//	uint8 FaultOccurrences;//���ϳ��ִ���
//}DTCExtendedDataInfo;

/* FDT (Fault Detection Counter) Thresholds per ISO 14229-1 */
#define FDT_MAX               (127)
#define FDT_MIN               (-128)
#define FDT_STEP_PASS         (-1)    /* Decrement on test passed */
#define FDT_STEP_FAIL         (+2)    /* Increment on test failed */
#define FDT_CONFIRM_THRESH    (127)   /* Threshold for ConfirmedDTC = 1 */
#define FDT_HEAL_THRESH       (-128)  /* Threshold for ConfirmedDTC = 0 (deconfirmed) */

/* Aging Counter Thresholds */
#define AGN_MAX               (40u)   /* Aging counter max before deconfirmation */
#define AGN_STEP              (1u)    /* Increment per passed cycle */

/* Debounce / Confirmation */
#define DTC_DEBOUNCE_CYCLES   (3u)    /* Cycles for PendingDTC confirmation */

/* DTC Status Byte Bit Mask per ISO 14229-1 */
#define DTC_STATUS_TF         (0x01u) /* bit0: TestFailed */
#define DTC_STATUS_TFTMC      (0x02u) /* bit1: TestFailedThisMonitoringCycle */
#define DTC_STATUS_PDTC       (0x04u) /* bit2: PendingDTC */
#define DTC_STATUS_CDTC       (0x08u) /* bit3: ConfirmedDTC */
#define DTC_STATUS_TNCSC      (0x10u) /* bit4: TestNotCompletedSinceLastClear */
#define DTC_STATUS_TFSC       (0x20u) /* bit5: TestFailedSinceLastClear */
#define DTC_STATUS_TNCTMC     (0x40u) /* bit6: TestNotCompletedThisMonitoringCycle */
#define DTC_STATUS_WIR        (0x80u) /* bit7: WarningIndicatorRequested */

/* Initial status after DTC clear: bits 4 and 6 set */
#define DTC_STATUS_INIT       (DTC_STATUS_TNCSC | DTC_STATUS_TNCTMC)

/* DTC status byte - ISO 14229-1 compliant */
typedef union
{
	uint8 byteAll;
	struct
	{
		uint8 TestFailed :1;                         /* bit0: Current test result */
		uint8 TestFailedThisMonitoringCycle :1;      /* bit1: Failed this cycle */
		uint8 PendingDTC :1;                         /* bit2: Waiting for confirmation */
		uint8 ConfirmedDTC :1;                       /* bit3: Confirmed by aging/FDT */
		uint8 TestNotCompleteSinceLastClear :1;      /* bit4: Not tested since clear */
		uint8 TestFailedSinceLastClear :1;           /* bit5: Failed since last clear */
		uint8 TestNotCompleteThisMonitoringCycle :1; /* bit6: Not tested this cycle */
		uint8 WarningIndicatorRequested :1;          /* bit7: MIL/warning requested */
	} bit;
} dtc_status_t;

typedef enum
{
	ON = 0x01u,
	OFF = 0x02u
}DTCStatuCanUpdate;



/* Snapshot data record (Freeze Frame) */
typedef struct
{
	uint16 record_did;
	uint16 record_data;
} snap_data_t;

/* Snapshot data storage management */
typedef struct
{
	uint16 base;    /* EEPROM base address for this DTC */
	uint16 current; /* Current snapshot write index (0 ~ SANP_RECORD_MAX_NUM-1) */
} SnapDataTypedef;

/* Extended Data Record - per ISO 14229-1 */
typedef struct
{
	uint8  occurrenceCounter;     /* Number of times fault occurred (0-255), lifetime counter */
	uint8  agingCounter;          /* Aging counter for deconfirmation (0-AGN_MAX) */
	uint8  faultOccurrenceSinceClear; /* Fault count since last clear */
	sint16 fdt_cnt;               /* Fault Detection Counter (-128 ~ +127) */
} DTCExtendedData_t;

/* DTC Data Table Entry - OEM Standard */
typedef struct
{
	dtc_did_name    dtc_code;       /* DTC identifier (3-byte ISO format) */
	dtc_status_t    dtc_st;         /* DTC status byte per ISO 14229-1 */
	SnapDataTypedef dtcSnapData;    /* Snapshot (freeze frame) storage info */
	DTCExtendedData_t extData;      /* Extended data (aging, occurrence) */
	uint8           debounceCnt;    /* Debounce counter for PendingDTC */
	uint8           testPeriodMs;   /* Test period in milliseconds */
	uint32          lastTestTime;   /* Timestamp of last test execution */
	DTCTestFunciton testFunHandler; /* Fault detection function pointer */
} dtc_data_table;

extern uint8 isDtcStatuCanUpdate;

uint16 getDTCCountByStatusMask(uint8 status_mask);
uint16 getDTCByStatusMask(uint8 *p_dtc, uint8 status_mask);
uint16 getDTCSupportedDtc(uint8 *p_dtc);

/* ����DTC ��ȡ��Ӧ��״̬��ʶ */
dtc_status_t getStatusByDtcCode(uint32 dtc);
/* �����Ƿ�ȷ�� */
uint8 IsFaultConfirmed(DTCStatusType status);
/* ��ȡ������Ϣ */
uint8 getDTCSanpData(uint32 dtc_code,uint8 record_idx,uint8 *p_snap_data,uint8 *p_snap_len);
/* ��ȡ��չ������Ϣ */
uint8 getDTCExtData(uint32 dtc_code,uint8 record_num,uint8 *p_ext_data,uint8 *p_ext_len);

void clearDTCByGroup(uint32 group);
void dtcAddSnapData(uint32 dtc_code,snap_data_t *p_record,uint8 record_len);
void dtcInit(void);
void dtcTestMainProc(void);

#endif /* UDSDIAGNOSTIC_UDS_DTC_H_ */
