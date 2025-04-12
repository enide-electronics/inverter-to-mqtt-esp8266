/*
  GrowattPriorityBatteryFirstACChargerConfigTask.h - Library header for the ESP8266/ESP32 Arduino platform
  This task enables or disables the AC charger for the battery first priority
  It answers on: "settings/priority/bat/ac"
    
  The payload should be in the following format:
  on|off
  
  Written by JF enide.electronics (at) enide.net
  Licensed under GNU GPLv3
*/
#ifndef GROWATT_PRIORITY_BAT_FIRST_AC_CHARGER_CONFIG_TASK_H
#define GROWATT_PRIORITY_BAT_FIRST_AC_CHARGER_CONFIG_TASK_H

#include "../Task.h"
#include <ModbusMaster.h>

#define TOPIC_SETTINGS_PRIORITY_BAT_FIRST_AC_CHARGER_TASK "settings/priority/bat/ac"

class GrowattPriorityBatteryFirstACChargerConfigTask : public Task {
    private:
        ModbusMaster * node;
        String mqttPayload;
        
    public:
        GrowattPriorityBatteryFirstACChargerConfigTask(ModbusMaster * node, const String &mqttPayload);
        virtual ~GrowattPriorityBatteryFirstACChargerConfigTask();
        virtual String subtopic();
        virtual bool run();
};

#endif