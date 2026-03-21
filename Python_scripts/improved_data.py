import os
import pandas as pd
import numpy as np
import glob
import shutil

# --- KONFIGURACJA ---
INPUT_DIR = "D:\\Pulpit\\STM\\Magic\\Magic\\Python_scripts\\cleaned_data"
OUTPUT_DIR = "D:\\Pulpit\\STM\\Magic\\Magic\\Python_scripts\\augmented_data"

def apply_jitter(df, sigma=0.02):
    """Dodaje losowy szum biały do każdej osi."""
    augmented = df.copy()
    for col in ['AX', 'AY', 'AZ', 'GX', 'GY', 'GZ']:
        augmented[col] += np.random.normal(0, sigma, size=len(df))
    return augmented

def apply_scaling(df, range=(0.9, 1.1)):
    """Mnoży amplitudę sygnału przez losowy współczynnik."""
    augmented = df.copy()
    factor = np.random.uniform(range[0], range[1])
    for col in ['AX', 'AY', 'AZ', 'GX', 'GY', 'GZ']:
        augmented[col] *= factor
    return augmented

def apply_magnitude_shift(df, shift_max=0.05):
    """Przesuwa cały sygnał w górę lub w dół o stałą wartość."""
    augmented = df.copy()
    for col in ['AX', 'AY', 'AZ', 'GX', 'GY', 'GZ']:
        shift = np.random.uniform(-shift_max, shift_max)
        augmented[col] += shift
    return augmented

def apply_rotation(df, angle_range=(-5, 5)):
    """Symuluje lekkie obrócenie urządzenia (mieszanie osi X i Y)."""
    augmented = df.copy()
    angle = np.radians(np.random.uniform(angle_range[0], angle_range[1]))
    cos_a = np.cos(angle)
    sin_a = np.sin(angle)
    
    # Rotacja wokół osi Z (mieszamy AX z AY)
    old_ax = augmented['AX'].values
    old_ay = augmented['AY'].values
    augmented['AX'] = old_ax * cos_a - old_ay * sin_a
    augmented['AY'] = old_ax * sin_a + old_ay * cos_a
    return augmented

def apply_time_warping(df):
    """Lekko zniekształca czas (przyspiesza/spowalnia fragmenty)."""
    # Używamy prostej metody: lekka zmiana wartości w kolumnach przy zachowaniu czasu
    # Dla sieci neuronowej wygląda to jak szybszy/wolniejszy ruch
    augmented = df.copy()
    # Tutaj stosujemy technice 'rolling mean' o losowym oknie, aby zmienić dynamikę
    window = np.random.randint(2, 5)
    for col in ['AX', 'AY', 'AZ', 'GX', 'GY', 'GZ']:
        augmented[col] = augmented[col].rolling(window=window, min_periods=1, center=True).mean()
    return augmented

def run_augmentation():
    all_files = glob.glob(os.path.join(INPUT_DIR, "mode_*", "*.csv"))
    
    if not all_files:
        print("Brak plików do augmentacji!")
        return

    print(f"Rozpoczynam augmentację dla {len(all_files)} plików...")

    for file_path in all_files:
        # Odczyt danych
        df = pd.read_csv(file_path)
        
        # Ścieżka bazowa (folder mode_X)
        rel_path = os.path.relpath(file_path, INPUT_DIR)
        base_name = os.path.splitext(rel_path)[0]
        mode_dir = os.path.join(OUTPUT_DIR, os.path.dirname(rel_path))
        os.makedirs(mode_dir, exist_ok=True)

        # 1. Zapisz oryginał
        df.to_csv(os.path.join(OUTPUT_DIR, rel_path), index=False)

        # 2. Wykonaj i zapisz augmentacje
        augmentations = {
            "jitter": apply_jitter(df),
            "scale": apply_scaling(df),
            "shift": apply_magnitude_shift(df),
            "rotate": apply_rotation(df),
            "warp": apply_time_warping(df)
        }

        for suffix, aug_df in augmentations.items():
            aug_filename = f"{os.path.basename(base_name)}_{suffix}.csv"
            aug_df.to_csv(os.path.join(mode_dir, aug_filename), index=False)

    print("-" * 50)
    print(f"AUGMENTACJA ZAKOŃCZONA. Dane w: {OUTPUT_DIR}")
    print(f"Liczba plików wzrosła x6.")

if __name__ == "__main__":
    run_augmentation()