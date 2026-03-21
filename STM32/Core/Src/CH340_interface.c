#include "CH340_interface.h"
#include <string.h> 

struct UART_DATA uart_data;

void send_data_over_uart(struct UART_DATA* data) {
    uint8_t buffer[30]; // 6*4b (float) + 4b (uint32) + 2*1b (uint8) = 30 bajtów
    int index = 0;

    // 1. Akcelerometr (3 * 4b)
    memcpy(buffer + index, &data->Accel_X, 4); index += 4;
    memcpy(buffer + index, &data->Accel_Y, 4); index += 4;
    memcpy(buffer + index, &data->Accel_Z, 4); index += 4;

    // 2. Żyroskop (3 * 4b)
    memcpy(buffer + index, &data->Gyro_X, 4);  index += 4;
    memcpy(buffer + index, &data->Gyro_Y, 4);  index += 4;
    memcpy(buffer + index, &data->Gyro_Z, 4);  index += 4;

    // 3. Timestamp (4b) - WAŻNE: Musi być tutaj, żeby pasować do <ffffffLBB
    memcpy(buffer + index, &data->timestamp, 4); index += 4;

    // 4. Statusy (2 * 1b)
    buffer[index++] = (uint8_t)data->mode;
    buffer[index++] = (uint8_t)data->recording;

    // Wysyłka - index wyniesie teraz dokładnie 30
    HAL_UART_Transmit(&huart1, buffer, index, HAL_MAX_DELAY);
}

void convert_mpu_data_to_uart(struct MPU6050_Data* mpu_data, struct UART_DATA* uart_data) {
    uart_data->Accel_X = mpu_data->Accel_X;
    uart_data->Accel_Y = mpu_data->Accel_Y;
    uart_data->Accel_Z = mpu_data->Accel_Z;
    uart_data->Gyro_X = mpu_data->Gyro_X;
    uart_data->Gyro_Y = mpu_data->Gyro_Y;
    uart_data->Gyro_Z = mpu_data->Gyro_Z;
    uart_data->timestamp = mpu_data->timestamp;
}