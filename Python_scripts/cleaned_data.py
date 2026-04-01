import os
import pandas as pd
import numpy as np
import glob

# --- KONFIGURACJA ---
INPUT_DIR = "D:\\Pulpit\\STM\\Magic\\Magic\\Python_scripts\\raw_data"
OUTPUT_DIR = "D:\\Pulpit\\STM\\Magic\\Magic\\Python_scripts\\cleaned_data"

MIN_SAMPLES = 10  # filtr śmieci

# 🔥 MAPOWANIE folderów
MODE_TO_LABEL = {
    "mode_0": "L",
    "mode_1": "kolo",
    "mode_2": "krzyz"
}


def compute_global_dt(all_files):
    all_diffs = []

    for file_path in all_files:
        df = pd.read_csv(file_path)

        df = df.sort_values(by='Relative_Time_ms')
        df = df.drop_duplicates(subset=['Relative_Time_ms'])

        if len(df) < MIN_SAMPLES:
            continue

        time_vals = df['Relative_Time_ms'].values
        diffs = np.diff(time_vals)

        if len(diffs) == 0:
            continue

        local_median = np.median(diffs)

        # usuwamy outliery (np. dropy pakietów)
        diffs = diffs[(diffs > 0) & (diffs < 3 * local_median)]

        all_diffs.extend(diffs)

    if not all_diffs:
        raise ValueError("Brak danych do wyliczenia dt!")

    return np.median(all_diffs)


def process_files():
    all_files = glob.glob(os.path.join(INPUT_DIR, "mode_*", "*.csv"))

    if not all_files:
        print("Brak plików!")
        return

    print(f"Znaleziono {len(all_files)} plików")

    # 🔥 GLOBALNY dt
    target_dt = compute_global_dt(all_files)

    print(f"🔥 GLOBALNY dt: {target_dt:.3f} ms")
    print("-" * 50)

    columns = ['AX', 'AY', 'AZ', 'GX', 'GY', 'GZ']

    processed = 0
    skipped = 0

    for file_path in all_files:

        # 🔥 wyciągamy folder mode_X
        mode_folder = os.path.basename(os.path.dirname(file_path))

        # 🔥 mapowanie na label
        target_label = MODE_TO_LABEL.get(mode_folder)

        if target_label is None:
            print(f"❌ Nieznany folder: {mode_folder}")
            skipped += 1
            continue

        # 🔥 nazwa pliku (z prefixem żeby uniknąć kolizji)
        file_name = f"{mode_folder}_{os.path.basename(file_path)}"

        # 🔥 katalog docelowy
        target_dir = os.path.join(OUTPUT_DIR, target_label)
        os.makedirs(target_dir, exist_ok=True)

        target_file_path = os.path.join(target_dir, file_name)

        # --- wczytanie ---
        df = pd.read_csv(file_path)

        df = df.sort_values(by='Relative_Time_ms')
        df = df.drop_duplicates(subset=['Relative_Time_ms'])

        if len(df) < MIN_SAMPLES:
            print(f"❌ SKIP (za mało próbek): {file_path}")
            skipped += 1
            continue

        old_time = df['Relative_Time_ms'].values

        # 🔥 liczba próbek po interpolacji
        num_samples = int(np.floor((old_time[-1] - old_time[0]) / target_dt))

        if num_samples < MIN_SAMPLES:
            print(f"❌ SKIP (za krótki sygnał): {file_path}")
            skipped += 1
            continue

        # 🔥 nowa siatka czasu
        new_time = old_time[0] + np.arange(num_samples) * target_dt

        new_df = pd.DataFrame()

        # 🔥 nowy, równy czas (ms)
        new_df['Time_ms'] = new_time - new_time[0]

        # interpolacja
        for col in columns:
            new_df[col] = np.interp(new_time, old_time, df[col].values)

        new_df = new_df.round(4)

        # zapis
        new_df.to_csv(target_file_path, index=False)

        processed += 1
        print(f"✅ {file_name} → {num_samples} próbek")

    print("-" * 50)
    print(f"✔ Przetworzono: {processed}")
    print(f"❌ Pominięto: {skipped}")
    print("🎯 GOTOWE")


if __name__ == "__main__":
    process_files()