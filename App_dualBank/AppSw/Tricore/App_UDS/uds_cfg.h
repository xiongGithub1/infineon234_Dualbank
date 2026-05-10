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
/* DTC ���� */
#define DTC_CODE_MAX_NUM (8u)
//#define DTC_FORMAT_15031                  (0x00)
//#define DTC_FORMAT_14229                  (0x01)
#define DTC_AVAILABILITY_STATUS_MASK      (0x7F)

#define DTC_DID_MAX_NUM					(8u)		// ֧�ֵ�DTC DID����
#define SANP_EEPROM_BASE_ADDR			(0xA000u)	// �������ݵĿ�ʼ��ַ
#define SANP_RECORD_MAX_NUM				(4u)		// ÿ��dtc did���洢����������
#define SANP_DATA_DID_NUM				(8u)		// ÿ�����մ洢����������
#define SANP_DATA_PER_SIZE				(SANP_DATA_DID_NUM * 4)		// ÿ�����յ����ݳ���=�������� * 4(4��ʾdid + data��32λ)
#define VIN_F190						"W0L00043MB541326"	// ����VIN��
#define BSID_F180						"1.2.3.4"			// �����汾��
// ������
typedef enum
{
	P150019 = 0x00150019u,
	P150101 = 0x00150101u,
	P150201 = 0x00150201u,
	P150417 = 0x00150417u
}dtc_did_name;
// ����ID
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
