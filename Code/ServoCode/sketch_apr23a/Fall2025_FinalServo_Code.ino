// UNO SPECIFIC CODE: #include <Servo.h>
#include <ESP32Servo.h> // ESP32 SPECIFIC CODE

Servo myServo;
// UNO CODE: int pwm = 11;

void setup() {
  // UNO SPECIFIC CODE: pinMode(pwm, OUTPUT)
  // UNO SPECIFIC CODE: myServo.attach(11);
  
  myServo.attach(15); // ESP32 SPECIFIC CODE
}

// for modifications: tweak delay(# of milliseconds)
void loop() {
  myServo.writeMicroseconds(1700); // makes it go forward 
  delay(7000);

  myServo.writeMicroseconds(1500); // makes it stop
  delay(1000);

  myServo.writeMicroseconds(1300); // makes it go backward
  delay(7000);
}
