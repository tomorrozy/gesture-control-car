#include "led.h"

/* ============================================================
 * 板载 LED 初始化
 * 低电平点亮，所以初始化后先输出高电平（熄灭状态）
 * ============================================================ */
void LED_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOA, ENABLE);

    /* PC13 - 板载 LED0 */
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_13;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOC, &GPIO_InitStructure);
    GPIO_SetBits(GPIOC, GPIO_Pin_13);

    /* PA5 - LED1（注意：该脚与 nRF24L01 的 MOSI 共用，
     * main() 里 NRF24L01_Init() 在其后执行并重新配置为 SPI 输出，
     * 所以实际使用时 LED1 不会正常工作，保留仅为兼容原有接线） */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    GPIO_SetBits(GPIOA, GPIO_Pin_5);
}
