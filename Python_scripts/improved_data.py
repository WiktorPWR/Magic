import os
import pandas as pd
import numpy as np
import glob

# --- KONFIGURACJA ---
INPUT_DIR = r"D:\Pulpit\STM\Magic\Magic\Python_scripts\cleaned_data"
OUTPUT_DIR = r"D:\Pulpit\STM\Magic\Magic\Python_scripts\augmented_data"

# Definicja klas (zgodnie z Twoim wcześniejszym schematem)
CLASSES = ["L", "kolo", "krzyz", "tlo"]

# --- FUNKCJE AUGMENTACJI ---
def apply_jitter(df, sigma=0.015): # Zmniejszyłem nieco sigmę dla realizmu
    augmented = df.copy()
    for col in ['AX', 'AY', 'AZ', 'GX', 'GY', 'GZ']:
        augmented[col] += np.random.normal(0, sigma, size=len(df))
    return augmented

def apply_scaling(df, range_val=(0.85, 1.15)):
    augmented = df.copy()
    factor = np.random.uniform(range_val[0], range_val[1])
    for col in ['AX', 'AY', 'AZ', 'GX', 'GY', 'GZ']:
        augmented[col] *= factor
    return augmented

def apply_magnitude_shift(df, shift_max=0.04):
    augmented = df.copy()
    for col in ['AX', 'AY', 'AZ', 'GX', 'GY', 'GZ']:
        shift = np.random.uniform(-shift_max, shift_max)
        augmented[col] += shift
    return augmented

def apply_rotation(df, angle_range=(-10, 10)):
    augmented = df.copy()
    angle = np.radians(np.random.uniform(angle_range[0], angle_range[1]))
    cos_a, sin_a = np.cos(angle), np.sin(angle)
    # Rotacja wokół osi Z (mieszamy AX z AY oraz GX z GY)
    for prefix in ['A', 'G']:
        ox, oy = augmented[f'{prefix}X'].values, augmented[f'{prefix}Y'].values
        augmented[f'{prefix}X'] = ox * cos_a - oy * sin_a
        augmented[f'{prefix}Y'] = ox * sin_a + oy * cos_a
    return augmented

def apply_time_warping(df):
    augmented = df.copy()
    # Lekkie wygładzenie (imitacja wolniejszego/szybszego ruchu)
    window = np.random.randint(2, 4)
    for col in ['AX', 'AY', 'AZ', 'GX', 'GY', 'GZ']:
        augmented[col] = augmented[col].rolling(window=window, min_periods=1, center=True).mean()
    return augmented

# --- GŁÓWNA PĘTLA ---
def run_augmentation():
    # Szukamy we wszystkich podfolderach (L, kolo, krzyz, tlo)
    all_files = glob.glob(os.path.join(INPUT_DIR, "**", "*.csv"), recursive=True)
    
    if not all_files:
        print(f"❌ Nie znaleziono plików w {INPUT_DIR}!")
        print("Upewnij się, że w folderze znajdują się podfoldery: L, kolo, krzyz, tlo")
        return

    print(f"🚀 Start. Znaleziono {len(all_files)} plików (Gesty + Tło)...")

    processed_count = 0

    for file_path in all_files:
        class_name = os.path.basename(os.path.dirname(file_path))
        
        # Sprawdzenie czy klasa jest na liście dopuszczalnych
        if class_name not in CLASSES:
            continue

        base_filename = os.path.splitext(os.path.basename(file_path))[0]
        target_dir = os.path.join(OUTPUT_DIR, class_name)
        os.makedirs(target_dir, exist_ok=True)

        try:
            df = pd.read_csv(file_path)
            
            # Generowanie wariantów
            # Dla 'tlo' (background) augmentacja jest równie ważna, 
            # by model nie nauczył się tła "na pamięć"
            variants = {
                "orig": df,
                "jitter": apply_jitter(df),
                "scale": apply_scaling(df),
                "shift": apply_magnitude_shift(df),
                "rotate": apply_rotation(df),
                "warp": apply_time_warping(df)
            }

            for suffix, data in variants.items():
                new_name = f"{base_filename}_{suffix}.csv"
                save_path = os.path.join(target_dir, new_name)
                data.to_csv(save_path, index=False)
            
            processed_count += 1
            print(f"✅ Przetworzono [{class_name}]: {base_filename}", end='\r')

        except Exception as e:
            print(f"\n❌ Błąd przy pliku {file_path}: {e}")

    print("\n" + "-" * 50)
    print(f"✅ GOTOWE! Dane zapisane w: {OUTPUT_DIR}")
    print(f"📊 Przetworzono oryginalnych plików: {processed_count}")
    print(f"📈 Wygenerowano łącznie plików: {processed_count * 6}")

if __name__ == "__main__":
    run_augmentation()