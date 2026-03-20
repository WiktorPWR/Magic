#ifndef MPU6050_INTERFACE_H
#define MPU6050_INTERFACE_H


#define MPU6050_ADDR 0x68 << 1 // I2C address of MPU6050 (shifted for HAL library)

#define WHO_AM_I_REG 0x75
#define PWR_MGMT_1_REG 0x6B
#define SMPRT_DIV_REG 0x19
#define CONFIG_REG 0x1A
#define GYRO_CONFIG_REG 0x1B
#define ACCEL_CONFIG_REG 0x1C

struct MPU6050_Data {
    float Accel_X;
    float Accel_Y;
    float Accel_Z;
    float Gyro_X;
    float Gyro_Y;
    float Gyro_Z;
};

extern struct MPU6050_Data mpu6050_data;

void MPU6050_Init(void);

void MPU6050_Read_Accel(struct MPU6050_Data* data);

void MPU6050_Read_Gyro(struct MPU6050_Data* data);

void MPU6050_Read_All(struct MPU6050_Data* data);






#endif /* MPU6050_INTERFACE_H */