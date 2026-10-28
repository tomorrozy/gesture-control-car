#ifndef __DELAY_H
#define __DELAY_H

#include "sys.h"

/* 延时模块，基于 SysTick 实现，不占用额外定时器。
 * 打开 SYSTEM_SUPPORT_OS 时：
 *   - SysTick 中断交给 FreeRTOS 做任务节拍
 *   - delay_ms 在调度器启动后会自动转为 vTaskDelay，不阻塞低优先级任务
 */
void delay_init(void);      /* 初始化 SysTick，必须在其它延时函数之前调用 */
void delay_us(u32 nus);     /* 微秒级延时，忙等 */
void delay_ms(u32 nms);     /* 毫秒级延时，RTOS 下可能触发任务切换 */
void delay_xms(u32 nms);    /* 毫秒级延时，强制忙等，不触发任务切换 */

#endif
