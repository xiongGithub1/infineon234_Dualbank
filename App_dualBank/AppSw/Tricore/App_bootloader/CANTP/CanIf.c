
/**********************************************************************************************************************
 * \file    CanIf.c
 * \brief
 * \version V1.0.0
 *********************************************************************************************************************/



/*********************************************************************************************************************/
/*-----------------------------------------------------Includes------------------------------------------------------*/
/*********************************************************************************************************************/
#include    "CanIf.h"
#include    "App_bootloader_cfg.h"
#include    "IfxMultican.h"
#include    "IfxMultican_Can.h"
#include    "Bsp.h"


/*********************************************************************************************************************/
/*-------------------------------------------------Global variables--------------------------------------------------*/
/*********************************************************************************************************************/
app_data_stt gAppData;



/*********************************************************************************************************************/
/*--------------------------------------------Private Variables/Constants--------------------------------------------*/
/*********************************************************************************************************************/

/*********************************************************************************************************************/
/*------------------------------------------------Function Prototypes------------------------------------------------*/
/*********************************************************************************************************************/
void CanIf_wait_ms(uint32 ms);


/*********************************************************************************************************************/
/*---------------------------------------------Function Implementations----------------------------------------------*/
/*********************************************************************************************************************/


/*
** ============================================================================
** @Function    ：
** @Description ：After receive mo , fill buff msg to canbuff
** 接收的消息，始终还是要遵循TC234接收数据的结构，因为主芯片已经决定了数据结构，曾军20210616
** @Parameters  ：
** @Returns     ：   0=ok
** @Date        ：
** ============================================================================
*/
uint8 CanIf_fillBuffer(IfxMultican_Message msg,  diag_can_stt *canbuff)
{
    uint8 i;
    uint8 receiveFrameState = 1;	// 接收帧状态，1：初始值，0：单帧或连续帧最后一帧，2：连续帧首帧或中间帐，4：流控帧
    uint16 len;
    un4byte_union temp32un;
    uint32 temp  = 0;
    uint8  rxdata[8];

    /* get id */
    temp32un.b16 = msg.id;		// 因为temp32un是联合体，因此里面的成员变量 b32和b8[4]共用一块内存，对b32赋值，相当于也同时给b8[4]也赋值了，曾军20210616
    canbuff->idStandardFrameh = temp32un.b8[1];
    canbuff->idStandardFramel = temp32un.b8[0];

    /* get the length */
//    Can_receivePdu.sduLength = msg.lengthCode;

    /* get data */			// 假设发送数据为 0x 11 22 33 44 55 66 77 88  那么msg.data[0]=0x 44 33 22 11,	msg.data[1]= 0x 88 77 66 55
    						// 实际发送的数据为：0x 11 08 3d 24 a0 02 00 00
    temp =  msg.data[0];	// 那么msg.data[0]=0x 24 3d 08 11,	msg.data[1]= 0x 00 00 02 a0
    for(i=0;i<4;i++)
    {
        rxdata[i] = 0xff & (temp >> (i*8));		// rxdata[3]=11 08 3d 24
    }
    temp =  msg.data[1];
    for(i=0;i<4;i++)
    {
        rxdata[4+i] = 0xff & (temp >> (i*8));	// rxdata[7]=11 08 3d 24 a0 02 00 00
    }

    /* get  information type */
    canbuff->protocolType = rxdata[0] >> 4;		// 第0字节取高4位，确定该数据帧的类型

    /*SF*/
    if(SINGLE_FRAME == canbuff->protocolType)			 // 单帧
    {
        canbuff->dataLength = 0x0f &  rxdata[0];
        for(i=0;  i<canbuff->dataLength;  i++)
        {
            canbuff->data[i] = rxdata[i+1];
            canbuff->dataCnt = i;
        }
        receiveFrameState = 0;
    }
    /*FF*/
    else if(FIRST_FRAME == canbuff->protocolType)		// 首帧
    {
        canbuff->dataLength = (uint16)( (0x0f & (uint16)rxdata[0] ) << 8) |  rxdata[1];		// 第0字节的低4位作为高字节+第1字节作为低字节，作为字节长度，曾军20220727
        for(i=0;i<6;i++)
        {
            canbuff->data[i] = rxdata[i+2];	// 首帧从第2字节开始记录成数据，并缓存	canbuff->data[6]=0x 3d 24 a0 02 00 00
        }
        canbuff->dataCnt  = 0x06;
        receiveFrameState = 2;
    }
    /*CF*/
    else if(CONSECUTIVE_FRAME == canbuff->protocolType)		// 连续帧
    {
        len = canbuff->dataLength - canbuff->dataCnt;

        if (len<=0x07)			// 字节长度小于7时，证明为该连续数据的最后一帧
        {
            for(i=0;i<len;i++)
            {
                canbuff->data[canbuff->dataCnt++] = rxdata[i+1];
            }
            receiveFrameState = 0;
        }
        else					// 字节长度大于7时，连续记录连续帧的数据
        {
            for(i=0;i<0x07;i++)
            {
                canbuff->data[canbuff->dataCnt++] = rxdata[i+1];
            }
            receiveFrameState = 2;
        }
    }

    /*FC*/
    else if(FLOW_CONTROL_CTS_FRAME == canbuff->protocolType)	// 流控帧
    {
    	receiveFrameState = 4;
    }

    return(receiveFrameState);
}

