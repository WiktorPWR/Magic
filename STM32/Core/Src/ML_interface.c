#include "ML_interface.h"
#include "network.h"
#include "network_data.h"
#include <string.h>

// 1. Bufory pamięci - rozmiar brany z makra wygenerowanego przez AI
AI_ALIGNED(32) static ai_u8 activations[AI_NETWORK_DATA_ACTIVATIONS_SIZE];

// 2. Uchwyty dla modelu
static ai_handle network_handle = AI_HANDLE_NULL;
static ai_network_report report;

bool ML_Init(void) {
    ai_error err;

    // 1. Tworzenie instancji sieci
    // Upewnij się, że AI_NETWORK_DATA_CONFIG jest zdefiniowane w network_data.h
    err = ai_network_create(&network_handle, AI_NETWORK_DATA_CONFIG);
    if (err.type != AI_ERROR_NONE) return false;

    // 2. Przygotowanie parametrów
    ai_network_params params;

    // Pobieramy bazową konfigurację (to wypełnia formaty danych i flagi)
    if (!ai_network_data_params_get(&params)) {
        return false;
    }

    // 3. RĘCZNE POWIĄZANIE WAG I AKTYWACJI (Najbezpieczniejsza metoda)
    // Używamy bezpośrednio funkcji z network_data.h, rzutując na ai_handle
    
    // Ustawienie wag (z Flasha)
    params.params = ai_network_data_weights_buffer_get(ai_network_data_weights_get());
    
    // Ustawienie aktywacji (Twój bufor w RAM)
    params.activations = ai_network_data_activations_buffer_get((ai_handle)activations);

    // 4. Inicjalizacja sieci
    if (!ai_network_init(network_handle, &params)) {
        // Jeśli tu wejdziesz, sprawdź w debuggerze: 
        // ai_network_get_error(network_handle)
        return false;
    }

    // 5. Pobranie raportu
    if (!ai_network_get_report(network_handle, &report)) {
        return false;
    }

    return true;
}
/**
 * @param input_data_600_floats: Tablica 100 próbek * 6 osi
 */
/**
 * @param out_confidences: Wskaźnik na tablicę float[4], gdzie zapiszemy pewność KAŻDEJ klasy
 */
int ML_RunInference(float* input_data_600_floats, float* out_confidences) {
    
    ai_buffer ai_input = report.inputs[0];
    ai_buffer ai_output = report.outputs[0];

    ai_input.data = AI_HANDLE_PTR(input_data_600_floats);
    
    // Wyniki lokalne (AI zapisze tutaj 4 wartości)
    float results[4] = {0.0f};
    ai_output.data = AI_HANDLE_PTR(results);

    if (ai_network_run(network_handle, &ai_input, &ai_output) != 1) {
        return -1;
    }

    // 1. Znajdź najlepszą klasę (ArgMax)
    int best_class = 0;
    float max_score = -1.0f;

    for (int i = 0; i < 4; i++) {
        // Kopiujemy wszystkie wyniki do tablicy wyjściowej, 
        // żebyś miał do nich dostęp w main.c
        if (out_confidences) {
            out_confidences[i] = results[i];
        }

        if (results[i] > max_score) {
            max_score = results[i];
            best_class = i;
        }
    }

    return best_class;
}