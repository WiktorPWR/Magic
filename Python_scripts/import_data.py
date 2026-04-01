import serial
import os
import csv
from datetime import datetime

SERIAL_PORT = 'COM5'
BAUD_RATE = 9600
BASE_DIR = "D:\\Pulpit\\STM\\Magic\\Magic\\Python_scripts\\raw_data"

def get_csv_filename():
    return datetime.now().strftime("sesja_%H_%M_%S.csv")

def parse_line(line):
    """
    Parsuje dokładnie:
    A:-58,3,90 | G:-303,357,-165 | M:0 | R:1 | T:5120
    """
    try:
        parts = [p.strip() for p in line.split('|')]

        # A:...
        acc = parts[0].split(':')[1].split(',')
        ax, ay, az = [int(x)/100.0 for x in acc]

        # G:...
        gyro = parts[1].split(':')[1].split(',')
        gx, gy, gz = [int(x)/100.0 for x in gyro]

        # M:...
        mode = int(parts[2].split(':')[1])

        # R:...
        recording = int(parts[3].split(':')[1])

        # T:...
        timestamp = int(parts[4].split(':')[1])

        return ax, ay, az, gx, gy, gz, mode, recording, timestamp

    except Exception:
        return None


def parse_uart_data():
    os.makedirs(BASE_DIR, exist_ok=True)

    current_file = None
    csv_writer = None
    is_recording = False
    mcu_start_time = None
    current_mode = 0

    try:
        with serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1) as ser:
            print(f"Nasłuchiwanie na {SERIAL_PORT}...")

            while True:
                line = ser.readline().decode(errors='ignore').strip()

                if not line:
                    continue

                # ========================
                # ====== START ===========
                # ========================
                if line == "START":
                    if not is_recording:
                        is_recording = True
                        mcu_start_time = None

                        mode_dir = os.path.join(BASE_DIR, f"mode_{current_mode}")
                        os.makedirs(mode_dir, exist_ok=True)

                        file_path = os.path.join(mode_dir, get_csv_filename())
                        current_file = open(file_path, 'w', newline='')
                        csv_writer = csv.writer(current_file)

                        csv_writer.writerow([
                            'MCU_Time_ms',
                            'Relative_Time_ms',
                            'AX', 'AY', 'AZ',
                            'GX', 'GY', 'GZ'
                        ])

                        print(f"\n[START] Nagrywanie -> {file_path}")
                    continue

                # ========================
                # ====== STOP ============
                # ========================
                if line == "STOP":
                    if is_recording:
                        is_recording = False
                        if current_file:
                            current_file.close()
                            current_file = None
                        print(f"\n[STOP] Plik zapisany.")
                    continue

                # ========================
                # ====== DANE ============
                # ========================
                parsed = parse_line(line)
                if not parsed:
                    continue

                ax, ay, az, gx, gy, gz, mode, rec, timestamp = parsed
                current_mode = mode  # zapamiętujemy tryb

                # ustawienie czasu startowego (pierwsza próbka)
                if is_recording and mcu_start_time is None:
                    mcu_start_time = timestamp

                if is_recording and csv_writer:
                    rel_time = timestamp - mcu_start_time
                    csv_writer.writerow([
                        timestamp, rel_time,
                        ax, ay, az,
                        gx, gy, gz
                    ])

                # live podgląd
                print(f"T:{timestamp:6} | M:{mode} | AX:{ax:6.2f}", end='\r')

    except serial.SerialException as e:
        print(f"\nBłąd portu: {e}")
    except KeyboardInterrupt:
        print("\nZatrzymano przez użytkownika.")
    finally:
        if current_file:
            current_file.close()


if __name__ == "__main__":
    parse_uart_data()