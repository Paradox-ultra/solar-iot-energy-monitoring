import serial
import csv
import time
import os
from datetime import datetime

PORT = 'COM16'         # Change to your ESP32 COM Port if different
BAUD = 115200
OUTPUT_CSV = 'solar_realtime_data.csv'

print("==================================================")
print("   REAL-TIME SOLAR DATA LOGGER (ESP32 to CSV)    ")
print("==================================================")
print(f"Connecting to: {PORT} at {BAUD} baud...")
print(f"Saving output to: {os.path.abspath(OUTPUT_CSV)}")

headers = [
    'Timestamp',
    'Solar_Voltage_V',
    'Solar_Current_mA',
    'Solar_Power_mW',
    'Total_Energy_mWh',
    'Panel_Temperature_C',
    'ADC_Light_A0'
]

file_exists = os.path.isfile(OUTPUT_CSV)

try:
    ser = serial.Serial(PORT, BAUD, timeout=2)
    time.sleep(2)
    print("Port connected successfully! Logging data (Press Ctrl+C to stop)...\n")

    with open(OUTPUT_CSV, mode='a', newline='') as f:
        writer = csv.writer(f)
        if not file_exists:
            writer.writerow(headers)
            f.flush()

        while True:
            line = ser.readline().decode('utf-8', errors='ignore').strip()
            if not line:
                continue

            # Process CSV Rows sent by ESP32
            if line.startswith("CSV_ROW,"):
                parts = line.split(",")[1:]
                timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
                row = [timestamp] + parts
                writer.writerow(row)
                f.flush()
                print(f"[{timestamp}] Voltage: {parts[0]}V | Current: {parts[1]}mA | Power: {parts[2]}mW | Temp: {parts[4]}C")
            elif "SYSTEM" in line:
                print(f"{line}")

except KeyboardInterrupt:
    print("\nData logging stopped by user.")
except Exception as e:
    print(f"\nError connecting to serial port: {e}")
finally:
    if 'ser' in locals() and ser.is_open:
        ser.close()
        print("Serial port closed.")
