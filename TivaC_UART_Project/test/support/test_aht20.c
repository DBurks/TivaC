#include "unity.h"

void setUp(void) {}
void tearDown(void) {}

void test_ConvertRawToCelsius_ShouldReturnCorrectValue(void) {
    // 1. Define 'Fake' Data
    uint32_t fake_raw = 0x66666; 
    
    // 2. Program the Mock
    // We expect our code to call HAL_I2C_ReadByte and we tell it to return fake_raw
    HAL_I2C_ReadByte_ExpectAndReturn(0, fake_raw); 

    // 3. Execute and Assert
    float result = aht20_decode_temp(fake_raw);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 24.5f, result);
}