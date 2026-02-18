// ============================
// Heltec LoRa Ping-Pong + Sensor Burst
// Behavior:
// - Take ONE new sensor reading at the start of each burst.
// - Build ONE payload string from that reading.
// - Send the SAME payload 5 times spaced across BURST_PERIOD_MS.
// - After EACH TX, go into RX waiting for an incoming packet.
// - If an "ACK" is received, immediately restart with a NEW reading/burst.
//
// Sensor update:
// - Reads MPX5050 analog voltage via ESP32 ADC
// - Uses voltage divider undo
// - Uses MPX5050 transfer function: Vout = Vs*(0.018*P + 0.04)
// - Converts kPa -> cmH2O (1 kPa = 10.1972 cmH2O)
//
// NOTE: ESP32 ADC is NOT 5V tolerant. Divider REQUIRED.
// ============================

#include "LoRaWan_APP.h"
#include "Arduino.h"
#include "stdlib.h"
#include <math.h>

// ---------------- LoRa config ----------------
#define RF_FREQUENCY                                915000000 // Hz - change to proper frequency for transmission
#define TX_OUTPUT_POWER                             14        // dBm - change for power output of the radio

#define LORA_BANDWIDTH                              0
#define LORA_SPREADING_FACTOR                       7
#define LORA_CODINGRATE                             1
#define LORA_PREAMBLE_LENGTH                        8
#define LORA_SYMBOL_TIMEOUT                         0
#define LORA_FIX_LENGTH_PAYLOAD_ON                  false
#define LORA_IQ_INVERSION_ON                        false

// Wait for a response after each send (ms)
#define RX_TIMEOUT_VALUE                            1200

#define BUFFER_SIZE                                 64

// ---------------- Burst behavior ----------------
static const uint8_t  BURST_COUNT      = 5;
static const uint32_t BURST_PERIOD_MS  = 300000;              // every 300 seconds
static const uint32_t INTER_SEND_MS    = BURST_PERIOD_MS / 5; // 60000ms
static const uint32_t REC_ACK_WAIT     = 1200000;             // 20 mins

// ---------------- Sensor config ----------------
// Set this to the ADC-capable GPIO you actually wired the divider output to.
#define ANALOG_PIN                                  2  // <-- CHANGE THIS if needed

// Voltage divider values (match your hardware) - ADDED
// Rtop: sensor Vout -> ADC node
// Rbottom: ADC node -> GND
static constexpr float R_TOP_OHMS  = 10000.0f;   // e.g. 10k
static constexpr float R_BOT_OHMS  = 22000.0f;   // e.g. 22k

// MPX5050 supply voltage (the sensor is powered from ~5V)
static constexpr float VS_VOLTS    = 5.0f;

// MPX5050 transfer function constants (datasheet nominal)
static constexpr float A_SLOPE     = 0.018f;  // 1/kPa
static constexpr float B_OFFSET    = 0.04f;   // unitless

// Unit conversion
static constexpr float KPA_TO_CMH2O = 10.1972f;

// Optional: take multiple ADC samples and average to reduce noise
static constexpr int ADC_SAMPLES = 8;

// Moving average settings
static const int numReadings = 10;
float readings[numReadings];
int readIndex = 0;
float total_height = 0.0f;
float average_height = 0.0f;

// NEW: how many valid samples we have so far
uint8_t sampleCount = 0;

// Your calibration/offset params
// trimPressure: additive correction in kPa (set to 0 if you don't want it)
float trimPressure = 0.43f;
// zeroHeight: subtract height offset in cmH2O (your existing behavior)
float zeroHeight   = 2.80f;

float pressure_kpa = 0.0f;
float cm_h2o       = 0.0f;

// ---------------- Radio globals ----------------
char txpacket[BUFFER_SIZE];
char rxpacket[BUFFER_SIZE];
char burstPacket[BUFFER_SIZE];

static RadioEvents_t RadioEvents;

void OnTxDone(void);
void OnTxTimeout(void);
void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr);
void OnRxTimeout(void);

// ---------------- State machine ----------------
typedef enum {
  LOWPOWER,
  STATE_SCHEDULE,
  STATE_RX
} States_t;

States_t state;

