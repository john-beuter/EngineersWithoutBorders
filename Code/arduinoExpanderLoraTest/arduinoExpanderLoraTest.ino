#include <Wire.h>

#define SC18IS602B_ADDR 0x28  // I2C address of SC18IS602B

// LoRa SX1278 SPI Commands
#define REG_OP_MODE 0x01
#define REG_FIFO 0x00
#define REG_FIFO_ADDR_PTR 0x0D
#define REG_PAYLOAD_LENGTH 0x22
#define REG_MODEM_CONFIG1 0x1D
#define REG_MODEM_CONFIG2 0x1E
#define REG_FREQ_MSB 0x06
#define REG_FREQ_MID 0x07
#define REG_FREQ_LSB 0x08
#define REG_TX_BASE_ADDR 0x0E
#define REG_FIFO_TX_BASE_ADDR 0x0F
#define REG_IRQ_FLAGS 0x12
#define REG_DIO_MAPPING1 0x40
#define REG_PA_CONFIG 0x09

void setup() {
    Serial.begin(115200);
    Wire.begin();  // Start I2C communication

    Serial.println("Configuring SC18IS602B for SPI communication...");

    // Configure SC18IS602B for SPI Mode 0, Clock = 1MHz
    Wire.beginTransmission(SC18IS602B_ADDR);
    Wire.write(0xF0);  // SPI Configuration Register
    Wire.write(0x00);  // SPI Mode 0 (CPOL=0, CPHA=0), Clock divider = 1MHz
    Wire.endTransmission();
    delay(100);

    uint8_t opMode = readLoRaRegister(REG_OP_MODE);
    Serial.print("OP_MODE register: 0x");
    Serial.println(opMode, HEX);


    Serial.println("Configuring LoRa module...");
    configureLoRa();
}

void loop() {
    Serial.println("Sending LoRa packet...");
    sendLoRaPacket();
    delay(5000);  // Send packet every 5 seconds
}

// Function to write to LoRa registers via SC18IS602B
void writeLoRaRegister(uint8_t reg, uint8_t value) {
    Wire.beginTransmission(SC18IS602B_ADDR);
    Wire.write(0x01);  // SPI Write Command
    Wire.write(reg | 0x80);  // Write Mode (MSB 1)
    Wire.write(value);
    Wire.endTransmission();
    delay(10);
}

// Function to configure LoRa module (SX1278)
void configureLoRa() {
    writeLoRaRegister(REG_OP_MODE, 0x00);  // Set to Sleep mode
    delay(10);

    // Set frequency to 915MHz
    writeLoRaRegister(REG_FREQ_MSB, 0xE4);
    writeLoRaRegister(REG_FREQ_MID, 0xC0);
    writeLoRaRegister(REG_FREQ_LSB, 0x00);

    // Set Modem Config
    writeLoRaRegister(REG_MODEM_CONFIG1, 0x72);
    writeLoRaRegister(REG_MODEM_CONFIG2, 0x74);

    // Set TX power
    writeLoRaRegister(REG_PA_CONFIG, 0x8F);  // Max power

    // Set TX base address
    writeLoRaRegister(REG_FIFO_TX_BASE_ADDR, 0x80);
    writeLoRaRegister(REG_FIFO_ADDR_PTR, 0x80);

    Serial.println("LoRa module configured.");
}

// Function to send a packet via LoRa
void sendLoRaPacket() {
    Serial.println("Sending LoRa SPI test pattern...");

    // Set LoRa to Standby mode
    writeLoRaRegister(REG_OP_MODE, 0x01);
    delay(10);

    // Set FIFO pointer
    writeLoRaRegister(REG_FIFO_ADDR_PTR, 0x80);

    // Define simple test pattern
    uint8_t testPattern[] = { 0xAA, 0x55, 0xFF, 0x00 }; // Easy-to-read values
    //AA = 10101010
    //55 = 01010101
    //FF = 11111111    
    
    for (int i = 0; i < sizeof(testPattern); i++) {
        Serial.print("SPI Data Sent: ");
        Wire.beginTransmission(SC18IS602B_ADDR);
        Wire.write(0x01);  // SPI Write Command
        Wire.write(REG_FIFO | 0x80);
        Wire.write(testPattern[i]);
        Wire.endTransmission();
        delay(5);

        // Print sent byte in HEX format
        Serial.print("0x");
        Serial.print(testPattern[i], HEX);
        Serial.println();
        delay(5000);
    }

    Serial.println();  // Move to the next line in the Serial Monitor

    // Set Payload Length
    writeLoRaRegister(REG_PAYLOAD_LENGTH, sizeof(testPattern));

    // Set to TX Mode
    writeLoRaRegister(REG_OP_MODE, 0x03);
    Serial.println("Packet sent!");
}

uint8_t readLoRaRegister(uint8_t reg) {
    Wire.beginTransmission(SC18IS602B_ADDR);
    Wire.write(0x02);            // SPI Read command
    Wire.write(reg & 0x7F);      // MSB 0 for read
    Wire.endTransmission(false); // Send repeated start

    Wire.requestFrom(SC18IS602B_ADDR, 1);
    if (Wire.available()) {
        return Wire.read();
    }
    return 0xFF;  // Error
}


