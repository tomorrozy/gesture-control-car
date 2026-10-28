#include "sys.h"
#include "usart.h"

/* ============================================================
 * 串口1 驱动
 * ------------------------------------------------------------
 * 1. 把 printf 重定向到 USART1，方便调试时直接打印变量
 * 2. 提供中断接收，收到换行符视为一帧结束
 * ============================================================ */

/* ---------------- printf 重定向 ---------------- */
/* 不依赖 MicroLIB：告诉链接器不要链接半主机（semihosting）相关代码，
 * 不然在没接调试器时程序会卡死在 BKPT 指令上 */
#pragma import(__use_no_semihosting)

/* 半主机模式需要这两个符号，给个空实现即可 */
struct __FILE
{
    int handle;
};

FILE __stdout;

void _sys_exit(int x)
{
    x = x;
}

/* printf 最终会逐字符调用 fputc */
int fputc(int ch, FILE *f)
{
    while ((USART1->SR & 0x40) == 0)    /* 0x40 = TXE，等待发送寄存器空 */
    {
        /* 空转等待 */
    }
    USART1->DR = (u8)ch;
    return ch;
}

/* ---------------- 初始化与中断 ---------------- */

#if EN_USART1_RX

u8  USART_RX_BUF[USART_REC_LEN];
u16 USART_RX_STA = 0;

void uart_init(u32 bound)
{
    GPIO_InitTypeDef  GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef  NVIC_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1 | RCC_APB2Periph_GPIOA, ENABLE);

    /* PA9 = USART1_TX，复用推挽输出 */
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    /* PA10 = USART1_RX，浮空输入 */
    GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    /* 接收中断：抢占优先级 3，子优先级 3
     * 注意：FreeRTOS 下这个数值要满足
     * configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY 的限制 */
    NVIC_InitStructure.NVIC_IRQChannel                   = USART1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority        = 3;
    NVIC_InitStructure.NVIC_IRQChannelCmd                = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    USART_InitStructure.USART_BaudRate            = bound;
    USART_InitStructure.USART_WordLength          = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits            = USART_StopBits_1;
    USART_InitStructure.USART_Parity              = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode                = USART_Mode_Rx | USART_Mode_Tx;

    USART_Init(USART1, &USART_InitStructure);
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
    USART_Cmd(USART1, ENABLE);
}

/* 串口1 中断：按行缓存，收到 '\n' 认为一帧结束
 * USART_RX_STA 的 bit15 置 1 表示有新数据，读取方处理完需清零 */
void USART1_IRQHandler(void)
{
    u8 ch;

    if (USART_GetITStatus(USART1, USART_IT_RXNE) == RESET)
    {
        return;
    }

    ch = (u8)USART_ReceiveData(USART1);

    if (ch == '\n')
    {
        USART_RX_BUF[USART_RX_STA & 0x7FFF] = '\0';
        USART_RX_STA |= 0x8000;             /* 标记收完一帧 */
    }
    else if ((USART_RX_STA & 0x8000) == 0)  /* 上一帧还没被取走就先丢弃新数据 */
    {
        if ((USART_RX_STA & 0x7FFF) < (USART_REC_LEN - 1))
        {
            USART_RX_BUF[USART_RX_STA & 0x7FFF] = ch;
            USART_RX_STA++;
        }
        else
        {
            USART_RX_STA = 0;               /* 溢出，丢掉重来 */
        }
    }
}

#else   /* 不使能接收：只保留发送能力 */

void uart_init(u32 bound)
{
    GPIO_InitTypeDef  GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1 | RCC_APB2Periph_GPIOA, ENABLE);

    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    USART_InitStructure.USART_BaudRate            = bound;
    USART_InitStructure.USART_WordLength          = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits            = USART_StopBits_1;
    USART_InitStructure.USART_Parity              = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode                = USART_Mode_Tx;

    USART_Init(USART1, &USART_InitStructure);
    USART_Cmd(USART1, ENABLE);
}

#endif
