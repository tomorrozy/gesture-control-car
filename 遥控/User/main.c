#include "system.h"
#include "SysTick.h"
#include "led.h"
#include "key.h"
#include "OLED.h"
#include "time4.h"
#include "nRF24L01_API.h"
#include <stdio.h>
#include <string.h>
#include "MPU6050.h"

/* ==================== 通信协议定义 ==================== */
#define FRAME_HEAD        0xA5
#define FRAME_TAIL        0x0D

#define GESTURE_NONE   0
#define GESTURE_FWD    1
#define GESTURE_BACK   2
#define GESTURE_LEFT   3
#define GESTURE_RIGHT  4
#define GESTURE_STOP   5

#define PAYLOAD_LEN  9
#define FRAME_LEN    (4 + PAYLOAD_LEN)   /* = 13 字节有效载荷 */

typedef struct {
    float   pitch;
    float   roll;
    uint8_t gesture;
} AttitudePayload_t;

uint8_t ID;
int16_t AX, AY, AZ, GX, GY, GZ;
MPU6050_Angle angle;

#define menu3_1  0
#define menu3_2  1
#define menu3_3  2
#define menu3_4  3
#define menu3_5  4

#define menu3  menu3_5
#define menu  menu3_1

#define menu3_101  101
#define menu3_102  102
#define menu3_103  103
#define menu3_104  104
#define menu3_105  105

u8 buf_data[32];
u16 Time_ms=0, Time_ms2, esp_ms=0;
float humi, humi2, temp, temp2;
u8 Begin=0;
int Level2_menu=0, Level_menu=0, Secondary_menu=0;
int folat=0;

void Key_1menu(void);
void OLED_1menu(void);

/* ==================== 协议打包与手势判断 ==================== */
static uint8_t Gesture_Map(float pitch, float roll)
{
    const float TH = 15.0f;   
    
    /* 如果左右反了，把这里的 LEFT 和 RIGHT 互换即可 */
    if (pitch >  TH) return GESTURE_FWD;
    if (pitch < -TH) return GESTURE_BACK;
    if (roll  >  TH) return GESTURE_LEFT; 
    if (roll  < -TH) return GESTURE_RIGHT;
    
    return GESTURE_STOP;
}

static int Protocol_Pack(const AttitudePayload_t *p, uint8_t *out)
{
    uint8_t chk = 0, i;
    /* 清空整个 32 字节缓冲，防止杂乱数据干扰 */
    memset(out, 0, 32); 
    
    out[0] = FRAME_HEAD;
    out[1] = PAYLOAD_LEN;
    memcpy(out + 2, &p->pitch, 4);
    memcpy(out + 6, &p->roll,  4);
    out[10] = p->gesture;
    
    /* 计算校验和 */
    for (i = 1; i < 2 + PAYLOAD_LEN; i++) chk ^= out[i];
    out[2 + PAYLOAD_LEN] = chk;
    out[3 + PAYLOAD_LEN] = FRAME_TAIL;
    
    return FRAME_LEN;
}
/* ========================================================= */

int main()
{	
	//初始化
	SysTick_Init(72);
	LED_Init();				//初始化LED					
	Key_Init();				//初始化按键
	OLED_Init();			//初始化OLED
	TIM4_Init(1000-1,72-1); //定时1ms
	MPU6050_Init();		    //MPU6050初始化
	MPU6050_CalibrateGyro(200);  
	MPU6050_CalibrateGyro(1000);
    NRF24L01_Init();
    
	while(NRF24L01_Check())
    {
        OLED_Clear();
		OLED_ShowString(1,1,"ON",8);
		OLED_Update(); 
    }
   
    // 初始化为发送器模式，使用地址1
    NRF24L01_TX_Mode_Init((uchar*)TX_ADDR1);
	
	while(1)
	{	
		OLED_1menu();
	}
}

//显示一级菜单并发送数据
void OLED_1menu(void)
{
    float pitch, roll;
    uint8_t gesture;
    AttitudePayload_t payload;
    uint8_t tx_frame[32]; // 固定32字节缓冲
    
	OLED_Clear();
	MPU6050_GetAngle(&angle);
	MPU6050_GetData(&AX, &AY, &AZ, &GX,& GY, &GZ);
	
    /* 获取角度并限幅 */
    pitch = angle.Pitch * -1.0f;
    roll  = angle.Roll  * -1.0f;
    
    if (pitch >  80.0f) pitch =  80.0f;
    if (pitch < -80.0f) pitch = -80.0f;
    if (roll  >  80.0f) roll  =  80.0f;
    if (roll  < -80.0f) roll  = -80.0f;

    /* 获取手势指令 */
    gesture = Gesture_Map(pitch, roll);

    /* OLED 显示 */
	OLED_ShowSignedNum(88, 1,  (int)pitch, 3, OLED_8X16);
	OLED_ShowSignedNum(88, 16, (int)roll,  3, OLED_8X16);
    OLED_ShowString(1, 32, "G:", 16);
    OLED_ShowNum(24, 32, gesture, 2, OLED_8X16);
	
    /* 每 100ms 发送一次二进制帧 */
	if(Time_ms > 100)
	{
        Time_ms = 0;
        payload.pitch   = pitch;
        payload.roll    = roll;
        payload.gesture = gesture;
        Protocol_Pack(&payload, tx_frame);
        
        /* ★ 强制发送 32 字节 */
        NRF24L01_SendData(tx_frame, 32); 
	}
	OLED_Update();
}

//一级菜单按键
void Key_1menu(void)
{
	if(KEY1_LongpressClick)
	{
		KEYDISABLE;
		Level_menu=!Level_menu;
	} 
	else if(KEY1_Shortpress)
	{
		KEYDISABLE;
		Level2_menu=!Level2_menu;
	}
	else if(KEY2_Shortpress)
	{
		KEYDISABLE;
	}
	else if(KEY2_LongpressBatter)
	{
		KEYDISABLE;
	}
	else if(KEY3_Shortpress)
	{
		KEYDISABLE;
	}
	else if(KEY4_Shortpress)
	{
		KEYDISABLE;
	}
	else if(KEY5_Shortpress)
	{
		KEYDISABLE;
	}
	else if(KEYALLOne)
	{
		KEYALL_purge;
	}
}

void TIM4_IRQHandler(void)
{
	if(TIM_GetITStatus(TIM4,TIM_IT_Update))
	{
		Time_ms++;
		Time_ms2++;
		esp_ms++;
		prekey();
	}
	TIM_ClearITPendingBit(TIM4,TIM_IT_Update);	
}

