#ifndef __NRF24L01_API_H
#define __NRF24L01_API_H

#include "system.h"
#include "SysTick.h"

/* 指令定义 */
#define nRF_READ_REG    0x00  //读配置寄存器
#define nRF_WRITE_REG   0x20  //写配置寄存器
#define RD_RX_PLOAD     0x61  //读RX有效数据
#define WR_TX_PLOAD     0xA0  //写TX有效数据
#define FLUSH_TX        0xE1  //清除TX FIFO
#define FLUSH_RX        0xE2  //清除RX FIFO
#define REUSE_TX_PL     0xE3  //重复使用上一包数据
#define NOP             0xFF  //空操作

/* 寄存器地址 */
#define CONFIG          0x00
#define EN_AA           0x01
#define EN_RXADDR       0x02
#define SETUP_AW        0x03
#define SETUP_RETR      0x04
#define RF_CH           0x05
#define RF_SETUP        0x06
#define STATUS          0x07
#define OBSERVE_TX      0x08
#define CD              0x09
#define RX_ADDR_P0      0x0A
#define RX_ADDR_P1      0x0B
#define RX_ADDR_P2      0x0C
#define RX_ADDR_P3      0x0D
#define RX_ADDR_P4      0x0E
#define RX_ADDR_P5      0x0F
#define TX_ADDR         0x10
#define RX_PW_P0        0x11
#define RX_PW_P1        0x12
#define RX_PW_P2        0x13
#define RX_PW_P3        0x14
#define RX_PW_P4        0x15
#define RX_PW_P5        0x16
#define FIFO_STATUS     0x17

/* 状态寄存器位定义 */
#define MAX_TX  0x10  //达到最大重发次数
#define TX_OK   0x20  //发送完成
#define RX_OK   0x40  //接收完成

/* 通道号定义（根据STATUS寄存器的RX_P_NO位） */
#define RX_P_NO_MASK    0x0E  // 位[3:1]表示通道号
#define RX_P_NO_P0      0x00  // 通道0
#define RX_P_NO_P1      0x02  // 通道1
#define RX_P_NO_P2      0x04  // 通道2
#define RX_P_NO_P3      0x06  // 通道3
#define RX_P_NO_P4      0x08  // 通道4
#define RX_P_NO_P5      0x0A  // 通道5

/* 数据类型定义 */
#define uchar unsigned char
#define uint unsigned int

/* 地址常量声明 */
extern const uchar TX_ADDR1[5];  // 发送器1地址（用于通道0）
extern const uchar TX_ADDR2[5];  // 发送器2地址（用于通道1）
extern const uchar RX_ADDR[5];   // 接收器地址（用于ACK）

/* 函数声明 */
void NRF24L01_Init(void);
void DelayUs(uchar num);
void DelayMs(uint ms);
uchar SPI_RW(uchar byte);
uchar NRF24L01_Write_Reg(uchar reg, uchar value);
uchar NRF24L01_Read_Reg(uchar reg);
uchar NRF24L01_Read_Buf(uchar reg, uchar *pBuf, uchar len);
uchar NRF24L01_Write_Buf(uchar reg, uchar *pBuf, uchar len);
uchar NRF24L01_Check(void);

/* 接收器相关函数（多通道版本） */
void NRF24L01_RX_MultiChannel_Init(void);
uchar NRF24L01_RxPacket_Multi(uchar *rxbuf, uchar *channel);
uchar NRF24L01_GetChannelFromStatus(uchar status);

/* 发送器相关函数 */
void NRF24L01_TX_Mode_Init(uchar *tx_addr);
uchar NRF24L01_TxPacket(uchar *txbuf);
void NRF24L01_SendData(uchar *data, uchar len);

#endif



