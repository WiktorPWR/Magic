#ifndef CH340_INTERFACE_H
#define CH340_INTERFACE_H

#include "main.h"
#include "MPU6050_interface.h"

//This data strucktre is used to set which symblo is drawn.
enum Mode {
    MODE_1 = 0,
    MODE_2 = 1,
    MODE_3 = 2
};

enum Recording{
    NOT_RECORDING = 0,
    RECORDING = 1
}

struct UART_DATA{
    float Accel_X;
    float Accel_Y;
    float Accel_Z;
    float Gyro_X;
    float Gyro_Y;
    float Gyro_Z;
    Mode mode;
    Recording recording;
}


void send_data_over_uart(struct UART_DATA* data);



#endif /* CH340_INTERFACE_H */