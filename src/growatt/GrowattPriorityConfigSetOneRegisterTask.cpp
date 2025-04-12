/*
  GrowattPriorityConfigSetOneRegisterTask.cpp - Library header for the ESP8266/ESP32 Arduino platform
  Abstract class that sets one register to a specific value
  
  The payload should be in integer format:
  0..NNNN
  
  Written by JF enide.electronics (at) enide.net
  Licensed under GNU GPLv3
*/

#include "GrowattPriorityConfigSetOneRegisterTask.h"
#include "../GLog.h"
     
GrowattPriorityConfigSetOneRegisterTask::GrowattPriorityConfigSetOneRegisterTask(ModbusMaster * node, const String &priorityName, const String &configName, const String &mqttPayload)
{
    this->node = node;
    this->priorityName = priorityName;
    this->configName = configName;
    this->mqttPayload = mqttPayload;
    this->mqttPayload.trim();
}

GrowattPriorityConfigSetOneRegisterTask::~GrowattPriorityConfigSetOneRegisterTask()
{
}

String GrowattPriorityConfigSetOneRegisterTask::subtopic()
{
    return String(F("settings/priority/")) + this->priorityName + "/" + this->configName;
}

bool GrowattPriorityConfigSetOneRegisterTask::run()
{
    GLOG::print("GrowattPriorityConfigSetOneRegisterTask::run ");
    GLOG::print((subtopic() + " payload=").c_str());
    GLOG::println(mqttPayload.c_str());
    
    setSuccessful(false);
    
    // value to set should be an integer
    uint16_t intValue = atoi(mqttPayload.c_str());
    
    uint16_t address = getAddress();
    if (address != 0xffff && isValid(intValue)) {
        
        // put the value we got from mqtt on tx buffer
        this->node->setTransmitBuffer(0, intValue);
        
        // write back to inverter
        uint8_t result = this->node->writeMultipleRegisters(address, 1);
        
        response().set((String(subtopic()) + F("/data")).c_str(), (String("addr=") + address + " " + this->configName + "=" + intValue).c_str());
        setSuccessful(result == this->node->ku8MBSuccess);
    }
    
    return isSuccessful();
}

