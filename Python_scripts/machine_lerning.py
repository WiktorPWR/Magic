import os
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import tensorflow as tf
from sklearn.model_selection import train_test_split
from sklearn.metrics import confusion_matrix, classification_report
import seaborn as sns

# --- KONFIGURACJA ---
DATA_DIR = "D:\\Pulpit\\STM\\Magic\\Magic\\Python_scripts\\augmented_data"
INPUT_COLS = ['AX', 'AY', 'AZ', 'GX', 'GY', 'GZ']
MODEL_NAME = "gestures_model_v1"

def load_data(base_path):
    X_list, y_list = [], []
    for mode in range(3):
        folder_path = os.path.join(base_path, f"mode_{mode}")
        if not os.path.exists(folder_path):
            print(f"Ostrzeżenie: Brak folderu {folder_path}")
            continue
            
        print(f"Wczytywanie: mode_{mode}...")
        for file in os.listdir(folder_path):
            if file.endswith(".csv"):
                df = pd.read_csv(os.path.join(folder_path, file))
                data = df[INPUT_COLS].values.astype(np.float32)
                X_list.append(data)
                y_list.append(np.full(len(data), mode)) # Każdy wiersz dostaje etykietę folderu
    
    return np.vstack(X_list), np.concatenate(y_list)

# 1. Ładowanie danych
X, y = load_data(DATA_DIR)

# 2. Podział na zbiory: 70% trening, 15% walidacja, 15% test
X_train, X_temp, y_train, y_temp = train_test_split(X, y, test_size=0.3, random_state=42, stratify=y)
X_val, X_test, y_val, y_test = train_test_split(X_temp, y_temp, test_size=0.5, random_state=42, stratify=y_temp)

print(f"\nRozmiary zbiorów:\nTrain: {len(X_train)}\nVal: {len(X_val)}\nTest: {len(X_test)}")

# 3. Model - klasyczny MLP (Multi-Layer Perceptron)
model = tf.keras.Sequential([
    tf.keras.layers.Input(shape=(6,)),
    tf.keras.layers.Dense(32, activation='relu'),
    tf.keras.layers.Dropout(0.2), # Zapobiega przeuczeniu
    tf.keras.layers.Dense(16, activation='relu'),
    tf.keras.layers.Dense(3, activation='softmax')
])

model.compile(optimizer='adam', 
              loss='sparse_categorical_crossentropy', 
              metrics=['accuracy'])

# 4. Trening
history = model.fit(
    X_train, y_train,
    epochs=100,
    batch_size=64,
    validation_data=(X_val, y_val),
    verbose=1
)

# 5. Zapisanie modelu w formacie standardowym TF (SavedModel)
model.save(f"{MODEL_NAME}.keras")
print(f"\nModel zapisany jako {MODEL_NAME}.keras")

# --- WIZUALIZACJA ---

def plot_history(history):
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(15, 5))

    # Wykres Straty (Loss)
    ax1.plot(history.history['loss'], label='Train Loss')
    ax1.plot(history.history['val_loss'], label='Val Loss')
    ax1.set_title('Funkcja Straty')
    ax1.set_xlabel('Epoka')
    ax1.legend()

    # Wykres Dokładności (Accuracy)
    ax2.plot(history.history['accuracy'], label='Train Acc')
    ax2.plot(history.history['val_accuracy'], label='Val Acc')
    ax2.set_title('Dokładność Modelu')
    ax2.set_xlabel('Epoka')
    ax2.legend()
    plt.show()

plot_history(history)

# 6. Testowanie na danych, których model nigdy nie widział (X_test)
y_pred = np.argmax(model.predict(X_test), axis=1)

print("\nRaport Klasyfikacji:")
print(classification_report(y_test, y_pred))

# Macierz pomyłek (Confusion Matrix)
plt.figure(figsize=(8, 6))
cm = confusion_matrix(y_test, y_pred)
sns.heatmap(cm, annot=True, fmt='d', cmap='Blues', xticklabels=[0,1,2], yticklabels=[0,1,2])
plt.xlabel('Przewidziane')
plt.ylabel('Prawdziwe')
plt.title('Macierz Pomyłek')
plt.show()