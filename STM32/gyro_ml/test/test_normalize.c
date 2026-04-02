#include "unity.h"
#include "mock_hal_i2c_mockable.h"
#include "mpu6050.h"

void setUp(void) {}
void tearDown(void) {}

void test_MPU6050_Read_Accel_basic(void) {
    struct MPU6050_Data data;

    // Symulujemy zwracane dane z sensora (raw)
    uint8_t buffer[6] = {0x00, 0x40, 0xFF, 0xC0, 0x01, 0x00}; // przykładowe wartości

    // Mówimy mockowi, żeby zwrócił HAL_OK i wypełnił buffer
    HAL_I2C_Mem_Read_StubWithCallback(
        [](I2C_HandleTypeDef *hi2c,
           uint16_t DevAddress,
           uint16_t MemAddress,
           uint16_t MemAddSize,
           uint8_t *pData,
           uint16_t Size,
           uint32_t Timeout) -> HAL_StatusTypeDef
        {
            for(int i=0;i<6;i++) pData[i] = buffer[i];
            return HAL_OK;
        });

    // Wywołujemy funkcję, którą testujemy
    MPU6050_Read_Accel(&data);

    // Sprawdzamy, czy wartości zostały poprawnie obliczone
    TEST_ASSERT_FLOAT_WITHIN(0.001, (float)((0x00<<8 | 0x40)/16384.0f), data.Accel_X);
    TEST_ASSERT_FLOAT_WITHIN(0.001, (float)((0xFF<<8 | 0xC0)/16384.0f), data.Accel_Y);
    TEST_ASSERT_FLOAT_WITHIN(0.001, (float)((0x01<<8 | 0x00)/16384.0f), data.Accel_Z);
}