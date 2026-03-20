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
    HAL_I2C_Mem_Read_DMA(&hi2c1, MPU6050_ADDR, WHO_AM_I_REG, I2C_MEMADD_SIZE_8BIT, &check, sizeof(check),HAL_MAX_DELAY);
    if (check == 0x68) {
        // MPU6050 is connected and responding
        data = 0x00; // Reset all sensors
        HAL_I2C_Mem_Write_DMA(&hi2c1, MPU6050_ADDR, PWR_MGMT_1_REG, I2C_MEMADD_SIZE_8BIT, &data, sizeof(data),HAL_MAX_DELAY);

        // Set sample rate to 1 kHz
        data = 0x07; // Sample rate divider (1 kHz / (1 + 7) = 125 Hz)
        HAL_I2C_Mem_Write_DMA(&hi2c1, MPU6050_ADDR, SMPRT_DIV_REG, I2C_MEMADD_SIZE_8BIT, &data, sizeof(data),HAL_MAX_DELAY);

        // Set gyroscope configuration
        data = 0x00; // No FS_SEL, 250 dps
        HAL_I2C_Mem_Write_DMA(&hi2c1, MPU6050_ADDR, GYRO_CONFIG_REG, I2C_MEMADD_SIZE_8BIT, &data, sizeof(data),HAL_MAX_DELAY);

        // Set accelerometer configuration
        data = 0x00; // No AFS_SEL, 2g
        HAL_I2C_Mem_Write_DMA(&hi2c1, MPU6050_ADDR, ACCEL_CONFIG_REG, I2C_MEMADD_SIZE_8BIT, &data, sizeof(data),HAL_MAX_DELAY);

    } 
}