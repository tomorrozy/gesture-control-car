#include "sys.h"

/* ============================================================
 * Cortex-M3 内核指令封装
 * ------------------------------------------------------------
 * ARMCC 不允许在普通 C 函数里内联 THUMB 汇编，所以这里用
 * __ASM volatile 内联汇编和 __asm 嵌入式汇编两种方式实现。
 * ============================================================ */

/* 进入睡眠模式，任何中断都能唤醒 */
void WFI_SET(void)
{
    __ASM volatile("wfi");
}

/* 关闭全局中断（PRIMASK = 1），用于临界区保护 */
void INTX_DISABLE(void)
{
    __ASM volatile("cpsid i");
}

/* 打开全局中断（PRIMASK = 0） */
void INTX_ENABLE(void)
{
    __ASM volatile("cpsie i");
}

/* 设置主栈指针 MSP，调用时第一个参数由 r0 传入 */
__asm void MSR_MSP(u32 addr)
{
    MSR MSP, r0
    BX  lr
}
