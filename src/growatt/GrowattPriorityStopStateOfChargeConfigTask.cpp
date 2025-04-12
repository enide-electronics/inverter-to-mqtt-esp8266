/*
  GrowattPriorityStopStateOfChargeConfigTask.h - Library header for the ESP8266/ESP32 Arduino platform
  This task set the SSOC value for Grid First or Battery First
  It answers on: 
    "settings/priority/bat/ssoc"
    "settings/priority/grid/ssoc"
    
  The payload should be in integer format:
  1..100
  
  Written by JF enide.electronics (at) enide.net
  Licensed under GNU GPLv3
*/

#include "GrowattPriorityStopStateOfChargeConfigTask.h"
#include "GrowattPriorityTaskCommon.h"
        
GrowattPriorityStopStateOfChargeConfigTask::GrowattPriorityStopStateOfChargeConfigTask(ModbusMaster * node, const String &priorityName, const String &mqttPayload) : GrowattPriorityConfigSetOneRegisterTask(node, priorityName, F(PRIORITY_CONFIG_SSOC), mqttPayload)
{
}

GrowattPriorityStopStateOfChargeConfigTask::~GrowattPriorityStopStateOfChargeConfigTask()
{
}

uint16_t GrowattPriorityStopStateOfChargeConfigTask::getAddress() const
{
    if (this->priorityName == F(TOPIC_VALUE_PRIORITY_BAT)) {
        return 1091;
    } else if (this->priorityName == F(TOPIC_VALUE_PRIORITY_GRID)) {
        return 1071;
    } else {
        return 0xffff;
    }
}

bool GrowattPriorityStopStateOfChargeConfigTask::isValid(uint16_t value) const
{
    return value >= 13 && value <= 100;
}
