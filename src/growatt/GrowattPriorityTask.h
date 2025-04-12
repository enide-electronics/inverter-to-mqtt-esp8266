/*
  GrowattPriorityTask.h - Library header for the ESP8266/ESP32 Arduino platform
  Changes the inverter priority to Load First, Battery First or Grid First
  
  Written by JF enide.electronics (at) enide.net
  Licensed under GNU GPLv3
*/
#include "../Task.h"
#include "GrowattPriorityTaskCommon.h"
#include <ModbusMaster.h>

class GrowattPriorityTask : public Task {
    /*
     * 0 Load First
     * 1 Bat First
     * 2 Grid First
     */
    private:
        ModbusMaster * node;
        String mqttValue;
        bool checkAndSetEnableBit(uint16_t startAddr, uint8_t bitValue);
        bool readPriorityStatus();
        
    public:
        GrowattPriorityTask(ModbusMaster * node, const String &mqttValue);
        virtual ~GrowattPriorityTask();

        virtual String subtopic();
        virtual bool run();
};