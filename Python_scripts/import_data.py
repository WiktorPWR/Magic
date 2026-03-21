import serial
import struct
import os
import csv
from datetime import datetime

# --- KONFIGURACJA DOPASOWANA DO NOWEGO KODU C ---
SERIAL_PORT = 'COM3' 
BAUD_RATE = 115200
# Rozmiar: 6*4b (floats) + 4b (uint32 timestamp) + 1b (mode) + 1b (recording) = 30 bajtów
DATA_SIZE = 30  
BASE_DIR = "D:\\Pulpit\\STM\\Magic\\Magic\\Python_scripts\\data"

def get_csv_filename():
    """Generuje nazwę pliku sesji."""
    return datetime.now().strftime("sesja_%H_%M_%S.csv")

def parse_uart_data():
    if not os.path.exists(BASE_DIR):
        os.makedirs(BASE_DIR, exist_ok=True)

    current_file = None
    csv_writer = None
    is_recording = False
    mcu_start_time = 0  # Tu zapiszemy czas z STM32 w momencie kliknięcia przycisku

    try:
        with serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1) as ser:
            print(f"Nasłuchiwanie na {SERIAL_PORT} (Paczka: {DATA_SIZE} bajtów)...")

            while True:
                raw_data = ser.read(DATA_SIZE)
                
                if len(raw_data) == DATA_SIZE:
                    # ROZPAKOWANIE ZGODNE Z TWOJĄ FUNKCJĄ C:
                    # < (little-endian)
                    # ffffff (6 floatów: Acc X,Y,Z, Gyro X,Y,Z)
                    # L (unsigned long / uint32: timestamp)
                    # B (unsigned char / uint8: mode)
                    # B (unsigned char / uint8: recording)
                    unpacked = struct.unpack('<ffffffLBB', raw_data)
                    
                    ax, ay, az = unpacked[0:3]
                    gx, gy, gz = unpacked[3:6]
                    mcu_timestamp = unpacked[6]  # Czas w ms z HAL_GetTick()
                    mode = unpacked[7]
                    recording_signal = unpacked[8]

                    # LOGIKA NAGRYWANIA
                    if recording_signal == 1:
                        if not is_recording:
                            # START SESJI
                            is_recording = True
                            mcu_start_time = mcu_timestamp  # Zapamiętujemy moment startu
                            
                            mode_dir = os.path.join(BASE_DIR, f"mode_{mode}")
                            os.makedirs(mode_dir, exist_ok=True)
                            
                            file_path = os.path.join(mode_dir, get_csv_filename())
                            current_file = open(file_path, 'w', newline='')
                            csv_writer = csv.writer(current_file)
                            
                            # Nagłówki kolumn
                            csv_writer.writerow(['MCU_Time_ms', 'Relative_Time_ms', 'AX', 'AY', 'AZ', 'GX', 'GY', 'GZ'])
                            print(f"\n[RECORDING START] Mode: {mode} -> {file_path}")

                        # Obliczamy czas od początku gestu (bardzo ważne dla AI)
                        relative_time = mcu_timestamp - mcu_start_time
                        
                        # Zapis danych
                        csv_writer.writerow([mcu_timestamp, relative_time, ax, ay, az, gx, gy, gz])
                    
                    else:
                        # KONIEC SESJI
                        if is_recording:
                            is_recording = False
                            if current_file:
                                current_file.close()
                                current_file = None
                            print(f"\n[RECORDING STOP] Plik zapisany.")

                    # Podgląd na żywo w konsoli (odświeżana linia)
                    print(f"Time: {mcu_timestamp:8}ms | Mode: {mode} | Rec: {recording_signal} | AccX: {ax:>6.2f}", end='\r')

    except serial.SerialException as e:
        print(f"\nBłąd portu: {e}")
    except KeyboardInterrupt:
        print("\nZatrzymano przez użytkownika.")
    finally:
        if current_file:
            current_file.close()

if __name__ == "__main__":
    parse_uart_data()