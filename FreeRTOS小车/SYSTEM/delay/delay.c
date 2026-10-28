#include "delay.h"
#include "sys.h"

#if SYSTEM_SUPPORT_OS
#include "FreeRTOS.h"
#include "task.h"
#endif

/* ============================================================
 * 基于 SysTick 的延时实现
 * ------------------------------------------------------------
 * SysTick 配成自由递减的 24 位计数器，每计数一次为 1 个 HCLK 周期。
 * 靠轮询 VAL 寄存器计算掉了多少个计数来实现精确忙等，比空循环可靠
 * （不会被编译器优化掉，也不受主频变化影响）。
 *
 * RTOS 模式下 SysTick 还兼职做任务节拍，所以中断里必须调用
 * xPortSysTickHandler()，否则调度器完全不会动。
 * ============================================================ */

static u32 s_cycles_per_us = 0;     /* 每微秒的 SysTick 计数个数 */
static u32 s_ms_per_tick   = 0;     /* 一个 RTOS 节拍等于多少毫秒 */

extern void xPortSysTickHandler(void);

/* SysTick 中断服务函数：调度器启动后交给 FreeRTOS 处理 */
void SysTick_Handler(void)
{
#if SYSTEM_SUPPORT_OS
    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)
    {
        xPortSysTickHandler();
    }
#endif
}

/* 初始化 SysTick 作为延时基准
 * 注意：必须在 FreeRTOS 启动之前调用，且在其它延时函数之前调用 */
void delay_init(void)
{
    u32 reload;

    /* SysTick 时钟源选 HCLK（不是 HCLK/8），与 FreeRTOS 的移植层约定一致 */
    SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK);

    s_cycles_per_us = SystemCoreClock / 1000000UL;

#if SYSTEM_SUPPORT_OS
    /* 中断周期 = 1 / configTICK_RATE_HZ 秒，用满 24 位寄存器 */
    reload        = SystemCoreClock / configTICK_RATE_HZ;
    s_ms_per_tick = 1000UL / configTICK_RATE_HZ;
#else
    reload        = SystemCoreClock / 1000UL;   /* 裸机：固定 1 ms 中断 */
    s_ms_per_tick = 1;
#endif

    SysTick->LOAD = reload - 1;
    SysTick->VAL  = 0;
    SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk;   /* 允许 SysTick 中断 */
    SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;    /* 启动计数器 */
}

/* 微秒级忙等延时
 * nus 的有效范围受 24 位计数器限制，单次不要超过几百毫秒 */
void delay_us(u32 nus)
{
    u32 target = nus * s_cycles_per_us;   /* 需要等待的计数个数 */
    u32 reload = SysTick->LOAD + 1;       /* 计数器溢出周期 */
    u32 last   = SysTick->VAL;            /* 进入时的计数值 */
    u32 acc    = 0;                       /* 已累计的计数 */

    while (acc < target)
    {
        u32 now = SysTick->VAL;

        if (now != last)
        {
            /* VAL 是递减计数器：没跨过 0 就直接相减，跨过了要补上整个周期 */
            acc += (now < last) ? (last - now) : (reload - now + last);
            last = now;
        }
    }
}

/* 毫秒级延时
 * 调度器已启动时优先用 vTaskDelay，让出 CPU 给其它任务 */
void delay_ms(u32 nms)
{
#if SYSTEM_SUPPORT_OS
    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)
    {
        if (nms >= s_ms_per_tick)
        {
            vTaskDelay(nms / s_ms_per_tick);
        }
        nms %= s_ms_per_tick;   /* 不足一个节拍的部分用忙等补齐 */
    }
#endif

    delay_us(nms * 1000UL);
}

/* 毫秒级延时，强制忙等
 * 用于不能触发任务切换的场合，比如中断上下文或临界区 */
void delay_xms(u32 nms)
{
    u32 i;

    for (i = 0; i < nms; i++)
    {
        delay_us(1000UL);
    }
}
