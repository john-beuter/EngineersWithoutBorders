#include "LoRaWan_APP.h"
#include "Arduino.h"
#include <WiFi.h>
#include <Firebase.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>

#define RF_FREQUENCY                                915000000 // Hz

#define TX_OUTPUT_POWER                             21        // dBm

#define LORA_BANDWIDTH                              0         // [0: 125 kHz,
                                                              //  1: 250 kHz,
                                                              //  2: 500 kHz,
                                                              //  3: Reserved]
#define LORA_SPREADING_FACTOR                       12         // [SF7..SF12]
#define LORA_CODINGRATE                             1         // [1: 4/5,
                                                              //  2: 4/6,
                                                              //  3: 4/7,
                                                              //  4: 4/8]
#define LORA_PREAMBLE_LENGTH                        8         // Same for Tx and Rx
#define LORA_SYMBOL_TIMEOUT                         0         // Symbols
#define LORA_FIX_LENGTH_PAYLOAD_ON                  false
#define LORA_IQ_INVERSION_ON                        false


#define RX_TIMEOUT_VALUE                            1000
#define BUFFER_SIZE                                 30 // Define the payload size here



// WiFi
const char* ssid = "GAVINLAPTOP 8050";
const char* password = "Richard14";

// Firebase
#define FIREBASE_HOST "https://boreholemonitoring-9c1d4-default-rtdb.firebaseio.com/"
Firebase fb(FIREBASE_HOST);


// Time
#define TIMEOFFSET -21600
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", TIMEOFFSET);
unsigned long lastUpload = 0;




char txpacket[BUFFER_SIZE];
char rxpacket[BUFFER_SIZE];
String incomingMessage = "";
bool packetReceived = false; // Flag to tell loop we have data

static RadioEvents_t RadioEvents;
int16_t txNumber;
int16_t rssi, rxSize;

int childCount;
JsonDocument docOutput;

void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr);
void OnTxDone();
void OnTxTimeout();

//checking if a duplicate measurement was sent in the last 20 minutes
bool duplicate(unsigned long lastTime){

   unsigned long currentTime = timeClient.getEpochTime();

   if (currentTime == 0 || lastTime == 0) {
    return false; 
  }

  unsigned long difference = currentTime - lastTime;


  if (difference <= 1080) {
    return true;
  } else {
    return false;
  }
}


void setup() {
  Serial.begin(115200);

  Mcu.begin(HELTEC_BOARD,SLOW_CLK_TPYE);

  RadioEvents.RxDone = OnRxDone;
  RadioEvents.TxDone = OnTxDone; 
  RadioEvents.TxTimeout = OnTxTimeout;

  Radio.Init(&RadioEvents);
  Radio.SetChannel(RF_FREQUENCY);


  // Setup RX Config
  Radio.SetRxConfig(MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
                    LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH,
                    LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
                    0, true, 0, 0, LORA_IQ_INVERSION_ON, true);

  // Setup TX Config (Needed for the ACK)
  Radio.SetTxConfig(MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
                    LORA_SPREADING_FACTOR, LORA_CODINGRATE,
                    LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
                    true, 0, 0, LORA_IQ_INVERSION_ON, 3000);


 //  Connect to WiFi
  
  Serial.print("Connecting to: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  int wifiCounter = 0;
  while (WiFi.status() != WL_CONNECTED && wifiCounter <40 ) {
    delay(500);
    Serial.print("-");
    wifiCounter ++;
  }

  // Check if we connected or timed out
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\nWiFi Failed! Restarting in 5 seconds...");
    delay(5000);
    ESP.restart(); // This hard-resets the board
  }


  Serial.println("\nWiFi Connected");

  timeClient.begin();

  if(fb.)

 //int value that changes
  if (fb.getInt("/ChildCount", childCount)) {
    Serial.print("Current Key Val ");
    Serial.println(childCount);
  } else {
    Serial.println("getInt failed");
  }

  // Start Listening
  Serial.println("into RX mode");
  Radio.Rx(0); // 0 means continuous receive mode
}

void loop() {



    Radio.IrqProcess();


  // If a packet was received in the background (handled by OnRxDone)

  if (packetReceived) {
    packetReceived = false; // Reset flag
    
    Serial.println("Processing Message: " + incomingMessage);



    //  Parse Data
    double readingValue = incomingMessage.toDouble();

    if (readingValue == 0.0){
      Serial.println("Invalid format. Returning to RX.");
      Radio.Rx(0);
      return;
    }

    // CHECK FOR DUPLICATES
    // If the last upload was less than 18 mins ago, skip this section
    if (duplicate(lastUpload)) {
      Serial.println("Duplicate measurement (Last one was < 18 mins ago). Skipping upload.");
      Radio.Rx(0); // Go back to listening
      return;      // Exit the loop iteration here
    }


    // Upload to Firebase
    childCount++;
    timeClient.update();
    
    time_t epochTime = timeClient.getEpochTime();
    struct tm* ptm = gmtime((time_t*)&epochTime);
    String date = String(ptm->tm_year + 1900) + "-" + String(ptm->tm_mon + 1) + "-" + String(ptm->tm_mday);
    String time = timeClient.getFormattedTime();
    lastUpload = timeClient.getEpochTime(); //update lastUpload with last upload time

    String output;
    docOutput["Current Date"] = date;
    docOutput["CurrentTime"] = time;
    docOutput["ReadingValue"] = readingValue;
    docOutput.shrinkToFit();
    serializeJson(docOutput, output);

    // Upload
    fb.setJson(String(childCount), output);
    fb.setInt("/ChildCount", childCount);
    Serial.println("Uploaded to Firebase: " + output);

    // 4. Send ACK
    Serial.println("Sending ACK...");
    // Prepare ACK payload
    sprintf(txpacket, "ACK"); 
    Radio.Send((uint8_t *)txpacket, strlen(txpacket));

    // Wait 5 sec before listening again
    delay(5000);

    Serial.println("Wait complete. Back into RX mode.");
    Radio.Rx(0);
  }
}

// This runs automatically when a packet arrives
void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr) {
  // Copy payload to buffer
  memcpy(rxpacket, payload, size);
  rxpacket[size] = '\0';
  
  // Save to global string for the main loop to handle
  incomingMessage = String(rxpacket);
  
  Serial.printf("\r\nReceived packet \"%s\" with rssi %d\r\n", rxpacket, rssi);
  
  // Set flag so loop() knows to do the Firebase stuff
  packetReceived = true;
  
  // Note: We do NOT restart Radio.Rx(0) here immediately because
  // the main loop needs to process the data and send the ACK first.
}

void OnTxDone() {
  Serial.println("ACK Sent!");
  // Usually we would go back to RX here, but our loop handles the 2 minute delay.
}

void OnTxTimeout() {
  Serial.println("TX Timeout");
}

