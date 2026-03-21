#ifndef CH340_INTERFACE_H
#define CH340_INTERFACE_H

#include "main.h"
#include "MPU6050_interface.h"

//This data strucktre is used to set which symblo is drawn.
enum Mode {
    MODE_1 = 0,//Symbol krzyża
    MODE_2 = 1,//Symbol koła
    MODE_3 = 2,//do lazienki musze isc po migowemu
    NUMBER_OF_MODES
};

enum Recording{
    NOT_RECORDING = 0,
    RECORDING = 1
};

struct UART_DATA{
    float Accel_X;
    float Accel_Y;
    float Accel_Z;
    float Gyro_X;
    float Gyro_Y;
    float Gyro_Z;
    uint32_t timestamp;
    enum Mode mode;
    enum Recording recording;
};

extern struct UART_DATA uart_data;

void send_data_over_uart(struct UART_DATA* data);

void convert_mpu_data_to_uart(struct MPU6050_Data* mpu_data, struct UART_DATA* uart_data);

#endif /* CH340_INTERFACE_H */