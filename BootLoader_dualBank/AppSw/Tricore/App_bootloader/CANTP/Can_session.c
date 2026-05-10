
/**********************************************************************************************************************
 * \file    Can_Session.c
 * \brief
 * \version V1.0.0
 *********************************************************************************************************************/



/*********************************************************************************************************************/
/*-----------------------------------------------------Includes------------------------------------------------------*/
/*********************************************************************************************************************/
#include    "Can_Session.h"
//#include    "Std_Types.h"
#include    "Can.h"
#include    "CanIf.h"
#include    "App_bootloader_cfg.h"
/* mcu */
#include    "IfxFlash_cfg.h"
#include    "Flash.h"
#include    "Brd_led.h"
#include    "App_bootloader.h"
#include 	<stdio.h>
#include 	<stdlib.h>
#include 	<time.h>
#include "custom_delay.h"
#include "MultiCAN.h"


#define	defaultFill	0x55//发送的默认填充数值为0x55
#define nrpNumAdd 	0x40//正响应添加的数

/*********************************************************************************************************************/
/*-------------------------------------------------Global variables--------------------------------------------------*/
/*********************************************************************************************************************/
uint32 pageData[128];
uint8 checkPreProgramCondition = 0;
uint8 close_OR_open_DTCCode = 1;//0-close;1-open
int BackupSuccessFlag = 1;

/*********************************************************************************************************************/
/*--------------------------------------------Private Variables/Constants--------------------------------------------*/
/*********************************************************************************************************************/
//extern IFX_CONST IfxFlash_flashSector IfxFlash_pFlashTableLog[IFXFLASH_PFLASH_NUM_LOG_SECTORS];
/*********************************************************************************************************************/
/*------------------------------------------------Function Prototypes------------------------------------------------*/
/*********************************************************************************************************************/


/*********************************************************************************************************************/
/*---------------------------------------------Function Implementations----------------------------------------------*/
/*********************************************************************************************************************/

/*
** ============================================================================
** @Function    ：产生一个随机数
** @Description ：用于0x27的指令进行安全验证
** @Parameters  ：
** @Returns     ：
** @Date
** ============================================================================
*/
uint8 randomSeed(void)
{
    // 初始化随机数生成器，可以使用系统时钟来初始化
    srand((uint8)time(NULL));

    // 生成在[min, max]范围内的随机数
    uint8 min = 0x00;
    uint8 max = 0xff;
    return (min + rand() % (max - min + 1));
}

/*
** ============================================================================
** @Function    ：计算密钥
** @Description ：用于0x27的指令进行安全验证
** @Parameters  ：
** @Returns     ：
** @Date
** ============================================================================
*/
uint8 calcKey(uint8 gtSeed)
{
	return (gtSeed);
}

/*
** ============================================================================
** @Function    ：
** @Description ：校核内存地址及大小，用于0x23服务
** @Parameters  ：
** @Returns     ：
** @Date
** ============================================================================
*/
int CAN_SSN_checkMemoryAddrAndSize(uint32 addr, uint32 size )
{
    int r=noErr;

    /*
     * Dram0
     */
    if(( addr >= MCU_DSRAM0_Start) &&  (addr <= MCU_DSRAM0_End))
    {
        /*  addr ALIGNMENT_Size  */
        if( (addr % MCU_DSRAM0_ALIGNMENT_Size) > 0)
        {
            r |= para1Err;     /* para 1 err */
        }
        /*  check  addr ALIGNMENT_Size  */
        if((addr+size - 1) > MCU_DSRAM0_End)
        {
            r |= para2Err;
        }
    }

    /*
     * PF 0 and 1
     */
    else if(( addr >= FL_PFLASH_PF_ADDR_Start) &&  (addr <= FL_PFLASH_PF_ADDR_End))
    {
        /*  addr ALIGNMENT_Size  */
        if( (addr % FL_PFLASH_ALIGNMENT_Size) > 0)
        {
            r |= para1Err;     /* para 1 err */
        }
        /*  check  addr ALIGNMENT_Size  */
        if((addr + size - 1) >= (FL_PFLASH_PF_ADDR_End - 1))
        {
            r |= para2Err;
        }
    }

    /*
     * DF
     */
    else if(( addr >= FL_DFLASH_ADDR_Start) &&  (addr <= FL_DFLASH_ADDR_End))
    {

        /*  addr ALIGNMENT_Size  */
        if( (addr % FL_DFLASH_ALIGNMENT_Size) > 0)
        {
            r |= para1Err;     /* para 1 err */
        }
        /*  check  addr ALIGNMENT_Size  */
        if((addr + size - 1) > FL_DFLASH_ADDR_End)
        {
            r |= para2Err;
        }
    }

    /* out rage */
    else
    {
        r |=  para1Err;      /* para 1 err */
    }

    return r;
}

/*
** ============================================================================
** @Function    ：
** @Description ：按格式得到地址或长度，用于0x23服务
** @Parameters  ：
** @Returns     ：
** @Date
** ============================================================================
*/
int CAN_SSN_getFormatData(uint8 *bytes, uint8 length, uint32  *data )
{
    int r=0;
    int i;
    uint32 temp=0;;

    if ((length>0) && (length<5))
    {
        for(i=0;i<length;i++)
        {
            temp = (temp<<8) + bytes[i];
        }
        *data = temp;
        r=noErr;
    }
    else
    {
        r |= para2Err;        /*  para 2 err */
    }

    return r;
}

/*
** ============================================================================
** @Function    ：负响应的调用函数
** @Description ：故障出错的回应
** @Parameters  ：txdata->发送的数据，sID->请求的会话服务ID，nrcNum->故障代码
** @Returns     ：
** @Date        ：
** ============================================================================
*/
void CAN_SSN_TransmitNRC(diag_can_stt *txdata , uint8 sID, uint8 nrcNum)
{
    /*fill tx buffer of NRC*/
    txdata->idStandardFrameh = 0x03;
    txdata->idStandardFramel = 0xAA;

    txdata->dataLength = 3;
    txdata->data[0] = 0x7f;
    txdata->data[1] = sID;//sID
    txdata->data[2] = nrcNum;//xx

    /*transmit tx buffer*/
    CanIf_TransmitBuffer(&g_MulticanBasic.drivers.canNode1MsgObjTx, txdata );//0x 7f sID xx
}

/*
** ============================================================================
** @Function    ：正响应的调用函数
** @Description ：正确指令的回应
** @Parameters  ：txdata->发送的数据，RPNum->回应的数量
** @Returns     ：
** @Date        ：
** ============================================================================
*/
void CAN_SSN_TransmitPRC(diag_can_stt *txdata, uint8 RPNum)
{
    txdata->idStandardFrameh = 0x03;
    txdata->idStandardFramel = 0xAA;

    if(RPNum < 6)
    {
        txdata->dataLength = 6;//发送长度
    }
    else
    {
    	txdata->dataLength = RPNum;
    }

    /*fill tx buffer of PRC*/
    //其实后四个字节每两两进行组合，代表最大容忍时间
	for(uint8 i=RPNum; i<txdata->dataLength; i++)//以0x55填充除需要发的的
	{
		txdata->data[i] = defaultFill;//0x55 55 55 55
	}

    /*transmit tx buffer*/
    CanIf_TransmitBuffer(&g_MulticanBasic.drivers.canNode1MsgObjTx, txdata );//0x
}


