#include <LoRa.h>
#include <SPI.h>

// Define LoRa pins based on your wiring
#define LORA_SS D8    // SX1278 NSS (Chip Select)
#define LORA_RST D0   // SX1278 Reset
#define LORA_DIO0 D2  // SX1278 DIO0 (interrupt)

void setup() {
  Serial.begin(115200);
  delay(1000);     // Brief delay for serial monitor stability

  Serial.println("LoRa Receiver Initializing...");

  // LoRa Module Reset
  pinMode(LORA_RST, OUTPUT);
  digitalWrite(LORA_RST, LOW);
  delay(10);
  digitalWrite(LORA_RST, HIGH);
  delay(10);

  Serial.println("Setting LoRa pins...");
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);

  yield();

  Serial.println("Initializing LoRa module...");
  if (!LoRa.begin(433E6)) {
    Serial.println("Starting LoRa failed! Check wiring and power.");
    while (1);
  }

  // Additional LoRa settings for improved stability
  LoRa.setSignalBandwidth(125E3);  // Set bandwidth to 125 kHz (default)
  LoRa.setSpreadingFactor(10);     // Increase spreading factor to match transmitter
  LoRa.setCodingRate4(5);          // Set coding rate to add error correction

  Serial.println("LoRa module initialized successfully!");
}

void loop() {
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    Serial.print("Received packet: ");
    while (LoRa.available()) {
      Serial.print((char)LoRa.read());
    }
    Serial.println();
  } //else {
  //   Serial.println("No packet received.");
  // }
  delay(500);  // Reduced delay to improve packet capture rate
}
