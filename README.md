# ESP32 Solar Energy & Environmental Monitoring

ESP32 firmware and USB serial logger for INA219, ADS1115 and DS18B20 modules.

## Hardware wiring

| Module | Module pin | ESP32 |
|---|---|---|
| INA219 | VCC / GND | 3V3 / GND |
| INA219 | SDA / SCL | GPIO21 / GPIO22 |
| ADS1115 | VDD / GND | 3V3 / GND |
| ADS1115 | SDA / SCL | GPIO21 / GPIO22 |
| DS18B20 | Red / Black / Yellow | 3V3 / GND / GPIO4 |

Install a 4.7 kOhm pull-up between DS18B20 DATA (GPIO4) and 3V3. Share GND across all modules. Expected I2C addresses: INA219 0x40, ADS1115 0x48 (ADDR tied to GND). Update firmware constants if address straps differ.

## Electrical safety and INA219

For high-side current sensing, connect source positive to INA219 VIN+, then VIN- to load positive. Source negative and ESP32 GND must be common. Verify solar panel open-circuit voltage and module/shunt ratings before connecting. Never exceed module voltage/current ratings. Firmware uses Adafruit INA219 32V/2A calibration; this must match the actual module shunt/current range.

## Upload firmware

Open firmware/solar_monitor.ino in Arduino IDE. Select correct ESP32 board and serial port. Install Adafruit INA219, Adafruit ADS1X15, OneWire and DallasTemperature libraries. Open Serial Monitor at 115200 baud. At boot, firmware scans I2C and reports each sensor initialization.

## Serial data protocol

Every approximately 2 seconds, firmware emits:

```text
CSV_ROW,Voltage_V,Current_mA,Power_mW,Energy_mWh,Temperature_C,ADC0
```

Unavailable float readings use -999; unavailable ADC uses -1. These sentinels distinguish sensor failure from a genuine zero. Python logger converts sentinels to blank CSV fields but preserves valid zero readings.

## Diagnose missing modules

Flash diagnostics/i2c_scanner.ino if main firmware does not detect INA219/ADS1115. Expected addresses are 0x40 and 0x48. If missing, inspect 3V3, GND, SDA/SCL, soldered headers and address straps. An I2C ACK does not prove the sensor is producing correct measurements.

For DS18B20, check probe polarity, GPIO4 data and the 4.7 kOhm pull-up. A detected probe can still fail temperature conversion; main firmware reports read errors separately.

## Run USB serial CSV logger

Python 3.9+:

```bash
python -m venv .venv
# Windows: .venv\\Scripts\\activate
# Linux/macOS: source .venv/bin/activate
python -m pip install -r requirements.txt
```

With exactly one serial port connected:

```bash
python logger/data_logger.py
```

Or specify a port:

```bash
python logger/data_logger.py --port COM5
python logger/data_logger.py --port /dev/ttyUSB0 --output logs/solar.csv
```

Close Arduino Serial Monitor before launching the logger because only one application can generally own the serial port. Stop with Ctrl+C. Output defaults to solar_realtime_data.csv with UTC timestamps. Diagnostic lines are printed but not written as data rows.

## Parser tests (no hardware required)

```bash
python -m pip install -r requirements.txt
python -m unittest discover -s tests -v
```

Tests cover genuine zero values, failed-sensor sentinels, ignored diagnostic lines and malformed row rejection.

## Hardware validation checklist

- [ ] Compile firmware in Arduino IDE using the libraries above and selected ESP32 board.
- [ ] Confirm I2C scan sees both expected addresses on the actual board.
- [ ] Confirm DS18B20 is found and returns plausible temperature.
- [ ] Compare INA219 against a trusted meter with a known, safe test load.
- [ ] Apply a known ADS1115 input voltage within ADC input limits and confirm response.
- [ ] Run Python logger and inspect generated CSV.

This update is source-code and parser-tested. It has not been physically tested against an ESP32, sensor modules, solar panel or load circuit. Do not claim hardware operation until the checklist is completed.

## Project layout

- firmware/solar_monitor.ino — main ESP32 firmware.
- diagnostics/i2c_scanner.ino — standalone I2C scanner.
- logger/data_logger.py — USB serial CSV logger.
- tests/test_logger.py — parser unit tests.
- requirements.txt — Python dependency.
