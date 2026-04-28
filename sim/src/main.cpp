#include <Arduino.h>
#include <SPI.h>
#include <RadioLib.h>
#include "utilities.h"
#include "lorastruct.h"

// ─── LoRa ────────────────────────────────────────────────────────────────────
Module  radioMod(LORA_CS_PIN, LORA_DIO1_PIN, LORA_RST_PIN, LORA_BUSY_PIN, SPI);
SX1262  lora(&radioMod);

volatile bool radioIRQ = false;
void dio1ISR() { radioIRQ = true; }

// ─── State ───────────────────────────────────────────────────────────────────
static uint8_t  simState       = 0;    // last commanded state we accepted
static uint8_t  lastCmdSeq     = 0;
static uint8_t  metricSeq      = 0;
static uint32_t totalReceived  = 0;
static uint32_t totalCRCErrors = 0;

// ─── Simulated telemetry values (slowly drift to look realistic) ──────────
static uint16_t sim_distance_m = 42;
static uint16_t sim_amps_x10   = 0;
static uint16_t sim_vesc_mV    = 50400;  // 50.4 V

static const char* stateLabel(uint8_t s) {
  switch (s) {
    case 0: return "STOP";
    case 1: return "SPD-1";
    case 2: return "SPD-2";
    case 3: return "SPD-3";
    case 4: return "SPD-4";
    case 5: return "SPD-5";
    case 6: return "SPD-6";
    default: return "???";
  }
}

static const char* lineStateChar(uint8_t s) {
  if (s == 0) return "R";  // Ready
  return "A";              // Armed
}

// ─── Print a separator line ───────────────────────────────────────────────────
static void printSep() {
  Serial.println("─────────────────────────────────────────");
}

// ─── Handle incoming CmdPacket ────────────────────────────────────────────────
static void handleCmd(const CmdPacket* cmd, float rssi, float snr) {
  totalReceived++;
  printSep();
  Serial.printf("▼ CMD RECEIVED  seq:%u  state:%u (%s)\n",
                cmd->seq, cmd->state, stateLabel(cmd->state));
  Serial.printf("  RSSI: %.1f dBm   SNR: %.1f dB\n", rssi, snr);
  Serial.printf("  Total rx: %lu  CRC errors: %lu\n", totalReceived, totalCRCErrors);

  simState   = cmd->state;
  lastCmdSeq = cmd->seq;

  // Send ACK immediately
  AckPacket ack{};
  ack.type = 0x03;
  ack.seq  = cmd->seq;
  int16_t err = lora.transmit(reinterpret_cast<uint8_t*>(&ack), sizeof(ack));
  if (err == RADIOLIB_ERR_NONE) {
    Serial.printf("  ▲ ACK sent      seq:%u\n", ack.seq);
  } else {
    Serial.printf("  ✗ ACK tx failed  code:%d\n", err);
  }
  lora.startReceive();
}

// ─── Send periodic MetricsPacket ─────────────────────────────────────────────
static void sendMetrics() {
  // Drift simulated values
  if (simState > 0) {
    sim_distance_m += 1;
    sim_amps_x10    = simState * 25;  // 2.5 A per speed step
  } else {
    sim_amps_x10 = 0;
  }
  // Slowly drain voltage
  if (sim_vesc_mV > 44000) sim_vesc_mV -= 2;

  MetricsPacket m{};
  m.type        = 0x02;
  m.seq         = ++metricSeq;
  m.lastCmdSeq  = lastCmdSeq;
  m.amps_x10    = sim_amps_x10;
  m.distance_m  = sim_distance_m;
  m.vesc_mV     = sim_vesc_mV;
  m.lineState   = (simState == 0) ? 'R' : 'A';

  int16_t err = lora.transmit(reinterpret_cast<uint8_t*>(&m), sizeof(m));
  if (err == RADIOLIB_ERR_NONE) {
    Serial.printf("▲ METRICS  seq:%u  state:%s  dist:%um  %.1fA  %.2fV\n",
                  m.seq,
                  stateLabel(simState),
                  m.distance_m,
                  m.amps_x10 / 10.0f,
                  m.vesc_mV  / 1000.0f);
  } else {
    Serial.printf("✗ METRICS tx failed  code:%d\n", err);
  }
  lora.startReceive();
}

