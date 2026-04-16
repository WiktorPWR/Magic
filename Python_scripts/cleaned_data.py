import os
import pandas as pd
import glob
import numpy as np

# --- KONFIGURACJA ---
INPUT_DIR = "D:\\Pulpit\\STM\\Magic\\Magic\\Python_scripts\\raw_data"
OUTPUT_DIR = "D:\\Pulpit\\STM\\Magic\\Magic\\Python_scripts\\cleaned_data"

SAMPLES_PER_GESTURE = 60 

MODE_TO_LABEL = {
    "mode_0": "L",
    "mode_1": "kolo",
    "mode_2": "krzyz",
    "mode_3": "tlo"
}

def process_files():
    all_files = glob.glob(os.path.join(INPUT_DIR, "mode_*", "*.csv"))
    
    if not all_files:
        print("Brak plików!")
        return

    # Słownik do tymczasowego przechowywania ramek danych przed balansem
    data_storage = {label: [] for label in MODE_TO_LABEL.values()}

    print("🔄 Krok 1: Wczytywanie i Normalizacja...")
    for file_path in all_files:
        mode_folder = os.path.basename(os.path.dirname(file_path))
        label = MODE_TO_LABEL.get(mode_folder)
        if not label: continue

        df = pd.read_csv(file_path)
        df = df.drop_duplicates(subset=['Relative_Time_ms'])

        # --- NORMALIZACJA FIZYCZNA ---
        # Sprowadzamy dane do podobnej skali (TinyML/STM32 lubi małe floaty)
        if all(col in df.columns for col in ['AX', 'AY', 'AZ', 'GX', 'GY', 'GZ']):
            df[['AX', 'AY', 'AZ']] = df[['AX', 'AY', 'AZ']] / 2.0     # Zakładając +-2g
            df[['GX', 'GY', 'GZ']] = df[['GX', 'GY', 'GZ']] / 250.0   # Zakładając +-250 dps
        
        # Cięcie na kawałki po 60 próbek
        num_chunks = len(df) // SAMPLES_PER_GESTURE
        for i in range(num_chunks):
            start = i * SAMPLES_PER_GESTURE
            chunk = df.iloc[start:start + SAMPLES_PER_GESTURE]
            data_storage[label].append(chunk)

    # --- KROK 2: BALANSOWANIE ---
    print("\n⚖️ Krok 2: Balansowanie klas...")
    # Liczymy ile mamy przykładów w każdej klasie
    counts = {label: len(chunks) for label, chunks in data_storage.items()}
    for label, count in counts.items():
        print(f" - {label}: {count} próbek")

    # Znajdujemy najmniejszą klasę (ale nie mniejszą niż np. 10, żeby nie zepsuć bazy)
    min_samples = max(10, min(counts.values()))
    print(f"\n🎯 Cel: Każda klasa zostanie ograniczona do {min_samples} przykładów.")

    # --- KROK 3: ZAPISYWANIE ---
    processed_total = 0
    for label, chunks in data_storage.items():
        target_dir = os.path.join(OUTPUT_DIR, label)
        os.makedirs(target_dir, exist_ok=True)
        
        # Mieszamy próbki, żeby nie brać tylko tych z początku nagrania
        np.random.shuffle(chunks)
        balanced_chunks = chunks[:min_samples]

        for idx, chunk in enumerate(balanced_chunks):
            chunk.to_csv(os.path.join(target_dir, f"{label}_{idx}.csv"), index=False)
            processed_total += 1

    print("-" * 50)
    print(f"🎯 GOTOWE! Zapisano łącznie {processed_total} zbalansowanych i znormalizowanych plików.")

if __name__ == "__main__":
    process_files()