import numpy as np
import tensorflow as tf
import os
import pandas as pd
from sklearn.metrics import classification_report, confusion_matrix
import seaborn as sns
import matplotlib.pyplot as plt

# --- KONFIGURACJA ---
TFLITE_MODEL_PATH = r"modele\model_gestow (6).tflite" 
DATA_DIR = r"D:\Pulpit\STM\Magic\Magic\Python_scripts\augmented_data"
CLASSES = ['L', 'kolo', 'krzyz', 'tlo']
# ZMIANA: Teraz oczekujemy 100 próbek (wierszy) na gest
MAX_SAMPLES = 100 
COLS = ['AX', 'AY', 'AZ', 'GX', 'GY', 'GZ']

# 1. ŁADOWANIE INTERPRETERA TFLITE
interpreter = tf.lite.Interpreter(model_path=TFLITE_MODEL_PATH)
interpreter.allocate_tensors()

input_details = interpreter.get_input_details()
output_details = interpreter.get_output_details()

print(f"✅ Model wczytany. Oczekiwany kształt wejścia: {input_details[0]['shape']}")


# 2. FUNKCJA DO TESTOWANIA POJEDYNCZEGO GESTU
def predict_tflite(data_100x6):
    # Dodanie wymiaru batch (1, 100, 6) i konwersja na float32
    # Skoro dane są już znormalizowane, po prostu je przygotowujemy pod model
    input_data = np.expand_dims(data_100x6, axis=0).astype(np.float32)
    
    # --- USUNIĘTO NORMALIZACJĘ (dane są już gotowe) ---

    interpreter.set_tensor(input_details[0]['index'], input_data)
    interpreter.invoke()
    
    output_data = interpreter.get_tensor(output_details[0]['index'])
    return np.argmax(output_data), output_data[0]

# 3. ZBIERANIE DANYCH TESTOWYCH
X_test_list = []
y_true = []

print("🔄 Wczytywanie danych do testu...")
for idx, label in enumerate(CLASSES):
    folder = os.path.join(DATA_DIR, label)
    if not os.path.exists(folder): 
        print(f"⚠️ Brak folderu: {folder}")
        continue
    
    files = [f for f in os.listdir(folder) if f.endswith('.csv')]
    for file in files:
        df = pd.read_csv(os.path.join(folder, file))
        
        # ZMIANA: Sprawdzamy czy plik ma co najmniej 100 wierszy
        if len(df) >= MAX_SAMPLES:
            data = df[COLS].values[:MAX_SAMPLES] # Pobieramy dokładnie pierwsze 100 wierszy
            X_test_list.append(data)
            y_true.append(idx)
        else:
            # Opcjonalnie: info o za krótkich plikach
            # print(f"Pomijam {file} - zbyt mało danych ({len(df)})")
            pass

# 4. URUCHOMIENIE TESTÓW
y_pred = []
if len(X_test_list) == 0:
    print("❌ BŁĄD: Nie znaleziono żadnych próbek o długości 100!")
else:
    for sample in X_test_list:
        class_idx, _ = predict_tflite(sample)
        y_pred.append(class_idx)

    # 5. WYNIKI
    print("\n" + "="*30)
    print("📊 RAPORT KLASYFIKACJI TFLITE (100x6)")
    print("="*30)
    print(classification_report(y_true, y_pred, target_names=CLASSES))

    # Macierz pomyłek
    cm = confusion_matrix(y_true, y_pred)
    plt.figure(figsize=(10, 7))
    sns.heatmap(cm, annot=True, fmt='d', cmap='Greens', xticklabels=CLASSES, yticklabels=CLASSES)
    plt.xlabel('Przewidziane (Model)')
    plt.ylabel('Prawdziwe (Dane)')
    plt.title('Macierz Pomyłek - Model TFLite (100x6)')
    plt.show()