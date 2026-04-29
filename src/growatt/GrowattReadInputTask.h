/*
  GrowattReadInputTask.h - Library header for the ESP8266/ESP32 Arduino platform
  Reads input registers and returns them in HEX
  
  Written by JF enide.electronics (at) enide.net
  Licensed under GNU GPLv3
*/
#ifndef GROWATT_TASK_READ_INPUT_H
#define GROWATT_TASK_READ_INPUT_H

#include "../Task.h"
#include <ModbusMaster.h>

#define TOPIC_SETTINGS_READ_INPUT_TASK "settings/read_input"

class GrowattReadInputTask : public Task {
    private:
        ModbusMaster * node;
        uint16_t addr;
        uint8_t length;
        
    public:
        GrowattReadInputTask(ModbusMaster * node, uint16_t startAddr, uint8_t length);
        virtual ~GrowattReadInputTask();
        virtual String subtopic();
        virtual bool run();
};
#endif
