/**********************************************************************************************************************
 * \file    tool_class.c
 * \brief
 * \version V1.0.0
 * \date    2022年2月28日
 * \author  Administrator
 *********************************************************************************************************************/
#include "tool_class.h"






void tl_memset(void *s, int c, int count)
{
	char *xs = (char *)s;//将 void * 类型的指针 s 转换为 char * 类型的指针 xs。
						 //这是因为 void * 是通用指针类型，不能直接进行指针算术运算（如 ++），而 char * 是字节指针，可以逐字节操作内存。
	while (count--)
		*xs++ = (char)c;
}

void tl_memcpy(void *dst, const void *src, int count)
{
	char *tmp = (char *)dst, *s = (char *)src;
	int len;

	if (tmp <= s || tmp > (s + count))
	{
		while (count--)
			*tmp ++ = *s ++;
	}
	else
	{
		for (len = count; len > 0; len --)
			tmp[len - 1] = s[len - 1];
	}
}

void tl_queue_init(DataQueue *pdq)
{
	pdq->rear = 0;
	pdq->front = 0;
	//tl_memset(pdq->msg,0,sizeof(pdq->msg));
}


void tl_queue_add_item(DataQueue *pdq,QueueMsgObject *pObject)
{
	if(((pdq->rear + 1) % QUEUE_MAXSIZE) == pdq->front)
	{
		// 队列已满，执行覆盖
		//tl_queue_init();
		tl_queue_init(pdq);
	}
	tl_memcpy(&(pdq->msg[pdq->rear]),pObject,sizeof(QueueMsgObject));
	pdq->rear = (pdq->rear + 1) % QUEUE_MAXSIZE;
}

uint8 tl_queue_take_item(DataQueue *pdq,QueueMsgObject *pObject)
{
	if(pdq->rear == pdq->front)
	{
		// 队列为空
		//pObject = ((void *) 0);
		return 0;
	}
	tl_memcpy(pObject,&(pdq->msg[pdq->front]),sizeof(QueueMsgObject));
	pdq->front = (pdq->front + 1) % QUEUE_MAXSIZE;
	return 1;
}
// 从EEPROM中读取数据,成功返回读到的数量,失败返回0
uint8 tl_read_from_eeprom(uint32 addr,uint8 *pDstData,uint8 count)
{
	// TODO
	return count;
}
// 从FLASH中读取数据,成功返回读到的数量,失败返回0
uint8 tl_read_from_flash(uint32 addr,uint8 *pDstData,uint8 count)
{
	// TODO
	return count;
}
// 向EEPROM中写入数据,成功返回写入的字节数,失败返回0
uint8 tl_write_to_eeprom(uint32 addr,uint8 *pData,uint8 count)
{
	// TODO
	return count;
}
// 向FLASH中写入数据,成功返回写入的字节数,失败返回0
uint8 tl_write_to_flash(uint32 addr,uint8 *pData,uint8 count)
{
	// TODO
	return count;
}
