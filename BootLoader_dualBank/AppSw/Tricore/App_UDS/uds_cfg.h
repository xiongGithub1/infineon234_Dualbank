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
 * \date    2022年2月10日
 * \author  Administrator
 *********************************************************************************************************************/
#ifndef UDSDIAGNOSTIC_UDS_CFG_H_
#define UDSDIAGNOSTIC_UDS_CFG_H_
#include "uds_common.h"

//以下两个模式只能打开一个
////#define DIAGNOSTIC_MODE_FOR_APP				//APP妯″紡
#define DIAGNOSTIC_MODE_FOR_BOOTLOADER	//BOOTLOADER妯″紡

/*****************************************************************************/


/*****************************************************************************/
/* DTC Configuration - ISO 14229-1 / OEM Standard */
#define DTC_CODE_MAX_NUM                  (8u)
#define DTC_AVAILABILITY_STATUS_MASK      (0x7F)

#define DTC_DID_MAX_NUM                   (8u)
#define SANP_EEPROM_BASE_ADDR             (0xA000u)
#define SANP_RECORD_MAX_NUM               (4u)
#define SANP_DATA_DID_NUM                 (8u)
#define SANP_DATA_PER_SIZE                (SANP_DATA_DID_NUM * 4)

typedef enum
{
	/* Network Communication DTCs */
	DTC_U0100 = 0x0000C100u,
	DTC_U0121 = 0x0000C121u,

	/* Powertrain DTCs */
	DTC_P0601 = 0x00000601u,
	DTC_P0605 = 0x00000605u,

	/* Body DTCs */
	DTC_B1000 = 0x00009000u,
	DTC_B1001 = 0x00009001u,

	DTC_RESERVED_1 = 0x00000000u,
	DTC_RESERVED_2 = 0x00000000u
} dtc_did_name;

typedef enum
{
	SNAP_DID_SYS_VOLTAGE   = 0xF442u,
	SNAP_DID_AMB_TEMP      = 0xF446u,
	SNAP_DID_CAN_STATUS    = 0xF501u,
	SNAP_DID_RUN_TIME      = 0xF50Au,
	SNAP_DID_BANK_STATUS   = 0xF510u,
	SNAP_DID_BOOT_CNT      = 0xF511u,
	SNAP_DID_SW_VERSION    = 0xF188u,
	SNAP_DID_HW_VERSION    = 0xF193u
} snap_did_name;

/* 22服务的读写DID */
typedef enum
{
	F15A = 0xF15Au,	//代表读写指纹信息
	F14A = 0xF14Au,	//代表bootloader版本号
	F187 = 0xF187u,	//代表零部件号
	F18A = 0xF18Au,	//代表供应商代号
	F197 = 0xF197u,	//代表控制器型号
	F193 = 0xF193u,	//代表ECU硬件版本号
	F195 = 0xF195u,	//代表ECU软件版本号
	F18C = 0xF18Cu,	//代表控制器出厂编号
	F190 = 0xF190u, //车身VIN
}rw_data_did;

#define VIN_F190                          "W0L00043MB541326"
#define BSID_F180                         "1.2.3.4"

#endif /* UDSDIAGNOSTIC_UDS_CFG_H_ */
