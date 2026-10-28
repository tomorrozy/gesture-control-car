#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "led.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "nRF24L01_API.h"
#include "oled.h"
#include "Motor.h"
#include "Encoder.h"
#include <string.h>
#include <stdio.h>
#include <math.h>

/* ==================== 核心开关 ==================== */
#define SCREEN_FLIP 1     // 屏幕倒装(1)，正装(0)
#define REVERSE_TURN 1    // 左右反了改这里(0变1)

float pwm_L_temp;
float pwm_R_temp ;
/* ==================== 通信协议（内联） ==================== */
#define FRAME_HEAD        0xA5
#define FRAME_TAIL        0x0D

#define GESTURE_NONE   0
#define GESTURE_FWD    1
#define GESTURE_BACK   2
#define GESTURE_LEFT   3
#define GESTURE_RIGHT  4
#define GESTURE_STOP   5

#define PAYLOAD_LEN  9
#define FRAME_LEN    (4 + PAYLOAD_LEN)   /* = 13 */

typedef struct {
    float   pitch;
    float   roll;
    uint8_t gesture;
} AttitudePayload_t;

static int Protocol_Unpack(const uint8_t *buf, int len, AttitudePayload_t *p)
{
    uint8_t chk = 0, i;
    if (len != FRAME_LEN)             return -1;
    if (buf[0] != FRAME_HEAD)         return -1;
    if (buf[1] != PAYLOAD_LEN)        return -1;
    if (buf[len - 1] != FRAME_TAIL)   return -1;
    for (i = 1; i < 2 + PAYLOAD_LEN; i++) chk ^= buf[i];
    if (chk != buf[2 + PAYLOAD_LEN])  return -1;
    memcpy(&p->pitch,   buf + 2, 4);
    memcpy(&p->roll,    buf + 6, 4);
    p->gesture = buf[10];
    return 0;
}
/* ========================================================= */

/* ==================== 速度环 PID ==================== */
#define PID_OUT_MAX   100.0f
#define PID_INT_MAX   100.0f    

typedef struct {
    float kp, ki, kd;
    float target;
    float integral;
    float prev_err;
} PID_t;

static void PID_Init(PID_t *p, float kp, float ki, float kd)
{
    p->kp = kp; p->ki = ki; p->kd = kd;
    p->target = 0;
    p->integral = 0;
    p->prev_err = 0;
}

static float PID_Calc(PID_t *p, float feedback)
{
    float err = p->target - feedback;
    p->integral += err;
    if (p->integral >  PID_INT_MAX) p->integral =  PID_INT_MAX;
    if (p->integral < -PID_INT_MAX) p->integral = -PID_INT_MAX;

    float out = p->kp * err + p->ki * p->integral;
    p->prev_err = err;

    if (out > PID_OUT_MAX) out = PID_OUT_MAX;
    if (out < -PID_OUT_MAX) out = -PID_OUT_MAX;
    return out;
}
/* ========================================================= */

/* ==================== 全局变量 ==================== */
volatile float   g_pitch   = 0;
volatile float   g_roll    = 0;
volatile uint8_t g_gesture = GESTURE_STOP;
volatile uchar   channel;
char str[20];

PID_t pid_left, pid_right;
float g_target_left = 0, g_target_right = 0;
float g_act_left    = 0, g_act_right    = 0;
int16_t g_pwm_disp_L = 0, g_pwm_disp_R = 0;

SemaphoreHandle_t xMutex;
StaticSemaphore_t xMutexBuf;

/* ==================== 任务定义 ==================== */
#define START_STK_SIZE   128
#define TASK_RX_STK      256
#define TASK_CTRL_STK    256
#define TASK_DISP_STK    256

#define PRIO_START  1
#define PRIO_RX     3
#define PRIO_CTRL   2
#define PRIO_DISP   1

StackType_t  StartStack[START_STK_SIZE];   StaticTask_t StartTCB;   TaskHandle_t StartH;
StackType_t  RxStack[TASK_RX_STK];         StaticTask_t RxTCB;      TaskHandle_t RxH;
StackType_t  CtrlStack[TASK_CTRL_STK];     StaticTask_t CtrlTCB;    TaskHandle_t CtrlH;
StackType_t  DispStack[TASK_DISP_STK];     StaticTask_t DispTCB;    TaskHandle_t DispH;

static StackType_t  IdleStack[configMINIMAL_STACK_SIZE];
static StaticTask_t IdleTCB;
static StackType_t  TimerStack[configTIMER_TASK_STACK_DEPTH];
static StaticTask_t TimerTCB;

void vApplicationGetIdleTaskMemory(StaticTask_t **ppx, StackType_t **pps, uint32_t *sz)
{ *ppx = &IdleTCB; *pps = IdleStack; *sz = configMINIMAL_STACK_SIZE; }

void vApplicationGetTimerTaskMemory(StaticTask_t **ppx, StackType_t **pps, uint32_t *sz)
{ *ppx = &TimerTCB; *pps = TimerStack; *sz = configTIMER_TASK_STACK_DEPTH; }

