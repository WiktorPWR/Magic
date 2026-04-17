#include "CH340_interface.h"
#include <string.h> 
#include <stdio.h>

struct UART_DATA uart_data;

// Deklaracja uchwytu UART, jeśli jest w innym pliku
extern UART_HandleTypeDef huart1;

void send_data_over_uart(struct UART_DATA* data) {
    char text_buffer[128]; // Bufor na tekst
    
    // Konwertujemy floaty na inty z mnożnikiem 100, aby zachować precyzję
    // Przykład: 1.234f -> 123
    int ax = (int)(data->Accel_X * 100);
    int ay = (int)(data->Accel_Y * 100);
    int az = (int)(data->Accel_Z * 100);
    int gx = (int)(data->Gyro_X * 100);
    int gy = (int)(data->Gyro_Y * 100);
    int gz = (int)(data->Gyro_Z * 100);

    // Składamy tekst do wysłania (używamy %d dla int)
    int len = sprintf(text_buffer, "A:%d,%d,%d | G:%d,%d,%d | M:%u | R:%u | T:%lu\r\n",
    ax, ay, az, gx, gy, gz,
    (unsigned int)data->mode,
    (unsigned int)data->recording,
    (unsigned long)data->timestamp
    );

    // Wysyłamy tekst przez UART
    HAL_UART_Transmit(&huart1, (uint8_t*)text_buffer, len, 100);
}


void convert_mpu_data_to_uart(struct MPU6050_Data* mpu_data, struct UART_DATA* uart_data) {
    uart_data->Accel_X = mpu_data->Accel_X;
    uart_data->Accel_Y = mpu_data->Accel_Y;
    uart_data->Accel_Z = mpu_data->Accel_Z;
    uart_data->Gyro_X = mpu_data->Gyro_X;
    uart_data->Gyro_Y = mpu_data->Gyro_Y;
    uart_data->Gyro_Z = mpu_data->Gyro_Z;
    //uart_data->timestamp = mpu_data->timestamp;
}