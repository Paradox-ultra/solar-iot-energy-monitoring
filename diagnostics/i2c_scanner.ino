#include <Wire.h>
static constexpr int SDA_PIN = 21;
static constexpr int SCL_PIN = 22;
void setup() {
  Serial.begin(115200);
  delay(1000);
  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(100000);
  Serial.println("ESP32 I2C scanner: SDA=21 SCL=22, 100 kHz");
  int found = 0;
  for (uint8_t address = 1; address < 127; ++address) {
    Wire.beginTransmission(address);
    if (Wire.endTransmission() == 0) {
      Serial.printf("Found I2C device at 0x%02X", address);
      if (address == 0x40) Serial.print(" (expected INA219)");
      if (address == 0x48) Serial.print(" (expected ADS1115 when ADDR=GND)");
      Serial.println();
      ++found;
    }
  }
  Serial.printf("Scan complete: %d device(s) found.\n", found);
  if (!found) Serial.println("No devices: check 3V3, GND, SDA/SCL, solder joints and address straps.");
}
void loop() { delay(10000); }
