#include <ESP8266WiFi.h>
#include <Firebase.h>
#include <NTPClient.h>
#include <WiFiClientSecure.h>
#include <WiFiUdp.h>

// Replace with your network credentials
const char* ssid = "ICS The Foundry";
const char* password = "v724ydArhp";

// Firebase project credentials
#define FIREBASE_HOST "https://boreholemonitoring-9c1d4-default-rtdb.firebaseio.com/"
#define TIMEOFFSET -21600
const unsigned long interval = 20000;

#include <ArduinoJson.h>

/* Use the following instance for Test Mode (No Authentication) */
Firebase fb(FIREBASE_HOST);
/* Use the following instance for Locked Mode (With Authentication) */
// Firebase fb(REFERENCE_URL, AUTH_TOKEN);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", TIMEOFFSET);
JsonDocument docOutput;
unsigned long previousMillis = 0;
unsigned int currentCount;

void setup() {
  Serial.begin(115200);
  #if !defined(ARDUINO_UNOWIFIR4)
    WiFi.mode(WIFI_STA);
  #else
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);
  #endif
  WiFi.disconnect();
  delay(1000);

  /* Connect to WiFi */
  Serial.println();
  Serial.println();
  Serial.print("Connecting to: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print("-");
    delay(500);
  }

  Serial.println();
  Serial.println("WiFi Connected");
  Serial.println();
  timeClient.begin();

  #if defined(ARDUINO_UNOWIFIR4)
    digitalWrite(LED_BUILTIN, HIGH);
  #endif

  currentCount = fb.getInt("ChildCount");
  Serial.println("Current Key Val" + currentCount);

}

void loop() {
  unsigned long currentMillis = millis();
  if(currentMillis - previousMillis >= interval){
    previousMillis = currentMillis;
    long startTime = millis();

    currentCount++;
    timeClient.update();
    time_t epochTime = timeClient.getEpochTime();
    struct tm *ptm = gmtime ((time_t *) &epochTime);
    int monthDay = ptm->tm_mday;
    int currentMonth = ptm->tm_mon+1;
    int currentYear = ptm->tm_year+1900;
    String currentDate = String(currentYear) + "-" + String(currentMonth) + "-" + String(monthDay);
    String output;
    docOutput["Current Date"] = currentDate;
    docOutput["CurrentTime"] = timeClient.getFormattedTime();
    docOutput["ReadingValue"] = ((random(0,99) / 100.0) + random(20, 90));
    docOutput.shrinkToFit();
    serializeJson(docOutput, output);
    fb.setJson(String(currentCount), output);
    fb.setInt("/ChildCount", currentCount);
    Serial.println(timeClient.getFormattedTime());
    Serial.println(currentDate);
    Serial.println(currentCount);

    long endTime = millis();
    unsigned long timeTaken = endTime - startTime;
    unsigned long adjustedDelay = interval - timeTaken;
    if(adjustedDelay > 0){
      delay(adjustedDelay);
    }
  }
}