int16_t Rssi = 0;
int16_t rxSize = 0;
int16_t txNumber = 0;

// Scheduler variables
uint32_t burstStartMs = 0;
uint8_t  sendIndex = 0;

// ACK-triggered restart
volatile bool ackRestartRequested = false;
#define ACK_TOKEN "ACK"

// ---------------- Sensor helpers ---------------- ADDED
static float readAdcVolts(int pin) {
  // Prefer calibrated read if available in your core
  #if defined(ARDUINO_ARCH_ESP32)
    // Many ESP32 Arduino cores provide analogReadMilliVolts()
    // If yours does, it's usually more accurate than raw scaling.
    #if defined(analogReadMilliVolts)
      uint32_t mv = 0;
      for (int i = 0; i < ADC_SAMPLES; i++) mv += (uint32_t)analogReadMilliVolts(pin);
      mv /= ADC_SAMPLES;
      return (float)mv / 1000.0f;
    #else
      uint32_t raw = 0;
      for (int i = 0; i < ADC_SAMPLES; i++) raw += (uint32_t)analogRead(pin);
      raw /= ADC_SAMPLES;
      raw = constrain((int)raw, 0, 4095);
      return (float)raw * (3.3f / 4095.0f);
    #endif
  #else
    // Fallback
    uint32_t raw = 0;
    for (int i = 0; i < ADC_SAMPLES; i++) raw += (uint32_t)analogRead(pin);
    raw /= ADC_SAMPLES;
    raw = constrain((int)raw, 0, 4095);
    return (float)raw * (3.3f / 4095.0f) + 10;
  #endif
}

// calculate output data ONCE per burst
static void updateSensorAverageOnce() {
  // 1) Read ADC node voltage (after divider)
  float Vadc = readAdcVolts(ANALOG_PIN);

  // 2) Undo the divider to recover sensor Vout
  float divRatio = (R_BOT_OHMS / (R_TOP_OHMS + R_BOT_OHMS));  // ~0.69
  float Vout = Vadc / divRatio;

  // 3) Convert Vout -> pressure using MPX5050 nominal transfer function
  //    P(kPa) = ((Vout/Vs) - B) / A
  float PkPa = ((Vout / VS_VOLTS) - B_OFFSET) / A_SLOPE;

  // Optional clamp (helps if ADC noise makes slightly negative)
  if (PkPa < 0.0f) PkPa = 0.0f;

  // Apply your trim correction
  PkPa += trimPressure;

  pressure_kpa = PkPa;

  // 4) kPa -> cm H2O
  cm_h2o = pressure_kpa * KPA_TO_CMH2O;

  // 5) Moving average ring buffer update
  total_height -= readings[readIndex];
  readings[readIndex] = cm_h2o;
  total_height += readings[readIndex];

  readIndex++;
  if (readIndex >= numReadings) readIndex = 0;

  if (sampleCount < numReadings) sampleCount++;

  average_height = (total_height / sampleCount) - zeroHeight;

  // Debug prints (optional)
  Serial.printf("ADC=%.3f V, Vout=%.3f V, P=%.2f kPa, H=%.2f cmH2O, Avg=%.2f\n",
                Vadc, Vout, pressure_kpa, cm_h2o, average_height);
}

// ---------------- Burst control ----------------
static void startNewBurst() {
  burstStartMs = millis();
  sendIndex = 0;

  updateSensorAverageOnce();

  String dataString = String(average_height, 2);

  txNumber++;

  snprintf(burstPacket, BUFFER_SIZE, "%s", dataString.c_str());

  Serial.printf("\n--- New burst #%d ---\n", txNumber);
  Serial.printf("Average height: %.2f cm H2O\n", average_height);
  Serial.printf("Burst payload: \"%s\"\n", burstPacket);
}

