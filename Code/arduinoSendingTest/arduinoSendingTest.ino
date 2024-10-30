#include <SPI.h>
#include <LoRa.h>

void setup() {
  // Start the serial communication at 9600 baud rate
  Serial.begin(115200);
  Serial.println("LoRa Sender");

  LoRa.setPins(10,9,2);
  //NSS = pin 10, REST = 9, DIO0 = pin 2 
  
  while (!Serial);  
  if (!LoRa.begin(433E6)) { // or 433E6, the MHz speed of yout module
    Serial.println("Starting LoRa failed!");
    while (1);
  }
}

void loop() {
  Serial.println("Sending packet...");
  LoRa.beginPacket();           // Start a packet
  LoRa.print("Hello, ESP8266!"); // Add payload to the packet
  LoRa.endPacket();             // Finish the packet and send it
  
  delay(2000); 
  // while(1);
}
