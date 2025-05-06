/*
  GrowattPriorityTimeConfigTask.h - Library header for the ESP8266/ESP32 Arduino platform
  This task handle setting time ranges for all the growatt sph priorities
  Meaning it will answer messages to these subtopics:
    "settings/priority/bat/t1"
    "settings/priority/bat/t2"
    "settings/priority/bat/t3"
    "settings/priority/load/t1"
    "settings/priority/load/t2"
    "settings/priority/load/t3"
    "settings/priority/grid/t1"
    "settings/priority/grid/t2"
    "settings/priority/grid/t3"
  The payload should be in the following format (start end):
  HH:MM HH:MM
  It will not touch the enable bit.
  
  Written by JF enide.electronics (at) enide.net
  Licensed under GNU GPLv3
*/
#ifndef GROWATT_PRIORITY_TIME_CONFIG_TASK_H
#define GROWATT_PRIORITY_TIME_CONFIG_TASK_H

#include "../Task.h"
#include <ModbusMaster.h>

typedef struct timeRange {
  uint8_t startHour;
  uint8_t startMinute;

  uint8_t endHour;
  uint8_t endMinute;
} TimeRange;


class GrowattPriorityTimeConfigTask : public Task {
    private:
        ModbusMaster * node;
        String priorityName;
        String timeName;
        String mqttPayload;
        uint16_t getStartAddress();
        bool parseTimeRanges(TimeRange *tr);
        bool parseTimeString(String hhmm, uint8_t *h, uint8_t *m);
        
    public:
        GrowattPriorityTimeConfigTask(ModbusMaster * node, const String &priorityName, const String &timeName, const String &mqttPayload);
        virtual ~GrowattPriorityTimeConfigTask();
        virtual String subtopic();
        virtual bool run();
};

#endif