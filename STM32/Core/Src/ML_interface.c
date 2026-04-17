#include "ML_interface.h"
#include "network.h"
#include "network_data.h"
#include <string.h>

// 1. Bufory pamięci zgodnie z raportem
// Activations (RAM) - 8936 bajtów
AI_ALIGNED(32) static ai_u8 activations[AI_NETWORK_DATA_ACTIVATIONS_SIZE];

// 2. Uchwyty dla modelu
static ai_handle network_handle = AI_HANDLE_NULL;

// Bufory wejścia/wyjścia (same struktury opisujące dane)
static ai_buffer ai_input[AI_NETWORK_IN_NUM];
static ai_buffer ai_output[AI_NETWORK_OUT_NUM];

bool ML_Init(void) {
    ai_error err;

    // Tworzenie instancji sieci
    err = ai_network_create(&network_handle, AI_NETWORK_DATA_CONFIG);
    if (err.type != AI_ERROR_NONE) return false;

    // Konfiguracja parametrów (Wagi we Flashu i Aktywacje w RAM)
    const ai_network_params params = {
        AI_NETWORK_DATA_WEIGHTS_GET(),
        AI_NETWORK_DATA_ACTIVATIONS_GET(activations)
    };

    // Inicjalizacja sieci
    if (!ai_network_init(network_handle, &params)) {
        return false;
    }

    // Pobranie opisów buforów (żeby wiedzieć, gdzie sieć chce dostać dane)
    ai_network_inputs_get(network_handle, ai_input);
    ai_network_outputs_get(network_handle, ai_output);

    return true;
}




/**
 * @brief Uruchamia model
 * @param input_data_360_floats Wskaźnik na 360 floatów (60 próbek * 6 osi)
 * @param out_confidence Wskaźnik na float, gdzie zapiszemy pewność siebie modelu
 * @return int ID wygrywającej klasy (0-3)
 */
int ML_RunInference(float* input_data_360_floats, float* out_confidence) {
    
    // 1. Skopiuj dane do bufora wejściowego sieci
    // ai_input[0].data to wskaźnik do wnętrza bufora activations
    memcpy(ai_input[0].data, input_data_360_floats, 60 * 6 * sizeof(float));

    // 2. Uruchom model
    ai_i32 n_batch = ai_network_run(network_handle, &ai_input[0], &ai_output[0]);
    if (n_batch != 1) return -1;

    // 3. Odczytaj wyniki (Output: 1x4 floaty)
    float* results = (float*)ai_output[0].data;

    // 4. Znajdź najlepszą klasę (ArgMax)
    int best_class = 0;
    float max_score = 0.0f;

    for (int i = 0; i < 4; i++) {
        if (results[i] > max_score) {
            max_score = results[i];
            best_class = i;
        }
    }

    if (out_confidence) *out_confidence = max_score;
    return best_class;
}