import os
import pandas as pd
import numpy as np
import glob

# --- KONFIGURACJA ---
INPUT_DIR = "D:\\Pulpit\\STM\\Magic\\Magic\\Python_scripts\\raw_data"
OUTPUT_DIR = "D:\\Pulpit\\STM\\Magic\\Magic\\Python_scripts\\cleaned_data"

def clean_and_interpolate_data():
    # 1. Pobranie listy wszystkich plików CSV w strukturze folderów
    all_files = glob.glob(os.path.join(INPUT_DIR, "mode_*", "*.csv"))
    
    if not all_files:
        print("Nie znaleziono żadnych plików CSV!")
        return

    print(f"Znaleziono {len(all_files)} plików do przetworzenia.")

    # 2. Wyznaczenie wzorcowego interwału (Delta T) na podstawie pierwszego pliku
    first_df = pd.read_csv(all_files[0])
    # Obliczamy różnice między kolejnymi wierszami w kolumnie Relative_Time_ms
    time_diffs = first_df['Relative_Time_ms'].diff().dropna()
    target_dt = time_diffs.mean()
    
    print(f"Wyznaczony wzorcowy interwał (target_dt): {target_dt:.2f} ms")
    print("-" * 50)

    for file_path in all_files:
        # Pobranie nazwy folderu (mode_X) i nazwy pliku
        relative_path = os.path.relpath(file_path, INPUT_DIR)
        target_file_path = os.path.join(OUTPUT_DIR, relative_path)
        
        # Tworzenie folderu docelowego, jeśli nie istnieje
        os.makedirs(os.path.dirname(target_file_path), exist_ok=True)

        # 3. Wczytanie danych
        df = pd.read_csv(file_path)
        
        # Usuwamy ewentualne duplikaty w czasie (jeśli MCU wysłał dwie próbki z tym samym ms)
        df = df.drop_duplicates(subset=['Relative_Time_ms'])

        # Pobieramy surowe dane czasowe i wartości sensorów
        old_time = df['Relative_Time_ms'].values
        columns_to_interpolate = ['AX', 'AY', 'AZ', 'GX', 'GY', 'GZ']
        
        # 4. Tworzenie nowej "idealnej" siatki czasu
        # Startujemy od 0, kończymy na ostatniej zarejestrowanej próbce, skok co target_dt
        new_time = np.arange(old_time[0], old_time[-1], target_dt)

        # Tworzymy nowy DataFrame dla przeliczonych danych
        new_df = pd.DataFrame({'Relative_Time_ms': new_time})

        # 5. Interpolacja dla każdej osi z osobna
        for col in columns_to_interpolate:
            # np.interp(nowe_punkty, stare_punkty, stare_wartosci)
            new_df[col] = np.interp(new_time, old_time, df[col].values)

        # Zaokrąglamy wartości dla czystości zapisu (np. do 4 miejsc po przecinku)
        new_df = new_df.round(4)

        # 6. Zapis do nowego pliku
        new_df.to_csv(target_file_path, index=False)
        print(f"Przetworzono: {relative_path} | Próbek: {len(old_time)} -> {len(new_time)}")

    print("-" * 50)
    print(f"PROCES ZAKOŃCZONY. Dane zapisane w: {OUTPUT_DIR}")

if __name__ == "__main__":
    clean_and_interpolate_data()