uint8 CAN2_SendAllow=0;
/*
** ============================================================================
** @Function    ：
** @Description ：
** @Parameters  ：
** @Returns     ：
** @Date        ：
** ============================================================================
*/
void CAN_SSN_ServiceID10(diag_can_stt *rxdata,  diag_can_stt *txdata)
{
    uint8 *p;
    uint16 *p16;

    uint8 flag[2] = {0x5a,0xa5};
    uint8 r=0;
    uint32 i;

    switch(rxdata->data[1])//对应上位机的sendobj.Data[2]
    {
    	//	defaultSession	√
        case 0x01 ://can一直发送的状态--对标uds的default对话--10 01
        	if(rxdata->dataLength >= 3)//获取的消息的长度不对
        	{
        		CAN_SSN_TransmitNRC(txdata,rxdata->data[0],NRC_10_incorrectMessageLengthOrInvalidFormat);//发送故障码--0x 7f 10 13
        	}
        	else//正响应回复
        	{
        		CAN2_SendAllow=0;//允许can发送
        		txdata->data[0] = 0x50;
        		txdata->data[1] = rxdata->data[1];//0x01
        		/*transmit tx buffer*/
        		CAN_SSN_TransmitPRC(txdata, 2);// 0x 50 01 55 55 55 55 (上位机会先在CAN接收区显示这个数据)
        	}
            break;

		//	ProgrammingSession
        case 0x02 ://rxdata->data[1]=0x02     /*进入编程模式（BL）*/
//        	if(checkPreProgramCondition == 0)//编程的预先条件未通过
//        	{
//        		CAN_SSN_TransmitNRC(txdata,rxdata->data[0],NRC_10_conditionsNotCorrect);//发送故障码--0x 7f 10 13
//        	}
//        	else
//        	{
        		 if(rxdata->data[2] == connected)/*联机*/
				{
					CAN2_SendAllow=1;//不允许can发送
					txdata->data[0] = 0x50;
					txdata->data[1] = rxdata->data[1];//0x02
					txdata->data[2] = rxdata->data[2];//0x10
					p = (uint8 *)RAM_BOOT_MODE_Addr;//取地址0x7002Dffc(RAM_BOOT_MODE_KEEP)的数据--进入强制boot模式--RAM_BOOT_MODE_KEEP=0xf0b1
					txdata->data[3] = p[1];//数据1 0xf0
					txdata->data[4] = p[0];//数据2 0xb1
					/*transmit tx buffer*/
					CAN_SSN_TransmitPRC(txdata, 5);// 0x 50 02 10 f0 b1(上位机会先在CAN接收区显示这个数据)
				}
				else if(rxdata->data[2] == jumpToApp)  /*进入APP*///普通模式
				{
					CAN2_SendAllow=0;
					//相当于Id号码，对应上位机，一一匹配
					txdata->data[0] = 0x50;
					txdata->data[1] = rxdata->data[1];//0x02
					txdata->data[2] = rxdata->data[2];//0xa0

					*(uint16 *)RAM_BOOT_MODE_Addr = 0xb2b3;
					p = (uint8 *)RAM_BOOT_MODE_Addr;
					txdata->data[3] = p[1];//0xb2
					txdata->data[4] = p[0];//0xb3
					/*transmit tx buffer*/
					CAN_SSN_TransmitPRC(txdata, 5);// 0x 50 02 a0 b2 b3(上位机会先在CAN接收区显示这个数据)
				}
				else if(rxdata->data[2] == writeFlag)	/* 扩展功能，写标志 */
				{
				   	if((*(uint32 *)FL_APP_Update_FLAG_Addr) == 0)
					{
				   		Flash_BackupAppBlocks();
					}

					p16 = (uint16 *)FL_APP_FLAG_Addr;//读取地址0xa0018000(S6)的数据
					if(*p16 == 0)//S6没有数据的话，0xa55a u进行对标志位赋值
					{
						flag[0] = (uint8)FL_APP_FLAG;//0x5a
						flag[1] = (uint8)(FL_APP_FLAG >> 8 );//0xa5

						/*
						 * write flag
						 * */
	#if 0
						r = FLASH_ProgramFlashPageFunc(FL_APP_PFLASH_Addr,flag,2);
	#endif
						uint32 data[8];
						data[0] = *(uint32 *)FL_APP_FLAG_Addr;//读取地址0xa0018000(S6)的数据
						if(FL_APP_FLAG == data[0])     /* 已经有标志 */
						{
							r=0;//写0表示可以进入app模式
						}
						else//没有存储数据，标志不匹配
						{
							/* 1. erase sector */
							Flash_erasePFlash_port( FL_APP_FLAG_Addr );//清楚存储数据的标志位
							/* 2  write page */
							for(i=0;i<8;i++)
							{
								data[i] = 0;//清空数组数据
							}
							data[0] = FL_APP_FLAG;//0x 0000 a55a u
							//将地址0xa0018000(S6)写入标志位数据0x 0000 a55a u
							Flash_writePFlash_port( FL_APP_FLAG_Addr, data,  PFLASH_PAGE_LENGTH);    /* min = 1page = 32byte */

							//清空重新将数据0x 0000 a55a u写进入了地址0xa0018000(S6)
							if( FL_APP_FLAG == *(uint32 *)FL_APP_FLAG_Addr)//0xa55au的flash标志位的数据等于flash地址存储的数据
							{
								r=0;//数据写完，表示可以进入app模式

//								g_backToBeforeCodeS.loadIsSuccessResult = 0xaa;//正常下载成功
//								tempData[0] = g_backToBeforeCodeS.loadIsSuccessResult;
//								Flash_eraseDFlash_port(0xaf004000);//由于内存由1可以写0，不可以0写1，就先擦除
//								Flash_writeDFlash_port( 0xaf004000, tempData,  DFLASH_PAGE_LENGTH); //更新当前版本数据
							}
							else//仍然不匹配数据
							{
								r=3;//下载不成功/校验失败--故障
							}
						}
					}//end in S6没有数据(可以进行写标志)
					else{//S6有数据的话
						r = 2;//之前写过了，多次写记录?
					}//end in S6有数据
					if(r == 0)//S6没有数据但是通过上面将标志位写进去了
					{
						/* 写完标志后自动 跳到 APP */
						p = (uint8 *)RAM_BOOT_MODE_Addr;
						txdata->data[0] = 0x50;//
						txdata->data[1] = rxdata->data[1];//
						txdata->data[2] = 0xf0;//0xf0
						txdata->data[3] = p[1];//0xb2
						txdata->data[4] = p[0];//0xb3
						/*transmit tx buffer*/
						CAN_SSN_TransmitPRC(txdata, 5);// 0x 50 02 f0 b2 b3(上位机会先在CAN接收区显示这个数据)
					}//end in r=0
					else//r=2 或者 r=3
					{
						CAN_SSN_TransmitNRC(txdata,rxdata->data[0], NRC_10_incorrectMessageLengthOrInvalidFormat);//发送故障码--0x7f 10 13
					}//end in r=2 或者 r=3
//				}
        	}
        	break;

		//	extendedDiagnosticSession
        case 0x03 ://一般诊断仪启动之后，会给ECU发送10 03，让ECU进extendedDiagnosticSession(非defaultSession)中--对标uds的扩展对话--10 03
        	if(rxdata->dataLength >= 3)//获取的消息的长度不对
        	{
        		CAN_SSN_TransmitNRC(txdata,rxdata->data[0],NRC_10_incorrectMessageLengthOrInvalidFormat);//发送故障码--0x 7f 10 13
        	}
        	else//正响应回复
        	{
        		CAN2_SendAllow=1;//不允许can发送
        		txdata->data[0] = rxdata->data[0] + nrpNumAdd;//0x50
        		txdata->data[1] = rxdata->data[1];//0x03
        		/*transmit tx buffer*/
        		CAN_SSN_TransmitPRC(txdata, 2);// 0x 50 03 55 55 55 55 (上位机会先在CAN接收区显示这个数据)
        	}
            break;

		//	safetySystemDiagnosticSession
        case 0x04 ://rxdata->data[1]=0x04     /*进入联机模式（BL）--导致进入强制boot模式，即标志位变为RAM_BOOT_MODE_KEEP=0xf0b1*/

            break;

        default://0x10命令下面的子功能不支持（未进行定义的功能）
        	CAN_SSN_TransmitNRC(txdata,rxdata->data[0],NRC_10_subFunctionNotSupported);//发送故障码--0x 7f 10 12
            break;
    }
}

