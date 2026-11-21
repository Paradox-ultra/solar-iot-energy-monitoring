/*
 * Corrected ESP32 Solar Monitor Firmware
 * Hardware: INA219 (I2C 0x40), ADS1115 (I2C 0x48), DS18B20 (GPIO 4)
 *
 * Serial protocol retained for compatibility:
 * CSV_ROW,Voltage_V,Current_mA,Power_mW,Energy_mWh,Temperature_C,ADC0
 *
 * Failed/unavailable sensors are emitted as -999 (or -1 for ADC), NOT zero.
 */

#include <Wire.h>
#include <Adafruit_INA219.h>
#include <Adafruit_ADS1X15.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <math.h>

static constexpr uint8_t SDA_PIN = 21;
static constexpr uint8_t SCL_PIN = 22;
static constexpr uint8_t ONE_WIRE_PIN = 4;
static constexpr uint8_t INA219_ADDRESS = 0x40;
static constexpr uint8_t ADS1115_ADDRESS = 0x48;
static constexpr float INVALID_FLOAT = -999.0f;
static constexpr int16_t INVALID_ADC = -1;
static constexpr uint32_t SAMPLE_INTERVAL_MS = 2000;

Adafruit_INA219 ina219(INA219_ADDRESS);
Adafruit_ADS1115 ads;
OneWire oneWire(ONE_WIRE_PIN);
DallasTemperature ds18b20(&oneWire);

bool inaReady = false;
bool adsReady = false;
bool tempReady = false;
float totalEnergy_mWh = 0.0f;
uint32_t previousSampleMs = 0;

bool i2cDevicePresent(uint8_t address) {
  Wire.beginTransmission(address);
  return Wire.endTransmission() == 0;
}

void scanI2CBus() {
  Serial.println("[DIAG] Scanning I2C bus...");
  uint8_t found = 0;
  for (uint8_t address = 1; address < 127; ++address) {
    Wire.beginTransmission(address);
    if (Wire.endTransmission() == 0) {
      Serial.printf("[I2C] Device found at 0x%02X\n", address);
      ++found;
    }
  }
  if (found == 0) {
    Serial.println("[I2C][ERROR] No devices found. Check 3V3, GND, SDA=GPIO21, SCL=GPIO22 and solder joints.");
  }
  if (!i2cDevicePresent(INA219_ADDRESS))
    Serial.println("[I2C][ERROR] INA219 expected at 0x40 not detected.");
  if (!i2cDevicePresent(ADS1115_ADDRESS))
    Serial.println("[I2C][ERROR] ADS1115 expected at 0x48 not detected; check ADDR strap (GND=0x48).");
}

void initializeSensors() {
  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(100000);
  delay(50);
  scanI2CBus();

  inaReady = ina219.begin();
  if (inaReady) {
    ina219.setCalibration_32V_2A();
    Serial.println("[SYSTEM] INA219 initialized at 0x40.");
  } else {
    Serial.println("[SYSTEM][ERROR] INA219 initialization failed; power readings invalid.");
  }

  adsReady = ads.begin(ADS1115_ADDRESS, &Wire);
  if (adsReady) {
    ads.setGain(GAIN_ONE);
    Serial.println("[SYSTEM] ADS1115 initialized at 0x48.");
  } else {
    Serial.println("[SYSTEM][ERROR] ADS1115 initialization failed; ADC invalid.");
  }

  ds18b20.begin();
  ds18b20.setResolution(12);
  const int deviceCount = ds18b20.getDeviceCount();
  tempReady = deviceCount > 0;
  if (tempReady) {
    Serial.printf("[SYSTEM] DS18B20 detected: %d device(s) on GPIO %u.\n", deviceCount, ONE_WIRE_PIN);
  } else {
    Serial.println("[SYSTEM][ERROR] No DS18B20. Check probe polarity, GPIO4 DATA, common GND and 4.7k pull-up to 3V3.");
  }
}

void setup() {
  Serial.begin(115200);
  delay(1200);
  Serial.println();
  Serial.println("=== ESP32 SOLAR MONITOR / SENSOR DIAGNOSTICS ===");
  initializeSensors();
  Serial.println("[SYSTEM] CSV output begins below. Serial Monitor: 115200 baud.");
  previousSampleMs = millis();
}

void loop() {
  const uint32_t now = millis();
  const float elapsedHours = (now - previousSampleMs) / 3600000.0f;
  previousSampleMs = now;

  float busVoltage_V = INVALID_FLOAT;
  float current_mA = INVALID_FLOAT;
  float power_mW = INVALID_FLOAT;
  float temperature_C = INVALID_FLOAT;
  int16_t adc0 = INVALID_ADC;

  if (inaReady) {
    const float v = ina219.getBusVoltage_V();
    const float i = ina219.getCurrent_mA();
    const float p = ina219.getPower_mW();
    if (isfinite(v) && isfinite(i) && isfinite(p)) {
      busVoltage_V = v;
      current_mA = i;
      power_mW = p;
      if (power_mW > 0.0f && elapsedHours > 0.0f)
        totalEnergy_mWh += power_mW * elapsedHours;
    } else {
      Serial.println("[READ][ERROR] INA219 returned non-finite reading.");
    }
  }

  if (adsReady) {
    adc0 = ads.readADC_SingleEnded(0);
  }

  if (tempReady) {
    ds18b20.requestTemperatures();
    const float t = ds18b20.getTempCByIndex(0);
    if (t != DEVICE_DISCONNECTED_C && isfinite(t) && t >= -55.0f && t <= 125.0f) {
      temperature_C = t;
    } else {
      Serial.println("[READ][ERROR] DS18B20 disconnected or invalid; check data and pull-up.");
    }
  }

  Serial.printf("[STATUS] INA219=%s ADS1115=%s DS18B20=%s\n",
                inaReady ? "OK" : "MISSING",
                adsReady ? "OK" : "MISSING",
                tempReady ? "OK" : "MISSING");

  // Fixed seven-field row consumed by logger/data_logger.py.
  Serial.print("CSV_ROW,");
  Serial.print(busVoltage_V, 3); Serial.print(',');
  Serial.print(current_mA, 2); Serial.print(',');
  Serial.print(power_mW, 2); Serial.print(',');
  Serial.print(totalEnergy_mWh, 3); Serial.print(',');
  Serial.print(temperature_C, 2); Serial.print(',');
  Serial.println(adc0);

  delay(SAMPLE_INTERVAL_MS);
}
