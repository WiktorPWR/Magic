# Magic Gesture Recognition: CNN + STM32 + MPU6050

A complete Edge AI pipeline for recognizing hand gestures (L-shape, Circle, Cross, and Background) using a 3-axis accelerometer and gyroscope. The project covers real-time data acquisition, heavy data augmentation for model robustness, and on-device inference logic.

## 📋 Table of Contents
* [Overview](#overview)
* [Hardware Setup](#hardware-setup)
* [Data Acquisition & Sampling](#data-acquisition--sampling)
* [Data Augmentation (The "Bulletproof" Model)](#data-augmentation)
* [CNN Architecture](#cnn-architecture)
* [Real-Time Inference Logic](#real-time-inference-logic)

---

## 🚀 Overview
This project uses an **STM32F411** to capture motion data from an **MPU6050** IMU. The data is processed by a **Convolutional Neural Network (CNN)** to identify specific patterns in 3D space. To make the model resistant to how a user holds the device, a custom 3D rotation augmentation script was implemented.

## ✍️ Recognized Symbols
1.  **L**: Uppercase letter L.
2.  **Circle**: Circular motion.
3.  **Cross (+)**: Vertical and horizontal strokes.
4.  **Background**: Idle state or random noise.

---

## 🏗 Data Acquisition & Sampling
The core of the system relies on a consistent sampling strategy:
* **Sample Window:** Each gesture is captured over a **2-second** window.
* **Sampling Rate:** 100 measurements per window (one sample every **20ms**).
* **Input Shape:** Each sample is a matrix of `(100, 6)`, representing 3 axes of acceleration ($A_x, A_y, A_z$) and 3 axes of gyroscope data ($G_x, G_y, G_z$).

---

## 🔄 Data Augmentation
To ensure the model works regardless of how the sensor is tilted, the dataset was expanded using a custom Python script.
* **3D Rotation:** Data is rotated by random angles ($\pm 90^\circ$) on X and Y axes to simulate different hand orientations.
* **Jittering:** Adding Gaussian noise to simulate sensor inaccuracy.
* **Scaling:** Randomly scaling the magnitude of the signal to account for different movement speeds.
* **Time Warping:** Using rolling means to simulate slight changes in movement duration.

> **Result:** The original dataset was multiplied by **x40** (10 random angles × 4 variants each).

---

## 🧠 CNN Architecture
The model is designed to capture both micro-movements and global patterns:

```python
model = tf.keras.Sequential([
    tf.keras.layers.Input(shape=(MAX_SAMPLES, 6)),
    
    # 1. Capture local micro-movements
    tf.keras.layers.Conv1D(64, 5, padding='same', activation='relu'),
    tf.keras.layers.BatchNormalization(),
    tf.keras.layers.MaxPooling1D(2),

    # 2. Capture broader dependencies (key for Cross and L-shape)
    tf.keras.layers.Conv1D(128, 3, padding='same', activation='relu'),
    tf.keras.layers.BatchNormalization(),
    tf.keras.layers.MaxPooling1D(2),

    # 3. Sequence understanding
    tf.keras.layers.Conv1D(64, 3, padding='same', activation='relu'),
    tf.keras.layers.GlobalAveragePooling1D(),

    # 4. Decision Head
    tf.keras.layers.Dense(64, activation='relu'),
    tf.keras.layers.Dropout(0.3),
    tf.keras.layers.Dense(32, activation='relu'),
    tf.keras.layers.Dense(len(CLASSES), activation='softmax')
])






