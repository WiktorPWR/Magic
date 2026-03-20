#include "CH340_interface.h"
#include <string.h> 

struct UART_DATA uart_data;

void send_data_over_uart(struct UART_DATA* data) {
    uint8_t buffer[30];
    int index = 0;

    // Convert float values to bytes and store in buffer
    memcpy(buffer + index, &data->Accel_X, sizeof(float));
    index += sizeof(float);
    memcpy(buffer + index, &data->Accel_Y, sizeof(float));
    index += sizeof(float);
    memcpy(buffer + index, &data->Accel_Z, sizeof(float));
    index += sizeof(float);
    memcpy(buffer + index, &data->Gyro_X, sizeof(float));
    index += sizeof(float);
    memcpy(buffer + index, &data->Gyro_Y, sizeof(float));
    index += sizeof(float);
    memcpy(buffer + index, &data->Gyro_Z, sizeof(float));
    index += sizeof(float);

    // Add mode and recording status to buffer
    buffer[index++] = (uint8_t)data->mode;
    buffer[index++] = (uint8_t)data->recording;

    // Send the buffer over UART
    HAL_UART_Transmit(&huart1, buffer, index, HAL_MAX_DELAY);
}

void convert_mpu_data_to_uart(struct MPU6050_Data* mpu_data, struct UART_DATA* uart_data) {
    uart_data->Accel_X = mpu_data->Accel_X;
    uart_data->Accel_Y = mpu_data->Accel_Y;
    uart_data->Accel_Z = mpu_data->Accel_Z;
    uart_data->Gyro_X = mpu_data->Gyro_X;
    uart_data->Gyro_Y = mpu_data->Gyro_Y;
    uart_data->Gyro_Z = mpu_data->Gyro_Z;
}