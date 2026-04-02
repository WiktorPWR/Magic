#ifndef HAL_I2C_MOCAKABLE_H
#define HAL_I2C_MOCKABLE_H

#include "D:\Pulpit\STM\Magic\Magic\STM32\Drivers\STM32F4xx_HAL_Driver\Inc\stm32f4xx_hal_i2c.h"

HAL_StatusTypeDef HAL_I2C_Mem_Read(I2C_HandleTypeDef *hi2c,
                                   uint16_t DevAddress,
                                   uint16_t MemAddress,
                                   uint16_t MemAddSize,
                                   uint8_t *pData,
                                   uint16_t Size,
                                   uint32_t Timeout);

#endif // HAL_I2C_MOCKABLE_H