// ─── setup() ─────────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  delay(300);

  printSep();
  Serial.println("  XIAO WINCH SIMULATOR");
  Serial.println("  Tests remote → winch LoRa link");
  printSep();
  Serial.println("  Listening for CmdPackets (type 0x01)");
  Serial.println("  Responds with AckPacket  (type 0x03)");
  Serial.println("  Sends MetricsPacket      (type 0x02) every 1 s");
  printSep();

  SPI.begin();
  pinMode(LORA_CS_PIN, OUTPUT);
  digitalWrite(LORA_CS_PIN, HIGH);

  Serial.printf("[LoRa] init  %.1f MHz  SF%d  BW%.0f  CR4/%d  %ddBm ...\n",
                LORA_FREQUENCY_MHZ, LORA_SPREADING_FACTOR,
                LORA_BANDWIDTH_KHZ, LORA_CODING_RATE,
                LORA_OUTPUT_POWER_DBM);

  int16_t err = lora.begin(LORA_FREQUENCY_MHZ);
  if (err != RADIOLIB_ERR_NONE) {
    Serial.printf("[LoRa] INIT FAILED  code:%d  — check wiring!\n", err);
    while (true) { delay(1000); }
  }

  lora.setBandwidth(LORA_BANDWIDTH_KHZ);
  lora.setSpreadingFactor(LORA_SPREADING_FACTOR);
  lora.setCodingRate(LORA_CODING_RATE);
  lora.setOutputPower(LORA_OUTPUT_POWER_DBM);
  lora.setCurrentLimit(LORA_CURRENT_LIMIT_MA);
  lora.setDio2AsRfSwitch(true);
  lora.setRfSwitchPins(LORA_RXEN_PIN, RADIOLIB_NC);  // D5 HIGH=RX, LOW=TX
  lora.setDio1Action(dio1ISR);
  lora.startReceive();

  Serial.println("[LoRa] ready — waiting for remote...");
  printSep();
}

// ─── loop() ──────────────────────────────────────────────────────────────────
void loop() {
  // ── Receive ──────────────────────────────────────────────────────────────
  if (radioIRQ) {
    radioIRQ = false;

    uint8_t buf[16];
    size_t  len = sizeof(buf);
    int16_t res = lora.readData(buf, len);

    if (res == RADIOLIB_ERR_NONE && len > 0) {
      if (buf[0] == 0x01 && len >= sizeof(CmdPacket)) {
        handleCmd(reinterpret_cast<CmdPacket*>(buf),
                  lora.getRSSI(), lora.getSNR());
        return;  // ACK sent, startReceive called inside handleCmd
      } else {
        // Unexpected packet type — just log it
        Serial.printf("? UNKNOWN pkt  type:0x%02X  len:%u  RSSI:%.1f  SNR:%.1f\n",
                      buf[0], (unsigned)len, lora.getRSSI(), lora.getSNR());
      }
    } else if (res == RADIOLIB_ERR_CRC_MISMATCH) {
      totalCRCErrors++;
      Serial.printf("✗ CRC error  (total CRC errors: %lu)\n", totalCRCErrors);
    } else {
      Serial.printf("✗ rx error  code:%d\n", res);
    }
    lora.startReceive();
  }

  // ── Periodic metrics ─────────────────────────────────────────────────────
  static uint32_t lastMetrics = 0;
  uint32_t now = millis();
  if (now - lastMetrics >= METRICS_PERIOD_MS) {
    lastMetrics = now;
    sendMetrics();
  }
}
