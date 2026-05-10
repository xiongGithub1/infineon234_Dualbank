/**********************************************************************************************************************
 * \file    tool_class.h
 * \brief
 * \version V1.0.0
 * \date    2022年2月28日
 * \author  Administrator
 *********************************************************************************************************************/
#ifndef TOOLS_TOOL_CLASS_H_
#define TOOLS_TOOL_CLASS_H_

#include "Platform_Types.h"

#define QUEUE_MAXSIZE	4

typedef struct
{
	uint16 id;
	uint8 data[8];
}QueueMsgObject;

typedef struct
{
	int front;//队首指针
	int rear;//队尾指针
	//int qlen;// 队列元素长度
	QueueMsgObject msg[QUEUE_MAXSIZE];
}DataQueue;


void tl_memset(void *s, int c, int count);
void tl_memcpy(void *dst, const void *src, int count);
void tl_queue_init(DataQueue *pdq);
void tl_queue_add_item(DataQueue *pdq,QueueMsgObject *pObject);// 入队
uint8 tl_queue_take_item(DataQueue *pdq,QueueMsgObject *pObject);// 出队

uint8 tl_read_from_eeprom(uint32 addr,uint8 *pDstData,uint8 count);
uint8 tl_write_to_eeprom(uint32 addr,uint8 *pData,uint8 count);

uint8 tl_read_from_flash(uint32 addr,uint8 *pDstData,uint8 count);
uint8 tl_write_to_flash(uint32 addr,uint8 *pData,uint8 count);

#endif /* TOOLS_TOOL_CLASS_H_ */
