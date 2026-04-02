#include "mpu6050.h"
#include "hal_i2c_mockable.h"




void MPU6050_Read_Accel(struct MPU6050_Data* data) {
    uint8_t buffer[6];
    if(HAL_I2C_Mem_Read(&hi2c1, MPU6050_ADDR, ACCEL_XOUT_H_REG, I2C_MEMADD_SIZE_8BIT, buffer, 6, 100) == HAL_OK) {
        data->Accel_X = (float)(int16_t)(buffer[0]<<8 | buffer[1]) / 16384.0f;
        data->Accel_Y = (float)(int16_t)(buffer[2]<<8 | buffer[3]) / 16384.0f;
        data->Accel_Z = (float)(int16_t)(buffer[4]<<8 | buffer[5]) / 16384.0f;
    }
}

void MPU6050_Read_Gyro(struct MPU6050_Data* data) {
    uint8_t buffer[6];
    if(HAL_I2C_Mem_Read(&hi2c1, MPU6050_ADDR, GYRO_XOUT_H_REG, I2C_MEMADD_SIZE_8BIT, buffer, 6, 100) == HAL_OK) {
        data->Gyro_X = (float)(int16_t)(buffer[0]<<8 | buffer[1]) / 131.0f;
        data->Gyro_Y = (float)(int16_t)(buffer[2]<<8 | buffer[3]) / 131.0f;
        data->Gyro_Z = (float)(int16_t)(buffer[4]<<8 | buffer[5]) / 131.0f;
    }
}