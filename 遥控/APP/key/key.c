#include "Key.h"

u8 KEYnum=0;
uint8_t KEYFlags=1;
uint8_t KEY1Flags=0;//按键1标志位
uint8_t KEY2Flags=0;
uint8_t KEY3Flags=0;
uint8_t KEY4Flags=0;
uint8_t KEY5Flags=0;
uint8_t KEY6Flags=0;
u16 KEY1ms=0; //按键0时长
u16 KEY2ms=0;
u16 KEY3ms=0;
u16 KEY4ms=0;
u16 KEY5ms=0;
u16 KEY6ms=0;


/*******************************************************************************
* 函 数 名         : KEY_Init
* 函数功能		   : 按键初始化
* 输    入         : 无
* 输    出         : 无
*******************************************************************************/
void Key_Init(void)
{

	GPIO_InitTypeDef GPIO_InitStructure; //定义结构体变量	
	RCC_APB2PeriphClockCmd(KEY1_PORT_RCC,ENABLE);
	RCC_APB2PeriphClockCmd(KEY2_PORT_RCC,ENABLE);
	RCC_APB2PeriphClockCmd(KEY3_PORT_RCC,ENABLE);
	RCC_APB2PeriphClockCmd(KEY4_PORT_RCC,ENABLE);
	RCC_APB2PeriphClockCmd(KEY5_PORT_RCC,ENABLE);
//	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO,ENABLE);
	
	
	GPIO_InitStructure.GPIO_Pin=KEY1_PIN;//选择你要设置的IO口
	GPIO_InitStructure.GPIO_Mode=GPIO_Mode_IPU;//上拉输入  
	GPIO_Init(KEY1_PORT,&GPIO_InitStructure);/* 初始化GPIO */
	
	GPIO_InitStructure.GPIO_Pin=KEY2_PIN;
	GPIO_Init(KEY2_PORT,&GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin=KEY3_PIN;
	GPIO_Init(KEY3_PORT,&GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin=KEY4_PIN;
	GPIO_Init(KEY4_PORT,&GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin=KEY5_PIN;
	GPIO_Init(KEY5_PORT,&GPIO_InitStructure);
}

/*******************************************************************************
* 函 数 名         : prekey
* 函数功能		   : 按键按下检测
* 输    入         : 无
* 输    出         : 无
*******************************************************************************/
void prekey(void)
{
	if(!KEY1 && KEY2 && KEY3 && KEY4 && KEY5) //任意一个按键按下
	{
		KEY1ms++;KEY1Flags=1;
	}
	if(KEY1 && !KEY2 && KEY3 && KEY4 && KEY5) //任意一个按键按下
	{
		KEY2ms++;KEY2Flags=1;
	}
	if(KEY1 && KEY2 && !KEY3 && KEY4 && KEY5) //任意一个按键按下
	{
		KEY3ms++;KEY3Flags=1;
	}
	if(KEY1 && KEY2 && KEY3 && !KEY4 && KEY5) //任意一个按键按下
	{
		KEY4ms++;KEY4Flags=1;
	}
	if(KEY1 && KEY2 && KEY3 && KEY4 && !KEY5 ) //任意一个按键按下
	{
		KEY5ms++;KEY5Flags=1;
	}
	if(!KEY1 && !KEY2 && KEY3 && KEY4 && KEY5 && KEY1ms<presstime && KEY2ms<presstime ) //任意一个按键按下
	{
		KEY1ms=0;KEY1Flags=0;KEY2ms=0;KEY2Flags=0;KEY6ms++;KEY6Flags=1;
	}
}




//if(KEY1_LongpressClick)单击长按
//	{	KEYDISABLE;
//		if(key>0)
//		{
//			key--;
//		}
//	}
//	else if(KEY1_Shortpress)单击短按
//	{
//		KEYDISABLE;
//		if(key<1000)
//		{
//			key++;
//		}
//	}
//	else if(KEY2_LongpressBatter)连续长按
//	{
//		KEYDISABLE;
//		if(key>0)
//		{
//			key--;
//		}
//	}

/*******************************************************************************
* 函 数 名         : Key_GetState
* 函数功能		   : 按键短按与长按检测
* 输    入         : 无
* 输    出         : 1-10
*******************************************************************************/
//uint8_t Key_GetState(void)
//{
//	if(!KEY1 && KEY1ms>=presstime)
//	{
//		return 6;
//	}
//	else if(KEY1 && KEY1Flags)
//	{
//		return 1;KEY1Flags=0;KEY1ms=0;
//	}
//	if(!KEY2 && KEY2ms>=presstime)
//	{
//		return 7;
//	}
//	else if(KEY2 && KEY2Flags)
//	{
//		return 2;KEY2Flags=0;KEY2ms=0;
//	}
//	if(!KEY3 && KEY3ms>=presstime)
//	{
//		return 8;
//	}
//	else if(KEY3 && KEY3Flags)
//	{
//		return 3;KEY3Flags=0;KEY3ms=0;
//	}
//	if(!KEY4 && KEY4ms>=presstime)
//	{
//		return 9;
//	}
//	else if(KEY4 && KEY4Flags)
//	{
//		return 4;KEY4Flags=0;KEY4ms=0;
//	}
//	if(!KEY5 && KEY5ms>=presstime)
//	{
//		return 10;
//	}
//	else if(KEY5 && KEY5Flags)
//	{
//		return 5;KEY5Flags=0;KEY5ms=0;
//	}
//	if(KEY1 && KEY2 && KEY3 && KEY4 && KEY5)
//	{
//		KEY1Flags=0;
//		KEY2Flags=0;
//		KEY3Flags=0;
//		KEY4Flags=0;
//		KEY5Flags=0;
//		
//		KEY1ms=0;
//		KEY2ms=0;
//		KEY3ms=0;
//		KEY4ms=0;
//		KEY5ms=0;
//	}
//	
////	return 0;
//}	












