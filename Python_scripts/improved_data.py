import os
import pandas as pd
import numpy as np
import glob

# --- KONFIGURACJA ---
INPUT_DIR = r"D:\Pulpit\STM\Magic\Magic\Python_scripts\cleaned_data"
OUTPUT_DIR = r"D:\Pulpit\STM\Magic\Magic\Python_scripts\augmented_data"
CLASSES = ["L", "kolo", "krzyz", "tlo"]

# ILE LOSOWYCH KĄTÓW DODAĆ DO KAŻDEGO PLIKU?
NUM_RANDOM_ANGLES = 10

# --- FUNKCJE TRANSFORMACJI ---

def apply_3d_rotation(df):
    """Obraca dane o losowy kąt w osiach X i Y (symulacja pozycji ciała)"""
    augmented = df.copy()
    # Losujemy kąty dla obu osi (pełen zakres, żeby model był pancerny)
    angle_x = np.radians(np.random.uniform(-90, 90))
    angle_y = np.radians(np.random.uniform(-90, 90))
    
    cx, sx = np.cos(angle_x), np.sin(angle_x)
    cy, sy = np.cos(angle_y), np.sin(angle_y)

    for p in ['A', 'G']:
        x, y, z = augmented[f'{p}X'].values, augmented[f'{p}Y'].values, augmented[f'{p}Z'].values
        
        # Obrót wokół X
        new_y = y * cx - z * sx
        new_z = y * sx + z * cx
        # Obrót wokół Y
        new_x = x * cy + new_z * sy
        final_z = -x * sy + new_z * cy
        
        augmented[f'{p}X'], augmented[f'{p}Y'], augmented[f'{p}Z'] = new_x, new_y, final_z
    return augmented

def apply_jitter(df, sigma=0.01):
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

def apply_time_warping(df):
    augmented = df.copy()
    window = np.random.randint(2, 4)
    for col in ['AX', 'AY', 'AZ', 'GX', 'GY', 'GZ']:
        augmented[col] = augmented[col].rolling(window=window, min_periods=1, center=True).mean()
    return augmented

# --- GŁÓWNA LOGIKA ---

def run_augmentation():
    all_files = glob.glob(os.path.join(INPUT_DIR, "**", "*.csv"), recursive=True)
    if not all_files:
        print("❌ Brak plików!")
        return

    print(f"🚀 Start. Oryginalnych plików: {len(all_files)}")
    generated_total = 0

    for file_path in all_files:
        class_name = os.path.basename(os.path.dirname(file_path))
        if class_name not in CLASSES: continue

        base_name = os.path.splitext(os.path.basename(file_path))[0]
        target_dir = os.path.join(OUTPUT_DIR, class_name)
        os.makedirs(target_dir, exist_ok=True)

        try:
            df_orig = pd.read_csv(file_path)
            
            # --- ETAP 1: GENEROWANIE BAZOWYCH POZYCJI (KĄTY) ---
            base_variants = [("orig", df_orig)]
            for i in range(NUM_RANDOM_ANGLES):
                rotated_df = apply_3d_rotation(df_orig)
                base_variants.append((f"rot{i}", rotated_df))

            # --- ETAP 2: AUGMENTACJA KAŻDEJ POZYCJI ---
            for pos_name, pos_df in base_variants:
                # Definiujemy zestaw końcowych wariantów dla danej pozycji
                final_variants = {
                    f"{pos_name}_raw": pos_df,
                    f"{pos_name}_jit": apply_jitter(pos_df),
                    f"{pos_name}_sc": apply_scaling(pos_df),
                    f"{pos_name}_warp": apply_time_warping(pos_df)
                }

                for suffix, final_data in final_variants.items():
                    save_name = f"{base_name}_{suffix}.csv"
                    final_data.to_csv(os.path.join(target_dir, save_name), index=False)
                    generated_total += 1

            print(f"✅ Przetworzono: {base_name} ({len(base_variants) * 4} plików)", end='\r')

        except Exception as e:
            print(f"\n❌ Błąd: {file_path} -> {e}")

    print(f"\n\n📊 STATYSTYKI:")
    print(f"PLIKI WEJŚCIOWE: {len(all_files)}")
    print(f"PLIKI WYGENEROWANE: {generated_total}")
    print(f"MNOŻNIK: x{generated_total / len(all_files):.1f}")

if __name__ == "__main__":
    run_augmentation()