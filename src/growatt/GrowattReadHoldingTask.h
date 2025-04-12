/*
  GrowattReadHolding.h - Library header for the ESP8266/ESP32 Arduino platform
  Reads holding registers and returns them in HEX
  
  Written by JF enide.electronics (at) enide.net
  Licensed under GNU GPLv3
*/
#ifndef GROWATT_TASK_READ_HOLDING_H
#define GROWATT_TASK_READ_HOLDING_H

#include "../Task.h"
#include <ModbusMaster.h>

#define TOPIC_SETTINGS_READ_HOLDING_TASK "settings/read_holding"

class GrowattReadHoldingTask : public Task {
    private:
        ModbusMaster * node;
        uint16_t addr;
        uint8_t length;
        
    public:
        GrowattReadHoldingTask(ModbusMaster * node, uint16_t startAddr, uint8_t length);
        virtual ~GrowattReadHoldingTask();
        virtual String subtopic();
        virtual bool run();
};
#endif