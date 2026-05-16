/**********************************************************************************************************************
 * \file    tool_class.c
 * \brief
 * \version V1.0.0
 * \date    2022��2��28��
 * \author  Administrator
 *********************************************************************************************************************/
#include "tool_class.h"
#include "Flash.h"





void tl_memset(void *s, int c, int count)
{
	char *xs = (char *)s;//�� void * ���͵�ָ�� s ת��Ϊ char * ���͵�ָ�� xs��
						 //������Ϊ void * ��ͨ��ָ�����ͣ�����ֱ�ӽ���ָ���������㣨�� ++������ char * ���ֽ�ָ�룬�������ֽڲ����ڴ档
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
		// ����������ִ�и���
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
		// ����Ϊ��
		//pObject = ((void *) 0);
		return 0;
	}
	tl_memcpy(pObject,&(pdq->msg[pdq->front]),sizeof(QueueMsgObject));
	pdq->front = (pdq->front + 1) % QUEUE_MAXSIZE;
	return 1;
}
// ��EEPROM�ж�ȡ����,�ɹ����ض���������,ʧ�ܷ���0
uint8 tl_read_from_eeprom(uint32 addr,uint8 *pDstData,uint8 count)
{
	// TODO
	return count;
}
// ��FLASH�ж�ȡ����,�ɹ����ض���������,ʧ�ܷ���0
uint8 tl_read_from_flash(uint32 addr,uint8 *pDstData,uint8 count)
{
	// TODO
	return count;
}
// ��EEPROM��д������,�ɹ�����д����ֽ���,ʧ�ܷ���0
uint8 tl_write_to_eeprom(uint32 addr,uint8 *pData,uint8 count)
{
	// TODO
	return count;
}
/* Write data to DFlash.
 * Returns number of bytes written on success, 0 on failure.
 * Data length is rounded up to nearest DFLASH_PAGE_LENGTH (8 bytes).
 * Caller must ensure the target DFlash sector is erased beforehand.
 */
uint8 tl_write_to_flash(uint32 addr, uint8 *pData, uint8 count)
{
	uint32 alignedBuf[64]; /* 256 bytes / 4 = 64 words */
	uint8 i;
	uint32 writeLen;

	if (count == 0)
	{
		return 0;
	}

	/* Clear aligned buffer */
	for (i = 0; i < 64; i++)
	{
		alignedBuf[i] = 0u;
	}

	/* Copy data to aligned buffer */
	for (i = 0; i < count; i++)
	{
		((uint8*)alignedBuf)[i] = pData[i];
	}

	/* Round up to nearest 8-byte page for DFlash */
	writeLen = ((count + DFLASH_PAGE_LENGTH - 1u) / DFLASH_PAGE_LENGTH) * DFLASH_PAGE_LENGTH;

	Flash_writeDFlash_port(addr, alignedBuf, writeLen);

	return count;
}
