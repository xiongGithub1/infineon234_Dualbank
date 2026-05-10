/*
 * custom_delay.h
 *
 *  Created on: 2018年9月18日
 *      Author: Zhang Yunhao
 *
 *  说明：
 *   delay方法。 供程序中延时使用
 *
 *   基于芯片内部stm模块。通过上电即运行的64位定时器实现定时
 *   依赖于英飞凌官方iLLD中的 Bsp.h Bsp.c文件。
 *   使用前必须先运行 void initTime(void) 函数以初始化时间常数
 */

#ifndef DELAY_CUSTOM_DELAY_H_
#define DELAY_CUSTOM_DELAY_H_

#include "SysSe/Bsp/Bsp.h"

extern void delay_init(void);

extern void delay_us(uint16 nus);

extern void delay_ms(uint16 nms);





#endif /* DELAY_CUSTOM_DELAY_H_ */
