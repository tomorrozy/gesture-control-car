#ifndef __SYS_H
#define __SYS_H

#include "stm32f10x.h"

/* 系统是否运行 RTOS：1=支持（delay 会调用 vTaskDelay），0=裸机 */
#define SYSTEM_SUPPORT_OS           1


/* ============================================================
 * GPIO 位带（bit-band）操作
 * ------------------------------------------------------------
 * Cortex-M3 的位带机制：外设区 0x40000000 起的每一个位，在别名区
 * 0x42000000 起都有一个 32 位字与之对应。写入这个字就等于直接改
 * 对应引脚的电平，不需要"读整个 ODR -> 改位 -> 写回"。
 * 好处是单条指令完成，不会被中断打断导致读改写竞态。
 *
 * 地址换算：
 *   alias = 0x42000000 + (addr - 0x40000000) * 32 + bit * 4
 * ============================================================ */
#define PERIPH_BITBAND(addr, bit)                                   \
    ((volatile uint32_t *)(0x42000000UL                               \
        + (((uint32_t)(addr) - 0x40000000UL) << 5)                    \
        + ((uint32_t)(bit) << 2)))

/* 各 GPIO 端口的 ODR（输出）与 IDR（输入）寄存器地址
 * ODR 在 GPIO 结构体中的偏移是 0x0C，IDR 是 0x08 */
#define GPIOA_ODR_ADDR      (GPIOA_BASE + 0x0C)
#define GPIOB_ODR_ADDR      (GPIOB_BASE + 0x0C)
#define GPIOC_ODR_ADDR      (GPIOC_BASE + 0x0C)
#define GPIOD_ODR_ADDR      (GPIOD_BASE + 0x0C)
#define GPIOE_ODR_ADDR      (GPIOE_BASE + 0x0C)
#define GPIOF_ODR_ADDR      (GPIOF_BASE + 0x0C)
#define GPIOG_ODR_ADDR      (GPIOG_BASE + 0x0C)

#define GPIOA_IDR_ADDR      (GPIOA_BASE + 0x08)
#define GPIOB_IDR_ADDR      (GPIOB_BASE + 0x08)
#define GPIOC_IDR_ADDR      (GPIOC_BASE + 0x08)
#define GPIOD_IDR_ADDR      (GPIOD_BASE + 0x08)
#define GPIOE_IDR_ADDR      (GPIOE_BASE + 0x08)
#define GPIOF_IDR_ADDR      (GPIOF_BASE + 0x08)
#define GPIOG_IDR_ADDR      (GPIOG_BASE + 0x08)

/* 用法：PAout(1) = 1; 令 PA1 输出高电平；读 PBin(0) 得到 PB0 的电平
 * 注意 n 必须小于 16 */
#define PAout(n)    (*PERIPH_BITBAND(GPIOA_ODR_ADDR, (n)))
#define PAin(n)     (*PERIPH_BITBAND(GPIOA_IDR_ADDR, (n)))

#define PBout(n)    (*PERIPH_BITBAND(GPIOB_ODR_ADDR, (n)))
#define PBin(n)     (*PERIPH_BITBAND(GPIOB_IDR_ADDR, (n)))

#define PCout(n)    (*PERIPH_BITBAND(GPIOC_ODR_ADDR, (n)))
#define PCin(n)     (*PERIPH_BITBAND(GPIOC_IDR_ADDR, (n)))

#define PDout(n)    (*PERIPH_BITBAND(GPIOD_ODR_ADDR, (n)))
#define PDin(n)     (*PERIPH_BITBAND(GPIOD_IDR_ADDR, (n)))

#define PEout(n)    (*PERIPH_BITBAND(GPIOE_ODR_ADDR, (n)))
#define PEin(n)     (*PERIPH_BITBAND(GPIOE_IDR_ADDR, (n)))

#define PFout(n)    (*PERIPH_BITBAND(GPIOF_ODR_ADDR, (n)))
#define PFin(n)     (*PERIPH_BITBAND(GPIOF_IDR_ADDR, (n)))

#define PGout(n)    (*PERIPH_BITBAND(GPIOG_ODR_ADDR, (n)))
#define PGin(n)     (*PERIPH_BITBAND(GPIOG_IDR_ADDR, (n)))


/* 底层内核指令封装，调试/低功耗场景备用，当前工程未调用 */
void WFI_SET(void);         /* 进入睡眠，等待中断唤醒 */
void INTX_DISABLE(void);    /* 关全局中断 */
void INTX_ENABLE(void);     /* 开全局中断 */
void MSR_MSP(u32 addr);     /* 设置主栈指针 */

#endif
