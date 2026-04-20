import os
import zipfile
import pandas as pd
import numpy as np
import tensorflow as tf
import matplotlib.pyplot as plt
import seaborn as sns
from sklearn.model_selection import train_test_split
from sklearn.preprocessing import LabelEncoder
from sklearn.metrics import confusion_matrix

# ==========================================
# 1. WYPAKOWANIE DANYCH
# ==========================================
# Zakładamy, że plik nazywa się data.zip i jest w głównym katalogu Colaba
ZIP_FILE_PATH = '/content/sample_data/augmented_data.zip'
if os.path.exists(ZIP_FILE_PATH):
    with zipfile.ZipFile(ZIP_FILE_PATH, 'r') as zip_ref:
        zip_ref.extractall('dataset')
    print("✅ Dane wypakowane pomyślnie!")
else:
    print("❌ BŁĄD: Nie znaleziono pliku augmented_data.zip! Wgraj go do panelu po lewej.")

# ==========================================
# 2. KONFIGURACJA I ŁADOWANIE DANYCH
# ==========================================
# Ścieżka zależy od tego, jak spakowałeś foldery.
# Jeśli w ZIPie był folder 'augmented_data', to ścieżka poniżej jest poprawna:
DATA_PATH = "dataset/augmented_data"
CLASSES = ['L', 'kolo', 'krzyz','tlo']
MAX_SAMPLES = 100  # Ilość próbek na jeden gest (dopasuj do swoich danych)

X = []
y = []

print("🔄 Wczytywanie plików CSV...")
for label in CLASSES:
    class_path = os.path.join(DATA_PATH, label)
    if not os.path.exists(class_path):
        print(f"⚠️ Uwaga: Folder {label} nie istnieje w {DATA_PATH}")
        continue

    files = [f for f in os.listdir(class_path) if f.endswith('.csv')]
    for file in files:
        df = pd.read_csv(os.path.join(class_path, file))
        # Wybieramy osie czujników
        data = df[['AX', 'AY', 'AZ', 'GX', 'GY', 'GZ']].values

        # Standaryzacja długości (Padding/Truncating)
        if len(data) > MAX_SAMPLES:
            data = data[:MAX_SAMPLES]
        else:
            data = np.pad(data, ((0, MAX_SAMPLES - len(data)), (0, 0)), mode='constant')

        X.append(data)
        y.append(label)

X = np.array(X)
y = np.array(y)

# Kodowanie etykiet (Label Encoding -> One-Hot)
encoder = LabelEncoder()
y_encoded = encoder.fit_transform(y)
y_onehot = tf.keras.utils.to_categorical(y_encoded)

# Podział na zbiór treningowy i testowy (80/20)
X_train, X_test, y_train, y_test = train_test_split(X, y_onehot, test_size=0.2, random_state=42)

print(f"✅ Załadowano {len(X)} przykładów.")
print(f"📊 Kształt danych wejściowych (X): {X.shape} (Ilość, Próbki, Osie)")

# ==========================================
# 3. BUDOWA MODELU (POD STM32 / TinyML)
# ==========================================
model = tf.keras.Sequential([
    tf.keras.layers.Input(shape=(MAX_SAMPLES, 6)),

    # 1. Wyłapywanie lokalnych mikro-ruchów (szumy vs. początek ruchu)
    tf.keras.layers.Conv1D(64, 5, padding='same', activation='relu'),
    tf.keras.layers.BatchNormalization(),
    tf.keras.layers.MaxPooling1D(2),

    # 2. Wyłapywanie szerszych zależności (kluczowe dla krzyża i litery L)
    tf.keras.layers.Conv1D(128, 3, padding='same', activation='relu'),
    tf.keras.layers.BatchNormalization(),
    tf.keras.layers.MaxPooling1D(2),

    # 3. Warstwa, która "rozumie" sekwencję (zamiast prostego Flatten)
    tf.keras.layers.Conv1D(64, 3, padding='same', activation='relu'),
    tf.keras.layers.GlobalAveragePooling1D(), 

    # 4. Rozbudowana część decyzyjna
    tf.keras.layers.Dense(64, activation='relu'),
    tf.keras.layers.Dropout(0.3), # Ochrona przed przeuczeniem na tło
    tf.keras.layers.Dense(32, activation='relu'),
    tf.keras.layers.Dense(len(CLASSES), activation='softmax')
])

model.compile(optimizer='adam', loss='categorical_crossentropy', metrics=['accuracy'])
model.summary()

# ==========================================
# 4. TRENOWANIE
# ==========================================
print("🚀 Start trenowania...")
history = model.fit(X_train, y_train, epochs=100, validation_data=(X_test, y_test), batch_size=32, verbose=1)

# ==========================================
# 5. GENEROWANIE WYKRESÓW
# ==========================================
plt.figure(figsize=(15, 5))

# Wykres Accuracy
plt.subplot(1, 3, 1)
plt.plot(history.history['accuracy'], label='Train Acc', color='blue')
plt.plot(history.history['val_accuracy'], label='Val Acc', color='orange')
plt.title('Dokładność (Accuracy)')
plt.legend()
plt.grid(True)

# Wykres Loss
plt.subplot(1, 3, 2)
plt.plot(history.history['loss'], label='Train Loss', color='blue')
plt.plot(history.history['val_loss'], label='Val Loss', color='orange')
plt.title('Strata (Loss)')
plt.legend()
plt.grid(True)

# Macierz Pomyłek (Confusion Matrix)
plt.subplot(1, 3, 3)
y_pred = model.predict(X_test)
cm = confusion_matrix(np.argmax(y_test, axis=1), np.argmax(y_pred, axis=1))
sns.heatmap(cm, annot=True, fmt='d', cmap='Blues', xticklabels=CLASSES, yticklabels=CLASSES)
plt.title('Macierz Pomyłek')
plt.ylabel('Prawdziwe')
plt.xlabel('Przewidziane')

plt.tight_layout()
plt.show()

# ==========================================
# 6. EXPORT DO TFLITE
# ==========================================
print("📦 Konwertowanie modelu do formatu .tflite...")
converter = tf.lite.TFLiteConverter.from_keras_model(model)
tflite_model = converter.convert()

with open('model_gestow.tflite', 'wb') as f:
    f.write(tflite_model)

print("✅ GOTOWE! Możesz pobrać plik 'model_gestow.tflite' z panelu plików.")