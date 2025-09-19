#include <Wire.h>
#include <SC18IS602.h>

SC18IS602 i2cspi = SC18IS602(SC18IS602_ADDRESS_000);

#define REG_VERSION 0x42  // LoRa chip version register

void setup() {
  Serial.begin(115200);
  Wire.begin();

  Serial.println("Initializing LoRa module...");

  i2cspi.begin(0);  // Initialize SC18IS602 in SPI mode 0

  // Read LoRa version register
  uint8_t version = readLoRaRegister(REG_VERSION);

  if (version == 0x12) {
    Serial.println("LoRa module initialized successfully!");
  } else {
    Serial.print("Failed to initialize LoRa module. Version read: 0x");
    Serial.println(version, HEX);
  }
}

void loop() {
}

uint8_t readLoRaRegister(uint8_t reg) {
  uint8_t command = reg & 0x7F;  // MSB = 0 for read operation
  uint8_t value;

  setNSS(true);
  i2cspi.transfer(command);
  value = i2cspi.transfer(0x00);  // Dummy byte to receive data
  setNSS(false);

  return value;
}

void setNSS(bool state) {
  uint8_t gpioValue = state ? 0xFE : 0xFD;  // GPIO0 High/Low

  Wire.beginTransmission(0x28);
  Wire.write(0xF0);  // GPIO Write Command
  Wire.write(gpioValue);
  Wire.endTransmission();
}
