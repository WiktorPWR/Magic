import numpy as np
import tensorflow as tf
import os
import pandas as pd
import matplotlib.pyplot as plt
from sklearn.metrics import confusion_matrix, ConfusionMatrixDisplay

# --- KONFIGURACJA ---
PATH_TO_MODEL = r"D:\Pulpit\STM\Magic\Magic\Python_scripts\model_gestow_2026-04-11_13-24.tflite"
DATA_DIR = r"D:\Pulpit\STM\Magic\augmented_data\augmented_data"
CATEGORIES = ["kolo", "krzyz", "L", "tlo"]
COLS = ['AX', 'AY', 'AZ', 'GX', 'GY', 'GZ']

# 1. Załadowanie modelu i sprawdzenie czego on oczekuje
interpreter = tf.lite.Interpreter(model_path=PATH_TO_MODEL)
interpreter.allocate_tensors()

input_details = interpreter.get_input_details()
output_details = interpreter.get_output_details()

# Sprawdzamy kształt wejścia: [batch, rows, channels] -> np. [1, 40, 6]
input_shape = input_details[0]['shape']
EXPECTED_ROWS = input_shape[1]
EXPECTED_COLS = input_shape[2]

print(f"Model oczekuje: {EXPECTED_ROWS} wierszy i {EXPECTED_COLS} kolumn.")

def prepare_data(df, target_rows):
    # Wybieramy tylko potrzebne kolumny
    data = df[COLS].values
    
    current_rows = len(data)
    
    if current_rows > target_rows:
        # Jeśli za dużo danych - ucinamy środek lub koniec (tu: bierzemy początek)
        return data[:target_rows, :]
    elif current_rows < target_rows:
        # Jeśli za mało - dodajemy zera na końcu (Padding)
        pad_width = target_rows - current_rows
        return np.pad(data, ((0, pad_width), (0, 0)), mode='constant')
    else:
        return data

def predict_tflite(interpreter, input_data):
    # Dodanie wymiaru batcha (1, rows, cols)
    input_tensor = np.expand_dims(input_data, axis=0).astype(np.float32)
    
    interpreter.set_tensor(input_details[0]['index'], input_tensor)
    interpreter.invoke()
    
    output = interpreter.get_tensor(output_details[0]['index'])
    return np.argmax(output), output[0]

y_true = []
y_pred = []

# 2. Przetwarzanie plików
for idx, category in enumerate(CATEGORIES):
    folder_path = os.path.join(DATA_DIR, category)
    if not os.path.exists(folder_path):
        print(f"Pominięto folder: {category} (brak folderu)")
        continue
        
    files = [f for f in os.listdir(folder_path) if f.endswith('.csv')]
    print(f"Przetwarzanie {category}: {len(files)} plików")

    for file_name in files:
        try:
            df = pd.read_csv(os.path.join(folder_path, file_name))
            
            # Dopasowanie rozmiaru do modelu
            processed_data = prepare_data(df, EXPECTED_ROWS)
            
            # Klasyfikacja
            pred_idx, _ = predict_tflite(interpreter, processed_data)
            
            y_true.append(idx)
            y_pred.append(pred_idx)
            
        except Exception as e:
            print(f"Błąd w {file_name}: {e}")

# 3. Wizualizacja wyników
if not y_true:
    print("Nie znaleziono żadnych danych do przetestowania!")
else:
    accuracy = (np.array(y_true) == np.array(y_pred)).mean()
    print(f"\n--- WYNIK KOŃCOWY ---")
    print(f"Skuteczność: {accuracy * 100:.2f}%")

    fig, ax = plt.subplots(figsize=(10, 8))
    cm = confusion_matrix(y_true, y_pred)
    disp = ConfusionMatrixDisplay(confusion_matrix=cm, display_labels=CATEGORIES)
    disp.plot(cmap=plt.cm.Oranges, ax=ax)
    plt.title(f"Macierz Pomyłek\n(Model: {EXPECTED_ROWS} próbek)")
    plt.show()