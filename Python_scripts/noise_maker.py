import os
import pandas as pd
import numpy as np
import glob

# --- KONFIGURACJA ---
BASE_AUGMENTED_DIR = r"D:\Pulpit\STM\Magic\augmented_data\augmented_data"
OUTPUT_DIR = os.path.join(BASE_AUGMENTED_DIR, "tlo")
MAX_SAMPLES = 50  # Wymagane przez model
MIN_STABLE_LEN = 5 # Szukamy stabilnych 5 próbek

# Progi stabilności (różnica max-min w oknie 5 próbek)
ACCEL_THRESHOLD = 0.07 
GYRO_THRESHOLD = 15.0

def extract_micro_noise():
    os.makedirs(OUTPUT_DIR, exist_ok=True)
    all_files = glob.glob(os.path.join(BASE_AUGMENTED_DIR, "**", "*.csv"), recursive=True)
    
    print(f"🔍 Przeszukiwanie {len(all_files)} plików pod kątem mikro-ciszy...")
    
    noise_count = 0
    target_count = 500 # Ile plików 'tlo' chcemy wygenerować

    for file_path in all_files:
        if "tlo" in file_path: continue
        
        df = pd.read_csv(file_path)
        cols = ['AX', 'AY', 'AZ', 'GX', 'GY', 'GZ']
        
        # Przesuwamy okno o rozmiarze 5 próbek
        for i in range(len(df) - MIN_STABLE_LEN):
            window = df.iloc[i : i + MIN_STABLE_LEN]
            
            # Obliczamy stabilność (max - min) dla każdej osi
            diffs = window[cols].max() - window[cols].min()
            
            accel_stable = all(diffs[['AX', 'AY', 'AZ']] < ACCEL_THRESHOLD)
            gyro_stable = all(diffs[['GX', 'GY', 'GZ']] < GYRO_THRESHOLD)
            
            if accel_stable and gyro_stable:
                # Mamy stabilne 5 próbek! Teraz budujemy z nich 50 próbek dla modelu.
                # Powielamy te 5 próbek 10 razy, dodając mikro-szum, żeby nie były identyczne
                base_data = window[cols].values
                synthetic_50 = np.tile(base_data, (10, 1)) 
                
                # Dodajemy jitter, aby model nie uczył się idealnych powtórzeń
                noise = np.random.normal(0, 0.005, synthetic_50.shape)
                synthetic_50 += noise
                
                # Zapisujemy
                new_df = pd.DataFrame(synthetic_50, columns=cols)
                
                # Dodajemy czas (72ms krok)
                new_df.insert(0, 'Time_ms', np.arange(MAX_SAMPLES) * 72.0)
                
                save_path = os.path.join(OUTPUT_DIR, f"noise_gen_{noise_count}.csv")
                new_df.to_csv(save_path, index=False)
                
                noise_count += 1
                break # Idziemy do następnego pliku źródłowego, żeby dane były różnorodne

        if noise_count >= target_count:
            break

    print("-" * 50)
    print(f"✅ Sukces! Wygenerowano {noise_count} plików klasy 'tlo'.")
    print(f"📊 Teraz Twój model rozpozna spoczynek nawet po krótkim fragmencie stabilizacji.")

if __name__ == "__main__":
    extract_micro_noise()