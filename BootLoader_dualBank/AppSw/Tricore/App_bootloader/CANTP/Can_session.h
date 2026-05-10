
/**********************************************************************************************************************
 * \file    Can_session.h
 * \brief
 * \version V1.0.0
 * \date
 *********************************************************************************************************************/


#ifndef CANTP_CAN_SESSION_H_
#define CANTP_CAN_SESSION_H_

/*********************************************************************************************************************/
/*-----------------------------------------------------Includes------------------------------------------------------*/
/*********************************************************************************************************************/
#include    "Std_Types.h"
#include    "App_bootloader_cfg.h"
#include    "CanIf.h"

/*********************************************************************************************************************/
/*------------------------------------------------------Macros-------------------------------------------------------*/
/*********************************************************************************************************************/
/*
 * NRC
 */
#define NRC_positiveResponse                            (0)         /* PR */
#define NRC_generalReject                               (0x10)     /* GR */
#define NRC_serviceNotSupported                         (0x11)     /* SNS */
#define NRC_sub_functionNotSupported                    (0x12)     /* SFNS */
#define NRC_incorrectMessageLengthOrlnvalidFormat       (0x13)     /* IMLOIF  数据长度，地址格式错误 */
#define NRC_responseTooLong                             (0x14)     /* RTL */
#define NRC_busyRepeatRequest                           (0x21)     /* BRR */
#define NRC_conditionsNotCorrectt                       (0x22)     /* CNC */
#define NRC_requestSequenceError                        (0x24)     /* RSE 请求序列错误*/
#define NRC_noResponseFromSubnetComponent               (0x25)     /* NRFSC 没有来自子网组件的响应*/
#define NRC_FailurePreventsExecutionOfRequestedAction   (0x26)     /* FPEORA 失败阻止执行所请求的操作 */
#define NRC_requestOutOfRange                           (0x31)     /* ROOR 请求超出范围*/
#define NRC_securityAccessDenied                        (0x33)     /* SAD 拒绝访问 */
#define NRC_invalidKey                                  (0x35)     /* IK 无效钥匙 */
#define NRC_exceedNumberOfAttempts                      (0x36)     /* ENOA 超过尝试次数 */
#define NRC_requiredTimeDelayNotExpired                 (0x37)     /* RTDNE 所需时间 延迟未过期 */
#define NRC_uploadDownloadNotAccepted                   (0x70)     /* UDNA 上传 下载 不接受 */
#define NRC_transferDataSuspended                       (0x71)     /* TDS 暂停传输数据 */
#define NRC_generalProgrammingFailure                   (0x72)     /* GPF 一般编程失败 */
#define NRC_wrongBlockSequenceCounter                   (0x73)     /* WBSC 错误的块序列计数器 */
#define NRC_requestCorrectlyReceived_ResponsePending    (0x78)     /* RCRRP 请求正确接收 响应待定  */
#define NRC_sub_functionNotSupportedlnActiveSession     (0x7e)     /* RCRRP 子功能不支持 活动会话  */
#define NRC_serviceNotSupportedlnActiveSession          (0x7f)     /* SNSIAS 服务不支持 活动会话  */
#define NRC_rpmTooHigh                                  (0x81)     /* RPMTH rpm太高  */
#define NRC_rpmToolow                                   (0x82)     /* RPMTH rpm太低  */
#define NRC_enginelsRunning                             (0x83)     /* EIR 发动机正在运行  */
#define NRC_enginelsNotRunning                          (0x84)     /* EINR 发动机不运转  */
#define NRC_engineRunTimeTooLow                         (0x85)     /* ERTIL 发动机运行时间过低   */
#define NRC_temperatureTooHigh                          (0x86)     /* TEMPTH 温度过高    */
#define NRC_temperatureTooLow                           (0x87)     /* TEMPTL 温度过低    */
#define NRC_vehicleSpeedTooHigh                         (0x88)     /* VSTH 车速过高    */
#define NRC_vehicleSpeedTooLow                          (0x89)     /* VSTL 车速过低    */
#define NRC_throttlePedalTooHigh                        (0x8a)     /* TPTH 油门踏板太高     */
#define NRC_throttlePedalTooLow                         (0x8b)     /* TPTLL 油门踏板太高低    */
#define NRC_transmissionRangeNotlnNeutral               (0x8c)     /* TRNIN 变速箱范围不在空挡    */
#define NRC_transmissionRangeNotlnGear                  (0x8d)     /* TRNIG 变速箱范围不在档位上    */
#define NRC_brakeSwitchNotClosed                        (0x8f)     /* BSNC 刹车开关未关闭   */
#define NRC_shifterleverNotlnPark                       (0x90)     /* SLNIP 变速杆不在驻车状态   */
#define NRC_torqueConverterClutchlocked                 (0x91)     /* TCCL 扭矩转换器 离合器锁定   */
#define NRC_voltageTooHigh                              (0x92)     /* VTH 电压过高   */
#define NRC_voltageTooLow                               (0x93)     /* VTL 电压过低   */

