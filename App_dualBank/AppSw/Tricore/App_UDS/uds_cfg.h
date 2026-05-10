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
//#define DIAGNOSTIC_MODE_FOR_APP				//APP诊断模式
#define DIAGNOSTIC_MODE_FOR_BOOTLOADER	//BOOTLOADER诊断模式

/*****************************************************************************/


/*****************************************************************************/
/* DTC 配置 */
#define DTC_CODE_MAX_NUM (8u)
//#define DTC_FORMAT_15031                  (0x00)
//#define DTC_FORMAT_14229                  (0x01)
#define DTC_AVAILABILITY_STATUS_MASK      (0x7F)

#define DTC_DID_MAX_NUM					(8u)		// 支持的DTC DID数量
#define SANP_EEPROM_BASE_ADDR			(0xA000u)	// 快照数据的开始地址
#define SANP_RECORD_MAX_NUM				(4u)		// 每个dtc did最多存储多少条快照
#define SANP_DATA_DID_NUM				(8u)		// 每条快照存储的数据数量
#define SANP_DATA_PER_SIZE				(SANP_DATA_DID_NUM * 4)		// 每个快照的数据长度=数据数量 * 4(4表示did + data共32位)
#define VIN_F190						"W0L00043MB541326"	// 车身VIN号
#define BSID_F180						"1.2.3.4"			// 软件版本号
// 故障码
typedef enum
{
	P150019 = 0x00150019u,
	P150101 = 0x00150101u,
	P150201 = 0x00150201u,
	P150417 = 0x00150417u
}dtc_did_name;
// 快照ID
typedef enum
{
	B001 = 0xB001u,
	B002 = 0xB002u,
	B003 = 0xB003u,
	B004 = 0xB004u,
	B005 = 0xB005u,
	B006 = 0xB006u,
	B007 = 0xB007u,
	B008 = 0xB008u
}snap_did_name;

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



#endif /* UDSDIAGNOSTIC_UDS_CFG_H_ */
