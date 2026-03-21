#include "MPU6050_interface.h"
#include "main.h"
#include "stm32f4xx_hal_def.h"
#include "stm32f4xx_hal_i2c.h"
#include <stdint.h>


/// Global variable to hold MPU6050 data
struct MPU6050_Data mpu6050_data;

uint8_t check;
uint8_t data;

void MPU6050_Init(void) {
    HAL_I2C_Mem_Read_DMA(&hi2c1, MPU6050_ADDR, WHO_AM_I_REG, I2C_MEMADD_SIZE_8BIT, &check, sizeof(check));
    if (check == 0x68) {
        // MPU6050 is connected and responding
        data = 0x00; // Reset all sensors
        HAL_I2C_Mem_Write_DMA(&hi2c1, MPU6050_ADDR, PWR_MGMT_1_REG, I2C_MEMADD_SIZE_8BIT, &data, sizeof(data));

        // Set sample rate to 1 kHz
        data = 0x07; // Sample rate divider (1 kHz / (1 + 7) = 125 Hz)
        HAL_I2C_Mem_Write_DMA(&hi2c1, MPU6050_ADDR, SMPRT_DIV_REG, I2C_MEMADD_SIZE_8BIT, &data, sizeof(data));

        // Set gyroscope configuration
        data = 0x00; // No FS_SEL, 250 dps
        HAL_I2C_Mem_Write_DMA(&hi2c1, MPU6050_ADDR, GYRO_CONFIG_REG, I2C_MEMADD_SIZE_8BIT, &data, sizeof(data));

        // Set accelerometer configuration
        data = 0x00; // No AFS_SEL, 2g
        HAL_I2C_Mem_Write_DMA(&hi2c1, MPU6050_ADDR, ACCEL_CONFIG_REG, I2C_MEMADD_SIZE_8BIT, &data, sizeof(data));

    } 
}


void MPU6050_Read_Accel(struct MPU6050_Data* data) {
    uint8_t buffer[6];
    HAL_I2C_Mem_Read_DMA(&hi2c1, MPU6050_ADDR, ACCEL_XOUT_H_REG, I2C_MEMADD_SIZE_8BIT, buffer, sizeof(buffer));
    data->Accel_X = (float)(int16_t)(buffer[0] << 8 | buffer[1]) / 16384.0f; // Convert to g
    data->Accel_Y = (float)(int16_t)(buffer[2] << 8 | buffer[3]) / 16384.0f; // Convert to g
    data->Accel_Z = (float)(int16_t)(buffer[4] << 8 | buffer[5]) / 16384.0f; // Convert to g
}

void MPU6050_Read_Gyro(struct MPU6050_Data* data) {
    uint8_t buffer[6];
    HAL_I2C_Mem_Read_DMA(&hi2c1, MPU6050_ADDR, GYRO_XOUT_H_REG, I2C_MEMADD_SIZE_8BIT, buffer, sizeof(buffer));
    data->Gyro_X = (float)(int16_t)(buffer[0] << 8 | buffer[1]) / 131.0f; // Convert to dps
    data->Gyro_Y = (float)(int16_t)(buffer[2] << 8 | buffer[3]) / 131.0f; // Convert to dps
    data->Gyro_Z = (float)(int16_t)(buffer[4] << 8 | buffer[5]) / 131.0f; // Convert to dps
}

void MPU6050_Read_All(struct MPU6050_Data* data) {
    MPU6050_Read_Accel(data);
    MPU6050_Read_Gyro(data);
    data->timestamp = HAL_GetTick(); // Update timestamp
}