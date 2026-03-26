#include "MPU6050_interface.h"
#include "main.h"
#include "stm32f4xx_hal_def.h"
#include "stm32f4xx_hal_i2c.h"
#include <stdint.h>
#include <string.h>

/// Global variable to hold MPU6050 data
struct MPU6050_Data mpu6050_data;

uint8_t check;
uint8_t data;

void MPU6050_Init(void) {
    uint8_t check = 0;
    uint8_t data;

    // 1. Sprawdź czy czujnik żyje (WHO_AM_I)
    HAL_I2C_Mem_Read(&hi2c1, MPU6050_ADDR, WHO_AM_I_REG, I2C_MEMADD_SIZE_8BIT, &check, 1, 1000);

    if (check == 0x68) {
        // 2. Wybudź czujnik (PWR_MGMT_1) - MPU6050 domyślnie startuje w uśpieniu!
        data = 0x00; 
        HAL_I2C_Mem_Write(&hi2c1, MPU6050_ADDR, PWR_MGMT_1_REG, I2C_MEMADD_SIZE_8BIT, &data, 1, 100);

        // 3. Ustaw częstotliwość próbkowania (Sample Rate)
        data = 0x07; // 125 Hz
        HAL_I2C_Mem_Write(&hi2c1, MPU6050_ADDR, SMPRT_DIV_REG, I2C_MEMADD_SIZE_8BIT, &data, 1, 100);

        // 4. Konfiguracja żyroskopu (250 dps)
        data = 0x00;
        HAL_I2C_Mem_Write(&hi2c1, MPU6050_ADDR, GYRO_CONFIG_REG, I2C_MEMADD_SIZE_8BIT, &data, 1, 100);

        // 5. Konfiguracja akcelerometru (2g)
        data = 0x00;
        HAL_I2C_Mem_Write(&hi2c1, MPU6050_ADDR, ACCEL_CONFIG_REG, I2C_MEMADD_SIZE_8BIT, &data, 1, 100);
    }
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
    // 1. Wykonaj swoje standardowe odczyty (zapisują do mpu6050_data)
    MPU6050_Read_Accel(data);
    MPU6050_Read_Gyro(data);

    // 2. Porównaj KAŻDĄ wartość z poprzednią (wszystkie 6 osi)
    if (data->Accel_X == last_known_data.Accel_X && 
        data->Accel_Y == last_known_data.Accel_Y && 
        data->Accel_Z == last_known_data.Accel_Z &&
        data->Gyro_X  == last_known_data.Gyro_X  && 
        data->Gyro_Y  == last_known_data.Gyro_Y  && 
        data->Gyro_Z  == last_known_data.Gyro_Z) 
    {
        // Jeśli KAŻDA z 6 wartości jest identyczna co do bita - inkrementuj licznik
        freeze_counter++;
    } else {
        // Wystarczy, że jedna wartość się zmieniła - czujnik żyje!
        freeze_counter = 0; 
    }

    // 3. Jeśli 5 razy z rzędu dostałeś identyczną "paczkę" danych -> REANIMACJA
    if (freeze_counter >= 5) {
        freeze_counter = 0;
        
        char alert[] = "!!! KRYTYCZNE ZAMROZENIE MPU6050 - Resetuje... !!!\r\n";
        HAL_UART_Transmit(&huart1, (uint8_t*)alert, strlen(alert), 100);

        // Wywołaj inicjalizację, żeby obudzić czujnik i odświeżyć rejestry
        MPU6050_Init(); 
    }

    // 4. Zapisz obecny stan jako "wzorzec" do porównania w następnej pętli
    last_known_data = *data;
}