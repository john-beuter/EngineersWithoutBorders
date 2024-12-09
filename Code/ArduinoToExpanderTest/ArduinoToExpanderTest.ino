#include <Wire.h>
#include <SC18IS602.h>
#include <spi.h>

// Initialize SC18IS602 with default I2C address (0x28)
SC18IS602 i2cspi = SC18IS602(SC18IS602_ADDRESS_000);

void setup() {
    Serial.begin(9600);
    Wire.begin();

    Serial.println("Initializing SC18IS602...");
    i2cspi.begin(0);  // Initialize SC18IS602 in SPI mode 0
    Serial.println("SC18IS602 initialized successfully!");
}

void loop() {
    Serial.println("Testing SPI communication...");

    // Transfer first test byte (0x55)
    uint8_t response1 = i2cspi.transfer(0x55);
    Serial.print("Sent: 0x55, Received: 0x");
    Serial.println(response1, HEX);

    if (response1 != 0x55) {
        Serial.println("Communication Error on 0x55!");
        while (1);
    }

    // Transfer second test byte (0xAA)
    uint8_t response2 = i2cspi.transfer(0xAA);
    Serial.print("Sent: 0xAA, Received: 0x");
    Serial.println(response2, HEX);

    if (response2 != 0xAA) {
        Serial.println("Communication Error on 0xAA!");
        while (1);
    }

    Serial.println("SPI communication successful!");

    delay(2000);  // Wait 2 seconds for observation
}
