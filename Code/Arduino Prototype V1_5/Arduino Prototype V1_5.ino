#define pressureSensorPin A0


float voltage = 0.0;
int value = 0;
float pressure_kpa = 0.0;
float cm_h2o = 0.0;
int offset = 23;
float cur_height = 0.0;



// Code will run 'tasks' at different rates
// Task 0: collect sensor data every loop
// Task 1: Print data periodically to serial console for debugging purposes/backup to display.
// Task 2: Log data to SD card - long interval, every 10-30 min.
// Task 3: Refresh display rapidly.




void setup()
{
    Serial.begin(115200);
    pinMode(pressureSensorPin, INPUT);



    // Check for display (we still want it to collect data even if the display isn't present)


    // Check for SD card. If it isn't present, then show error


    // Display files on SD card



}

void loop()
{
    // Measure height every loop
    cur_height = getHeight();

    // Print data to serial console every 5 seconds for debugging.
    printDataSerial();

    // Log data to SD card
    logData();
    
    // Update/refresh display
    updateDisplay();

}


// Read sensor and calculate height of water
float getHeight()
{
    // Read from the sensor
    value = analogRead(pressureSensorPin) - offset;

    // Convert analog read to voltage
    voltage = value * (5.0/1023);

    // Convert voltage to pressure differential in kPa (2.17 is an offset for calibration)
    pressure_kpa = (((voltage/4.5)-0.04)/0.018) + 2.17;
    
    // Convert from kPa (pressure) to centimeters of h2o
    cm_h2o = (10.1972*pressure_kpa);

    return cm_h2o;
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

// Log data to SD card
void logData()
{
    static unsigned long sdMillis = millis();

    if (millis() - sdMillis > 60000)
    {
        // Log data

        sdMillis = millis();
    }

}

// Update display
void updateDisplay()
{
    static unsigned long dispMillis = millis();

    if (millis() - dispMillis > 100)
    {
        // Update display
    }

}