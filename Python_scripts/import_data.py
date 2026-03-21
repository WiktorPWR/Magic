import serial
import struct
import os
import csv
import time  # Dodano do precyzyjnego mierzenia czasu trwania sesji
from datetime import datetime

# --- KONFIGURACJA ---
SERIAL_PORT = 'COM3' 
BAUD_RATE = 115200
DATA_SIZE = 26  # 6 * float (4b) + 2 * uint8 (1b)
BASE_DIR = "D:\\Pulpit\\STM\\Magic\\Magic\\Python_scripts\\data"

def get_csv_filename():
    """Generuje unikalną nazwę pliku na podstawie aktualnej godziny."""
    return datetime.now().strftime("sesja_%H_%M_%S.csv")

def parse_uart_data():
    if not os.path.exists(BASE_DIR):
        os.makedirs(BASE_DIR)

    current_file = None
    csv_writer = None
    is_recording = False
    start_time = 0  # Punkt odniesienia dla relatywnego czasu

    try:
        with serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1) as ser:
            print(f"Nasłuchiwanie na {SERIAL_PORT}...")

            while True:
                raw_data = ser.read(DATA_SIZE)
                
                if len(raw_data) == DATA_SIZE:
                    # Rozpakowanie danych (6f = floats, BB = 2x uint8)
                    unpacked = struct.unpack('<ffffffBB', raw_data)
                    
                    ax, ay, az = unpacked[0:3]
                    gx, gy, gz = unpacked[3:6]
                    mode = unpacked[6]
                    recording_signal = unpacked[7]

                    # Logika nagrywania
                    if recording_signal == 1:
                        if not is_recording:
                            # START NOWEGO NAGRANIA
                            is_recording = True
                            start_time = time.time()  # Zerujemy czas dla tej sesji
                            
                            mode_dir = os.path.join(BASE_DIR, f"mode_{mode}")
                            if not os.path.exists(mode_dir):
                                os.makedirs(mode_dir)
                            
                            file_path = os.path.join(mode_dir, get_csv_filename())
                            current_file = open(file_path, 'w', newline='')
                            csv_writer = csv.writer(current_file)
                            
                            # Dodano 'Relative_Time' do nagłówka
                            csv_writer.writerow(['Timestamp', 'Relative_Time', 'AX', 'AY', 'AZ', 'GX', 'GY', 'GZ'])
                            print(f"\n[START] Zapis: {file_path}")

                        # Obliczanie czasu od rozpoczęcia nagrania (sekundy)
                        elapsed_time = time.time() - start_time
                        
                        # Precyzyjny znacznik daty/godziny
                        timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]
                        
                        # Zapis wiersza z uwzględnieniem czasu relatywnego (format: 4 miejsca po przecinku)
                        csv_writer.writerow([timestamp, f"{elapsed_time:.4f}", ax, ay, az, gx, gy, gz])
                    
                    else:
                        # KONIEC NAGRANIA
                        if is_recording:
                            is_recording = False
                            current_file.close()
                            current_file = None # Reset referencji
                            print("\n[STOP] Zakończono zapis pliku.")

                    # Podgląd w konsoli
                    print(f"Mode:{mode} | Rec:{recording_signal} | Acc: {ax:>6.2f} {ay:>6.2f} {az:>6.2f}", end='\r')

    except serial.SerialException as e:
        print(f"\nBłąd portu: {e}")
    except KeyboardInterrupt:
        print("\nZatrzymano przez użytkownika.")
    finally:
        if current_file:
            current_file.close()

if __name__ == "__main__":
    parse_uart_data()