/*
** ============================================================================
** @Function    ：软件复位
** @Description ：
** @Parameters  ：
** @Returns     ：
** @Date        ：
** ============================================================================
*/
void CAN_SSN_ServiceID11(diag_can_stt *rxdata,diag_can_stt *txdata)
{
	if(rxdata->dataLength >= 3)//获取的消息的长度不对
	{
		CAN_SSN_TransmitNRC(txdata,rxdata->data[0],NRC_10_incorrectMessageLengthOrInvalidFormat);//发送故障码--0x 7f 10 13
	}
	else//正响应回复
	{
		txdata->data[0] = rxdata->data[0] + nrpNumAdd;//0x51
		txdata->data[1] = rxdata->data[1];//0x01
		/*transmit tx buffer*/
		CAN_SSN_TransmitPRC(txdata, 2);// 0x 51 01 55 55 55 55 (上位机会先在CAN接收区显示这个数据)
	    switch(rxdata->data[1])//对应上位机的sendobj.Data[2]
	    {
	        case 0x01 ://HardReset->模拟KL30电源的重上电
	        	IfxPort_setPinHigh(LED1);//for test--2024-07-16
	        	break;

//	        case 0x02 ://KeyOffOnReset->模拟KL15点火钥匙的重启
//	        	break;

	        case 0x03 ://SoftReset
	        	SW_Reset();//软件复位，相当于初始化重新开始
	        	break;

//	        case 0x04 ://enableRapidPowerShutDown
//	        	break;

//	        case 0x05 ://disableRapidPowerShutDown（禁用快速断电）
//	        	break;

	        default://0x11命令下面的子功能不支持（未进行定义的功能）
	        	CAN_SSN_TransmitNRC(txdata,rxdata->data[0],NRC_11_subFunctionNotSupported);//发送故障码--0x 7f 11 12
	            break;
	    }
	}
}

/*
** ============================================================================
** @Function    ：清除诊断信息
** @Description ：
** @Parameters  ：
** @Returns     ：
** @Date        ：
** ============================================================================
*/
void CAN_SSN_ServiceID14(diag_can_stt *rxdata,diag_can_stt *txdata)
{
	//清除存在存储器的诊断信息
}


/*
** ============================================================================
** @Function    ：在此DID下面进行编程等操作
** @Description ：Read_Diagnostic_Information (Ox19) service
** @Parameters  ：
** @Returns     ：
** @Date        ：
** ============================================================================
*/
void CAN_SSN_ServiceID19(diag_can_stt *rxdata,diag_can_stt *txdata)
{
	if(rxdata->dataLength >= 4)//获取的消息的长度不对
	{
		CAN_SSN_TransmitNRC(txdata,rxdata->data[0],NRC_10_incorrectMessageLengthOrInvalidFormat);//发送故障码--0x 7f 10 13
	}
}

/*
** ============================================================================
** @Function    ：在此DID下面进行编程等操作
** @Description ： Read_Data_ByIdentifier (Ox22) service
** @Description ： 读版本信息--读取被刷新的ECU的状态（如：刷新的应用软件和数据）
** @Parameters  ：
** @Returns     ：
** @Date        ：
** ============================================================================
*/
void CAN_SSN_ServiceID22(diag_can_stt *rxdata,diag_can_stt *txdata)
{

    uint32 data[2] ;
    data[0] = (rxdata->data[4]<<16)+(rxdata->data[5]<<8)+(rxdata->data[6]);

	//将地址0xa0018000(S6)写入标志位数据0x 0000 a55a u
//    if((*(uint32 *)FL_APP_Update_FLAG_Addr) == 0)//首次下载，没有数据
//    {
//    	g_backToBeforeCodeS.isCpyFlag = TRUE;//拷贝标志，需要备份
//    	data[1] = g_backToBeforeCodeS.isCpyFlag;
//    	Flash_eraseDFlash_port(FL_APP_Update_FLAG_Addr);//由于内存由1可以写0，不可以0写1，就先擦除
//		Flash_writeDFlash_port( FL_APP_Update_FLAG_Addr, data,  DFLASH_PAGE_LENGTH);    /* min = 1page = 32byte */
//    }
//    else//有数据的话
//    {
    	//按时间来区分下载版本是否一样
    	if((*(uint32 *)FL_APP_Update_FLAG_Addr) == data[0])//下载的版本一样，不需要备份
    	{
    		//nothing to do
    		g_backToBeforeCodeS.isCpyFlag = FALSE;//拷贝标志，不需要备份
			data[1] = g_backToBeforeCodeS.isCpyFlag;
    		Flash_eraseDFlash_port(FL_APP_Update_FLAG_Addr);//由于内存由1可以写0，不可以0写1，就先擦除
    		Flash_writeDFlash_port( FL_APP_Update_FLAG_Addr, data,  DFLASH_PAGE_LENGTH); //更新当前版本数据
    	}
    	else//需要备份
    	{
    		g_backToBeforeCodeS.isCpyFlag = TRUE;//拷贝标志，需要备份
			data[1] = g_backToBeforeCodeS.isCpyFlag;
    		Flash_eraseDFlash_port(FL_APP_Update_FLAG_Addr);//由于内存由1可以写0，不可以0写1，就先擦除
    		Flash_writeDFlash_port( FL_APP_Update_FLAG_Addr, data,  DFLASH_PAGE_LENGTH); //更新当前版本数据
    	}
//    }

	if(rxdata->dataLength > 8)//获取的消息的长度不对
	{
		CAN_SSN_TransmitNRC(txdata,rxdata->data[0],NRC_22_incorrectMessageLengthOrInvalidFormat);//发送故障码--0x 7f 22 13
	}
	else//正响应回复
	{
	    switch(rxdata->data[1])//对应上位机的sendobj.Data[2]
	    {
	        case 0xF1 ://0xF1
	        	switch(rxdata->data[2])
	        	{
	        	 	 case 0x86 ://ActiveDiagnosticSessionDataldentifier
	     	        	switch(rxdata->data[3])
	     	        	{
	     	        	 	case 0x02 :
	     	        	 		//do sth.
	     	                   txdata->data[0] = rxdata->data[0] + 0x40;//0x62
	     	                   txdata->data[1] = rxdata->data[1];//设备id-F1
							   txdata->data[2] = rxdata->data[2];//设备id-86
							   txdata->data[3] = rxdata->data[3];//设备id-02
							   //以下要写进eeprom
	     	                   txdata->data[4] = rxdata->data[4];//date-24
	     	                   txdata->data[5] = rxdata->data[5];//date-12
	     	                   txdata->data[6] = rxdata->data[6];//date-13

	     	                   /*transmit tx buffer*/
	     	                   CAN_SSN_TransmitPRC(txdata, 7);// 0x 62 F1 86 02 24 08 23(上位机会先在CAN接收区显示这个数据)
	     	        	 		break;
							default://0xF1命令下面的子功能不支持（未进行定义的功能）
								CAN_SSN_TransmitNRC(txdata,rxdata->data[0],NRC_22_incorrectMessageLengthOrInvalidFormat);//发送故障码--0x 7f 22 12
								break;
	     	        	}
	        	 		break;
					 case 0x87 ://vehicleManufacturerSparePartNumberDataldentifier
						switch(rxdata->data[3])
						{
							case 0x02 ://Programming session
								//do sth.
								 break;
							default://0xF1命令下面的子功能不支持（未进行定义的功能）
								CAN_SSN_TransmitNRC(txdata,rxdata->data[0],NRC_22_incorrectMessageLengthOrInvalidFormat);//发送故障码--0x 7f 22 12
								break;
						}
						break;
					 case 0x88 ://vehicleManufacturerECUSoftwareNumberDataldentifier
						switch(rxdata->data[3])
						{
							case 0x02 ://Programming session
								//do sth.
								 break;
							default://0xF1命令下面的子功能不支持（未进行定义的功能）
								CAN_SSN_TransmitNRC(txdata,rxdata->data[0],NRC_22_incorrectMessageLengthOrInvalidFormat);//发送故障码--0x 7f 22 12
								break;
						}
						break;
					 case 0x89 ://vehicleManufacturerECUSoftwareVersionNumberDataldentifier
						switch(rxdata->data[3])
						{
							case 0x02 ://Programming session
								//do sth.
								 break;
							default://0xF1命令下面的子功能不支持（未进行定义的功能）
								CAN_SSN_TransmitNRC(txdata,rxdata->data[0],NRC_22_incorrectMessageLengthOrInvalidFormat);//发送故障码--0x 7f 22 12
								break;
						}
						break;
					 case 0x8A ://systemSupplierldentifierDataldentifier
						switch(rxdata->data[3])
						{
							case 0x02 ://Programming session
								//do sth.
								 break;
							default://0xF1命令下面的子功能不支持（未进行定义的功能）
								CAN_SSN_TransmitNRC(txdata,rxdata->data[0],NRC_22_incorrectMessageLengthOrInvalidFormat);//发送故障码--0x 7f 22 12
								break;
						}
						break;
					 case 0x90 ://VIN码
						switch(rxdata->data[3])
						{
							case 0x02 ://Programming session
								//do sth.
							   txdata->data[0] = 0x10;//0x10
							   txdata->data[1] = 0x0A;//0x0A
							   txdata->data[2] = rxdata->data[0] + 0x40;//0x62
							   txdata->data[3] = rxdata->data[1];//0xF1
							   txdata->data[4] = rxdata->data[2];//0x90
							   /*transmit tx buffer*/
							   CAN_SSN_TransmitPRC(txdata, 5);// 0x 10 0A 62 F1 90(上位机会先在CAN接收区显示这个数据)
							   break;
							default://0xF1命令下面的子功能不支持（未进行定义的功能）
								CAN_SSN_TransmitNRC(txdata,rxdata->data[0],NRC_22_incorrectMessageLengthOrInvalidFormat);//发送故障码--0x 7f 22 12
								break;
						}
						break;
					default://0xF1命令下面的子功能不支持（未进行定义的功能）
							CAN_SSN_TransmitNRC(txdata,rxdata->data[0],NRC_22_conditionsNotCorrect);//发送故障码--0x 7f 22 12
							break;
	        	}
	        	break;

	        case 0x0C:
	        	switch(rxdata->data[2])
				{
	        		case 0x04 ://获取发动机的轮速
	                    txdata->data[0] = rxdata->data[0] + 0x40;//0x62
					    txdata->data[1] = rxdata->data[1];//0x0C
					    txdata->data[2] = rxdata->data[2];//0x04
					   /*transmit tx buffer*/
					    CAN_SSN_TransmitPRC(txdata, 4);// 0x 62 0C 04(上位机会先在CAN接收区显示这个数据)
	        			break;
	        		default://0xF1命令下面的子功能不支持（未进行定义的功能）
						CAN_SSN_TransmitNRC(txdata,rxdata->data[0],NRC_22_conditionsNotCorrect);//发送故障码--0x 7f 22 12
						break;
				}


	        	break;

			default://0x22命令下面的子功能不支持（未进行定义的功能）
				CAN_SSN_TransmitNRC(txdata,rxdata->data[0],NRC_22_conditionsNotCorrect);//发送故障码--0x 7f 22 12
				break;
	    }
	}
}

