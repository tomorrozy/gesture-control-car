#include "stm32f10x.h"
#include "MyI2C.h"
#include "MPU6050_Reg.h"
#include "MPU6050.h"
#include <math.h>

#define MPU6050_ADDRESS		0xD0

#define GYRO_SCALE_FACTOR    16.4f
#define ACCEL_SCALE_FACTOR   2048.0f

#define ALPHA                0.95f
#define LOOP_TIME            0.01f

static float pitch = 0, roll = 0;
static float gyroX_offset = 0, gyroY_offset = 0;

void MPU6050_ReadMultiReg(uint8_t RegAddress, uint8_t *pData, uint8_t len)
{
	uint8_t i;
	MyI2C_Start();
	MyI2C_SendByte(MPU6050_ADDRESS);
	MyI2C_ReceiveAck();
	MyI2C_SendByte(RegAddress);
	MyI2C_ReceiveAck();
	
	MyI2C_Start();
	MyI2C_SendByte(MPU6050_ADDRESS | 0x01);
	MyI2C_ReceiveAck();
	
	for(i=0; i<len-1; i++)
	{
		pData[i] = MyI2C_ReceiveByte();
		MyI2C_SendAck(0);
	}
	pData[len-1] = MyI2C_ReceiveByte();
	MyI2C_SendAck(1);
	MyI2C_Stop();
}

void MPU6050_GetData(int16_t *AccX, int16_t *AccY, int16_t *AccZ, 
						int16_t *GyroX, int16_t *GyroY, int16_t *GyroZ)
{
	uint8_t buf[14];
	MPU6050_ReadMultiReg(MPU6050_ACCEL_XOUT_H, buf, 14);
	
	*AccX = (buf[0]  << 8) | buf[1];
	*AccY = (buf[2]  << 8) | buf[3];
	*AccZ = (buf[4]  << 8) | buf[5];
	*GyroX= (buf[8]  << 8) | buf[9];
	*GyroY= (buf[10] << 8) | buf[11];
	*GyroZ= (buf[12] << 8) | buf[13];
}

void MPU6050_Init(void)
{
	MyI2C_Init();
	MPU6050_WriteReg(MPU6050_PWR_MGMT_1, 0x01);
	MPU6050_WriteReg(MPU6050_PWR_MGMT_2, 0x00);
	MPU6050_WriteReg(MPU6050_SMPLRT_DIV, 0x09);
	MPU6050_WriteReg(MPU6050_CONFIG, 0x06);
	MPU6050_WriteReg(MPU6050_GYRO_CONFIG, 0x18);
	MPU6050_WriteReg(MPU6050_ACCEL_CONFIG, 0x18);
	pitch = 0;
	roll = 0;
}

void MPU6050_CalibrateGyro(uint16_t samples)
{
	int16_t ax,ay,az,gx,gy,gz;
	int32_t sumX=0, sumY=0;
	uint16_t i;
	
	for(i=0; i<samples; i++)
	{
		MPU6050_GetData(&ax,&ay,&az,&gx,&gy,&gz);
		sumX += gx;
		sumY += gy;
		for(uint32_t j=0;j<5000;j++);
	}
	
	gyroX_offset = (float)sumX / samples;
	gyroY_offset = (float)sumY / samples;
	pitch = 0;
	roll = 0;
}

// ========================= 【修复：完整解耦的加速度角度公式】 =========================
static void GetAccelAngles(int16_t ax, int16_t ay, int16_t az, float *acc_pitch, float *acc_roll)
{
	float ax_g = (float)ax / ACCEL_SCALE_FACTOR;
	float ay_g = (float)ay / ACCEL_SCALE_FACTOR;
	float az_g = (float)az / ACCEL_SCALE_FACTOR;

    // 修复：3轴完整解耦公式，大角度无串轴
    float sqrt_yz = sqrtf(ay_g*ay_g + az_g*az_g);
    *acc_pitch = atan2f(ax_g, sqrt_yz) * 57.2958f;
    
    float sqrt_xz = sqrtf(ax_g*ax_g + az_g*az_g);
    *acc_roll  = atan2f(-ay_g, sqrt_xz) * 57.2958f;
}
// ==============================================================================

void MPU6050_GetAngle(MPU6050_Angle *angle)
{
	int16_t ax,ay,az,gx,gy,gz;
	float acc_pitch, acc_roll;
	float gx_deg, gy_deg;
    float cos_roll, sin_roll, cos_pitch; // 新增：解耦滤波所需变量
	
	MPU6050_GetData(&ax,&ay,&az,&gx,&gy,&gz);
	GetAccelAngles(ax, ay, az, &acc_pitch, &acc_roll);
	
	gx_deg = (gx - gyroX_offset) / GYRO_SCALE_FACTOR;
	gy_deg = (gy - gyroY_offset) / GYRO_SCALE_FACTOR;

    // 修复：解耦互补滤波，彻底消除轴串扰
    cos_roll = cosf(roll * 3.14159f / 180.0f);
    sin_roll = sinf(roll * 3.14159f / 180.0f);
    cos_pitch = cosf(pitch * 3.14159f / 180.0f);

    // 陀螺仪数据解耦计算
    float pitch_gyro = gy_deg + gx_deg * sin_roll * tanf(pitch * 3.14159f / 180.0f);
    float roll_gyro  = gx_deg * cos_roll - gy_deg * sin_roll;

    // 解耦后的互补滤波
    pitch = ALPHA * (pitch + pitch_gyro * LOOP_TIME) + (1 - ALPHA) * acc_pitch;
    roll  = ALPHA * (roll  + roll_gyro  * LOOP_TIME) + (1 - ALPHA) * acc_roll;

	// 限幅
	if(pitch > 90) pitch=90; if(pitch < -90) pitch=-90;
	if(roll  > 90) roll=90;  if(roll  < -90) roll=-90;
	
	angle->Pitch = pitch;
	angle->Roll  = roll;
	angle->Yaw   = 0;
}

void MPU6050_WriteReg(uint8_t RegAddress, uint8_t Data)
{
	MyI2C_Start();
	MyI2C_SendByte(MPU6050_ADDRESS);
	MyI2C_ReceiveAck();
	MyI2C_SendByte(RegAddress);
	MyI2C_ReceiveAck();
	MyI2C_SendByte(Data);
	MyI2C_ReceiveAck();
	MyI2C_Stop();
}

uint8_t MPU6050_ReadReg(uint8_t RegAddress)
{
	uint8_t Data;
	MyI2C_Start();
	MyI2C_SendByte(MPU6050_ADDRESS);
	MyI2C_ReceiveAck();
	MyI2C_SendByte(RegAddress);
	MyI2C_ReceiveAck();
	
	MyI2C_Start();
	MyI2C_SendByte(MPU6050_ADDRESS | 0x01);
	MyI2C_ReceiveAck();
	Data = MyI2C_ReceiveByte();
	MyI2C_SendAck(1);
	MyI2C_Stop();
	return Data;
}

uint8_t MPU6050_GetID(void)
{
	return MPU6050_ReadReg(MPU6050_WHO_AM_I);
}