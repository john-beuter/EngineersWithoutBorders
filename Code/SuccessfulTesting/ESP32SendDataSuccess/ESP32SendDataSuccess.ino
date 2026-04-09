// file - preferences - additional board managers URLS - https://espressif.github.io/arduino-esp32/package_esp32_index.json
// boards - esp32 by espressif systems
// libraries - heltec esp32 dev-boards by Heltec
// Selecting board - Heltec Wifi LoRa 32 (V3)

// ============================
// Heltec LoRa Ping-Pong + Sensor Burst
// Behavior:
// - Take ONE new sensor reading at the start of each burst.
// - Build ONE payload string from that reading.
// - Send the SAME payload 5 times spaced across BURST_PERIOD_MS.
// - After EACH TX, go into RX waiting for an incoming packet.
// - If an "ACK" is received, immediately restart with a NEW reading/burst.
// ============================

#include "LoRaWan_APP.h"
#include "Arduino.h"
#include "stdlib.h"
#include <string.h>

// ---------------- LoRa config ----------------
#define RF_FREQUENCY                                915000000 // Hz
#define TX_OUTPUT_POWER                             21        // dBm

#define LORA_BANDWIDTH                              0
#define LORA_SPREADING_FACTOR                       12
#define LORA_CODINGRATE                             1
#define LORA_PREAMBLE_LENGTH                        8
#define LORA_SYMBOL_TIMEOUT                         0
#define LORA_FIX_LENGTH_PAYLOAD_ON                  false
#define LORA_IQ_INVERSION_ON                        false

// Wait for a response after each send (ms)
#define RX_TIMEOUT_VALUE                            1200

#define BUFFER_SIZE                                 64

// ---------------- Burst behavior ----------------
// Production version
// static const uint8_t  BURST_COUNT      = 5;
// static const uint32_t BURST_PERIOD_MS  = 300000;              // every 300 seconds
// static const uint32_t INTER_SEND_MS    = BURST_PERIOD_MS / 5; // 60000 ms
// static const uint32_t REC_ACK_WAIT     = 1200000;             // 20 mins

// Testing version
static const uint8_t  BURST_COUNT      = 5;
static const uint32_t BURST_PERIOD_MS  = 50000;               // every 50 seconds
static const uint32_t INTER_SEND_MS    = BURST_PERIOD_MS / 5; // 10 seconds
static const uint32_t REC_ACK_WAIT     = 30000;               // 30 secs

// ---------------- Sensor config ----------------
// IMPORTANT: Use a free ADC1 pin (e.g. GPIO2/3/4/5/6/7), NOT GPIO1
static const int   ADC_PIN = 3;         // <-- set your actual ADC pin
static const float VS      = 5.0f;      // sensor supply voltage (measure if possible)
static const float RTOP    = 10000.0f;  // divider top resistor (sensor -> ADC node)
static const float RBOT    = 22000.0f;  // divider bottom resistor (ADC node -> GND)

// If you want to use measured zero calibration, leave this true.
// If you want the exact same behavior as your quick debug sketch, set false.
static const bool USE_AUTO_ZERO = false;

// 1 kPa = 10.1972 cmH2O
static const float KPA_TO_CMH2O = 10.1972f;

// Your displayed offset from previous working code
static const float CMH2O_OFFSET = 6.5f;

static inline float dividerRatio() {
  // Vout = Vadc * (RTOP + RBOT) / RBOT
  return (RTOP + RBOT) / RBOT;
}

static float vout0 = 0.0f;          // measured Vout at zero differential pressure
static float average_height = 0.0f; // value that gets transmitted

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

// ---------------- Sensor helpers ----------------
static float readVoutOnce() {
  int raw = analogRead(ADC_PIN);
  float vadc = (raw / 4095.0f) * 3.3f;
  float vout = vadc * dividerRatio();
  return vout;
}

static void updateSensorAverageOnce() {
  int raw = analogRead(ADC_PIN);
  float vadc = (raw / 4095.0f) * 3.3f;
  float vout = vadc * dividerRatio();

  float p_kpa;

  if (USE_AUTO_ZERO) {
    // Uses measured zero point from setup()
    // At zero pressure: delta Vout = 0
    // Vout change per kPa = VS * 0.018
    p_kpa = (vout - vout0) / (VS * 0.018f);
  } else {
    // Same equation as your known-good debug code
    p_kpa = (vout / VS - 0.04f) / 0.018f;
  }

  // IMPORTANT: update the GLOBAL average_height, not a local variable
  average_height = (p_kpa * KPA_TO_CMH2O) + CMH2O_OFFSET;

  Serial.print("raw="); Serial.print(raw);
  Serial.print("  Vadc="); Serial.print(vadc, 3);
  Serial.print("V  Vout="); Serial.print(vout, 3);
  if (USE_AUTO_ZERO) {
    Serial.print("V  vout0="); Serial.print(vout0, 3);
  }
  Serial.print("V  P="); Serial.print(p_kpa, 3);
  Serial.print(" kPa  ");
  Serial.print(average_height, 2);
  Serial.println(" cmH2O");
}

