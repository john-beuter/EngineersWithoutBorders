Servo servo;
int pwm = 11;
int pos = 0;

void setup() {
pinMode(pwm, OUTPUT);
servo.attach(11);
}
//next steps:
//make it go 90 degress stop and back to start point
//make it delay for a certain amount of hours
void loop() {
  for(pos=0;pos<=180;pos+=1){
    servo.write(pos);
    delay(15);
  }
  for(pos=180;pos>=0;pos-=1){
    servo.write(pos);
    delay(15);
  }
}
