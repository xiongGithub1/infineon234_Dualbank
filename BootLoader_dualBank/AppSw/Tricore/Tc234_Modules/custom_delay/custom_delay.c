/*
 * custom_delay.c
 *
 *  Created on: 2018Äê9ÔÂ18ÈÕ
 *      Author: Zhang Yunhao
 *
 *
 *
 */

#include "custom_delay.h"


void delay_init(void)
{
	initTime();
}

void delay_us(uint16 nus)
{
	Ifx_TickTime timeout = TimeConst_1us * nus;

	waitTime(timeout);
}

void delay_ms(uint16 nms)
{
	Ifx_TickTime timeout = TimeConst_1ms * nms;

	waitTime(timeout);
}





