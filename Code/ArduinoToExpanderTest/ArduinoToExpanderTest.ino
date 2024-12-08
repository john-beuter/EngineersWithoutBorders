#include <Wire.h>
#include <SC18IS602.h>
#include <spi.h>
//Connect the MOSI and MISO with a wire and run this sketch.
SC18IS602 i2cspi=SC18IS602(SC18IS602_ADDRESS_000);
void setup()
{
    Serial.begin(9600);
    i2cspi.begin(0);
    Serial.println("Start SPI communication");  
}

void loop()
{

    //Serial.println(i2cspi.transfer(0xFF));
    if ( i2cspi.transfer(0x55) != 0x55) {
        Serial.println("Communication Error");
        while(1);
    }     
    if ( i2cspi.transfer(0xAA) != 0xAA) {
        Serial.println("Communication Error");
        while(1);
    }      
    
    Serial.println("Communication is good");
    delay(2000);
}
