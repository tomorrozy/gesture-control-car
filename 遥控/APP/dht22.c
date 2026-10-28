#include "stm32f10x.h"   // Device header
#include "DHT22.h"
#include "OLED.h"
 
/**
  * @brief	初始化传感器的接口
  * @param  None
  * @retval NOne
  */
void DHT22_Init(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOx,ENABLE);
	
	GPIO_InitTypeDef GPIO_InitSture;
	GPIO_InitSture.GPIO_Mode = GPIO_Mode_Out_PP;//推挽输出
	GPIO_InitSture.GPIO_Pin = DHT22_DAT_Pin;//可优化宏定义命名
	GPIO_InitSture.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOx,&GPIO_InitSture);
	
	GPIO_SetBits(GPIOx,DHT22_DAT_Pin);//让其置于高电平（低电平触发）
}
 
/**
  * @brief	主机发出复位信号及传感器响应信号
  * @param  None
  * @retval 返回1代表通信成功，返回0代表通信失败
  */
uint8_t DHT22_Start(void)//起始信号
{
	uint16_t Wite = 0;
	
	DHT22_Mode(0);//输出模式
	GPIO_ResetBits(GPIOx,DHT22_DAT_Pin);//CPU发出起始信号
	delay_ms(1);
	
	DHT22_Mode(1);
	while(GPIO_ReadInputDataBit(GPIOx,DHT22_DAT_Pin)&&Wite < 30)//释放总线时间
	{
		Wite ++;
		delay_us(1);
	}
	if(Wite >= 30) return 0;//0无效通信
	else Wite = 0;
	while(!GPIO_ReadInputDataBit(GPIOx,DHT22_DAT_Pin)&&Wite < 80)//响应信号低电平时间
	{
		Wite ++;
		delay_us(1);
	}
	if(Wite >= 80) return 0;//0无效通信
	else Wite = 0;
	while(GPIO_ReadInputDataBit(GPIOx,DHT22_DAT_Pin)&&Wite < 100)//响应高电平时间
	{
		Wite ++;
		delay_us(1);
	}
	if(Wite >= 100) return 0;//0无效通信
	return 1;
}
 
/**
  * @brief	读取一位数据
  * @param  None
  * @retval 返回一位数据
  */
uint8_t DHT22_ReadBit(void)//读取1bit数据
{	
	DHT22_Mode(1);
	
	while(GPIO_ReadInputDataBit(GPIOx,DHT22_DAT_Pin));//等待数据判断的低电平来
	
	while(!GPIO_ReadInputDataBit(GPIOx,DHT22_DAT_Pin));//等待数据判断的高电平来
	
	delay_us(40);//检测一字节数据用中间值40us的技巧
 
	if(GPIO_ReadInputDataBit(GPIOx,DHT22_DAT_Pin) == SET) return 1;
	else return 0;
}
 
/**
  * @brief	读取一字节数据
  * @param  None
  * @retval 返回一字节数据
  */
uint8_t DHT22_ReadByte(void)//读取1byte数据(8bit)
{
	uint8_t Date = 0;
	
	for(uint8_t i = 0;i < 8;i++)
	{
		Date<<=1;//原数据左移一位
		Date |= DHT22_ReadBit();//通过或运算更新数据
	}
	return Date;
}
 
/**
  * @brief  读取DHT22传感器数据并进行校验
  * @param  Humidity: 指向湿度值的指针，用于返回测量得到的湿度数据
  * @param  Temperature: 指向温度值的指针，用于返回测量得到的温度数据  
  * @retval None
  */
void DHT22_ReadDate(float* Humidity,float* Temperature)//读取校验数据，输出温湿度指针
{	
	uint8_t Date[5];
	uint8_t sum;
	while(DHT22_Start() == 0)//初始化
	DHT22_Mode(1);
	if(GPIO_ReadInputDataBit(GPIOx,DHT22_DAT_Pin) == 0)//确认是否在输出数据
	{
		for(uint8_t i = 0;i <5;i++)
		{
			Date[i] = DHT22_ReadByte();//用存8位的数组存5字节数据
		}
		
		sum = Date[0]+Date[1]+Date[2]+Date[3];//bug点
		
		if(sum == Date[4])//校验数据（因为校验不通过没数据）
		{
			GPIO_SetBits(GPIOA,GPIO_Pin_1);
			uint16_t Humi = (Date[0]<<8) | Date[1];//转化为16位
			uint16_t Temp = ((Date[2] & 0x7F)<<8) | Date[3];//按位有与运算，01111111清除不用的位
			*Humidity = (float)Humi / 10.0;//右值//将humi存入Humidity指针指向的地址
			if(Date[2] & 0x80)//如果高位位为1
			{
				*Temperature = (float)Temp / 10.0 * (-1.0);
			}
			else
			{
				*Temperature = (float)Temp / 10.0;
			}
		}
	}
}
 
/**
  * @brief	设置接口模式
  * @param  mode，可以是0（输出模式），1（输入模式）
  * @retval 返回一位数据
  */
void DHT22_Mode(uint8_t Mode)//mode=0 推挽输出//mode=1 上拉输入
{
	GPIO_InitTypeDef GPIO_InitSture;
	if(Mode == 0)//mode0
	{
		GPIO_InitSture.GPIO_Mode = GPIO_Mode_Out_PP;//推挽输出
		GPIO_InitSture.GPIO_Pin = DHT22_DAT_Pin;//可优化宏定义命名
		GPIO_InitSture.GPIO_Speed = GPIO_Speed_50MHz;
	}
	else
	{
		GPIO_InitSture.GPIO_Mode = GPIO_Mode_IPU;//上拉输入，有bug改为GPIO_Mode_IN_FLOATING浮空
		GPIO_InitSture.GPIO_Pin = DHT22_DAT_Pin ;
		GPIO_InitSture.GPIO_Speed = GPIO_Speed_50MHz;		
	}
	GPIO_Init(GPIOx,&GPIO_InitSture);
}


