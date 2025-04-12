/*
  GrowattPriorityConfigSetOneRegisterTask.h - Library header for the ESP8266/ESP32 Arduino platform
  Abstract class that sets one register to a specific value
  
  The payload should be in integer format:
  0..NNNN
  
  When overriding this class the user should implement getAddress and isPayloadValid
  
  Written by JF enide.electronics (at) enide.net
  Licensed under GNU GPLv3
*/
#ifndef GROWATT_PRIORITY_CONFIG_SET_ONE_REG_TASK_H
#define GROWATT_PRIORITY_CONFIG_SET_ONE_REG_TASK_H

#include "../Task.h"
#include <ModbusMaster.h>

class GrowattPriorityConfigSetOneRegisterTask : public Task {
    private:
        ModbusMaster * node;
    
    protected:
        String priorityName;
        String configName;
        String mqttPayload;

        // getAddress should return the address or 0xffff is not supported
        virtual uint16_t getAddress() const = 0;
        // isValid should do a sanity check of the value from mqttPayload
        virtual bool isValid(uint16_t value) const = 0;
        
    public:
        GrowattPriorityConfigSetOneRegisterTask(ModbusMaster * node, const String &priorityName, const String &configName, const String &mqttPayload);
        virtual ~GrowattPriorityConfigSetOneRegisterTask();
        virtual String subtopic();
        virtual bool run();
};

#endif