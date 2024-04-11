// EWB Tank Level Logger


#include <SPI.h>
#include <SD.h>

const int chipSelect = 10;
int numReadings = 20;
// Initializing variables
float readings[20];  // the readings from the analog input
int readIndex = 0;          // the index of the current reading
float total_height = 0;              // the running total
float average_height = 0;            // the average
float voltage = 0.0;
float pressure_kpa = 0.0;
float pressure_pa = 0.0;
float cm_h2o = 0.0;
float height = 0.0;
float last_height = 0.0;
float change_height = 0.0;

// User defined
int inputPin = A0;
// Using to zero pressure when no differential is present
float trimPressure = 0.43; 
float zeroHeight = 2.80;


void setup() {
  // Open serial communications and wait for port to open
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }


  Serial.print("Initializing SD card...");

  // see if the card is present and can be initialized
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    // dont do anything more
    while (1);
  
  }
  Serial.println("card initialized.");

    // initialize all the readings to 0:
  for (int thisReading = 0; thisReading < numReadings; thisReading++) {
    readings[thisReading] = 0;
  }
  
}


void loop()   {
  
  // subtract the last reading:
  total_height = total_height - readings[readIndex];
  
  // read from the sensor:
  voltage = (analogRead(inputPin) * (5.0/1023.0));
  pressure_kpa = (55.55*voltage/5.0) - 2.22 + trimPressure;
  
  cm_h2o = 0.0101972*pressure_kpa*1000;
  readings[readIndex] = cm_h2o;
  
  // add the reading to the total:
  total_height = total_height + readings[readIndex];
  // advance to the next position in the array:
  readIndex = readIndex + 1;

  // if we're at the end of the array...
  if (readIndex >= numReadings) {
    // ...wrap around to the beginning:
    readIndex = 0;
  }

  // calculate the average:
  average_height = (total_height / numReadings) - zeroHeight;
  
  
  // make a string for assembling the data to log
  String dataString = "";
  
  dataString += String(average_height);
  
  // open the file. note that only one file can be open at a time
  // so you have to close this one before opening another
  File dataFile = SD.open("data.csv", FILE_WRITE);

  // if the file is available write to it
  if (dataFile) {
    dataFile.println(dataString);
    dataFile.close();
    // print to the serial port too:
    Serial.println(dataString);
  }
  // if the file isn't open pop up an error
  else {
    Serial.println("error opening datalog.txt");
  }

  delay(50);
}
