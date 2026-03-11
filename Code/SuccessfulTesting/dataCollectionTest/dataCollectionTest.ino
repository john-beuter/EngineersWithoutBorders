// MPX5050DP quick ADC debug (ESP32-S3 / Heltec V3)
// IMPORTANT: Use a free ADC1 pin (e.g., GPIO2/3/4/5/6/7), NOT GPIO1 (VBAT sense).

#include <Arduino.h>

static const int ADC_PIN = 2;          // <-- CHANGE THIS to the GPIO you used (2/3/4/5/6/7 recommended)
static const float VS = 5.0f;          // sensor supply voltage (measure it!)
static const float RTOP = 10000.0f;    // divider top resistor (ohms)
static const float RBOT = 22000.0f;    // divider bottom resistor (ohms)

static inline float dividerRatio() {
  return (RTOP + RBOT) / RBOT;         // Vout = Vadc * (Rtop+Rbot)/Rbot
}

void setup() {
  Serial.begin(115200);
  delay(500);

  analogReadResolution(12);

  // On ESP32 family, this sets input range; 11dB is typical for ~0–3.3V
  analogSetPinAttenuation(ADC_PIN, ADC_11db);

  Serial.println("Starting MPX5050DP debug...");
}

void loop() {
  int raw = analogRead(ADC_PIN) + 1; //currently reading max = 4095

  // ESP32 Arduino often assumes ~3.3V reference for conversion
  float vadc = (raw / 4095.0f) * 3.3f;

  // Back-calculate sensor output before divider:   
  float vout = vadc * dividerRatio();

  // Datasheet transfer: Vout = VS * (0.018*P + 0.04)
  // => P(kPa) = (Vout/VS - 0.04) / 0.018
  float p_kpa = (vout / VS - 0.04f) / 0.018f;

  // 1 kPa = 10.1972 cmH2O
  float p_cmh2o = (p_kpa * 10.1972f)+5;

  Serial.print("raw="); Serial.print(raw);
  Serial.print("  Vadc="); Serial.print(vadc, 3);
  Serial.print("V  Vout="); Serial.print(vout, 3);
  Serial.print("V  P="); Serial.print(p_kpa, 3);
  Serial.print("kPa  "); Serial.print(p_cmh2o, 2);
  Serial.println(" cmH2O");

  delay(200);
}
