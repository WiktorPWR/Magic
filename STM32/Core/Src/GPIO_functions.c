#include "GPIO_functions.h"

void Mode_setting(struct UART_DATA* data) {
    // 1. Odczytujemy stany (zakładamy Pull-up, więc negujemy '!', żeby wciśnięty = 1)
    uint8_t b0 = (HAL_GPIO_ReadPin(mode_button_1_GPIO_Port, mode_button_1_Pin) == GPIO_PIN_SET);
    uint8_t b1 = (HAL_GPIO_ReadPin(mode_button_2_GPIO_Port, mode_button_2_Pin) == GPIO_PIN_SET);
    uint8_t b2 = (HAL_GPIO_ReadPin(mode_button_3_GPIO_Port, mode_button_3_Pin) == GPIO_PIN_SET);
    // 2. Składamy bity w jedną liczbę (b2 b1 b0)
    uint8_t button_pattern = (b2 << 2) | (b1 << 1) | b0;

    // 3. Wybieramy tryb na podstawie wzorca
    switch (button_pattern) {
        case 0b001: // Tylko przycisk 1
            data->mode = MODE_1;
            break;
        case 0b010: // Tylko przycisk 2
            data->mode = MODE_2;
            break;
        case 0b100: // Tylko przycisk 3
            data->mode = MODE_3;
            break;
        case 0b000:
            // Opcjonalnie: żaden nie wciśnięty
            break;
        default:
            // Opcjonalnie: kombinacja wielu przycisków (np. 1 i 2 naraz)
            break;
    }
}