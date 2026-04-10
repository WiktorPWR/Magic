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


void I2C_Reset(void){
    HAL_I2C_DeInit(&hi2c1);
    HAL_I2C_Init(&hi2c1);
}

void I2C_Scan(void) {
    uint8_t found_device = 0;
    for (uint16_t addr = 1; addr < 128; addr++) {
        if (HAL_I2C_IsDeviceReady(&hi2c1, (addr << 1), 3, 100) == HAL_OK) {
            char msg[30];
            sprintf(msg, "Device found at: 0x%02X\r\n", addr);
            HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), 100);
            found_device = 1;
        }
    }
    if(found_device == 0) {
        HAL_UART_Transmit(&huart1, (uint8_t*)"No I2C devices found!\r\n", 24, 100);
    }
}

HAL_StatusTypeDef MPU6050_Init(void){
    uint8_t check = 0;
    uint8_t data;

    if(HAL_I2C_IsDeviceReady(&hi2c1, MPU6050_ADDR, 3, 100) != HAL_OK)
    {
        return HAL_TIMEOUT; // Urządzenie nie jest gotowe
    }

    if (HAL_I2C_Mem_Read(&hi2c1, MPU6050_ADDR, WHO_AM_I_REG,I2C_MEMADD_SIZE_8BIT, &check, 1, 100) == HAL_OK)
    {
        if (check == 0x68)
        {
            data = 0x00;
            if (HAL_I2C_Mem_Write(&hi2c1, MPU6050_ADDR, PWR_MGMT_1_REG, I2C_MEMADD_SIZE_8BIT, &data, 1, 100) != HAL_OK)
            {
                return HAL_ERROR;
            }

            data = 0x07;
            if (HAL_I2C_Mem_Write(&hi2c1, MPU6050_ADDR, SMPRT_DIV_REG, I2C_MEMADD_SIZE_8BIT, &data, 1, 100) != HAL_OK)
            {
                return HAL_ERROR;
            }

            data = 0x00;
            if (HAL_I2C_Mem_Write(&hi2c1, MPU6050_ADDR, GYRO_CONFIG_REG, I2C_MEMADD_SIZE_8BIT, &data, 1, 100) != HAL_OK)
            {
                return HAL_ERROR;
            }

            data = 0x00;
            if (HAL_I2C_Mem_Write(&hi2c1, MPU6050_ADDR, ACCEL_CONFIG_REG, I2C_MEMADD_SIZE_8BIT, &data, 1, 100) != HAL_OK)
            {
                return HAL_ERROR;
            }

            return HAL_OK; // SUCCESS
        }
    }

    // jak nie działa → reset I2C i próbuj jeszcze raz
    I2C_Reset();
    HAL_Delay(10);
    return HAL_ERROR; // FAIL
}


HAL_StatusTypeDef MPU6050_Read_Accel(struct MPU6050_Data* data) {
    uint8_t buffer[6];
    // Zmieniamy na zwykły Read (blokujący) - dodajemy timeout 100ms
    if (HAL_I2C_Mem_Read(&hi2c1, MPU6050_ADDR, ACCEL_XOUT_H_REG, I2C_MEMADD_SIZE_8BIT, buffer, 6, 100) == HAL_OK) {
        data->Accel_X = (float)(int16_t)(buffer[0] << 8 | buffer[1]) / 16384.0f;
        data->Accel_Y = (float)(int16_t)(buffer[2] << 8 | buffer[3]) / 16384.0f;
        data->Accel_Z = (float)(int16_t)(buffer[4] << 8 | buffer[5]) / 16384.0f;
        return HAL_OK;
    }else {
        return HAL_ERROR;
    }

}

// HAL_StatusTypeDef MPU6050_Read_Gyro(struct MPU6050_Data* data) {
//     uint8_t buffer[6];
//     // Zmieniamy na zwykły Read (blokujący) - dodajemy timeout 100ms
//     if (HAL_I2C_Mem_Read(&hi2c1, MPU6050_ADDR, GYRO_XOUT_H_REG, I2C_MEMADD_SIZE_8BIT, buffer, 6, 100) == HAL_OK) {
//         data->Gyro_X = (float)(int16_t)(buffer[0] << 8 | buffer[1]) / 131.0f; // Convert to dps
//         data->Gyro_Y = (float)(int16_t)(buffer[2] << 8 | buffer[3]) / 131.0f; // Convert to dps
//         data->Gyro_Z = (float)(int16_t)(buffer[4] << 8 | buffer[5]) / 131.0f; // Convert to dps
//         return HAL_OK;
//     } else {
//         return HAL_ERROR;
//     }
// }

HAL_StatusTypeDef MPU6050_Read_Gyro(struct MPU6050_Data* data) {
    uint8_t buffer[6];
    // Zmieniamy na zwykły Read (blokujący) - dodajemy timeout 100ms
    if (HAL_I2C_Mem_Read(&hi2c1, MPU6050_ADDR, GYRO_XOUT_H_REG, I2C_MEMADD_SIZE_8BIT, buffer, 6, 100) == HAL_OK) {
        data->Gyro_X = (float)(int16_t)(buffer[0] << 8 | buffer[1]) / 100.0f; // Convert to dps
        data->Gyro_Y = (float)(int16_t)(buffer[2] << 8 | buffer[3]) / 100.0f; // Convert to dps
        data->Gyro_Z = (float)(int16_t)(buffer[4] << 8 | buffer[5]) / 100.0f; // Convert to dps
        return HAL_OK;
    } else {
        return HAL_ERROR;
    }
}

// Statyczne zmienne do porównania wewnątrz pliku .c
static struct MPU6050_Data last_known_data = {0};
static uint8_t freeze_counter = 0;

HAL_StatusTypeDef MPU6050_Read_All(struct MPU6050_Data* data) {
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
        I2C_Reset();
        HAL_Delay(10);
        if (MPU6050_Init() != HAL_OK) {
            return HAL_ERROR; // Jeśli reanimacja się nie powiedzie, zwróć błąd
        }
    }

    // 4. Zapisz obecny stan do porównania
    last_known_data = *data;
    return HAL_OK;
}


float ML_Input[50][6] = {0};

HAL_StatusTypeDef MPU6050_Read_And_Set_ML_Input(struct MPU6050_Data* data) {
    for(int i = 0; i < 50; i++) {
        if (MPU6050_Read_All(data) == HAL_OK) {
            ML_Input[i][0] = data->Accel_X;
            ML_Input[i][1] = data->Accel_Y;
            ML_Input[i][2] = data->Accel_Z;
            ML_Input[i][3] = data->Gyro_X;
            ML_Input[i][4] = data->Gyro_Y;
            ML_Input[i][5] = data->Gyro_Z;
        } else {
            return HAL_ERROR;
        }

        // Czekaj, aby zachować 20ms odstępu (50Hz)
        // To ważne, żeby gest trwał tyle samo co podczas zbierania danych do treningu!
        HAL_Delay(70); 
    }
    return HAL_OK;
}