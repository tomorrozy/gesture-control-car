#ifndef __KEY_H
#define __KEY_H

#include "system.h"
#include "stm32f10x.h"                  // Device header

//管脚定义
#define KEY1_PORT 			GPIOB  
#define KEY1_PIN 			GPIO_Pin_12
#define KEY1_PORT_RCC		RCC_APB2Periph_GPIOB

#define KEY2_PORT 			GPIOB  
#define KEY2_PIN 			GPIO_Pin_13
#define KEY2_PORT_RCC		RCC_APB2Periph_GPIOB

#define KEY3_PORT 			GPIOB  
#define KEY3_PIN 			GPIO_Pin_14
#define KEY3_PORT_RCC		RCC_APB2Periph_GPIOB

#define KEY4_PORT 			GPIOB  
#define KEY4_PIN 			GPIO_Pin_15
#define KEY4_PORT_RCC		RCC_APB2Periph_GPIOB

#define KEY5_PORT 			GPIOA  
#define KEY5_PIN 			GPIO_Pin_8
#define KEY5_PORT_RCC		RCC_APB2Periph_GPIOA 

//使用位操作定义
#define KEY1 	PBin(12)
#define KEY2 	PBin(13)
#define KEY3 	PBin(14)
#define KEY4 	PBin(15)
#define KEY5 	PAin(8)

//定义各个按键  
#define KEY1_LongpressClick !KEY1 && KEY1ms>=presstime && KEYFlags   //长按单击
#define KEY2_LongpressClick !KEY2 && KEY2ms>=presstime && KEYFlags 
#define KEY3_LongpressClick !KEY3 && KEY3ms>=presstime && KEYFlags 
#define KEY4_LongpressClick !KEY4 && KEY4ms>=presstime && KEYFlags 
#define KEY5_LongpressClick !KEY5 && KEY5ms>=presstime && KEYFlags 

#define KEY1_LongpressBatter !KEY1 && KEY1ms>=presstime    //长按连续
#define KEY2_LongpressBatter !KEY2 && KEY2ms>=presstime 
#define KEY3_LongpressBatter !KEY3 && KEY3ms>=presstime 
#define KEY4_LongpressBatter !KEY4 && KEY4ms>=presstime 
#define KEY5_LongpressBatter !KEY5 && KEY5ms>=presstime

#define KEY1_Shortpress KEY1 && KEY1Flags && KEYFlags     //短按单击     
#define KEY2_Shortpress KEY2 && KEY2Flags && KEYFlags
#define KEY3_Shortpress KEY3 && KEY3Flags && KEYFlags
#define KEY4_Shortpress KEY4 && KEY4Flags && KEYFlags
#define KEY5_Shortpress KEY5 && KEY5Flags && KEYFlags

#define KEYALLOne KEY1 && KEY2 && KEY3 && KEY4 && KEY5


#define KEYALL_purge KEY1Flags=KEY1ms=KEY2Flags=KEY2ms=KEY3Flags=KEY3ms=KEY4Flags=KEY4ms=KEY5Flags=KEY5ms=0;KEYENABLE
#define KEYENABLE KEYFlags=1        
#define KEYDISABLE KEYFlags=0  	
#define presstime 500			//长按时间ms


extern uint8_t KEYFlags;   
					 
extern uint8_t KEY1Flags;//按键1标志位
extern uint8_t KEY2Flags;
extern uint8_t KEY3Flags;
extern uint8_t KEY4Flags;
extern uint8_t KEY5Flags;
extern uint8_t KEY6Flags;

extern u16 KEY1ms; //按键0时长
extern u16 KEY2ms;
extern u16 KEY3ms;
extern u16 KEY4ms;
extern u16 KEY5ms;
extern u16 KEY6ms;



void Key_Init(void);
uint8_t Key_GetState(void);
void prekey(void);

#endif
