/*
  GrowattReadHolding.cpp - Library header for the ESP8266/ESP32 Arduino platform
  Reads holding registers and returns them in HEX
  
  Written by JF enide.electronics (at) enide.net
  Licensed under GNU GPLv3
*/
#include "GrowattReadHoldingTask.h"


void append(String &s, uint16_t v, bool appendColon) {
    // leading zero
    if (v < 16) {
        s += '0';
    }
    
    // value
    s += String(v, HEX);
    
    // separator
    if (appendColon) {
        s += ':';
    }
}

GrowattReadHoldingTask::GrowattReadHoldingTask(ModbusMaster * node, uint16_t startAddr, uint8_t length) {
    this->node = node;
    this->addr = startAddr;
    this->length = length;
}

GrowattReadHoldingTask::~GrowattReadHoldingTask() {
}

String GrowattReadHoldingTask::subtopic() {
    return F(TOPIC_SETTINGS_READ_HOLDING_TASK);
}

bool GrowattReadHoldingTask::run() {
    
    GLOG::print(F("GrowattReadHoldingTask::run "));
    GLOG::println((subtopic() + " addr=" + this->addr + " len=" + this->length).c_str());
    
    setSuccessful(false);
    
    // invalid arguments
    if (length > 64 || addr > 1124) {
        return false;
    }
    
    // no length?
    if (length == 0) {
        response().set((String(F(TOPIC_SETTINGS_READ_HOLDING_TASK)) + "/data").c_str(), "");
        setSuccessful(true);
    } else {    
        uint8_t result = this->node->readHoldingRegisters(addr, length);
        
        if (result == this->node->ku8MBSuccess) {
            String holdingInHex;
            
            for (uint8_t i = 0; i < length - 1; i++) {
                append(holdingInHex, node->getResponseBuffer(i), true);
            }
            append(holdingInHex, node->getResponseBuffer(length - 1), false);
            
            response().set((String(F(TOPIC_SETTINGS_READ_HOLDING_TASK)) + F("/data")).c_str(), holdingInHex.c_str());
            setSuccessful(true);
        } else {
            // do nothing, successful is already false
        }
    }
    
    return isSuccessful();
}