// ---------------- Setup ----------------
void setup() {
  Serial.begin(115200);
  Mcu.begin(HELTEC_BOARD, SLOW_CLK_TPYE);

  // ADC setup (important)
  #if defined(ARDUINO_ARCH_ESP32)
    analogReadResolution(12);
    // 11dB attenuation is the usual setting to allow near 3.3V range
    #if defined(analogSetPinAttenuation)
      analogSetPinAttenuation(ANALOG_PIN, ADC_11db);
    #elif defined(analogSetAttenuation)
      analogSetAttenuation(ADC_11db);
    #endif
  #endif

  // Init readings to 0
  for (int i = 0; i < numReadings; i++) readings[i] = 0.0f;

  RadioEvents.TxDone    = OnTxDone;
  RadioEvents.TxTimeout = OnTxTimeout;
  RadioEvents.RxDone    = OnRxDone;
  RadioEvents.RxTimeout = OnRxTimeout;

  Radio.Init(&RadioEvents);
  Radio.SetChannel(RF_FREQUENCY);

  Radio.SetTxConfig(MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
                    LORA_SPREADING_FACTOR, LORA_CODINGRATE,
                    LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
                    true, 0, 0, LORA_IQ_INVERSION_ON, 3000);

  Radio.SetRxConfig(MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
                    LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH,
                    LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
                    0, true, 0, 0, LORA_IQ_INVERSION_ON, true);

  state = STATE_SCHEDULE;
}

// ---------------- Loop ----------------
void loop() {
  switch (state) {

    case STATE_SCHEDULE: {
      uint32_t now = millis();

      // If ACK asked us to restart, start a new burst immediately
      if (ackRestartRequested) {
        ackRestartRequested = false;
        startNewBurst();
        now = millis();
      }

      // Start first burst or roll every BURST_PERIOD_MS
      if (burstStartMs == 0 || (uint32_t)(now - burstStartMs) >= BURST_PERIOD_MS) {
        startNewBurst();
      }

      // Send up to 5 repeats on schedule
      if (sendIndex < BURST_COUNT) {
        uint32_t nextSendTime = burstStartMs + (sendIndex * INTER_SEND_MS);

        if ((int32_t)(now - nextSendTime) >= 0) {
          snprintf(txpacket, BUFFER_SIZE, "%s", burstPacket);

          Serial.printf("\n-----------------------------------------\n");
          Serial.printf("Attempt %u/%u to send\n", sendIndex + 1, BURST_COUNT);
          Serial.printf("sending packet \"%s\" , length %d\n",
                        txpacket, (int)strlen(txpacket));

          Radio.Send((uint8_t *)txpacket, strlen(txpacket));
          sendIndex++;

          state = LOWPOWER;  // wait for TX done IRQ
        } else {
          state = LOWPOWER;
        }
      } else {
        state = LOWPOWER;
      }
      break;
    }

    case STATE_RX:
      Serial.println("into RX mode");
      Radio.Rx(RX_TIMEOUT_VALUE);
      state = LOWPOWER;
      break;

    case LOWPOWER:
      Radio.IrqProcess();

      // If no IRQ moved us elsewhere, re-enter schedule
      if (state == LOWPOWER) {
        state = STATE_SCHEDULE;
      }
      break;

    default:
      state = STATE_SCHEDULE;
      break;
  }
}

// ---------------- Callbacks ----------------
void OnTxDone(void) {
  Serial.print("TX done......\n");
  state = STATE_RX;   // after every send, go RX
}

void OnTxTimeout(void) {
  Radio.Sleep();
  Serial.print("TX Timeout......\n");
  state = STATE_SCHEDULE;
}

void OnRxTimeout(void) {
  Radio.Sleep();
  Serial.println("RX Timeout......");
  state = STATE_SCHEDULE;
}

void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr) {
  Rssi = rssi;
  rxSize = size;

  if (size >= BUFFER_SIZE) size = BUFFER_SIZE - 1;

  memcpy(rxpacket, payload, size);
  rxpacket[size] = '\0';

  Radio.Sleep();

  Serial.printf("\nreceived packet \"%s\" with Rssi %d , length %d , SNR %d\n",
                rxpacket, Rssi, rxSize, snr);

  // If ACK received, restart with a NEW reading/burst
  if (strcmp(rxpacket, ACK_TOKEN) == 0) {
    Serial.println("ACK received -> waiting for 20 mins -> restarting with a new reading");
    delay(REC_ACK_WAIT);
    ackRestartRequested = true;

    // Clear current burst state so next schedule pass starts immediately
    burstStartMs = 0;
    sendIndex = 0;
  } else {
    Serial.println("back to scheduler");
  }

  state = STATE_SCHEDULE;
}
