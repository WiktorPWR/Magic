import numpy as np
import tensorflow as tf
import os
import pandas as pd
from sklearn.metrics import classification_report, confusion_matrix
import seaborn as sns
import matplotlib.pyplot as plt

# --- KONFIGURACJA ---
TFLITE_MODEL_PATH = "modele\model_gestow (5).tflite"  # Ścieżka do Twojego pliku
DATA_DIR = "D:\\Pulpit\\STM\\Magic\\Magic\\Python_scripts\\augmented_data"
CLASSES = ['L', 'kolo', 'krzyz', 'tlo']
MAX_SAMPLES = 60
COLS = ['AX', 'AY', 'AZ', 'GX', 'GY', 'GZ']

# 1. ŁADOWANIE INTERPRETERA TFLITE
interpreter = tf.lite.Interpreter(model_path=TFLITE_MODEL_PATH)
interpreter.allocate_tensors()

input_details = interpreter.get_input_details()
output_details = interpreter.get_output_details()

print(f"✅ Model wczytany. Oczekiwany kształt wejścia: {input_details[0]['shape']}")

# 2. FUNKCJA DO TESTOWANIA POJEDYNCZEGO GESTU
def predict_tflite(data_60x6):
    # Dodanie wymiaru batch (1, 60, 6) i konwersja na float32
    input_data = np.expand_dims(data_60x6, axis=0).astype(np.float32)
    
    # Normalizacja (taka sama jak przy trenowaniu!)
    input_data[:, :, 0:3] = input_data[:, :, 0:3] / 2.0
    input_data[:, :, 3:6] = input_data[:, :, 3:6] / 250.0

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
    if not os.path.exists(folder): continue
    
    files = [f for f in os.listdir(folder) if f.endswith('.csv')]
    for file in files:
        df = pd.read_csv(os.path.join(folder, file))
        if len(df) >= MAX_SAMPLES:
            data = df[COLS].values[:MAX_SAMPLES]
            X_test_list.append(data)
            y_true.append(idx)

# 4. URUCHOMIENIE TESTÓW
y_pred = []
for sample in X_test_list:
    class_idx, _ = predict_tflite(sample)
    y_pred.append(class_idx)

# 5. WYNIKI
print("\n" + "="*30)
print("📊 RAPORT KLASYFIKACJI TFLITE")
print("="*30)
print(classification_report(y_true, y_pred, target_names=CLASSES))

# Macierz pomyłek
cm = confusion_matrix(y_true, y_pred)
plt.figure(figsize=(10, 7))
sns.heatmap(cm, annot=True, fmt='d', cmap='Greens', xticklabels=CLASSES, yticklabels=CLASSES)
plt.xlabel('Przewidziane (Model)')
plt.ylabel('Prawdziwe (Dane)')
plt.title('Macierz Pomyłek - Model TFLite')
plt.show()