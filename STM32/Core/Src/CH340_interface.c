#include "CH340_interface.h"
#include <string.h> 
#include <stdio.h>

struct UART_DATA uart_data;

// Deklaracja uchwytu UART, jeśli jest w innym pliku
extern UART_HandleTypeDef huart1;