void start_task(void *p);
void task_rx(void *p);
void task_ctrl(void *p);
void task_disp(void *p);

/* ==================== main ==================== */
int main(void)
{
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
    delay_init();
    uart_init(115200); 

    OLED_Init();
    LED_Init();
    Motor_Init();
    Encoder_Init();
    NRF24L01_Init();

    while(NRF24L01_Check())
    {
        OLED_Clear();
        OLED_ShowChinese(1, 1, "无线模块错误");
        OLED_Update();
    }
    OLED_Clear();
    OLED_ShowChinese(1, 1, "无线模块正常");
    OLED_Update();

    NRF24L01_RX_MultiChannel_Init();

    /* ★ 重点修改：把 Ki 从 0.2 提到 1.5。它能让 PWM 强顶着上 100 */
    PID_Init(&pid_left,  2.0f, 1.50f, 0.0f);
    PID_Init(&pid_right, 2.0f, 1.50f, 0.0f);

    xMutex = xSemaphoreCreateMutexStatic(&xMutexBuf);
    StartH = xTaskCreateStatic(start_task, "start", START_STK_SIZE, NULL, PRIO_START, StartStack, &StartTCB);
    vTaskStartScheduler();
}

void start_task(void *p)
{
    RxH   = xTaskCreateStatic(task_rx,   "rx",   TASK_RX_STK,   NULL, PRIO_RX,   RxStack,   &RxTCB);
    CtrlH = xTaskCreateStatic(task_ctrl, "ctrl", TASK_CTRL_STK, NULL, PRIO_CTRL, CtrlStack, &CtrlTCB);
    DispH = xTaskCreateStatic(task_disp, "disp", TASK_DISP_STK, NULL, PRIO_DISP, DispStack, &DispTCB);
    vTaskDelete(StartH);
}

