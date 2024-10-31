#include <SPI.h>
#include <LoRa.h>

#define LORA_SS_PIN D8
#define LORA_RST_PIN D0
#define LORA_DIO0_PIN D2

const long frequency = 915E6; // LoRa frequency (adjust as needed)

void setup() {
  Serial.begin(9600);
  while (!Serial);

  LoRa.setPins(LORA_SS_PIN, LORA_RST_PIN, LORA_DIO0_PIN);
  if (!LoRa.begin(frequency)) {
    Serial.println("LoRa initialization failed!");
    while (1);
  }
  Serial.println("LoRa Receiver initialized successfully.");
}

void loop() {
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    String receivedData = "";
    while (LoRa.available()) {
      receivedData += (char)LoRa.read();
    }

    Serial.println("Received packet: " + receivedData);

    // Parse and process the received data
    int pressureIndex = receivedData.indexOf("P:");
    int waterLevelIndex = receivedData.indexOf("WL:");
    if (pressureIndex != -1 && waterLevelIndex != -1) {
      float pressure = receivedData.substring(pressureIndex + 2, waterLevelIndex - 1).toFloat();
      float waterLevel = receivedData.substring(waterLevelIndex + 3).toFloat();

      Serial.print("Pressure: ");
      Serial.print(pressure);
      Serial.print(" Pa, Water Level: ");
      Serial.print(waterLevel);
      Serial.println(" cm");
    }
  }
}