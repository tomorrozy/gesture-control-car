#ifndef __MPU6050_H
#define __MPU6050_H

#include <stdint.h>

typedef struct {
    float Pitch;
    float Roll;  
    float Yaw;
} MPU6050_Angle;

void MPU6050_WriteReg(uint8_t RegAddress, uint8_t Data);
uint8_t MPU6050_ReadReg(uint8_t RegAddress);
void MPU6050_ReadMultiReg(uint8_t reg, uint8_t *data, uint8_t len);

void MPU6050_Init(void);
uint8_t MPU6050_GetID(void);
void MPU6050_GetData(int16_t *AccX, int16_t *AccY, int16_t *AccZ, 
                     int16_t *GyroX, int16_t *GyroY, int16_t *GyroZ);

void MPU6050_CalibrateGyro(uint16_t samples);
void MPU6050_GetAngle(MPU6050_Angle *angle);

#endif