/*
** ============================================================================
** @Function    ：通过地址读取内存
** @Description ：       ReadMemoryByAddress (Ox23) service
** @Parameters  ：
** @Returns     ：
** @Date        ：
** ============================================================================
*/
void CAN_SSN_ServiceID23(diag_can_stt *rxdata,diag_can_stt *txdata)
{
    uint32 addr=0,i;
    uint32 dataLength=0;
    uint8 addrBytes,dataBytes;
    uint8 *p;
    int  r=0;

    CAN2_SendAllow=1;//不允许can发送

    CAN_CleanTxBuffer(txdata);
    /* get byte length */
    addrBytes = rxdata->data[1] & 0x0f;//地址
    dataBytes = (rxdata->data[1] & 0xf0) >> 4;//数据

    /* check nrc 13 and get data  */
    r = CAN_SSN_getFormatData( &rxdata->data[2], addrBytes, &addr);      /* data[2] : sdi 23 + format  */
    r |= CAN_SSN_getFormatData( &rxdata->data[2 + addrBytes], dataBytes, &dataLength);      /* data[ ] :sdi 23 +format + adddrt  */

    if(r > 0)
    {
        CAN_SSN_TransmitNRC(txdata ,rxdata->data[0], NRC_incorrectMessageLengthOrlnvalidFormat);
    }
    else        /* r=0 */
    {
        /* check nrc 31 */
        r = CAN_SSN_checkMemoryAddrAndSize(addr, dataLength);
    }

    if(r > 0)
    {
        /* send nrc 31 */
        CAN_SSN_TransmitNRC(txdata , rxdata->data[0], NRC_requestOutOfRange);//是否为dflash的地址
    }
    else
    {
        /*read data and fill buffer*/
        txdata->data[0] = 0x63;//0x23 + 0x40
        txdata->dataLength =  (uint16)dataLength + 1;//数据+1
        p = (uint8 *)addr;

        for(i=0;i<dataLength;i++)
        {
            txdata->data[1 + i] = *p;
            p++;
        }
        /*transmit tx buffer*/
        CAN_SSN_TransmitPRC(txdata, (uint8)txdata->dataLength);//0x
    }

    return ;
}

/*
** ============================================================================
** @Function    ：安全访问服务
** @Description ：这个是ecu重置（0x11）的先提条件
** @Parameters  ：
** @Returns     ：
** @Date        ：
** ============================================================================
*/
uint8 clockFlag = 0;
void CAN_SSN_ServiceID27(diag_can_stt *rxdata,diag_can_stt *txdata)
{
//	如何解锁安全模式？
//	第一步：客户端发送seed请求
//	第二步：服务端发出seed
//	第三步：客户端发送key密钥，依据服务发出的seed进行处理
//	第四步：服务端分析客户端发过来的key密钥，如果无误则完成解锁功能
	switch(rxdata->data[1] % 2)//对应上位机的sendobj.Data[2]
	{
		case 0x00 ://send key
			if(rxdata->dataLength > 4)//获取的消息的长度不对
			{
				CAN_SSN_TransmitNRC(txdata,rxdata->data[0],NRC_3e_incorrectMessageLengthOrInvalidFormat);//发送故障码--0x 7f 27 13
			}
			else//正响应回复
			{
				txdata->data[0] = rxdata->data[0];//0x27
				txdata->data[1] = rxdata->data[1];//0x02
//		    		txdata->data[2] = calcKey(rxdata->data[2]);
//		    		txdata->data[3] = calcKey(rxdata->data[3]);

				if(((rxdata->data[2]) != calcKey(rxdata->data[2]))&&((rxdata->data[3]) != calcKey(rxdata->data[3])))//key计算错误
				{
					CAN_SSN_TransmitNRC(txdata,rxdata->data[0],NRC_27_invalidKey);//发送故障码--0x 7f 27 35
				}
				else//发送计算正确的key
				{
					/*transmit tx buffer*/
					clockFlag = 1; //可以进行解锁
					CAN_SSN_TransmitPRC(txdata, 2);// 0x 27 02 55 55 55 55 (上位机会先在CAN接收区显示这个数据)
				}
			}
			break;

		case 0x01 ://send seed request
			if(rxdata->dataLength > 2)//获取的消息的长度不对
			{
				CAN_SSN_TransmitNRC(txdata,rxdata->data[0],NRC_3e_incorrectMessageLengthOrInvalidFormat);//发送故障码--0x 7f 27 13
			}
			else//正响应回复
			{
				txdata->data[0] = rxdata->data[0] + nrpNumAdd;//0x67
				txdata->data[1] = rxdata->data[1];//0x01
				txdata->data[2] = randomSeed();//发送生成的随机种子数  high8
				txdata->data[3] = randomSeed();//发送生成的随机种子数  low 8
				/*transmit tx buffer*/
				CAN_SSN_TransmitPRC(txdata, 4);// 0x 67 01 55 55 55 55 (上位机会先在CAN接收区显示这个数据)
			}
			break;

		default://0x27命令下面的子功能不支持（未进行定义的功能）
			CAN_SSN_TransmitNRC(txdata,rxdata->data[0],NRC_27_subFunctionNotSupported);//发送故障码--0x 7f 27 12
			break;
	}
}


