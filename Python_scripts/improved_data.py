import os
import pandas as pd
import numpy as np
import glob

# --- KONFIGURACJA ---
# Skrypt bierze stąd: D:\...\cleaned_data\kolo\...
INPUT_DIR = r"D:\Pulpit\STM\Magic\Magic\Python_scripts\cleaned_data"
# Skrypt zapisuje tutaj: D:\...\augmented_data\kolo\...
OUTPUT_DIR = r"D:\Pulpit\STM\Magic\Magic\Python_scripts\augmented_data"

# --- FUNKCJE AUGMENTACJI ---
def apply_jitter(df, sigma=0.02):
    augmented = df.copy()
    for col in ['AX', 'AY', 'AZ', 'GX', 'GY', 'GZ']:
        augmented[col] += np.random.normal(0, sigma, size=len(df))
    return augmented

def apply_scaling(df, range_val=(0.9, 1.1)):
    augmented = df.copy()
    factor = np.random.uniform(range_val[0], range_val[1])
    for col in ['AX', 'AY', 'AZ', 'GX', 'GY', 'GZ']:
        augmented[col] *= factor
    return augmented

def apply_magnitude_shift(df, shift_max=0.05):
    augmented = df.copy()
    for col in ['AX', 'AY', 'AZ', 'GX', 'GY', 'GZ']:
        shift = np.random.uniform(-shift_max, shift_max)
        augmented[col] += shift
    return augmented

def apply_rotation(df, angle_range=(-5, 5)):
    augmented = df.copy()
    angle = np.radians(np.random.uniform(angle_range[0], angle_range[1]))
    cos_a, sin_a = np.cos(angle), np.sin(angle)
    # Rotacja wokół osi Z (mieszamy AX z AY)
    old_ax, old_ay = augmented['AX'].values, augmented['AY'].values
    augmented['AX'] = old_ax * cos_a - old_ay * sin_a
    augmented['AY'] = old_ax * sin_a + old_ay * cos_a
    return augmented

def apply_time_warping(df):
    augmented = df.copy()
    window = np.random.randint(2, 5)
    for col in ['AX', 'AY', 'AZ', 'GX', 'GY', 'GZ']:
        augmented[col] = augmented[col].rolling(window=window, min_periods=1, center=True).mean()
    return augmented

# --- GŁÓWNA PĘTLA ---
def run_augmentation():
    # Szukamy wszystkich plików .csv w podfolderach cleaned_data (L, kolo, krzyz)
    all_files = glob.glob(os.path.join(INPUT_DIR, "**", "*.csv"), recursive=True)
    
    if not all_files:
        print(f"❌ Nie znaleziono plików w {INPUT_DIR}!")
        return

    print(f"🚀 Start. Znaleziono {len(all_files)} plików do przetworzenia...")

    for file_path in all_files:
        # 1. Pobierz nazwę folderu (kolo, krzyz lub L)
        class_name = os.path.basename(os.path.dirname(file_path))
        # 2. Pobierz nazwę pliku bez .csv
        base_filename = os.path.splitext(os.path.basename(file_path))[0]
        
        # 3. Stwórz ścieżkę do zapisu (np. augmented_data/kolo)
        target_dir = os.path.join(OUTPUT_DIR, class_name)
        os.makedirs(target_dir, exist_ok=True)

        # Wczytaj dane
        df = pd.read_csv(file_path)

        # --- GENEROWANIE WARIANTÓW ---
        variants = {
            "orig": df, # oryginał
            "jitter": apply_jitter(df),
            "scale": apply_scaling(df),
            "shift": apply_magnitude_shift(df),
            "rotate": apply_rotation(df),
            "warp": apply_time_warping(df)
        }

        # Zapisz wszystkie 6 wariantów do folderu klasy
        for suffix, data in variants.items():
            new_name = f"{base_filename}_{suffix}.csv"
            save_path = os.path.join(target_dir, new_name)
            data.to_csv(save_path, index=False)

    print("-" * 50)
    print(f"✅ GOTOWE! Dane zapisane w: {OUTPUT_DIR}")
    print(f"📊 Każdy folder (L, kolo, krzyz) zawiera teraz oryginalne i zmodyfikowane pliki CSV.")

if __name__ == "__main__":
    run_augmentation()