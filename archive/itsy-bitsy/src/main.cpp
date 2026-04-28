#include <Arduino.h>
#include <bluefruit.h>

#define SERVICE_UUID "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHAR_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHAR_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"

BLEService winchService(SERVICE_UUID);
BLECharacteristic txChar(CHAR_UUID_TX);
BLECharacteristic rxChar(CHAR_UUID_RX);

// Dummy data variables
uint8_t state = 1;
uint16_t lineDistance = 0;
uint8_t lineState = 0; // 0=Ready, 1=Armed, 2=Stopped
uint16_t amps = 0;
uint8_t tempController = 20;
int8_t rssi = -60;
uint16_t lostPackages = 0;
uint8_t winchBattery = 100;
uint8_t remoteBattery = 85;
bool remoteCharging = false;

void rxCallback(uint16_t conn_hdl, BLECharacteristic* chr, uint8_t* data, uint16_t len);
void updateDummyData();
void sendTelemetry();

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);
  
  Serial.println("=== Winch Remote Starting ===");
  
  Bluefruit.begin();
  Bluefruit.setName("Winch");
  
  char name[32];
  Bluefruit.getName(name, sizeof(name));
  Serial.print("Device name: ");
  Serial.println(name);
  
  Bluefruit.setTxPower(4);
  
  winchService.begin();
  
  txChar.setProperties(CHR_PROPS_NOTIFY);
  txChar.setPermission(SECMODE_OPEN, SECMODE_NO_ACCESS);
  txChar.setFixedLen(20);
  txChar.begin();
  
  rxChar.setProperties(CHR_PROPS_WRITE);
  rxChar.setPermission(SECMODE_NO_ACCESS, SECMODE_OPEN);
  rxChar.setWriteCallback(rxCallback);
  rxChar.setFixedLen(20);
  rxChar.begin();
  
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();
  Bluefruit.Advertising.addService(winchService);
  Bluefruit.Advertising.addName();
  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.setInterval(160, 244);
  Bluefruit.Advertising.setFastTimeout(30);
  Bluefruit.Advertising.start(0);
  
  Serial.println("Advertising started");
  Serial.print("Service UUID: ");
  Serial.println(SERVICE_UUID);
}

void rxCallback(uint16_t conn_hdl, BLECharacteristic* chr, uint8_t* data, uint16_t len) {
  Serial.print("RX: ");
  for(uint16_t i=0; i<len; i++) {
    Serial.print(data[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
}

void loop() {
  static uint32_t lastUpdate = 0;
  
  if (millis() - lastUpdate > 500) { // Update twice per second
    updateDummyData();
    sendTelemetry();
    lastUpdate = millis();
  }
}

void updateDummyData() {
  // Slowly increment/cycle values
  static uint32_t counter = 0;
  counter++;
  
  // State cycles 1-5 every 50 seconds
  state = ((counter / 100) % 5) + 1;
  
  // Line distance increases 0-1500m
  lineDistance = (counter * 5) % 1500;
  
  // Line state cycles: Ready -> Armed -> Stopped
  lineState = (counter / 20) % 3;
  
  // Amps vary 0-300A
  amps = (counter * 3) % 300;
  
  // Temp increases slowly 20-80°C
  tempController = 20 + ((counter / 10) % 60);
  
  // RSSI varies -40 to -80
  rssi = -40 - ((counter / 5) % 40);
  
  // Lost packages increments slowly
  lostPackages = counter / 20;
  
  // Winch battery slowly drains 100% -> 0%
  winchBattery = 100 - ((counter / 10) % 100);
  
  // Remote battery varies
  remoteBattery = 50 + ((counter / 3) % 50);
  
  // Charging toggles every 30 seconds
  remoteCharging = (counter / 60) % 2;
}

void sendTelemetry() {
  if (!Bluefruit.connected()) return;
  
  // Pack data into 20 bytes
  uint8_t data[20];
  
  data[0] = state;                      // byte 0: State (1-5)
  data[1] = (lineDistance >> 8) & 0xFF; // byte 1-2: Line Distance (uint16)
  data[2] = lineDistance & 0xFF;
  data[3] = lineState;                  // byte 3: Line State (0-2)
  data[4] = (amps >> 8) & 0xFF;        // byte 4-5: Amps (uint16)
  data[5] = amps & 0xFF;
  data[6] = tempController;             // byte 6: Temp (uint8)
  data[7] = (uint8_t)rssi;             // byte 7: RSSI (int8 cast to uint8)
  data[8] = (lostPackages >> 8) & 0xFF; // byte 8-9: Lost Packages (uint16)
  data[9] = lostPackages & 0xFF;
  data[10] = winchBattery;              // byte 10: Winch Battery %
  data[11] = remoteBattery;             // byte 11: Remote Battery %
  data[12] = remoteCharging ? 1 : 0;    // byte 12: Charging flag
  // bytes 13-19: Reserved for future use
  
  txChar.notify(data, 20);
  
  // Debug output
  Serial.printf("State:%d Dist:%dm LineState:%d Amps:%dA Temp:%dC RSSI:%ddBm Lost:%d WinchBat:%d%% RemBat:%d%% Charging:%d\n",
                state, lineDistance, lineState, amps, tempController, rssi, lostPackages, 
                winchBattery, remoteBattery, remoteCharging);
}