/*
** ============================================================================
** @Function    ：安全访问服务
** @Description ：这个是uds通信控制（0x28）的先提条件
** @Parameters  ：
** @Returns     ：
** @Date        ：
** ============================================================================
*/
void CAN_SSN_ServiceID28(diag_can_stt *rxdata,diag_can_stt *txdata)
{
	if(rxdata->dataLength >= 4)//获取的消息的长度不对
	{
		CAN_SSN_TransmitNRC(txdata,rxdata->data[0],NRC_10_incorrectMessageLengthOrInvalidFormat);//发送故障码--0x 7f 10 13
	}
	else//正响应回复
	{
	    switch(rxdata->data[1])//对应上位机的sendobj.Data[2]
	    {
	        case 0x03 ://03表示关闭接收和发送
	        	switch(rxdata->data[2])
	        	{
	        		case 0x01 ://0x1禁止所有非诊断通信
	        			CAN2_SendAllow=1;//不允许can发送
			    		txdata->data[0] = rxdata->data[0] + nrpNumAdd;//0x68
			    		txdata->data[1] = rxdata->data[1];//0x03
			    		txdata->data[2] = rxdata->data[2];//0x01
			    		/*transmit tx buffer*/
			    		CAN_SSN_TransmitPRC(txdata, 3);// 0x 68 03 01 55 55 55 (上位机会先在CAN接收区显示这个数据)
	        			break;

	        		case 0x02 ://0x2恢复所有通信
	        			CAN2_SendAllow=0;//允许can发送
			    		txdata->data[0] = rxdata->data[0] + nrpNumAdd;//0x68
			    		txdata->data[1] = rxdata->data[1];//0x03
			    		txdata->data[2] = rxdata->data[2];//0x01
			    		/*transmit tx buffer*/
			    		CAN_SSN_TransmitPRC(txdata, 3);// 0x 68 03 02 55 55 55 (上位机会先在CAN接收区显示这个数据)
	        			break;

	        		case 0x03 ://0x03禁止特定的非诊断通信
//	        			CAN_SendAllow=0;//允许can发送
			    		txdata->data[0] = rxdata->data[0] + nrpNumAdd;//0x68
			    		txdata->data[1] = rxdata->data[1];//0x03
			    		txdata->data[2] = rxdata->data[2];//0x01
			    		/*transmit tx buffer*/
			    		CAN_SSN_TransmitPRC(txdata, 3);// 0x 68 03 03 55 55 55 (上位机会先在CAN接收区显示这个数据)
	        			break;

	        		case 0x04 ://0x04恢复特定的非诊断通信
//	        			CAN_SendAllow=0;//允许can发送
			    		txdata->data[0] = rxdata->data[0] + nrpNumAdd;//0x68
			    		txdata->data[1] = rxdata->data[1];//0x03
			    		txdata->data[2] = rxdata->data[2];//0x01
			    		/*transmit tx buffer*/
			    		CAN_SSN_TransmitPRC(txdata, 3);// 0x 68 03 04 55 55 55 (上位机会先在CAN接收区显示这个数据)
	        			break;

	        		default://0x28命令下面的子功能不支持（未进行定义的功能）
	        			CAN_SSN_TransmitNRC(txdata,rxdata->data[0],NRC_28_subFunctionNotSupported);//发送故障码--0x 7f 28 12
	        			break;
	        	}
	        	break;
			default://0x28命令下面的子功能不支持（未进行定义的功能）
				CAN_SSN_TransmitNRC(txdata,rxdata->data[0],NRC_28_conditionsNotCorrect);//发送故障码--0x 7f 28 22
				break;
	    }
	}
}

