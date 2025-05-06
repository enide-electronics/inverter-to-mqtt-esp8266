/*
  GrowattPriorityBatteryFirstACChargerConfigTask.h - Library header for the ESP8266/ESP32 Arduino platform
  This task enables or disables the AC charger for the battery first priority
  It answers on: "settings/priority/bat/ac"
    
  The payload should be in the following format:
  on|off
  
  Written by JF enide.electronics (at) enide.net
  Licensed under GNU GPLv3
*/
#include "GrowattPriorityBatteryFirstACChargerConfigTask.h"

GrowattPriorityBatteryFirstACChargerConfigTask::GrowattPriorityBatteryFirstACChargerConfigTask(ModbusMaster * node, const String &mqttPayload)
{
    this->node = node;
    this->mqttPayload = mqttPayload;
    this->mqttPayload.trim();
}

GrowattPriorityBatteryFirstACChargerConfigTask::~GrowattPriorityBatteryFirstACChargerConfigTask()
{
}

String GrowattPriorityBatteryFirstACChargerConfigTask::subtopic()
{
    return String(F(TOPIC_SETTINGS_PRIORITY_BAT_FIRST_AC_CHARGER_TASK));
}

bool GrowattPriorityBatteryFirstACChargerConfigTask::run()
{
    GLOG::print("GrowattPriorityBatteryFirstACChargerConfigTask::run ");
    GLOG::print((subtopic() + " payload=").c_str());
    GLOG::println(mqttPayload.c_str());
    
    setSuccessful(false);
    
    uint16_t acCharger;
    if (this->mqttPayload == "on" || this->mqttPayload == "true" || this->mqttPayload == "1") {
        acCharger = 1;
    } else if (this->mqttPayload == "off" || this->mqttPayload == "false" || this->mqttPayload == "0") {
        acCharger = 0;
    } else {
        return false;
    }
        
    // set the value we got from mqtt
    this->node->setTransmitBuffer(0, acCharger);
    
    // don't upset the inverter
    delay(1000);
    
    // write back to inverter
    uint8_t result = this->node->writeMultipleRegisters(1092, 1);
    
    response().set((String(subtopic()) + F("/data")).c_str(), (String("addr=1092 ac=") + acCharger).c_str());
    setSuccessful(result == this->node->ku8MBSuccess);

    return isSuccessful();
}

