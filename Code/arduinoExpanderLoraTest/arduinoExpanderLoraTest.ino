#include <Wire.h>
#include <SC18IS602.h>
#include <SPI.h>
#include <LoRa.h>

SC18IS602 i2cspi = SC18IS602(SC18IS602_ADDRESS_000);

void setup() {
  Serial.begin(115200);
  Serial.println("LoRa Sender with SC18IS602 SPI Communication");

  // Initialize SC18IS602
  i2cspi.begin(0);
  Serial.println("Start SPI communication");

  // Set LoRa pins
  LoRa.setPins(10, 9, 2);  // NSS = pin 10, REST = 9, DIO0 = pin 2 
  
  if (!LoRa.begin(433E6)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }

  // Additional LoRa settings for reliability
  LoRa.setSignalBandwidth(125E3);
  LoRa.setSpreadingFactor(10);
  LoRa.setCodingRate4(5);
}

void loop() {
  // SC18IS602 SPI Communication Test
  if (i2cspi.transfer(0x55) != 0x55) {
    Serial.println("SC18IS602 Communication Error");
    while(1);
  }     
  if (i2cspi.transfer(0xAA) != 0xAA) {
    Serial.println("SC18IS602 Communication Error");
    while(1);
  }      
  
  Serial.println("SC18IS602 Communication is good");

  // LoRa Transmission
  for (int i = 0; i < 10; i++) {
    Serial.print("Sending LoRa packet: ");
    Serial.println(i);
    LoRa.beginPacket();            
    LoRa.print("packet: ");
    LoRa.print(i); 
    LoRa.endPacket();             
    
    delay(3000);
  }

  delay(2000);  // Additional delay before next cycle
}