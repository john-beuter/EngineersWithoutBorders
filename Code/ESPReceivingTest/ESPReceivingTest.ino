#include <LoRa.h>
#include <SPI.h>

// Define LoRa pins based on your wiring
#define LORA_SS D8    // SX1278 NSS (Chip Select)
#define LORA_RST D0   // SX1278 Reset
#define LORA_DIO0 D2  // SX1278 DIO0 (interrupt)

void setup() {
  Serial.begin(115200);

  SPI.pins(D7, D6, D5, LORA_SS); // MOSI, MISO, SCK, SS
  SPI.begin();
  SPI.setFrequency(1E6); // Set SPI clock to 1 MHz for stability

  Serial.println("LoRa Receiver");

  Serial.println("Setting up LoRa pins...");
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);

  yield(); // Allow watchdog to reset

  Serial.println("Initializing LoRa module...");
  if (!LoRa.begin(433E6)) {  // Initialize LoRa at 433 MHz
      Serial.println("Starting LoRa failed!");
      while(1);
  } else {
    Serial.println("LoRa module initialized successfully");
  }
  Serial.println("LoRa receiver setup complete");
}

void loop() {
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    // Received a packet
    Serial.print("Received packet: ");
    while (LoRa.available()) {
      Serial.print((char)LoRa.read());
    }
    Serial.println();
  }
  // else Serial.println("not receiving packet");
  delay(10);
}