/*
** ============================================================================
** @Function    ：
** @Description ：
** @Parameters  ：
** @Returns     ：
** @Date        ：
** ============================================================================
*/
int  CanIf_TransmitBuffer(IfxMultican_Can_MsgObj *msgObj, diag_can_stt *canbuff)
{
    un4byte_union id;
    uint8	 i;
    int      r=1;
    volatile uint8  txdata[8];
    volatile uint32 dataLow  = 0;
    volatile uint32 dataHigh = 0;

    uint32 temp  = 0;
    IfxMultican_Message msg;

    /* get id */
    id.b8[1] = canbuff->idStandardFrameh;
    id.b8[0] = canbuff->idStandardFramel;

    if(0x07 >= canbuff->dataLength)//5
    {
        for(i=0; i<8; i++)
        {
            txdata[i] = 0x55;	// 先把要发送的所有数据0~7字节都用0x55填充
        }
        txdata[0] = (uint8)canbuff->dataLength; // 第0个字节为数据长度
        for(i=0; i<canbuff->dataLength; i++)	// 依次加入数据
        {
            txdata[i+1] = canbuff->data[i];//0x 50 04 f0 b2 b3
        }
        /* data to msg */
        dataLow = 0;
        dataHigh =0;
        for(i=0;i<4;i++)
        {
            temp = (uint32)txdata[i];
            dataLow |=  temp << (i * 8);
        }

        for(i=0;i<4;i++)
        {
            temp = (uint32)txdata[i+4];
            dataHigh |=  temp << (i * 8);
        }
        IfxMultican_Message_init(&msg, id.b16, dataLow, dataHigh, CAN_MO_DATA_Length);
        /* send msg to mo */
//      IfxMultican_Can_MsgObj_sendMessage(msgObj, &msg);
        while (IfxMultican_Can_MsgObj_sendMessage(msgObj, &msg) == IfxMultican_Status_notSentBusy)
        {}
    }
    else    /* ff + cf */
    {
        /* ff */
        txdata[0] = (FIRST_FRAME << 4) + (canbuff->dataLength >> 8);
        txdata[1] = (uint8)canbuff->dataLength;
        for(i=0; i<6; i++)
        {
            txdata[i+2] = canbuff->data[i];
        }

        /* data to msg  ,uint8 to uint32 */
        dataLow = 0;
        dataHigh =0;
        for(i=0;i<4;i++)
        {
            temp = (uint32)txdata[i];
            dataLow |=  temp << (i * 8);
        }
        for(i=0;i<4;i++)
        {
            temp = (uint32)txdata[i+4];
            dataHigh |=  temp << (i * 8);
        }
        IfxMultican_Message_init(&msg,
                id.b16,
                dataLow,
                dataHigh,
                CAN_MO_DATA_Length);
        /* send msg to mo */
        IfxMultican_Can_MsgObj_sendMessage(msgObj, &msg);

        CanIf_wait_ms(1);

        /* cf */
        canbuff->dataCnt = 6;
        canbuff->protocolFrameCnt = 0x21;
        for(i=0; i<8; i++)
        {
            txdata[i] = 0;
        }
        while(canbuff->dataCnt < canbuff->dataLength)
        {
            txdata[0] = canbuff->protocolFrameCnt ;

            for(i=0; i<7; i++)
            {
                txdata[i+1] = canbuff->data[canbuff->dataCnt];
                canbuff->dataCnt++;
            }

            /* data to msg  ,uint8 to uint32 */
            dataLow = 0;
            dataHigh =0;
            for(i=0;i<4;i++)
            {
                temp = (uint32)txdata[i];
                dataLow |=  temp << (i * 8);
            }
            for(i=0;i<4;i++)
            {
                temp = (uint32)txdata[i+4];
                dataHigh |=  temp << (i * 8);
            }
            IfxMultican_Message_init(&msg,
                    id.b16,
                    dataLow,
                    dataHigh,
                    CAN_MO_DATA_Length);

            /* cnt */
            if(canbuff->protocolFrameCnt >= 0x2f)
            {
                canbuff->protocolFrameCnt = 0x20;
            }
            else
            {
                canbuff->protocolFrameCnt++;
            }


            /* send msg to mo */
//            while (IfxMultican_Can_MsgObj_sendMessage(msgObj, &msg) != IfxMultican_Status_ok)
//            {}

            IfxMultican_Can_MsgObj_sendMessage(msgObj, &msg);
            CanIf_wait_ms(1);
        }
    }

    return r;
}

/*
** ============================================================================
** @Function    ：
** @Description ：
** @Parameters  ：
** @Returns     ：
** @Date        ：
** ============================================================================
*/
void CAN_CleanTxBuffer(diag_can_stt *canbuff)
{
    uint16 i;

    for(i=0; i<CAN_DATA_BUFF_Length; i++)
    {
        canbuff->data[i] = 0x55;
    }
}



/*
** ============================================================================
** @Function    ：
** @Description ：
** @Parameters  ：
** @Returns     ：
** @Date        ：
** ============================================================================
*/
void CanIf_wait_ms(uint32 ms)
{

    /* illd bsp */
    wait(TimeConst[TIMER_INDEX_1MS] * ms);

    return;
}

