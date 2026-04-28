#include <M5Stack.h>
#include "LoRaComm.h"
#include "Lorastruct.h"
#include "Display.h"
#include "StepperControl.h"
#include "can.h"

// ---- FreeRTOS SPI Mutex ----
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
SemaphoreHandle_t spiMutex; // Global mutex instance

// ---- Globals/State ----
LoRaComm lora;
Display display;
StepperControl stepper(lora);

volatile int lastLoRaRSSI = 0;
volatile float lastLoRaSNR = 0.0;

extern int32_t can_distance;
extern int32_t can_current;
extern int32_t can_temperature;

bool buttonToggled = false;
int32_t lineOutOffset = 0;

bool armLineStop = false;
bool lineStopActivated = false;
const int armLineStopThreshold = 40;
const int lineStopThreshold = 35;

unsigned long lastButtonPressTime = 0;
int buttonCPressCount = 0;

// Moved these to LoRa task
// static uint8_t lastCmdSeq = 0;
// static uint8_t metricsSeq = 0;
// static unsigned long lastMetrics = 0;
// static unsigned long lastDisplay = 0;
int currentState = 0;

static unsigned long lastDisplay = 0;  // display only

// ------------ LoRa FreeRTOS Task -------------
void LoRaTask(void *pvParameters) {
    static uint8_t lastCmdSeq = 0;
    static uint8_t metricsSeq = 0;
    static unsigned long lastMetrics = 0;

    for (;;) {
        CmdPacket cmd;

        // 1. Handle incoming commands
        if (lora.receiveCmd(cmd)) {
            currentState = cmd.state;
            lastCmdSeq = cmd.seq;

            // 🟢 Only allow state change from remote if NOT lineStopActivated
            if (!lineStopActivated) {
                currentState = cmd.state;
            } else {
                Serial.println("[LoRaTask] LineStop active - ignoring remote state change!");
            }

            // Send ACK
            AckPacket ack;
            ack.type = 0x03;
            ack.seq  = cmd.seq;
            ack.pad0 = 0;
            ack.pad1 = 0;
            lora.sendAck(ack);

            // Send metrics immediately after command
            MetricsPacket m;
            m.type        = 0x02;
            m.seq         = metricsSeq++;
            m.lastCmdSeq  = lastCmdSeq;
            m.amps_x10    = can_current * 10;
            m.distance_m  = abs(can_distance - lineOutOffset);
            m.vesc_mV     = 3700; // Replace with real value if available

            // --- State mapping for remote ---
            if (lineStopActivated) {
                m.lineState = 'S';    // Stopped
            } else if (armLineStop) {
                m.lineState = 'A';    // Armed
            } else {
                m.lineState = 'R';    // Ready
            }

            // Debug: print the outgoing packet
            uint8_t* p = (uint8_t*)&m;
            Serial.print("[LoRaTask] Raw metrics bytes (after cmd): ");
            for (int i = 0; i < sizeof(MetricsPacket); ++i) {
                Serial.printf("%02X ", p[i]);
            }
            Serial.println();

            lora.sendMetrics(m);
            lastMetrics = millis();

            // 🟢 Update RSSI/SNR for local display
            lastLoRaRSSI = lora.getLastRSSI();
            lastLoRaSNR  = lora.getLastSNR();
        }

        // 2. Send periodic metrics
        if (millis() - lastMetrics > METRICS_PERIOD_MS) {
            MetricsPacket m;
            m.type        = 0x02;
            m.seq         = metricsSeq++;
            m.lastCmdSeq  = lastCmdSeq;
            m.amps_x10    = can_current * 10;
            m.distance_m  = abs(can_distance - lineOutOffset);
            m.vesc_mV     = 3700;

            if (lineStopActivated) {
                m.lineState = 'S';
            } else if (armLineStop) {
                m.lineState = 'A';
            } else {
                m.lineState = 'R';
            }

            uint8_t* p = (uint8_t*)&m;
            Serial.print("[LoRaTask] Raw metrics bytes (periodic): ");
            for (int i = 0; i < sizeof(MetricsPacket); ++i) {
                Serial.printf("%02X ", p[i]);
            }
            Serial.println();

            lora.sendMetrics(m);
            lastMetrics = millis();

            // 🟢 Update RSSI/SNR for local display
            lastLoRaRSSI = lora.getLastRSSI();
            lastLoRaSNR  = lora.getLastSNR();
        }

        vTaskDelay(1); // Always yield to other tasks
    }
}




// ------------ Setup & Main Loop --------------