/*
** ============================================================================
** @Function    ：写数据服务
** @Description ：
** @Parameters  ：
** @Returns     ：
** @Date        ：
** ============================================================================
*/
void CAN_SSN_ServiceID2e(diag_can_stt *rxdata,diag_can_stt *txdata)
{
	if(rxdata->dataLength < 2)//获取的消息的长度不对
	{
		CAN_SSN_TransmitNRC(txdata,rxdata->data[0],NRC_2e_incorrectMessageLengthOrInvalidFormat);//发送故障码--0x 7f 2e 13
	}
	else//正响应回复
	{
		if(rxdata->dataLength > 8)//获取的消息的长度不对
		{
			CAN_SSN_TransmitNRC(txdata,rxdata->data[0],NRC_2e_incorrectMessageLengthOrInvalidFormat);//发送故障码--0x 7f 2e 13
		}
		else
		{
		    switch(rxdata->data[1])//对应上位机的sendobj.Data[2]
		    {
		    	case 0xF1:
		    		switch(rxdata->data[2])//对应上位机的sendobj.Data[2]
		    		{
		    			case 0x5A:
		    				txdata->data[0] = rxdata->data[0] + nrpNumAdd;//0x6e
		    				txdata->data[1] = rxdata->data[1];//0xF1
		    				txdata->data[2] = rxdata->data[2];//0x5A
		    				/*transmit tx buffer*/
		    				CAN_SSN_TransmitPRC(txdata, 3);// 0x 6e 03 04 55 55 55 (上位机会先在CAN接收区显示这个数据)
		    				break;

			    		default://0x2e命令下面的子功能不支持（未进行定义的功能）
							CAN_SSN_TransmitNRC(txdata,rxdata->data[0],NRC_2e_conditionsNotCorrect);//发送故障码--0x 7f 2e 22
							break;
		    		}
		    	break;

				default://0x28命令下面的子功能不支持（未进行定义的功能）
					CAN_SSN_TransmitNRC(txdata,rxdata->data[0],NRC_2e_conditionsNotCorrect);//发送故障码--0x 7f 28 22
					break;
		    }
		}

	}
}
/*
** ============================================================================
** @Function    ：查边界条件、清除闪存、对数据进行较验、对软硬件依赖性进行校验
** @Description ：       RoutineControl (Ox31) service
** @Description ：       刷写准备条件检查
** @Parameters  ：
** @Returns     ：
** @Date        ：   09-18
** ============================================================================
*/
void CAN_SSN_ServiceID31(diag_can_stt *rxdata,diag_can_stt *txdata)
{
    uint16 blockNum,routineIdentifier,u16Tmp;
    uint8 r,i;

    routineIdentifier =  rxdata->data[2];//data2
    routineIdentifier = routineIdentifier<<8;//数据都跑去高8位
    routineIdentifier += rxdata->data[3];//高八位为data2+低八位为data3

	if(rxdata->dataLength < 2)//获取的消息的长度不对
	{
		CAN_SSN_TransmitNRC(txdata,rxdata->data[0],NRC_31_incorrectMessageLengthOrInvalidFormat);//发送故障码--0x 7f 31 13
	}
	else//正响应回复
	{
		if(rxdata->dataLength > 8)//获取的消息的长度不对
		{
			CAN_SSN_TransmitNRC(txdata,rxdata->data[0],NRC_31_incorrectMessageLengthOrInvalidFormat);//发送故障码--0x 7f 31 13
		}
		else
		{
			switch(rxdata->data[1])
			{
				case 0x01 ://rxdata->data[1] = 0x01
					if(0xff00 == routineIdentifier )/*Erase - Pflash*/
					{
						blockNum = rxdata->data[4];
						blockNum = blockNum<<8;
						blockNum += rxdata->data[5];

						/*check Sector Index = boot sector*/
						if(blockNum < 2)
						{
							CAN_SSN_TransmitNRC(rxdata,rxdata->data[0],NRC_31_conditionsNotCorrect);//7f 31 e0->22
							return;
						}

						/*
						 * Erase
						 * */
						r = (uint8)Flash_erasePFlash_port( IfxFlash_pFlashTableLog[blockNum].start );//选着擦除哪一块

						/*fill tx buffer*/
						if(0 == r)/*SUCCESS*/
						{
							CAN_CleanTxBuffer(txdata);
							//正确的响应
							txdata->data[0] = 0x71;
							for(i=1;i<6;i++)
							{
								txdata->data[i] = rxdata->data[i];
							}
							/*transmit tx buffer*/
							CAN_SSN_TransmitPRC(txdata, 6);//0x71 01 ff 00 xx xx
						}
						else/*Err*/
						{
							/* NRC*/
							CAN_SSN_TransmitNRC(rxdata,rxdata->data[0],r);//发送故障情况
						}
					}

					else if(SID31_DID_EraseDFlash == routineIdentifier )/*Erase - D-flash*/
					{
						blockNum = rxdata->data[4];
						blockNum = blockNum<<8;
						blockNum += rxdata->data[5];

						/*
						 * Erase
						 * */
						r = (uint8)Flash_eraseDFlash_port( IfxFlash_dFlashTableEepLog[blockNum].start );//选着擦除哪一块

						/*fill tx buffer*/
						if(0 == r)/*SUCCESS*/
						{
							CAN_CleanTxBuffer(txdata);
							//正确的响应
							txdata->data[0] = 0x71;
							for(i=1;i<6;i++)
							{
								txdata->data[i] = rxdata->data[i];
							}

							/*transmit tx buffer*/
							CAN_SSN_TransmitPRC(txdata, 6);//0x
						}
						else/*Err*/
						{
							/* NRC*/
							CAN_SSN_TransmitNRC(rxdata,rxdata->data[0],r);
						}

					}
					else if(0x0203 == routineIdentifier)/*CheckProgrammingPreConditions*/        //data2 + data3
					{
						checkPreProgramCondition = 1;//编程的先决条件通过
						//正确的响应
						txdata->data[0] = 0x71;
						for(i=1;i<4;i++)
						{
							txdata->data[i] = rxdata->data[i];//01 02 03
						}
						u16Tmp = *(uint16 *)RAM_BOOT_MODE_Addr;//0x7002Dffc
						txdata->data[4] = (uint8)(u16Tmp>>8);//b2
						txdata->data[5] = (uint8)(u16Tmp);//b3
						u16Tmp = *(uint16 *)FL_APP_PRG_PFLASH_Addr;
						txdata->data[6] = (uint8)(u16Tmp>>8);//
						txdata->data[7] = (uint8)(u16Tmp);//

						/*transmit tx buffer*/
						CAN_SSN_TransmitPRC(txdata, 8);//0x 71 01 02 03 b2 b3 xx xx

					}
					break;
				default:
					CAN_SSN_TransmitNRC(rxdata,rxdata->data[0],NRC_31_subFunctionNotSupported);//子功能不支持
					break;
			}
		}
	}
}

/*
** ============================================================================
** @Function    ：
** @Description ： RequestDownload (Ox34) service
** @Parameters  ：不需要子功能
** @Returns     ：
** @Date        ：   07-25
** ============================================================================
*/
void CAN_SSN_ServiceID34(diag_can_stt *rxdata,diag_can_stt *txdata)
{
	if(rxdata->dataLength < 3)//获取的消息的长度不对--check minimum length
	{
		CAN_SSN_TransmitNRC(txdata,rxdata->data[0],NRC_34_incorrectMessageLengthOrInvalidFormat);//发送故障码--0x 7f 34 13
	}
	else//正响应回复
	{
		if((rxdata->data[1]<=0xff)&&(rxdata->data[2]<=0xff))//check dataFormatidentifier is valid AND addressAndLengthFormatidentifier is valid
		{
			if(rxdata->dataLength <= 8)//check full length
			{
				if(rxdata->data[3]<=0xff)//check memoryAddress/memorySize is valid
				{
					if(clockFlag==1)//0x27 命令检查通过--check security ok? for requested memory interval--差一个时间统计2024/08/08
					{
						if(1)//check condition--此处给什么条件？2024/08/08
						{
							if(1)//check download fault condition--此处给什么条件？2024/08/08
							{
								//注：以下注释是因为此暂时不需要手动给定，后面根据需求在解开注释
//								if(1)//check manufacturer/supplier specific check--此处给什么条件？2024/08/08
//								{
									//正确的响应
									txdata->data[0] = rxdata->data[0] + 0x40;//0x74
									txdata->data[1] = rxdata->data[1] & 0x0f;//传输块的首地址
									txdata->data[1] = (rxdata->data[1] & 0xf0) >> 4;//传输数据的数目

									/*transmit tx buffer*/
									CAN_SSN_TransmitPRC(txdata, 6);//0x
//								}
//								else//check manufacturer/supplier specific check
//								{
//									CAN_SSN_TransmitNRC(txdata,rxdata->data[0],0x00);//发送故障码--0x 7f 00 31--发送的数值是手动给的2024/08/08
//								}
							}
							else//check download fault condition
							{
								CAN_SSN_TransmitNRC(txdata,rxdata->data[0],NRC_34_generalProgrammingFailure);//发送故障码--0x 7f 70 31
							}
						}
						else//check condition
						{
							CAN_SSN_TransmitNRC(txdata,rxdata->data[0],NRC_34_conditionsNotCorrect);//发送故障码--0x 7f 22 31
						}
					}
					else//check security ok? for requested memory interval
					{
						CAN_SSN_TransmitNRC(txdata,rxdata->data[0],NRC_34_securityAccessDenied);//发送故障码--0x 7f 33 31
					}
				}
				else
				{
					CAN_SSN_TransmitNRC(txdata,rxdata->data[0],NRC_34_requestOutOfRange);//发送故障码--0x 7f 34 31
				}
			}
			else//>8
			{
				CAN_SSN_TransmitNRC(txdata,rxdata->data[0],NRC_34_incorrectMessageLengthOrInvalidFormat);//发送故障码--0x 7f 34 13
			}
		}
		else
		{
			CAN_SSN_TransmitNRC(txdata,rxdata->data[0],NRC_34_requestOutOfRange);//发送故障码--0x 7f 34 31
		}

	}
}