/* Ox94 - 0xEF reservedForSpecificConditionsNotCorrect */
/* OxFO - OxFE vehicleManufacturerSpecificConditionsNotCorrect */


/* nrc for sid 23 */
/* 0x13 ,0x31,13,31,33,22 */

/* did : sid 31 */
#define  SID31_DID_ErasePFlash      (0xff00)
#define  SID31_DID_EraseDFlash      (0xedfa)

#define NRC_SID_conditionsNotCorrect                        0x22


/*NRC of Service 10*/
#define NRC_10_subFunctionNotSupported                      0x12
#define NRC_10_incorrectMessageLengthOrInvalidFormat        0x13
#define NRC_10_conditionsNotCorrect                         0x22

/*NRC of Service 11*/ //20240715
#define NRC_11_subFunctionNotSupported                      0x12
#define NRC_11_incorrectMessageLengthOrInvalidFormat        0x13
#define NRC_11_conditionsNotCorrect                         0x22
#define NRC_11_securityAccessDenied                         0x33

/*NRC of Service 22*/
#define NRC_22_incorrectMessageLengthOrInvalidFormat        0x13
#define NRC_22_responseTooLong                         		0x14
#define NRC_22_conditionsNotCorrect                         0x22
//#define NRC_22_conditionsNotCorrect                         0x31
#define NRC_22_SecurityAccessDenied                         0x33

/*NRC of Service 23*/
#define NRC_23_incorrectMessageLengthOrInvalidFormat        0x13
#define NRC_23_conditionsNotCorrect                         0x22
#define NRC_23_requestOutOfRange                            0x31
#define NRC_23_SecurityAccessDenied                         0x33

/*NRC of Service 27*/ //20240717
#define NRC_27_subFunctionNotSupported                      0x12
#define NRC_27_incorrectMessageLengthOrInvalidFormat        0x13
#define NRC_27_conditionsNotCorrect                         0x22
#define NRC_27_requestSequenceError                         0x24
#define NRC_27_requestOutOfRange                            0x31
#define NRC_27_invalidKey		                            0x35
#define NRC_27_exceededNumberOfAttempts                     0x36
#define NRC_27_requiredTimerDelayNotExpired                 0x37


/*NRC of Service 28*/ //20240723
#define NRC_28_subFunctionNotSupported                      0x12
#define NRC_28_incorrectMessageLengthOrInvalidFormat        0x13
#define NRC_28_conditionsNotCorrect                         0x22

/*NRC of Service 2e*/ //20240814
#define NRC_2e_incorrectMessageLengthOrInvalidFormat        0x13
#define NRC_2e_conditionsNotCorrect                         0x22
#define NRC_2e_requestOutOfRange                            0x31
#define NRC_2e_securityAccessDenied                         0x33
#define NRC_2e_generalProgrammingFailure                    0x72

///*NRC of Service 3d*/
//#define NRC_3D_incorrectMessageLengthOrInvalidFormat        0x13
//#define NRC_3D_conditionsNotCorrect                         0x22
//#define NRC_3D_requestOutOfRange                            0x31
//#define NRC_3D_securityAccessDenied                         0x33
//#define NRC_3D_generalProgrammingFailure                    0x72
//#define NRC_3D_ADDRESS_ERROR                                0xFC
//#define NRC_3D_NOT_EMPTY                                    0xEe        /* Err Empty */


/*NRC of Service 3e*/ //20240715
#define NRC_3e_subFunctionNotSupported                      0x12
#define NRC_3e_incorrectMessageLengthOrInvalidFormat        0x13

