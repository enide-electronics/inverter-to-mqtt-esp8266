/*
  GrowattTaskFactory.h - Library header for the ESP8266/ESP32 Arduino platform
  Builds tasks based on the incoming topic and value (MQTT)
  
  Written by JF enide.electronics (at) enide.net
  Licensed under GNU GPLv3
*/
#ifndef GROWATT_TASK_FACTORY_H
#define GROWATT_TASK_FACTORY_H

#include "Task.h"
#include <list>
#include <ModbusMaster.h>

class GrowattTaskFactory {
    public:
        static Task* create(ModbusMaster *node, const String &topic, const String &value);
        static std::list<String> registeredSubtopics();
};
#endif