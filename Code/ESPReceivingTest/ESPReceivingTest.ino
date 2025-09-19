#include <LoRa.h>
#include <SPI.h>

#define LORA_SS   15  // D8
#define LORA_RST  16  // D0
#define LORA_DIO0  4  // D2

void setup() {
  Serial.begin(115200);
  while (!Serial);

  Serial.println("LoRa Receiver");

  SPI.begin();  // Use ESP8266 default SPI pins
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);
  LoRa.setSPIFrequency(1000000); //maybe

  Serial.println("Initializing LoRa module...");
  if (!LoRa.begin(433E6)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }

  Serial.println("LoRa module initialized successfully!");
}

void loop() {
  Serial.println("tick");
  delay(500);
}