// ---------------- Burst control ----------------
static void startNewBurst() {
  burstStartMs = millis();
  sendIndex = 0;

  // Take one fresh reading for this burst
  updateSensorAverageOnce();

  String dataString = String(average_height, 2);

  txNumber++;

  snprintf(burstPacket, BUFFER_SIZE, "%s", dataString.c_str());

  Serial.printf("\n--- New burst #%d ---\n", txNumber);
  Serial.printf("Average height: %.2f cmH2O\n", average_height);
  Serial.printf("Burst payload: \"%s\"\n", burstPacket);
}

// ---------------- Setup ----------------
void setup() {
  Serial.begin(115200);
  delay(500);

  Mcu.begin(HELTEC_BOARD, SLOW_CLK_TPYE);

  // ADC setup
  analogReadResolution(12);
  analogSetPinAttenuation(ADC_PIN, ADC_11db);

  Serial.println("Starting MPX5050DP debug...");

  // Optional auto-zero calibration
  if (USE_AUTO_ZERO) {
    Serial.println("Auto-zeroing...");
    const int N = 200;
    float sum = 0.0f;

    for (int i = 0; i < N; i++) {
      sum += readVoutOnce();
      delay(5);
    }

    vout0 = sum / N;

    Serial.print("Auto-zero done. vout0 = ");
    Serial.print(vout0, 4);
    Serial.print(" V, ideal zero point = ");
    Serial.print(0.04f * VS, 4);
    Serial.println(" V");
  } else {
    Serial.println("Auto-zero disabled. Using datasheet equation directly.");
  }

  // Radio event handlers
  RadioEvents.TxDone    = OnTxDone;
  RadioEvents.TxTimeout = OnTxTimeout;
  RadioEvents.RxDone    = OnRxDone;
  RadioEvents.RxTimeout = OnRxTimeout;

  // Radio init
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

      // Send up to BURST_COUNT repeats on schedule
      if (sendIndex < BURST_COUNT) {
        uint32_t nextSendTime = burstStartMs + (sendIndex * INTER_SEND_MS);

        if ((int32_t)(now - nextSendTime) >= 0) {
          snprintf(txpacket, BUFFER_SIZE, "%s", burstPacket);

          Serial.printf("\n-----------------------------------------\n");
          Serial.printf("Attempt %u/%u to send\n", sendIndex + 1, BURST_COUNT);
          Serial.printf("sending packet \"%s\", length %d\n",
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
      Serial.println("Entering RX mode...");
      Radio.Rx(RX_TIMEOUT_VALUE);
      state = LOWPOWER;
      break;

    case LOWPOWER:
      Radio.IrqProcess();

      // If no IRQ moved us elsewhere, re-enter scheduler
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
  Serial.println("TX done...");
  state = STATE_RX;   // after every send, go RX
}

void OnTxTimeout(void) {
  Radio.Sleep();
  Serial.println("TX Timeout...");
  state = STATE_SCHEDULE;
}

void OnRxTimeout(void) {
  Radio.Sleep();
  Serial.println("RX Timeout...");
  state = STATE_SCHEDULE;
}

void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr) {
  Rssi = rssi;
  rxSize = size;

  if (size >= BUFFER_SIZE) {
    size = BUFFER_SIZE - 1;
  }

  memcpy(rxpacket, payload, size);
  rxpacket[size] = '\0';

  Radio.Sleep();

  Serial.printf("\nreceived packet \"%s\" with RSSI %d, length %d, SNR %d\n",
                rxpacket, Rssi, rxSize, snr);

  // If ACK received, restart with a NEW reading/burst
  if (strcmp(rxpacket, ACK_TOKEN) == 0) {
    Serial.println("ACK received -> waiting -> restarting with a new reading");
    delay(REC_ACK_WAIT);
    ackRestartRequested = true;

    // Clear current burst state so next schedule pass starts immediately
    burstStartMs = 0;
    sendIndex = 0;
  } else {
    Serial.println("No ACK token, back to scheduler");
  }

  state = STATE_SCHEDULE;
}