/*NRC of Service 31*/
#define NRC_31_subFunctionNotSupported                      0x12
#define NRC_31_incorrectMessageLengthOrInvalidFormat        0x13
#define NRC_31_conditionsNotCorrect                         0x22
#define NRC_31_requestSequenceError                         0x24
#define NRC_31_requestOutOfRange                            0x31
#define NRC_31_securityAccessDenied                         0x33
#define NRC_31_generalProgrammingFailure		            0x72
#define NRC_31_SECTOR_INDEX_ERR                             0xE0  /* Err sector number */

/*NRC of Service 34*/
#define NRC_34_incorrectMessageLengthOrInvalidFormat        0x13
#define NRC_34_conditionsNotCorrect                         0x22
#define NRC_34_requestOutOfRange                            0x31
#define NRC_34_securityAccessDenied                         0x33
#define NRC_34_generalProgrammingFailure                    0x70

/*NRC of Service 36*/
#define NRC_36_incorrectMessageLengthOrInvalidFormat        0x13
#define NRC_36_requestSequenceError                         0x24
#define NRC_36_requestOutOfRange                            0x31
#define NRC_36_securityAccessDenied                         0x33
#define NRC_36_transDataSuspend			                    0x71
#define NRC_36_generalProgrammingFailure		            0x72
#define NRC_36_wrongBlockSequenceCounter		            0x73
#define NRC_36_voltageTooHigh			                    0x92
#define NRC_36_voltageTooLow			                    0x93

/*NRC of Service 37*/
#define NRC_37_incorrectMessageLengthOrInvalidFormat        0x13
#define NRC_37_requestSequenceError                         0x24
#define NRC_37_requestOutOfRange                            0x31
#define NRC_37_securityAccessDenied                         0x33
#define NRC_37_transDataSuspend			                    0x71
#define NRC_37_generalProgrammingFailure		            0x72

/*NRC of Service 85*/ //20240723
#define NRC_85_subFunctionNotSupported                      0x12
#define NRC_85_incorrectMessageLengthOrInvalidFormat        0x13
#define NRC_85_conditionsNotCorrect                         0x22
#define NRC_85_requestOutOfRange                            0x31

/*NRC of Service 86*/ //20240723
#define NRC_86_subFunctionNotSupported                      0x12
#define NRC_86_incorrectMessageLengthOrInvalidFormat        0x13
#define NRC_86_conditionsNotCorrect                         0x22
#define NRC_86_requestOutOfRange                            0x31

/* define sid 21 */
#define SID_21_RAM_0_Star                                    0x70000000
#define SID_21_RAM_0_End                                     0x7001BFFF

#define SID_21_RAM_1_Star                                    0x70000000
#define SID_21_RAM_1_End                                     0x7001BFFF

#define SID_21_RAM_2_Star                                    0x60000000
#define SID_21_RAM_2_End                                     0x6001DFFF

#define SID_21_RAM_2_Star                                    0x60000000
#define SID_21_RAM_2_End                                     0x6001DFFF


/*********************************************************************************************************************/
/*-------------------------------------------------Global variables--------------------------------------------------*/
/*********************************************************************************************************************/
extern uint8 CAN2_SendAllow;
extern uint8 clockFlag;
extern uint8 checkPreProgramCondition;//标志位判断编程下载预先条件是否检查通过
extern uint8 close_OR_open_DTCCode;//0x85命令的标志位，用来判断是根据正常操作条件启用/禁用诊断故障代码

/*********************************************************************************************************************/
/*-------------------------------------------------Data Structures---------------------------------------------------*/
/*********************************************************************************************************************/
 
