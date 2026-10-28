#ifndef __USART_H
#define __USART_H

#include "stdio.h"
#include "sys.h"

/* ============================================================
 * 串口1（USART1）：PA9=TX，PA10=RX，默认 115200 8N1
 * 主要用途：调试信息输出（printf 已重定向到串口1）
 * ============================================================ */

#define USART_REC_LEN       200     /* 接收缓冲长度 */
#define EN_USART1_RX        1       /* 1=使能接收中断，0=只发送 */

extern u8  USART_RX_BUF[USART_REC_LEN];
extern u16 USART_RX_STA;            /* bit15 收完一帧标志；bit14~0 已收到的字节数 */

void uart_init(u32 bound);          /* 初始化串口1，bound 为波特率 */

#endif
