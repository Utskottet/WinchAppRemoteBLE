#pragma once
#include <RadioLib.h>
#include "lorastruct.h"

int getLastRSSI();
float getLastSNR();

class LoRaComm {
public:
    void init();
    bool receiveCmd(CmdPacket& outCmd);
    void sendMetrics(const MetricsPacket& m);
    void sendAck(const AckPacket& ack);
    int getLastRSSI();
    float getLastSNR();
private:
    SX1262* lora = nullptr;
};