typedef enum
{
	//Diagnostic Management Functional Unit
	UDS_Diagnostic_SessionControl			= 0x10,//√
	UDS_ECU_Reset_softWare  				= 0x11,

	//Stored Data Transmission Functional Unit
	UDS_Clear_Diagnostic_Information        = 0x14,//√
	UDS_Read_Diagnostic_Information			= 0x19,//√

	//Data Transmission Functional Unit
	UDS_Read_Data_ByIdentifier				= 0x22,//√
	UDS_Write_Data_ByIdentifier				= 0x2e,//√

	UDS_Read_Memory_ByAddress				= 0x23,
	UDS_Write_Flash_ByAddress				= 0x3d,//WriteMemoryByAddress

	UDS_Read_ScalingData_ByIdentifier		= 0x24,
	UDS_Read_Data_ByPeriodicIdentifier		= 0x2a,
	UDS_Dynamically_DefineData_Identifier	= 0x2c,

	UDS_SECURITY_ACCESS 					= 0x27,//√
	UDS_Communication_Control				= 0x28,

	//Input/Output Control Functional Unit
	UDS_InputOutput_Control_ByIdentifier	= 0x2f,

	//Routine Functional Unit
	UDS_Routine_Control 					= 0x31,

	//Upload/Download Functional Unit
	UDS_Request_Download 					= 0x34,

	UDS_Request_Upload 						= 0x35,
	UDS_Transfer_Data 						= 0x36,
	UDS_Request_Transfer_Exit				= 0x37,
	UDS_Request_File_Transfer				= 0x38,


	UDS_Tester_Present  					= 0x3E,
	UDS_Access_Timing_Parameter				= 0x83,
	UDS_Secured_Data_Transmission			= 0x84,
	UDS_Control_DTCSetting					= 0x85,//0x28 服务一起使用
	UDS_Response_OnEvent					= 0x86,
	UDS_Link_Control						= 0x87,
}UDS_Command;

typedef struct
{
	uint8 timeInterval;//用于规定自己的诊断仪发送0x3E 服务的间隔
	uint8 timeoutService;//用于定义 ECU 收不到 0x3E 服务的 timeout 时间
}service3E_Time;

/*********************************************************************************************************************/
/*--------------------------------------------Private Variables/Constants--------------------------------------------*/
/*********************************************************************************************************************/

/*********************************************************************************************************************/
/*------------------------------------------------Function Prototypes------------------------------------------------*/
/*********************************************************************************************************************/

uint8 randomSeed(void);
uint8 calcKey(uint8 gtSeed);

int CAN_SSN_getFormatData(uint8 *bytes, uint8 length, uint32  *data );
int CAN_SSN_checkMemoryAddrAndSize(uint32 addr, uint32 size );

void CAN_SSN_TransmitNRC(diag_can_stt *txdata ,uint8 sID, uint8 nrcNum);
void CAN_SSN_TransmitPRC(diag_can_stt *txdata, uint8 RPNum);

void CAN_SSN_ServiceID10(diag_can_stt *rxdata,  diag_can_stt *txdata);//诊断会话控制服务√
void CAN_SSN_ServiceID11(diag_can_stt *rxdata,  diag_can_stt *txdata);//ECU复位服务√

void CAN_SSN_ServiceID14(diag_can_stt *rxdata,  diag_can_stt *txdata);//ECU复位服务√

void CAN_SSN_ServiceID19(diag_can_stt *rxdata,  diag_can_stt *txdata);
void CAN_SSN_ServiceID22(diag_can_stt *rxdata,diag_can_stt *txdata);//写设备下载信息
void CAN_SSN_ServiceID23(diag_can_stt *rxdata,diag_can_stt *txdata);//通过地址读取内存

void CAN_SSN_ServiceID27(diag_can_stt *rxdata,diag_can_stt *txdata);//安全访问服务√

void CAN_SSN_ServiceID28(diag_can_stt *rxdata,diag_can_stt *txdata);//安全访问服务√
void CAN_SSN_ServiceID2e(diag_can_stt *rxdata,diag_can_stt *txdata);//写数据

void CAN_SSN_ServiceID31(diag_can_stt *rxdata,diag_can_stt *txdata);//清除闪存
void CAN_SSN_ServiceID34(diag_can_stt *rxdata,diag_can_stt *txdata);
void CAN_SSN_ServiceID36(diag_can_stt *rxdata,diag_can_stt *txdata);//下载
void CAN_SSN_ServiceID37(diag_can_stt *rxdata,diag_can_stt *txdata);
void CAN_SSN_ServiceID3d(diag_can_stt *rxdata,diag_can_stt *txdata);

void CAN_SSN_ServiceID3e(diag_can_stt *rxdata,diag_can_stt *txdata);//诊断设备在线服务√

void CAN_SSN_ServiceID85(diag_can_stt *rxdata,diag_can_stt *txdata);
void CAN_SSN_ServiceID86(diag_can_stt *rxdata,diag_can_stt *txdata);

int CAN_SSN_ServiceWithId(void);

#endif /* CANTP_CAN_SESSION_H_ */
