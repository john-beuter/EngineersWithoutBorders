#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Wire.h>
#include <SD.h>
#include <string.h>

#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
#define PRESSURE_SENSOR_PIN A0


double voltage = 0.0;
int read_offset = 23;
int value = 0;
double pressure_kpa = 0.0;
double cm_h2o = 0.0;
double cur_height = 0.0;
int file_counter = 0;
String file_name;

bool DISP_ENABLED = false;

Adafruit_SSD1306 display(128, 32, &Wire, -1);
File data_file;

// EWB Prototype V1.5 Code
// Code will run 'tasks' at different rates
// Task 0: collect sensor data every loop
// Task 1: Print data periodically to serial console for debugging purposes/backup to display.
// Task 2: Log data to SD card - long interval, every 10-30 min.
// Task 3: Refresh display rapidly.




void setup()
{
    Serial.begin(115200);
    pinMode(PRESSURE_SENSOR_PIN, INPUT);

    // Check for display (we still want it to collect data even if the display isn't present)
    if (!DISP_ENABLED or !display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS))
    {
        Serial.println("Display not detected or disabled.");
    }
    else
    {
        // Clear buffer and initialize display.
        display.clearDisplay();
        display.display();

        //display.setTextSize(3);
        display.setFont(&FreeSans9pt7b);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(0, 20);
        display.println("Display OK");
        display.display();
        delay(3000);
    }

    // Check for SD card. If it isn't present, then show error
    // Rapidly blink LED to signify error.
    if (!SD.begin(10))
    {
        while (true)
        {
            Serial.println("SD card not detected.");
            // Print to display...
        }
    }
    else
    {
        Serial.println("SD card found & initialized.");
    }

    findUniqueFileName();
    file_name = generateFileName();

    File data_file = SD.open(file_name, FILE_WRITE);

    if (!data_file)
    {
        Serial.println("Couldn't open data file.");
    }

    // Add file header
    data_file.println("Time (ms), Water Level (cm)");
    data_file.close();

    Serial.print("------ Created file,");
    Serial.println(" starting data logging ------");

    delay(1000);
}

void loop()
{
    
    // Measure height every loop
    cur_height = getHeight();

    // Update/refresh display
    //updateDisplay();

    // Print data to serial console every 5 seconds for debugging.
    printDataSerial();

    // Log data to SD card
    logData();

}


// Read pressure sensor and calculate height of water
double getHeight()
{
    unsigned int avg_read = 0;
    
    // Read analog pin and average values
    for (int i=0; i<100; i++)
    {
        avg_read += analogRead(PRESSURE_SENSOR_PIN);
    }
    avg_read = (avg_read / 100) - read_offset;

    // Convert analog reading to voltage
    voltage = (avg_read * (5.0/1023.0));

    // Convert voltage to pressure
    pressure_kpa = (55.55*voltage/4.5);

    // Convert pressure differential to cm of H2O
    cm_h2o = 0.0101972*pressure_kpa*1000;

    return cm_h2o;
}

// Read DS18b20 temperature sensor
int getTemp()
{

}

// Periodically prints to serial console
void printDataSerial()
{
    // On first call, record starting time
    static unsigned long serialMillis = millis();

    // Check difference between current time and last time this was called
    if (millis() - serialMillis > 1000)
    {
        Serial.println("--- STATUS ---");
        Serial.print("Height (cm): ");
        Serial.println(cur_height);
        serialMillis = millis();
    }

}

// Function to generate a unique file name
void findUniqueFileName() {
  String fileName;
  do {
    fileName = "data";
    fileName += file_counter;
    fileName += ".csv";
    file_counter++;
  } while (SD.exists(fileName));
}

// Function to generate the current filename
String generateFileName() {
  String fileName = "data";
  fileName += (file_counter - 1); // Subtract 1 because fileCounter was incremented after finding the unique filename
  fileName += ".csv";
  return fileName;
}

// Log data to SD card
void logData()
{
    static unsigned long sdMillis = millis();

    if (millis() - sdMillis > 60000)
    {
        

        // Current time
        unsigned long current_time = millis();
        File data_file = SD.open(file_name, FILE_WRITE);

        if (data_file)
        {
    
            // Log data
            data_file.print(current_time);
            data_file.print(",");
            data_file.println(cur_height);
            
            // Close file
            data_file.close();

            // Confirm data has been logged
            Serial.println("Data logged to card!");
        }
        else
        {
            Serial.println("Error opening data file. No data logged.");
        }


        sdMillis = millis();
    }

}

// Periodically updates display
void updateDisplay()
{
    // On first call, record starting time
    static unsigned long dispMillis = millis();

    // Check difference between current time and last time this was called
    if (millis() - dispMillis > 5000)
    { 

        // Clear display
        display.clearDisplay();

        // Print reading
        display.setCursor(0, 20);
        display.print(cur_height);
        display.println(" cm");
        delay(1);
        display.display();
        
        
        dispMillis = millis();
    }

}

// Rapid LED blinking to indicate error.
void rapidBlink()
{
    static unsigned long rapidBlinkMillis = millis();

    if (millis() - rapidBlinkMillis > 1000)
    {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(1000);
        digitalWrite(LED_BUILTIN, LOW);
        rapidBlinkMillis = millis();
    }


}

// Rapid LED blinking to indicate OK.
void slowBlink()
{
    static unsigned long slowBlinkMillis = millis();

    if (millis() - slowBlinkMillis > 1000)
    {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(1000);
        digitalWrite(LED_BUILTIN, LOW);
        slowBlinkMillis = millis();
    }


}