void task_rx(void *p)
{
    uchar buf[32];
    uchar ch;
    AttitudePayload_t pl;

    while(1)
    {
        if(NRF24L01_RxPacket_Multi(buf, &ch) == 0)
        {
            if(Protocol_Unpack(buf, FRAME_LEN, &pl) == 0)
            {
                if (pl.pitch >  80.0f) pl.pitch =  80.0f;
                if (pl.pitch < -80.0f) pl.pitch = -80.0f;
                if (pl.roll  >  80.0f) pl.roll  =  80.0f;
                if (pl.roll  < -80.0f) pl.roll  = -80.0f;

                xSemaphoreTake(xMutex, portMAX_DELAY);
                g_pitch   = pl.pitch;
                g_roll    = pl.roll; 
                g_gesture = pl.gesture;
                xSemaphoreGive(xMutex);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

/* ==================== 任务：PID 控制 ==================== */
void task_ctrl(void *p)
{
    int16_t encL, encR;
    float pitch, roll;
    float targetL, targetR;

    const float DEAD_ZONE       = 10.0f;  
    const float TURN_THRESHOLD  = 15.0f;  
    
    const float MAX_SPEED       = 100.0f;  
    const float MIN_SPEED       = 20.0f;   

    while(1)
    {
        xSemaphoreTake(xMutex, portMAX_DELAY);
        pitch   = g_pitch;
        roll    = g_roll;
        xSemaphoreGive(xMutex);

        float speed_x = 0.0f; 
        float speed_y = 0.0f; 

        if (pitch > DEAD_ZONE) {
            speed_x = ((pitch - DEAD_ZONE) / (80.0f - DEAD_ZONE)) * MAX_SPEED;
        } else if (pitch < -DEAD_ZONE) {
            speed_x = ((pitch + DEAD_ZONE) / (80.0f - DEAD_ZONE)) * MAX_SPEED;
        }

        if (roll > TURN_THRESHOLD) {
            speed_y = ((roll - TURN_THRESHOLD) / (80.0f - TURN_THRESHOLD)) * MAX_SPEED;
        } else if (roll < -TURN_THRESHOLD) {
            speed_y = ((roll + TURN_THRESHOLD) / (80.0f - TURN_THRESHOLD)) * MAX_SPEED;
        }

#if REVERSE_TURN
        speed_y = -speed_y; 
#endif

        if (fabs(speed_x) < 0.1f && fabs(speed_y) < 0.1f) {
            targetL = 0; targetR = 0;
        } else {
            targetL = speed_x + speed_y;
            targetR = speed_x - speed_y;
        }

        if (targetL > MAX_SPEED)  targetL = MAX_SPEED;
        if (targetL < -MAX_SPEED) targetL = -MAX_SPEED;
        if (targetR > MAX_SPEED)  targetR = MAX_SPEED;
        if (targetR < -MAX_SPEED) targetR = -MAX_SPEED;

        encL = Encoder_Get(1);
        encR = Encoder_Get(2);

        /* 将 0~20 映射为 0~100 */
        g_act_left  = (encL / 52.0f / 0.02f / 9.27666f) * 5.0f;
        g_act_right = (encR / 52.0f / 0.02f / 9.27666f) * 5.0f;

        pid_left.target  = targetL;
        pid_right.target = targetR;
        
        pwm_L_temp = PID_Calc(&pid_left,  g_act_left);
        pwm_R_temp = PID_Calc(&pid_right, g_act_right);

        if (targetL > 0.1f) {
            if (pwm_L_temp > 0 && pwm_L_temp < 30) pwm_L_temp = 30; 
        } else if (targetL < -0.1f) {
            if (pwm_L_temp < 0 && pwm_L_temp > -30) pwm_L_temp = -30;
        } else {
            pwm_L_temp = 0;
        }

        if (targetR > 0.1f) {
            if (pwm_R_temp > 0 && pwm_R_temp < 30) pwm_R_temp = 30;
        } else if (targetR < -0.1f) {
            if (pwm_R_temp < 0 && pwm_R_temp > -30) pwm_R_temp = -30;
        } else {
            pwm_R_temp = 0;
        }

        if (pwm_L_temp > 100)  pwm_L_temp = 100;
        if (pwm_L_temp < -100) pwm_L_temp = -100;
        if (pwm_R_temp > 100)  pwm_R_temp = 100;
        if (pwm_R_temp < -100) pwm_R_temp = -100;

        Motor_SetPWM(1, (int16_t)pwm_L_temp);
        Motor_SetPWM(2, (int16_t)pwm_R_temp); 

        g_target_left  = targetL;
        g_target_right = targetR;
        g_pwm_disp_L   = (int16_t)pwm_L_temp;
        g_pwm_disp_R   = (int16_t)pwm_R_temp;

        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

/* ==================== 任务：OLED 显示 ==================== */
void task_disp(void *p)
{
    float pitch, roll;
    float tgtL, tgtR, actL, actR;

    while(1)
    {
        xSemaphoreTake(xMutex, portMAX_DELAY);
        pitch   = g_pitch;
        roll    = g_roll;
        xSemaphoreGive(xMutex);

        tgtL = g_target_left;  tgtR = g_target_right;
        actL = g_act_left;     actR = g_act_right;

        char status[20];
        int is_stop     = (fabs(pitch) <= 10.0f);
        int is_forward  = (pitch > 10.0f);
        int is_backward = (pitch < -10.0f);
        
        int is_straight = (fabs(roll) <= 15.0f); 
#if REVERSE_TURN
        int is_right    = (roll < -15.0f);
        int is_left     = (roll > 15.0f);
#else
        int is_right    = (roll > 15.0f);
        int is_left     = (roll < -15.0f);
#endif

        if(is_stop && is_straight)          strcpy(status, "停止");
        else if(is_forward && is_straight)  strcpy(status, "前进");
        else if(is_backward && is_straight) strcpy(status, "后退");
        else if(is_stop && is_right)        strcpy(status, "原地右转");
        else if(is_stop && is_left)         strcpy(status, "原地左转");
        else if(is_forward && is_right)     strcpy(status, "前进右转");
        else if(is_forward && is_left)      strcpy(status, "前进左转");
        else if(is_backward && is_right)    strcpy(status, "后退右转");
        else if(is_backward && is_left)     strcpy(status, "后退左转");
        else                                strcpy(status, "停止");

        float disp_tgtL = tgtL;
        float disp_tgtR = tgtR;
        float disp_actL = actL;
        float disp_actR = actR;

        OLED_Clear();
        
#if SCREEN_FLIP
        OLED_ShowChinese(1, 48, "姿态：");
        sprintf(str, "%3d,%3d", (int)pitch, (int)roll);
        OLED_ShowString(48, 48, str, 8);
        
        /* 状态和 PWM 分开放，防止字重叠 */
        OLED_ShowChinese(1, 32, "状态：");
        OLED_ShowChinese(48, 32, status); 
//        sprintf(str, "%d,%d", (int16_t)pwm_L_temp, (int16_t)pwm_R_temp);
//        OLED_ShowString(64, 32, str, 8);  // 把 PWM 放在最右边
        
        OLED_ShowChinese(1, 16, "目标：");
        sprintf(str, "%3d,%3d", (int)(disp_tgtL), (int)(disp_tgtR));
        OLED_ShowString(48, 16, str, 8);
        
        OLED_ShowChinese(1, 1, "实际：");
        sprintf(str, "%3d,%3d", (int)(disp_actL), (int)(disp_actR));
        OLED_ShowString(48, 1, str, 8);
#else
        OLED_ShowChinese(1,  1, "姿态：");
        sprintf(str, "%3d,%3d", (int)pitch, (int)roll);
        OLED_ShowString(48, 1, str, 8);
        
        OLED_ShowChinese(1, 16, "状态：");
        OLED_ShowChinese(48, 16, status); 
        
        OLED_ShowChinese(1, 32, "目标：");
        sprintf(str, "%3d,%3d", (int)(disp_tgtL), (int)(disp_tgtR));
        OLED_ShowString(48, 32, str, 8);
        
        OLED_ShowChinese(1, 48, "实际：");
        sprintf(str, "%3d,%3d", (int)(disp_actL), (int)(disp_actR));
        OLED_ShowString(48, 48, str, 8);
#endif

        OLED_Update();
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
