#include <SPI.h>
#include <LoRa.h>

#define SS    15  // D8
#define RST   16  // D0
#define DIO0   4  // D2

void setup() {
  Serial.begin(115200);
  while (!Serial);

  Serial.println("=== LoRa Receiver Init ===");

  Serial.println("1. Starting SPI...");
  SPI.begin();

  Serial.println("2. Setting LoRa pins...");
  LoRa.setPins(SS, RST, DIO0);

  Serial.println("3. Starting LoRa.begin()...");
  if (!LoRa.begin(433E6)) {
    Serial.println("❌ LoRa init failed");
    while (1);
  }

  Serial.println("✅ LoRa Receiver started");
}

void loop() {
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    Serial.print("Received: ");
    while (LoRa.available()) {
      Serial.print((char)LoRa.read());
    }
    Serial.println();
  }
}
