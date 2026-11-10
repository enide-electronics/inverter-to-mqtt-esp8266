/*
  GrowattReadInputTask.cpp - Library source for the ESP8266/ESP32 Arduino platform
  Reads input registers and returns them in HEX
  
  Written by JF enide.electronics (at) enide.net
  Licensed under GNU GPLv3
*/
#include "GrowattReadInputTask.h"
#include "GrowattUtils.h"

GrowattReadInputTask::GrowattReadInputTask(ModbusMaster * node, uint16_t startAddr, uint8_t length) {
    this->node = node;
    this->addr = startAddr;
    this->length = length;
}

GrowattReadInputTask::~GrowattReadInputTask() {
}

String GrowattReadInputTask::subtopic() {
    return F(TOPIC_SETTINGS_READ_INPUT_TASK);
}

bool GrowattReadInputTask::run() {
    
    GLOG::print(F("GrowattReadInputTask::run "));
    GLOG::println((subtopic() + F(" addr=") + this->addr + F(" len=") + this->length).c_str());
    
    setSuccessful(false);
    
    // invalid arguments
    if (length > 64 || addr > 3000) {
        return false;
    }
    
    // no length?
    if (length == 0) {
        response().set((String(F(TOPIC_SETTINGS_READ_INPUT_TASK)) + F("/data")).c_str(), "");
        setSuccessful(true);
    } else {    
        uint8_t result = this->node->readInputRegisters(addr, length);
        
        if (result == this->node->ku8MBSuccess) {
            String inputInHex;
            
            for (uint8_t i = 0; i < length - 1; i++) {
                GrowattUtils::appendHex(inputInHex, node->getResponseBuffer(i), true);
            }
            GrowattUtils::appendHex(inputInHex, node->getResponseBuffer(length - 1), false);
            
            response().set((String(F(TOPIC_SETTINGS_READ_INPUT_TASK)) + F("/data")).c_str(), inputInHex.c_str());
            setSuccessful(true);
        } else {
            // do nothing, successful is already false
        }
    }
    
    return isSuccessful();
}