void setup() {
    Serial.begin(115200);
    Serial.println("Starting setup");

    spiMutex = xSemaphoreCreateMutex();   // CREATE MUTEX FIRST
    Serial.println("Mutex created");

    M5.begin();
    Serial.println("M5Stack initialized");

    lora.init();
    Serial.println("LoRa initialized");

    // ---- Start LoRa task on core 1 with high priority ----
    xTaskCreatePinnedToCore(
        LoRaTask,        // Task function
        "LoRaTask",      // Name
        4096,            // Stack size
        NULL,            // Param
        2,               // Priority (2=high, 1=normal, 0=idle)
        NULL,            // Task handle (not used)
        1                // Core 1 (best for ESP32, keeps WiFi/BT on core 0)
    );
    Serial.println("LoRaTask started");

    display.init();
    Serial.println("Display initialized");

    stepper.setup();
    Serial.println("Stepper initialized");

    can_setup();
    Serial.println("CAN initialized");
}

void loop() {
    // --- Button C: Reset/Deactivate line stop ---
    if (M5.BtnC.wasPressed()) {
        unsigned long currentTime = millis();
        if (currentTime - lastButtonPressTime < 500) {
            buttonCPressCount++;
        } else {
            buttonCPressCount = 1;
        }
        lastButtonPressTime = currentTime;

        if (buttonCPressCount == 2) {
            lineStopActivated = false;
            armLineStop = false;
            Serial.println("LineStop deactivated");
        } else {
            lineOutOffset = can_distance;
            Serial.println("LineOut reset");
        }
    }

    // --- Calculate adjusted LineOut value ---
    int32_t adjustedLineOut = abs(can_distance - lineOutOffset);

    // --- Arm/Trigger line stop ---
    if (!armLineStop && adjustedLineOut > armLineStopThreshold) {
        armLineStop = true;
        Serial.println("LineStop armed");
    }
    if (armLineStop && !lineStopActivated && adjustedLineOut < lineStopThreshold) {
        lineStopActivated = true;
        Serial.println("LineStop activated");
    }

        // 🟢 --- Failsafe: Force STOP if lineStopActivated ---
    if (lineStopActivated) {
        currentState = 0;
    }

    // --- Only update display if something changed ---
    static int lastDisplayedState = -1;
    static int lastDisplayedLineOut = -1;
    static int lastDisplayedCurrent = -9999;
    static bool lastDisplayedLineStopArmed = false;
    static bool lastDisplayedLineStopActivated = false;
    static int lastDisplayedTemperature = -9999;
    static int lastDisplayedRSSI = 12345;    // impossible value to force update first loop
    static float lastDisplayedSNR = 12345.0; // impossible value to force update first loop
    //Serial.printf("[Main] armLineStop=%d, lineStopActivated=%d\n", armLineStop, lineStopActivated);


    if (
        currentState != lastDisplayedState ||
        adjustedLineOut != lastDisplayedLineOut ||
        can_current != lastDisplayedCurrent ||
        armLineStop != lastDisplayedLineStopArmed ||
        lineStopActivated != lastDisplayedLineStopActivated ||
        can_temperature != lastDisplayedTemperature ||
        lastLoRaRSSI != lastDisplayedRSSI ||
        lastLoRaSNR != lastDisplayedSNR
    ) {
        display.updateDisplay(
            currentState,
            lastLoRaRSSI,
            lastLoRaSNR,
            adjustedLineOut,
            can_current,
            armLineStop,
            lineStopActivated,
            can_temperature
        );
        lastDisplayedState = currentState;
        lastDisplayedLineOut = adjustedLineOut;
        lastDisplayedCurrent = can_current;
        lastDisplayedLineStopArmed = armLineStop;
        lastDisplayedLineStopActivated = lineStopActivated;
        lastDisplayedTemperature = can_temperature;
        lastDisplayedRSSI = lastLoRaRSSI;
        lastDisplayedSNR = lastLoRaSNR;
    }

    // --- Button A: Manual toggle stepper ---
    if (M5.BtnA.wasPressed()) {
        buttonToggled = !buttonToggled;
    }

    // --- Stepper logic ---
    if (buttonToggled) {
        stepper.runStepper();
    } else {
        if (currentState == 0) {
            stepper.stopAndCenter();
        } else {
            stepper.runStepper();
        }
    }
    stepper.update();

    // --- CAN ---
    can_send_message();
    can_receive_message();

    M5.update();
    delay(5); // keep loop responsive

    // (Optional: loop frequency debug)
    /*
    static unsigned long lastDebug = 0;
    static int loopCount = 0;
    loopCount++;
    if (millis() - lastDebug > 1000) {
        Serial.printf("Loops per second: %d\n", loopCount);
        loopCount = 0;
        lastDebug = millis();
    }
    */
}