/*
** ============================================================================
** @Function    ：
** @Description ： Transfer Data (Ox36) service
** @Parameters  ：不需要子功能
** @Returns     ：
** @Date        ：   08-12
** ============================================================================
*/
//uint8  blockSequenceCounter = 1;//标识当前传输的是第几个数据块，或者简单地说就是第几次调用 36 服务
//void CAN_SSN_ServiceID36(diag_can_stt *rxdata,diag_can_stt *txdata)
//{
//	blockSequenceCounter++;//每个后续的 TransferData 请求，其值将增加 1
//	if(rxdata->dataLength < 3)//获取的消息的长度不对--check minimum length
//	{
//		CAN_SSN_TransmitNRC(txdata,rxdata->data[0],NRC_36_incorrectMessageLengthOrInvalidFormat);//发送故障码--0x 7f 36 13
//	}
//	else//正响应回复
//	{
//		if(blockSequenceCounter > 0xFF)//重置序列号
//		{
//			blockSequenceCounter = 1;
//			CAN_SSN_TransmitNRC(txdata,rxdata->data[0],NRC_36_wrongBlockSequenceCounter);//发送故障码--0x 7f 36 73
//		}
//		else
//		{
////			if(rxdata->data[0]!=0x36)
//			if(1)//给个撒子条件？
//			{
//				CAN_SSN_TransmitNRC(txdata,rxdata->data[0],NRC_36_requestSequenceError);//发送故障码--0x 7f 36 24
//			}
//			else
//			{
//				if(rxdata->data[1])
//				{
//					CAN_SSN_TransmitNRC(txdata,rxdata->data[0],NRC_36_transDataSuspend);//发送故障码--0x 7f 36 71
//				}
//				else
//				{
//					//正确的响应
//					txdata->data[0] = rxdata->data[0] + 0x40;//0x76
//					txdata->data[1] = blockSequenceCounter;//传输块的首地址
//			//		txdata->data[1] = rxdata->data[2];//传输数据的数目
//
//					/*transmit tx buffer*/
//					CAN_SSN_TransmitPRC(txdata, 2);//0x76 xx
//				}
//			}
//		}
//	}
//}

/*
** ============================================================================
** @Function    ：
** @Description ： 退出传输数据 (Ox37) service
** @Parameters  ：不需要子功能
** @Returns     ：
** @Date        ：   08-12
** ============================================================================
*/
void CAN_SSN_ServiceID37(diag_can_stt *rxdata,diag_can_stt *txdata)
{
	uint8  blockSequenceCounter = 0;//标识当前传输的是第几个数据块，或者简单地说就是第几次调用 36 服务


	if(rxdata->dataLength < 3)//获取的消息的长度不对--check minimum length
	{
		CAN_SSN_TransmitNRC(txdata,rxdata->data[0],NRC_37_incorrectMessageLengthOrInvalidFormat);//发送故障码--0x 7f 36 13
	}
	else//正响应回复
	{
		if(blockSequenceCounter < 0xFF)
		{
			//正确的响应
			txdata->data[0] = rxdata->data[0] + 0x40;//0x77
			txdata->data[1] = blockSequenceCounter;//传输块的首地址
	//		txdata->data[1] = rxdata->data[2];//传输数据的数目

			/*transmit tx buffer*/
			CAN_SSN_TransmitPRC(txdata, 2);//0x77 xx
		}
		else
		{
			CAN_SSN_TransmitNRC(txdata,rxdata->data[0],NRC_37_requestOutOfRange);//发送故障码--0x 7f 36 31
		}
	}
}


/*
** ============================================================================
** @Function    ：进行flash数据的写入，根据选择的地址选择是PFLASH还是DFLASH
** @Description ：
** @Parameters  ：
** @Returns     ：none
** @Date        ：09-18
** ============================================================================
*/

void CAN_SSN_ServiceID3d(diag_can_stt *rxdata,diag_can_stt *txdata)
{

    uint32 addr=0, i;
    uint16 dataLength=0;
    uint8  addrBytes,dataBytes;
    uint8 *p,r;

    // 根据前面的假设，那么此时缓存数据的前面几个字节为：rxdata= 0x 3d 24  a0 02 00 00 01 00,运行到此函数第0字节应该为0x3d，曾军
    addrBytes =  rxdata->data[1] & 0x0f;            /* 低4位，地址字节数 */  //4
    dataBytes = (rxdata->data[1] & 0xf0) >> 4;      /* 高4位  = 长度字节数 */ //2

    /* get first addr */
    for(i=0;i<addrBytes;i++)//4
    {
        addr = addr << 8;//00 00 00 00      //1.00 00 00 a0; 2.00 00 a0 02; 3.00 a0 02 00; 4.a0 02 00 00
        addr += rxdata->data[2 + i];	// addr=0x 00 00 a0 02-----------00 00 00 00 + a0 02 00 00
    }

    /* get length */
    for(i=0;i<dataBytes;i++)//2------------//按页进行写入，一页32字节
    {
        dataLength = dataLength << 8;//00 00
        dataLength += rxdata->data[2 + addrBytes + i];	// 0x00 01--- 表示后面数据有256个-------2+4+0---00 00 + 00 01
    }

    p = (uint8 *)&pageData[0];
    for(i=0;i<dataLength; i++)		//get buff length--2--1.01; 2.00
    {
        p[i] = rxdata->data[2 + addrBytes + dataBytes + i];	// 从0开始算起，指针指到从缓存数据区的第8个字节开始，曾军20220727
    }

    //下载代码->传输数据->0x36
    if((addr >= FL_PFLASH_PF_ADDR_Start) && (addr < FL_PFLASH_PF_ADDR_End ))//在PFLASH地址范围内
    {
		g_backToBeforeCodeS.bootloaderState = bootloaderToUpdate;//当前bootloader的状态是在升级

    	r = (uint8)Flash_writePFlash_port( addr, pageData, dataLength );//写入PFLASH--写入成功返回0
    }
    else
    {
    	r = 2;//err故障记录
    }

    /*fill tx buffer*/
    if(0 == r)   /*SUCCESS*/
    {
        CAN_CleanTxBuffer(txdata);
        txdata->dataLength = 2 + addrBytes + dataBytes;//2+4+0

        for(i=0; i<txdata->dataLength; i++)
        {
            txdata->data[i] = rxdata->data[i];//3d 24 a0 08 00 00
        }
        txdata->data[0] += 0x40;	//answer SID--3d + 40 = 7d

        /*transmit tx buffer*/
        CAN_SSN_TransmitPRC(txdata, 6);//7d 24 a0 08 00 00
    }
    else/*Err*/
    {
        /*fill tx buffer0 of NRC*/
        CAN_CleanTxBuffer(txdata);
        txdata->data[0] = 0x7f;
        for(uint8 i=1;i<(txdata->dataLength + 1);i++)
        {
        	txdata->data[i] = rxdata->data[i-1];
        }
        /*transmit tx buffer*/
        CAN_SSN_TransmitNRC(txdata,(rxdata->data[0] + 0X40),NRC_36_requestSequenceError);//发送故障码--0x7f 7d 24 a0 08 00 00
    }
}

/*
** ============================================================================
** @Function    ：诊断设备在线服务
** @Description ：客服端向某服务端请求来确认此服务是否在线，客户端有没有跟服务端连接上
** @Parameters  ：
** @Returns     ：none
** @Date        ：2024-07-17
** ============================================================================
*/
void CAN_SSN_ServiceID3e(diag_can_stt *rxdata,diag_can_stt *txdata)
{
	if(rxdata->dataLength >= 3)//获取的消息的长度不对
	{
		CAN_SSN_TransmitNRC(txdata,rxdata->data[0],NRC_3e_incorrectMessageLengthOrInvalidFormat);//发送故障码--0x 7f 3e 13
	}
	else//正响应回复
	{
		if(rxdata->data[1]==0x00)//零子功能(ISO14229强制规定)
		{
			//1.先给定肯定回应
			txdata->data[0] = 0x40 + rxdata->data[0];//0x7e--诊断设备在线响应SID
			txdata->data[1] = rxdata->data[1];//0x00--子功能=零子功能

			//2.在进行发送
			CAN_SSN_TransmitPRC(txdata, 2);//0x 7e 00/80 55 55 55 55
		}
		else if(rxdata->data[1]==0x80)
		{
			//不需要进行响应
		}
		else//发送的数据有误
		{
			CAN_SSN_TransmitNRC(txdata,rxdata->data[0],NRC_3e_subFunctionNotSupported);//发送故障码--0x 7f 3e 12
		}
	}
}

