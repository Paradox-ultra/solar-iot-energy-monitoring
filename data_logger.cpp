/*
 ============================================================================
  PROJECT: IoT Solar Energy Monitoring & Logging System
  SENSORS: 
    - INA219 (Solar Panel Voltage, Current & Power)
    - ADS1115 (16-bit Precision ADC for light / analog channels)
    - DS18B20 (Solar Panel / Ambient Temperature)
 ============================================================================
*/

#include <Wire.h>
#include <Adafruit_INA219.h>
#include <Adafruit_ADS1X15.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// I2C Pins for ESP32
#define SDA_PIN 21
#define SCL_PIN 22

// DS18B20 1-Wire Pin
#define ONE_WIRE_BUS 4

Adafruit_INA219 ina219;
Adafruit_ADS1115 ads;
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

bool ina_ok = false;
bool ads_ok = false;
bool ds_ok  = false;

// Energy Accumulation (mWh)
float total_energy_mwh = 0.0;
unsigned long last_time = 0;

void setup() {
  Serial.begin(115200);
  delay(1500);

  Serial.println("\n========================================================");
  Serial.println("     SOLAR ENERGY & ENVIRONMENTAL MONITORING SYSTEM     ");
  Serial.println("========================================================");

  // Initialize I2C Bus
  Wire.begin(SDA_PIN, SCL_PIN);

  // 1. Initialize INA219
  if (ina219.begin()) {
    ina_ok = true;
    Serial.println("[SYSTEM] INA219 Solar Power Sensor: CONNECTED");
  } else {
    Serial.println("[SYSTEM] INA219: NOT DETECTED (Check 3V3, GND, D21, D22)");
  }

  // 2. Initialize ADS1115
  if (ads.begin(0x48)) {
    ads_ok = true;
    Serial.println("[SYSTEM] ADS1115 16-Bit ADC: CONNECTED");
  } else {
    Serial.println("[SYSTEM] ADS1115: NOT DETECTED (Check 3V3, GND, D21, D22)");
  }

  // 3. Initialize DS18B20 Temperature
  sensors.begin();
  if (sensors.getDeviceCount() > 0) {
    ds_ok = true;
    Serial.println("[SYSTEM] DS18B20 Temperature Probe: CONNECTED");
  } else {
    Serial.println("[SYSTEM] DS18B20: NOT DETECTED (Check Yellow=D4, 4.7k resistor)");
  }

  Serial.println("========================================================");
  Serial.println("Starting live solar measurements...\n");
  last_time = millis();
}

void loop() {
  unsigned long current_time = millis();
  float delta_hours = (current_time - last_time) / 3600000.0;
  last_time = current_time;

  float bus_voltage_V = 0.0;
  float current_mA    = 0.0;
  float power_mW      = 0.0;
  float temp_C        = -999.0;
  int16_t a0_val = 0, a1_val = 0;

  // 1. Read Solar Power from INA219
  if (ina_ok) {
    bus_voltage_V = ina219.getBusVoltage_V();
    current_mA    = ina219.getCurrent_mA();
    power_mW      = ina219.getPower_mW();

    // Prevent negative noise readings when solar panel is in dark
    if (bus_voltage_V < 0.05) bus_voltage_V = 0.0;
    if (current_mA < 0.1) current_mA = 0.0;
    if (power_mW < 0.1) power_mW = 0.0;

    // Accumulate total energy (mWh)
    total_energy_mwh += (power_mW * delta_hours);
  }

  // 2. Read ADS1115 Channels
  if (ads_ok) {
    a0_val = ads.readADC_SingleEnded(0);
    a1_val = ads.readADC_SingleEnded(1);
  }

  // 3. Read Temperature from DS18B20
  if (ds_ok) {
    sensors.requestTemperatures();
    temp_C = sensors.getTempCByIndex(0);
  }

  // --- Display Friendly Output in Serial Monitor ---
  Serial.println("----------------- [SOLAR DATA] -----------------");
  Serial.print("Solar Voltage : "); Serial.print(bus_voltage_V, 3); Serial.println(" V");
  Serial.print("Solar Current : "); Serial.print(current_mA, 2);    Serial.println(" mA");
  Serial.print("Solar Power   : "); Serial.print(power_mW, 2);       Serial.println(" mW");
  Serial.print("Total Energy  : "); Serial.print(total_energy_mwh, 3); Serial.println(" mWh");
  
  if (ds_ok && temp_C != -127.0 && temp_C > -50.0) {
    Serial.print("Temperature   : "); Serial.print(temp_C, 2); Serial.println(" °C");
  } else {
    Serial.println("Temperature   : Sensor not detected");
  }

  if (ads_ok) {
    Serial.print("ADC Light (A0): "); Serial.println(a0_val);
  }
  Serial.println("------------------------------------------------\n");

  // Format sent for the CSV Logger: DATA,Voltage,Current,Power,Energy,Temp,A0
  Serial.print("CSV_ROW,");
  Serial.print(bus_voltage_V, 3); Serial.print(",");
  Serial.print(current_mA, 2);    Serial.print(",");
  Serial.print(power_mW, 2);      Serial.print(",");
  Serial.print(total_energy_mwh, 3); Serial.print(",");
  Serial.print(temp_C, 2);        Serial.print(",");
  Serial.println(a0_val);

  delay(2000); // 2-second sampling interval
}
