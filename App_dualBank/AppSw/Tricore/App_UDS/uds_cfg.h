/*
 * @Author: qinXiong
 * @Date: 2026-04-29 09:02:27
 * @LastEditors: xiong&&2307975018@qq.com
 * @LastEditTime: 2026-04-29 16:15:28
 * @Description: 
 */
/**********************************************************************************************************************
 * \file    uds_cfg.h
 * \brief
 * \version V1.0.0
 * \date    2022��2��10��
 * \author  Administrator
 *********************************************************************************************************************/
#ifndef UDSDIAGNOSTIC_UDS_CFG_H_
#define UDSDIAGNOSTIC_UDS_CFG_H_
#include "uds_common.h"

//��������ģʽֻ�ܴ�һ��
#define DIAGNOSTIC_MODE_FOR_APP				//APPģʽ
//#define DIAGNOSTIC_MODE_FOR_BOOTLOADER	//BOOTLOADERģʽ

/*****************************************************************************/


/*****************************************************************************/
/* DTC Configuration - ISO 14229-1 / OEM Standard */
#define DTC_CODE_MAX_NUM                  (8u)
#define DTC_AVAILABILITY_STATUS_MASK      (0x7F)

#define DTC_DID_MAX_NUM                   (8u)        /* Max supported DTC entries */
#define SANP_EEPROM_BASE_ADDR             (0xA000u)   /* Snapshot EEPROM base address */
#define SANP_RECORD_MAX_NUM               (4u)        /* Snapshot records per DTC */
#define SANP_DATA_DID_NUM                 (8u)        /* DID entries per snapshot */
#define SANP_DATA_PER_SIZE                (SANP_DATA_DID_NUM * 4)

/* OEM Standard DTC Definitions (ISO 15031-6 / ISO 14229-1)
 * Encoding: [Category][Digit][Group][Fault]
 * U=Network, P=Powertrain, B=Body, C=Chassis
 */
typedef enum
{
	/* Network Communication DTCs */
	DTC_U0100 = 0x0000C100u,   /* Lost Communication with ECM/PCM (CAN BusOff) */
	DTC_U0121 = 0x0000C121u,   /* Lost Communication with ABS (CAN Ack Error) */

	/* Powertrain DTCs */
	DTC_P0601 = 0x00000601u,   /* Internal Control Module Memory Checksum Error */
	DTC_P0605 = 0x00000605u,   /* Internal Control Module Read Only Memory Error */

	/* Body DTCs */
	DTC_B1000 = 0x00009000u,   /* ECU Boot Failure Recorded */
	DTC_B1001 = 0x00009001u,   /* ECU Software Version Mismatch */

	/* Reserved / Placeholder */
	DTC_RESERVED_1 = 0x00000000u,
	DTC_RESERVED_2 = 0x00000000u
} dtc_did_name;

/* Snapshot DID Definitions (Freeze Frame data identifiers) */
typedef enum
{
	SNAP_DID_SYS_VOLTAGE   = 0xF442u,   /* System Voltage (mV) */
	SNAP_DID_AMB_TEMP      = 0xF446u,   /* Ambient Temperature (0.1C) */
	SNAP_DID_CAN_STATUS    = 0xF501u,   /* CAN Bus Status */
	SNAP_DID_RUN_TIME      = 0xF50Au,   /* ECU Run Time (s) */
	SNAP_DID_BANK_STATUS   = 0xF510u,   /* Active Bank Status */
	SNAP_DID_BOOT_CNT      = 0xF511u,   /* Boot Attempt Counter */
	SNAP_DID_SW_VERSION    = 0xF188u,   /* Software Version */
	SNAP_DID_HW_VERSION    = 0xF193u    /* Hardware Version */
} snap_did_name;

#define VIN_F190                          "W0L00043MB541326"
#define BSID_F180                         "1.2.3.4"

/* 22����Ķ�дDID */
typedef enum
{
	F15A = 0xF15Au,	//������дָ����Ϣ
	F14A = 0xF14Au,	//����bootloader�汾��
	F187 = 0xF187u,	//�����㲿����
	F18A = 0xF18Au,	//������Ӧ�̴���
	F197 = 0xF197u,	//�����������ͺ�
	F193 = 0xF193u,	//����ECUӲ���汾��
	F195 = 0xF195u,	//����ECU�����汾��
	F18C = 0xF18Cu,	//�����������������
	F190 = 0xF190u, //����VIN
}rw_data_did;



#endif /* UDSDIAGNOSTIC_UDS_CFG_H_ */