/*
** ============================================================================
** @Function    ：0x85命令是用来关闭ECU自己存储DTC的
** @Description ：设置禁止故障码
** @Parameters  ：
** @Returns     ：
** @Date        ：
** ============================================================================
*/
void CAN_SSN_ServiceID85(diag_can_stt *rxdata,diag_can_stt *txdata)
{
	if(rxdata->dataLength >= 3)//获取的消息的长度不对
	{
		CAN_SSN_TransmitNRC(txdata,rxdata->data[0],NRC_3e_incorrectMessageLengthOrInvalidFormat);//发送故障码--0x 7f 85 13
	}
	else//正响应回复
	{
		switch(rxdata->data[1])//设置禁止故障码
		{
			case 0x01://启用ECU存储DTC的功能
				//1.先给定肯定回应
				close_OR_open_DTCCode = 1;
				txdata->data[0] = 0x40 + rxdata->data[0];//0xC5--诊断设备在线响应SID
				txdata->data[1] = rxdata->data[1];//0x02--子功能=零子功能

//				txdata->data[3] = close_OR_open_DTCCode;
				CAN_SSN_TransmitPRC(txdata, 2);//0x 7e 00/80 ff 55 55 55
				break;

			case 0x02://关闭ECU存储DTC的功能
				//1.先给定肯定回应
				close_OR_open_DTCCode = 0;
				txdata->data[0] = 0x40 + rxdata->data[0];//0xC5--诊断设备在线响应SID
				txdata->data[1] = rxdata->data[1];//0x02--子功能=零子功能
//				txdata->data[2] = 0xFF;//关闭所有ECU的故障码存储功能,使用功能地址 FFh
//				txdata->data[3] = close_OR_open_DTCCode;
				CAN_SSN_TransmitPRC(txdata, 2);//0x 7e 00/80 ff 55 55 55
				break;

			default:
				CAN_SSN_TransmitNRC(txdata,rxdata->data[0],NRC_85_subFunctionNotSupported);//发送故障码--0x 7f 85 12
				break;
		}
	}
}

/*
** ============================================================================
** @Function    ：0x86命令是问答式，DTC产生以后会一直发送，需要再次采用0x86命令来关闭
** @Description ：设置禁止故障码
** @Parameters  ：
** @Returns     ：
** @Date        ：
** ============================================================================
*/
void CAN_SSN_ServiceID86(diag_can_stt *rxdata,diag_can_stt *txdata)
{
	if(rxdata->dataLength >= 3)//获取的消息的长度不对
	{
		CAN_SSN_TransmitNRC(txdata,rxdata->data[0],NRC_86_incorrectMessageLengthOrInvalidFormat);//发送故障码--0x 7f 86 13
	}
	else//正响应回复
	{
		//用于ECU的前期开发阶段--20240808--未进行编写
	}
}

/*
** ============================================================================
** @Function    ：
** @Description ：①预编程(开始->读软件版本信息0x22->切换到扩展模式0x10->禁止故障码设置0x85->禁止非刷新数据流0x28->刷新准备条件检查0x31)
** @Description ：②刷新(开始->切换到刷新模式0x10->安全验证0x27->应用存储区擦除0x31
** @Description ：->传输块的首地址，数据的数目0x34->传输数据0x36->本块传输完毕检查0x37->所有块传输完毕？
** @Description ：->所刷程序校验0x31(Y)
** @Description ：->->传输块的首地址，数据的数目0x34(N)---重新进行这一步开始
** @Description ：③重启控制器然后将诊断对话切换为正常模式
** @Parameters  ：
** @Returns     ：
** @Date        ：
** ============================================================================
*/
int CAN_SSN_ServiceWithId(void)
{
    int r = 0;

//    //1.先解锁
//    if(clockFlag==1)//解锁,可以执行后续的会话
//    {
        switch(gAppData.rx.data[0])
        {
            case UDS_Diagnostic_SessionControl:/*DiagnosticSessionControl */ // 诊断会话控制--进入BL--联机--编程写操作 0x10
                CAN_SSN_ServiceID10(&gAppData.rx, &gAppData.tx);
                break;

            case UDS_ECU_Reset_softWare:/*ECU_Reset_softWare */	// 软件复位，断电重启-240715 0x11
                CAN_SSN_ServiceID11(&gAppData.rx, &gAppData.tx);
                break;

            case UDS_Read_Data_ByIdentifier:					 //读版本信息 0x22
            	CAN_SSN_ServiceID22(&gAppData.rx, &gAppData.tx);
            	break;

            case UDS_Read_Memory_ByAddress:/*ReadMemoryByAddress */	// 通过地址读取内存 0x23
                CAN_SSN_ServiceID23(&gAppData.rx, &gAppData.tx);
                break;

            case UDS_SECURITY_ACCESS:/*SecurityAccess */ // 安全访问服务 0x27
                CAN_SSN_ServiceID27(&gAppData.rx, &gAppData.tx);
                break;

            case UDS_Communication_Control:/*CommunicationControl */ // 安全访问服务 0x28
                CAN_SSN_ServiceID28(&gAppData.rx, &gAppData.tx);
                break;

            case UDS_Routine_Control:/*RoutineControl  */	// 常规控制--进行Flash的擦除操作 0x31
//            	if(clockFlag==1)//解锁,可以执行后续的会话
            	CAN_SSN_ServiceID31(&gAppData.rx, &gAppData.tx);
                break;

            case UDS_Request_Download:/*WriteFlashByAddress */	//读取下载的首地址 0x34
                CAN_SSN_ServiceID34(&gAppData.rx, &gAppData.tx);
                break;

            case UDS_Write_Flash_ByAddress:/*WriteFlashByAddress */	// 通过地址写Flash 0x3d
                CAN_SSN_ServiceID3d(&gAppData.rx, &gAppData.tx);
                break;

            case UDS_Tester_Present:/*TesterPresent */	//诊断设备在线服务 0x3e
            	CAN_SSN_ServiceID3e(&gAppData.rx, &gAppData.tx);
                break;

            case UDS_Control_DTCSetting:/*TesterPresent */	//诊断设备在线服务 0x85
            	CAN_SSN_ServiceID85(&gAppData.rx, &gAppData.tx);
                break;

            default:
            	CAN_SSN_TransmitNRC(&gAppData.tx,gAppData.rx.data[0],NRC_SID_conditionsNotCorrect);//发送故障码--0x 7f xx 22
                break;
        }
//    }
//    else //未进行解锁，解锁
//    {
//		if(gAppData.rx.data[0] == UDS_SECURITY_ACCESS)/*SecurityAccess */			 // 安全访问服务
//		{
//			CAN_SSN_ServiceID27(&gAppData.rx, &gAppData.tx);
//		}
//		else//未解锁，发送的其他SID,即进入发送故障
//		{
//			CAN_SSN_TransmitNRC(&gAppData.tx,gAppData.rx.data[0],NRC_27_conditionsNotCorrect);//发送故障码--0x 7f xx 22
//		}
//    }
    return r;
}




