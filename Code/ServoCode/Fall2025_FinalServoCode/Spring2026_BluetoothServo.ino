// UNO SPECIFIC CODE: #include <Servo.h>
#include <ESP32Servo.h> // ESP32 SPECIFIC CODE
#include "BluetoothSerial.h"

//if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
//  error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
//endif

Servo myServo;
// UNO CODE: int pwm = 11;

void setup() {
  // UNO SPECIFIC CODE: pinMode(pwm, OUTPUT)
  // UNO SPECIFIC CODE: myServo.attach(11);

  Serial.begin(115200);

  myServo.attach(15); // ESP32 SPECIFIC CODE

  SerialBT.begin("ESP32test"); // the name should be whatever the bluetooth device name is!!
  Serial.println("The device started, now you can pair it with bluetooth!"); 
}

// for modifications: tweak delay(# of milliseconds)
void loop() {
  
  // PC -> bluetooth (we send commands back)
  //if (Serial.available()) {
  //  SerialBT.write(Serial.read());
  //}

  // Bluetooth -> PC (receiving commands)
  if (SerialBT.available()) {
    Serial.write(SerialBT.read());
  }
  delay(20)

  myServo.writeMicroseconds(1700); // makes it go forward 
  delay(7000);

  myServo.writeMicroseconds(1500); // makes it stop
  delay(1000);

  myServo.writeMicroseconds(1300); // makes it go backward
  delay(7000);
}
