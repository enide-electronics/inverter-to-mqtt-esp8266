/*
  GrowattPriorityPowerRatingConfigTask.cpp - Library header for the ESP8266/ESP32 Arduino platform
  This task set the Power Rating value for Grid First or Battery First
  It answers on: 
    "settings/priority/bat/pr"
    "settings/priority/grid/pr"
    
  The payload should be in integer format:
  1..100
  
  Written by JF enide.electronics (at) enide.net
  Licensed under GNU GPLv3
*/

#include "GrowattPriorityPowerRatingConfigTask.h"
#include "GrowattPriorityTaskCommon.h"
        
GrowattPriorityPowerRatingConfigTask::GrowattPriorityPowerRatingConfigTask(ModbusMaster * node, const String &priorityName, const String &mqttPayload) : GrowattPriorityConfigSetOneRegisterTask(node, priorityName, F(PRIORITY_CONFIG_PR), mqttPayload)
{
}

GrowattPriorityPowerRatingConfigTask::~GrowattPriorityPowerRatingConfigTask()
{
}

uint16_t GrowattPriorityPowerRatingConfigTask::getAddress() const
{
    if (this->priorityName == F(TOPIC_VALUE_PRIORITY_BAT)) {
        return 1090;
    } else if (this->priorityName == F(TOPIC_VALUE_PRIORITY_GRID)) {
        return 1070;
    } else {
        return 0xffff;
    }
}

bool GrowattPriorityPowerRatingConfigTask::isValid(uint16_t value) const
{
    // YES, 0 is not acceptable
    return value > 0 && value <= 100;
}
