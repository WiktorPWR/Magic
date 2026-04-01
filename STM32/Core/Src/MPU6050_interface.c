#include "MPU6050_interface.h"
#include "main.h"
#include "stm32f4xx_hal_def.h"
#include "stm32f4xx_hal_i2c.h"
#include <stdint.h>
#include <string.h>

/// Global variable to hold MPU6050 data
struct MPU6050_Data mpu6050_data;
extern I2C_HandleTypeDef hi2c1; // Zewnętrzna deklaracja handlera I2C
extern UART_HandleTypeDef huart1; // Zewnętrzna deklaracja handlera UART

uint8_t check;
uint8_t data;


void I2C_BusRecovery(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // 1. Ustaw GPIO jako open-drain
    GPIO_InitStruct.Pin = GPIO_PIN_6 | GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_SET); // SDA HIGH

    // 2. Clock pulses na SCL
    for(int i = 0; i < 9; i++)
    {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET);
        HAL_Delay(1);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_RESET);
        HAL_Delay(1);
    }

    // 3. STOP condition
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_SET);

    HAL_Delay(1);
}

void I2C_Reset(void)
{
    HAL_I2C_DeInit(&hi2c1);
    I2C_BusRecovery();
    HAL_I2C_Init(&hi2c1);
}


uint8_t MPU6050_Init(void)
{
    uint8_t check = 0;
    uint8_t data;

    for(int attempt = 0; attempt < 3; attempt++)
    {
        if (HAL_I2C_Mem_Read(&hi2c1, MPU6050_ADDR, WHO_AM_I_REG,
                             I2C_MEMADD_SIZE_8BIT, &check, 1, 100) == HAL_OK)
        {
            if (check == 0x68)
            {
                data = 0x00;
                HAL_I2C_Mem_Write(&hi2c1, MPU6050_ADDR, PWR_MGMT_1_REG, I2C_MEMADD_SIZE_8BIT, &data, 1, 100);

                data = 0x07;
                HAL_I2C_Mem_Write(&hi2c1, MPU6050_ADDR, SMPRT_DIV_REG, I2C_MEMADD_SIZE_8BIT, &data, 1, 100);

                data = 0x00;
                HAL_I2C_Mem_Write(&hi2c1, MPU6050_ADDR, GYRO_CONFIG_REG, I2C_MEMADD_SIZE_8BIT, &data, 1, 100);

                data = 0x00;
                HAL_I2C_Mem_Write(&hi2c1, MPU6050_ADDR, ACCEL_CONFIG_REG, I2C_MEMADD_SIZE_8BIT, &data, 1, 100);

                return 1; // SUCCESS
            }
        }

        // jak nie działa → reset I2C i próbuj jeszcze raz
        I2C_Reset();
        HAL_Delay(10);
    }

    return 0; // FAIL
}


void MPU6050_Read_Accel(struct MPU6050_Data* data) {
    uint8_t buffer[6];
    // Zmieniamy na zwykły Read (blokujący) - dodajemy timeout 100ms
    if (HAL_I2C_Mem_Read(&hi2c1, MPU6050_ADDR, ACCEL_XOUT_H_REG, I2C_MEMADD_SIZE_8BIT, buffer, 6, 100) == HAL_OK) {
        data->Accel_X = (float)(int16_t)(buffer[0] << 8 | buffer[1]) / 16384.0f;
        data->Accel_Y = (float)(int16_t)(buffer[2] << 8 | buffer[3]) / 16384.0f;
        data->Accel_Z = (float)(int16_t)(buffer[4] << 8 | buffer[5]) / 16384.0f;
    }
}

void MPU6050_Read_Gyro(struct MPU6050_Data* data) {
    uint8_t buffer[6];
    // Zmieniamy na zwykły Read (blokujący) - dodajemy timeout 100ms
    if (HAL_I2C_Mem_Read(&hi2c1, MPU6050_ADDR, GYRO_XOUT_H_REG, I2C_MEMADD_SIZE_8BIT, buffer, 6, 100) == HAL_OK) {
        data->Gyro_X = (float)(int16_t)(buffer[0] << 8 | buffer[1]) / 131.0f; // Convert to dps
        data->Gyro_Y = (float)(int16_t)(buffer[2] << 8 | buffer[3]) / 131.0f; // Convert to dps
        data->Gyro_Z = (float)(int16_t)(buffer[4] << 8 | buffer[5]) / 131.0f; // Convert to dps
    }
}

// Statyczne zmienne do porównania wewnątrz pliku .c
static struct MPU6050_Data last_known_data = {0};
static uint8_t freeze_counter = 0;

void MPU6050_Read_All(struct MPU6050_Data* data) {
    // 1. Wykonaj swoje standardowe odczyty
    MPU6050_Read_Accel(data);
    MPU6050_Read_Gyro(data);
    data->timestamp = HAL_GetTick(); // Dodaj timestamp do danych


    // 2. Porównaj KAŻDĄ wartość z poprzednią (wszystkie 6 osi)
    if (data->Accel_X == last_known_data.Accel_X && 
        data->Accel_Y == last_known_data.Accel_Y && 
        data->Accel_Z == last_known_data.Accel_Z &&
        data->Gyro_X  == last_known_data.Gyro_X  && 
        data->Gyro_Y  == last_known_data.Gyro_Y  && 
        data->Gyro_Z  == last_known_data.Gyro_Z) 
    {
        freeze_counter++;
    } else {
        freeze_counter = 0; 
    }

    // 3. Reanimacja po 5 identycznych odczytach
    if (freeze_counter >= 5) {
        freeze_counter = 0;
        
        char alert[] = "\r\n!!! KRYTYCZNE ZAMROZENIE MPU6050 - Diagnostyka... !!!\r\n";
        HAL_UART_Transmit(&huart1, (uint8_t*)alert, strlen(alert), 100);

        // --- DODANY SKANER MAGISTRALI ---
        char msg[64];
        uint8_t found_any = 0;
        for (uint8_t i = 1; i < 128; i++) {
            if (HAL_I2C_IsDeviceReady(&hi2c1, (uint16_t)(i << 1), 3, 5) == HAL_OK) {
                int len = sprintf(msg, "Skaner: Znaleziono urzadzenie 0x%02X\r\n", i);
                HAL_UART_Transmit(&huart1, (uint8_t*)msg, len, 100);
                found_any = 1;
            }
        }
        if (!found_any) {
            char error_msg[] = "Skaner: MAGISTRALA PUSTA! Sprawdz kable.\r\n";
            HAL_UART_Transmit(&huart1, (uint8_t*)error_msg, strlen(error_msg), 100);
        }
        // --------------------------------

        // Próba ponownej inicjalizacji
        char msg2[] = "Recovery: RESET I2C + MPU\r\n";
        HAL_UART_Transmit(&huart1, (uint8_t*)msg2, strlen(msg2), 100);

        I2C_Reset();

        if (!MPU6050_Init())
        {
            char fail[] = "Recovery FAILED!\r\n";
            HAL_UART_Transmit(&huart1, (uint8_t*)fail, strlen(fail), 100);
        }
        else
        {
            char ok[] = "Recovery OK!\r\n";
            HAL_UART_Transmit(&huart1, (uint8_t*)ok, strlen(ok), 100);
        }
    
    }

    // 4. Zapisz obecny stan do porównania
    last_known_data = *data;
}