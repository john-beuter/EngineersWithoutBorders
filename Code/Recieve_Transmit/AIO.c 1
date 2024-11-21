#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>
#include <Adafruit_MPL3115A2.h>

#define LORA_SS_PIN D8
#define LORA_RST_PIN D0
#define LORA_DIO0_PIN D2

Adafruit_MPL3115A2 sensor = Adafruit_MPL3115A2();

const float tankHeight = 100.0; // Tank height in cm
const float waterDensity = 1000.0; // Water density in kg/m^3
const float gravity = 9.81; // Acceleration due to gravity in m/s^2
const long frequency = 915E6; // LoRa frequency (adjust as needed)

void setup() {
  Serial.begin(9600);
  while (!Serial);

  // Initialize LoRa
  LoRa.setPins(LORA_SS_PIN, LORA_RST_PIN, LORA_DIO0_PIN);
  if (!LoRa.begin(frequency)) {
    Serial.println("LoRa initialization failed!");
    while (1);
  }
  Serial.println("LoRa initialized successfully.");

  // Initialize pressure sensor
  if (!sensor.begin()) {
    Serial.println("Could not find pressure sensor. Check wiring.");
    while (1);
  }
  sensor.setMode(MPL3115A2_BAROMETER);
  Serial.println("Pressure sensor initialized successfully.");
}

void loop() {
  // Read pressure and calculate water level
  float pressure = sensor.getPressure(); // Get pressure in Pascals
  float waterLevel = calculateWaterLevel(pressure);

  // Prepare data packet
  String packet = "P:" + String(pressure) + ",WL:" + String(waterLevel);

  // Send data via LoRa
  LoRa.beginPacket();
  LoRa.print(packet);
  LoRa.endPacket();

  // Print data to Serial for debugging
  Serial.println("Sent packet: " + packet);

  delay(5000); // Wait for 5 seconds before next transmission
}

float calculateWaterLevel(float pressure) {
  // Calculate water level based on pressure
  float waterLevel = (pressure - 101325) / (waterDensity * gravity) * 100; // in cm
  
  // Ensure water level is not negative (in case pressure is less than atmospheric)
  waterLevel = max(0, waterLevel);
  
  // Ensure water level doesn't exceed tank height
  waterLevel = min(waterLevel, tankHeight);

  return waterLevel;
}