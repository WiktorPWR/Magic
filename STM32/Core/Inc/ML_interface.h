#ifndef ML_INTERFACE_H
#define ML_INTERFACE_H

#include <stdint.h>
#include <stdbool.h>

// Definicje klas - zgodnie z Twoim modelem
#define ML_CLASS_L      0
#define ML_CLASS_KOLO   1
#define ML_CLASS_KRZYZ  2
#define ML_CLASS_TLO    3

// Funkcje interfejsu
bool ML_Init(void);
int  ML_RunInference(float* input_data_360_floats, float* out_confidence);

#endif // ML_INTERFACE_H