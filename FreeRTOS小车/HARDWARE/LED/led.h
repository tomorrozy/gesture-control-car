#ifndef __LED_H
#define __LED_H

#include "sys.h"

/* ============================================================
 * 板载指示灯
 * 低电平点亮（输出高电平时熄灭）
 * ============================================================ */

#define LED0    PCout(13)   /* PC13 */
#define LED1    PAout(5)    /* PA5：与 nRF24L01 的 MOSI 复用，实际以无线模块为准 */

void LED_Init(void);

#endif
