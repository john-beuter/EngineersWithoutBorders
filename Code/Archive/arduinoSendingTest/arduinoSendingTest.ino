#include <SPI.h> 
#include <LoRa.h>

void setup() {
  Serial.begin(115200);
  Serial.println("LoRa Sender");

  // Set LoRa pins
  LoRa.setPins(10, 9, 2);  // NSS = pin 10, REST = 9, DIO0 = pin 2 
  
  if (!LoRa.begin(433E6)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }

  // Additional LoRa settings for reliability
  LoRa.setSignalBandwidth(125E3);  // Set bandwidth to 125 kHz (default)
  LoRa.setSpreadingFactor(10);     // Increase spreading factor to improve range
  LoRa.setCodingRate4(5);          // Set coding rate to add error correction
}

void loop() {
  for (int i = 0; i < 10; i++) {
    Serial.print("Sending packet: ");
    Serial.println(i);
    LoRa.beginPacket();            
    LoRa.print("fart packet: ");
    LoRa.print(i); 
    LoRa.endPacket();             
    
    delay(3000);   // Increased delay to give receiver more processing time
  }
}
