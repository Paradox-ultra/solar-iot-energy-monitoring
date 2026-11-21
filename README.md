# IoT Solar Energy & Environmental Monitoring System ☀️📊

A complete Internet of Things (IoT) hardware and software solution to monitor, measure, and record solar panel power generation and environmental conditions in real time.

**Author:** [Rachith Bharadwaj](https://github.com/Rachith000bharadwaj)

---

## 📌 Project Overview
This project continuously measures:
- **Solar Panel Bus Voltage ($V$)**
- **Solar Current Flow ($mA$)**
- **Solar Power Output ($mW$)**
- **Cumulative Solar Energy Generated ($mWh$)**
- **Solar Panel / Ambient Temperature ($^\circ C$)**
- **16-bit Solar Radiation / Light Intensity ($ADC$)**

All telemetry is streamed from an **ESP32** microcontroller over USB Serial and automatically recorded with exact PC timestamps into a real-time **CSV file** on your computer.

---

## 🛠️ Hardware Components
1. **ESP32 Dev Module (30 or 38-Pin)**
2. **INA219 High-Side DC Voltage & Current Sensor Module**
3. **ADS1115 16-Bit I2C ADC Module**
4. **DS18B20 Waterproof Temperature Sensor Probe**
5. **Solar Panel** (DC Voltage up to 26V)
6. **Load Resistor / Power Load**
7. **4.7 kΩ Resistor** (1-Wire Pull-up)
8. **Breadboard & Jumper Wires**

---

## 🔌 Circuit Wiring & Pinout

### 1. Sensor to ESP32 Connections (3.3V Power & I2C Bus)
| Sensor | Sensor Pin | ESP32 Pin | Purpose |
| :--- | :--- | :--- | :--- |
| **INA219** | VCC | 3V3 | Power (3.3V) |
| **INA219** | GND | GND | Ground |
| **INA219** | SDA | GPIO 21 (D21) | I2C Data Line |
| **INA219** | SCL | GPIO 22 (D22) | I2C Clock Line |
| **ADS1115** | VDD | 3V3 | Power (3.3V) |
| **ADS1115** | GND | GND | Ground |
| **ADS1115** | SDA | GPIO 21 (D21) | Shared I2C Data |
| **ADS1115** | SCL | GPIO 22 (D22) | Shared I2C Clock |
| **DS18B20** | Red Wire | 3V3 | Power (3.3V) |
| **DS18B20** | Black Wire | GND | Ground |
| **DS18B20** | Yellow Wire | GPIO 4 (D4) | 1-Wire Data Line |

> **IMPORTANT:** Connect a **4.7 kΩ pull-up resistor** between **DS18B20 Yellow (D4)** and **3V3**.

### 2. Solar Panel & Load Circuit (High-Side Current Sensing)
```
[ SOLAR PANEL ]
  (+) Red Wire   ─────────► INA219 [ Vin+ ] (Screw terminal)
  
                            INA219 [ Vin- ] (Screw terminal)
                               │
                               ▼
                           (+) Positive of Load Resistor
                           (-) Negative of Load Resistor ───┐
                                                            │
  (-) Black Wire ───────────────────────────────────────────┴──► ESP32 GND
```

> **CRITICAL NOTE:** The pin headers on the INA219 and ADS1115 must be **soldered** to the PCB boards to guarantee electrical continuity.

---

## 💻 Software Setup

### 1. Arduino IDE Setup
1. Install **Arduino IDE 2.x**.
2. Go to **File → Preferences** and add the ESP32 Board URL:
   ```text
   https://espressif.github.io/arduino-esp32/package_esp32_index.json
   ```
3. Go to **Tools → Board → Boards Manager**, search for `esp32`, and install `esp32 by Espressif Systems`.
4. Install the following libraries via **Sketch → Include Library → Manage Libraries**:
   - `Adafruit INA219`
   - `Adafruit ADS1X15`
   - `OneWire`
   - `DallasTemperature`
5. Open `firmware/solar_monitor.ino`, select **ESP32 Dev Module** and your **COM Port**, then click **Upload (➡️)**.

### 2. Real-Time CSV Data Logger (Python)
Install requirements:
```bash
pip install -r requirements.txt
```

Run the logger:
```bash
python data_logger.py
```
A live CSV file named `solar_realtime_data.csv` will be created and updated every 2 seconds with timestamps.

---

## 📄 License
This project is licensed under the MIT License.
