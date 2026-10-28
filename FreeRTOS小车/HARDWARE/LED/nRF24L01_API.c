#include "nRF24L01_API.h"
/* 引脚定义 - 按照原理图修改，其余代码不变 */
#define NRF_CSN   PAout(2)
#define NRF_CE    PAout(3)
#define NRF_SCK   PAout(4)
#define NRF_MOSI  PAout(5)
#define NRF_MISO  PBin(0)
#define NRF_IRQ   PBin(1)

/* 常量定义 */
#define TX_ADR_WIDTH    5
#define RX_ADR_WIDTH    5
#define TX_PLOAD_WIDTH  32
#define RX_PLOAD_WIDTH  32
/* ============= 地址定义 ============= */
// 发送器1地址（接收器用通道0接收）
const uchar TX_ADDR1[TX_ADR_WIDTH] = {0x11, 0x11, 0x11, 0x11, 0x11};
// 发送器2地址（接收器用通道1接收）
const uchar TX_ADDR2[TX_ADR_WIDTH] = {0x22, 0x22, 0x22, 0x22, 0x22};
// 接收器地址（用于ACK应答）
const uchar RX_ADDR[RX_ADR_WIDTH]   = {0xAA, 0xAA, 0xAA, 0xAA, 0xAA};
/* 接收数据缓冲区 */
uchar RevTempDate[32];
/* ============= 延时函数 ============= */
void DelayUs(uchar num)
{
    uchar i, j;
    for(i = 0; i < num; i++)
        for(j = 0; j < 12; j++);  // 72MHz主频时约1us
}
void DelayMs(uint ms)
{
    uint i, j;
    for(i = 0; i < ms; i++)
        for(j = 0; j < 1200; j++);  // 72MHz主频时约1ms
}
/* ============= SPI读写函数 ============= */
uchar SPI_RW(uchar byte)
{
    uchar bit_ctr;
    uchar receive = 0;
    
    for(bit_ctr = 0; bit_ctr < 8; bit_ctr++)
    {
        // 发送一位数据
        if(byte & 0x80)
            NRF_MOSI = 1;
        else
            NRF_MOSI = 0;
        
        byte <<= 1;
        
        // 时钟上升沿
        NRF_SCK = 1;
        
        // 读取一位数据
        receive <<= 1;
        if(NRF_MISO)
            receive |= 0x01;
        
        // 时钟下降沿
        NRF_SCK = 0;
    }
    
    return receive;
}
/* ============= 寄存器操作函数 ============= */
uchar NRF24L01_Write_Reg(uchar reg, uchar value)
{
    uchar status;
    
    NRF_CSN = 0;
    status = SPI_RW(reg);
    SPI_RW(value);
    NRF_CSN = 1;
    
    return status;
}
uchar NRF24L01_Read_Reg(uchar reg)
{
    uchar value;
    
    NRF_CSN = 0;
    SPI_RW(reg);
    value = SPI_RW(NOP);
    NRF_CSN = 1;
    
    return value;
}
uchar NRF24L01_Read_Buf(uchar reg, uchar *pBuf, uchar len)
{
    uchar status, i;
    
    NRF_CSN = 0;
    status = SPI_RW(reg);
    for(i = 0; i < len; i++)
        pBuf[i] = SPI_RW(NOP);
    NRF_CSN = 1;
    
    return status;
}
uchar NRF24L01_Write_Buf(uchar reg, uchar *pBuf, uchar len)
{
    uchar status, i;
    
    NRF_CSN = 0;
    status = SPI_RW(reg);
    for(i = 0; i < len; i++)
        SPI_RW(pBuf[i]);
    NRF_CSN = 1;
    
    return status;
}
/* ============= 检测模块 ============= */
uchar NRF24L01_Check(void)
{
    uchar check_buf[5] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE};
    uchar read_buf[5] = {0};
    uchar i;
    
    NRF_CE = 0;
    NRF_CSN = 1;
    DelayMs(10);
    
    // 写入测试数据
    NRF24L01_Write_Buf(nRF_WRITE_REG + TX_ADDR, check_buf, 5);
    DelayMs(10);
    
    // 读出测试数据
    NRF24L01_Read_Buf(nRF_READ_REG + TX_ADDR, read_buf, 5);
    
    // 比较
    for(i = 0; i < 5; i++)
    {
        if(read_buf[i] != check_buf[i])
            return 1;  // 模块不存在
    }
    
    return 0;  // 模块存在
}
/* ============= GPIO初始化【重点修改这里】 ============= */
void NRF24L01_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    
    // 使能GPIOA、GPIOB时钟！！PB0 PB1用到，必须开GPIOB时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB, ENABLE);
    
    // PA2 → CSN 推挽输出
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    NRF_CSN = 1;
    
    // PA3 → CE 推挽输出
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    NRF_CE = 0;
    
    // PA4 → SCK 推挽输出
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    NRF_SCK = 0;
    
    // PA5 → MOSI 推挽输出
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    NRF_MOSI = 0;
    
    // PB0 → MISO 上拉输入
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    
    // PB1 → IRQ 上拉输入
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    
    DelayMs(100);
}
/* ============= 发送器模式初始化 ============= */
void NRF24L01_TX_Mode_Init(uchar *tx_addr)
{
    NRF_CE = 0;
    
    // 设置发送地址（自己的地址）
    NRF24L01_Write_Buf(nRF_WRITE_REG + TX_ADDR, tx_addr, TX_ADR_WIDTH);
    
    // 设置接收地址（用于接收ACK，必须和接收器的地址一致）
    NRF24L01_Write_Buf(nRF_WRITE_REG + RX_ADDR_P0, (uchar*)RX_ADDR, RX_ADR_WIDTH);
    
    // 设置有效数据宽度
    NRF24L01_Write_Reg(nRF_WRITE_REG + RX_PW_P0, RX_PLOAD_WIDTH);
    
    // 使能自动应答（通道0）
    NRF24L01_Write_Reg(nRF_WRITE_REG + EN_AA, 0x01);
    
    // 使能接收地址（通道0）
    NRF24L01_Write_Reg(nRF_WRITE_REG + EN_RXADDR, 0x01);
    
    // 设置自动重发（间隔500us，重发10次）
    NRF24L01_Write_Reg(nRF_WRITE_REG + SETUP_RETR, 0x1A);
    
    // 设置RF通道（2400MHz + 40MHz = 2440MHz）
    NRF24L01_Write_Reg(nRF_WRITE_REG + RF_CH, 40);
    
    // 设置RF参数（2Mbps，0dBm增益）
    NRF24L01_Write_Reg(nRF_WRITE_REG + RF_SETUP, 0x0F);
    
    // 配置寄存器：上电，CRC使能，16位CRC，发送模式
    NRF24L01_Write_Reg(nRF_WRITE_REG + CONFIG, 0x0E);
    
    // 清空TX FIFO
    NRF24L01_Write_Reg(FLUSH_TX, 0xFF);
    
    NRF_CE = 1;
    DelayUs(130);
}
/* ============= 接收器多通道模式初始化 ============= */
void NRF24L01_RX_MultiChannel_Init(void)
{
    NRF_CE = 0;
    
    // 通道0 - 接收发送器1的数据（使用TX_ADDR1地址）
    NRF24L01_Write_Buf(nRF_WRITE_REG + RX_ADDR_P0, (uchar*)TX_ADDR1, RX_ADR_WIDTH);
    NRF24L01_Write_Reg(nRF_WRITE_REG + RX_PW_P0, RX_PLOAD_WIDTH);
    
    // 通道1 - 接收发送器2的数据（使用TX_ADDR2地址）
    NRF24L01_Write_Buf(nRF_WRITE_REG + RX_ADDR_P1, (uchar*)TX_ADDR2, RX_ADR_WIDTH);
    NRF24L01_Write_Reg(nRF_WRITE_REG + RX_PW_P1, RX_PLOAD_WIDTH);
    
    // 使能自动应答（通道0和1）
    NRF24L01_Write_Reg(nRF_WRITE_REG + EN_AA, 0x03);  // 0b00000011
    
    // 使能接收地址（通道0和1）
    NRF24L01_Write_Reg(nRF_WRITE_REG + EN_RXADDR, 0x03);  // 0b00000011
    
    // 设置自动重发（间隔500us，重发10次）
    NRF24L01_Write_Reg(nRF_WRITE_REG + SETUP_RETR, 0x1A);
    
    // 设置RF通道（必须和发送器一致）
    NRF24L01_Write_Reg(nRF_WRITE_REG + RF_CH, 40);
    
    // 设置RF参数（2Mbps，0dBm增益）
    NRF24L01_Write_Reg(nRF_WRITE_REG + RF_SETUP, 0x0F);
    
    // 配置寄存器：上电，CRC使能，16位CRC，接收模式
    NRF24L01_Write_Reg(nRF_WRITE_REG + CONFIG, 0x0F);
    
    // 清空RX FIFO
    NRF24L01_Write_Reg(FLUSH_RX, 0xFF);
    
    NRF_CE = 1;  // 启动接收
    DelayUs(130);
}
/* ============= 从状态寄存器获取通道号 ============= */
uchar NRF24L01_GetChannelFromStatus(uchar status)
{
    return (status & RX_P_NO_MASK) >> 1;  // 返回通道号(0-5)
}
/* ============= 多通道接收数据包 ============= */
uchar NRF24L01_RxPacket_Multi(uchar *rxbuf, uchar *channel)
{
    uchar state;
    
    // 读取状态寄存器
    state = NRF24L01_Read_Reg(STATUS);
   
    // 检查是否有数据接收
    if(state & RX_OK)
    {
        // 获取通道号
        *channel = (state & RX_P_NO_MASK) >> 1;
        
        
        NRF24L01_Read_Buf(RD_RX_PLOAD, rxbuf, RX_PLOAD_WIDTH);
        return 0;  // 接收成功
    }
    
    return 1;  // 没有数据
}
/* ============= 发送数据包 ============= */
uchar NRF24L01_TxPacket(uchar *txbuf)
{
    uchar state;
    uint timeout = 0;
    
    NRF_CE = 0;
    
    // 写入数据到TX FIFO
    NRF24L01_Write_Buf(WR_TX_PLOAD, txbuf, TX_PLOAD_WIDTH);
    
    NRF_CE = 1;
    DelayUs(10);
    
    // 等待发送完成或超时
    timeout = 0;
    while(NRF_IRQ == 1)
    {
        timeout++;
        if(timeout > 50000)
            break;
    }
    
    // 读取状态
    state = NRF24L01_Read_Reg(STATUS);
    NRF24L01_Write_Reg(nRF_WRITE_REG + STATUS, state);
    
    if(state & MAX_TX)
    {
        NRF24L01_Write_Reg(FLUSH_TX, 0xFF);
        return 0x10;  // 发送失败
    }
    
    if(state & TX_OK)
    {
        return 0x20;  // 发送成功
    }
    
    return 0xFF;
}
/* ============= 发送数据（应用层） ============= */
void NRF24L01_SendData(uchar *data, uchar len)
{
    uchar send_buf[32] = {0};
    uchar i;
    
    // 填充数据
    for(i = 0; i < len && i < 32; i++)
        send_buf[i] = data[i];
    
    NRF24L01_TxPacket(send_buf);
}


