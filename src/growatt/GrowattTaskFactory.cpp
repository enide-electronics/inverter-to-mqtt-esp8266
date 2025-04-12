/*
  GrowattTaskFactory.cpp - Library header for the ESP8266/ESP32 Arduino platform
  Builds tasks based on the incoming topic and value (MQTT)
  
  Written by JF enide.electronics (at) enide.net
  Licensed under GNU GPLv3
*/
#include<StringSplitter.h>

#include "GrowattTaskFactory.h"
#include "GrowattPriorityTask.h"
#include "GrowattReadHoldingTask.h"
#include "GrowattPriorityTimeConfigTask.h"
#include "GrowattPriorityBatteryFirstACChargerConfigTask.h"
#include "GrowattPriorityPowerRatingConfigTask.h"
#include "GrowattPriorityStopStateOfChargeConfigTask.h"

Task* GrowattTaskFactory::create(ModbusMaster *node, const String &topic, const String &value) {
    Task *task = NULL;
    
    // set priority: "/settings/priority"
    if (topic == String(F(TOPIC_SETTINGS_PRIORITY))) {
        task = new GrowattPriorityTask(node, value);
    } else if (topic.startsWith(F(TOPIC_SETTINGS_PRIORITY))) {
        // per priority configs like ac, pr, ssoc
        auto ss = StringSplitter(topic, '/', 4); 
        
        if (ss.getItemCount() == 4) {
            String whichPriority = ss.getItemAtIndex(2);
            String whichConfig = ss.getItemAtIndex(3);
            
            if ((whichPriority == "bat" || whichPriority == "grid") && (whichConfig == "t1" || whichConfig == "t2" || whichConfig == "t3")) {
                task = new GrowattPriorityTimeConfigTask(node, ss.getItemAtIndex(2), ss.getItemAtIndex(3), value);
            } else if (whichPriority == "bat" && whichConfig == "ac") {
                task = new GrowattPriorityBatteryFirstACChargerConfigTask(node, value);
            } else if ((whichPriority == "bat" || whichPriority == "grid") && whichConfig == "pr") {
                task = new GrowattPriorityPowerRatingConfigTask(node, whichPriority, value);
            } else if ((whichPriority == "bat" || whichPriority == "grid") && whichConfig == "ssoc") {
                task = new GrowattPriorityStopStateOfChargeConfigTask(node, whichPriority, value);
            }
        }
    } else if (topic == String(F(TOPIC_SETTINGS_READ_HOLDING_TASK))) {
        String payload = value;
        String fields[2];
        uint8_t fieldsIndex = 0;
        
        while (payload.length() > 0) {
            int index = payload.indexOf(' ');
            if (index == -1) {
                // no space, keep whole/remaining string
                fields[fieldsIndex++] = payload;
                break;
            } else {
                fields[fieldsIndex++] = payload.substring(0, index);
                payload = payload.substring(index + 1);
            }
        }
        
        if (fieldsIndex == 2) {
            uint16_t addr = fields[0].toInt();
            uint8_t length = fields[1].toInt();
            task = new GrowattReadHoldingTask(node, addr, length);
        }
    }
    
    return task;
}

std::list<String> GrowattTaskFactory::registeredSubtopics() {
    std::list<String> topics;
    
    topics.push_back(F(TOPIC_SETTINGS_PRIORITY));
    topics.push_back(F(TOPIC_SETTINGS_READ_HOLDING_TASK));
    
    topics.push_back(F("settings/priority/bat/t1"));
    //topics.push_back(F("settings/priority/bat/t2"));
    //topics.push_back(F("settings/priority/bat/t3"));
    topics.push_back(F("settings/priority/bat/ac"));
    topics.push_back(F("settings/priority/bat/pr"));
    topics.push_back(F("settings/priority/bat/ssoc"));
    
    topics.push_back(F("settings/priority/grid/t1"));
    //topics.push_back(F("settings/priority/grid/t2"));
    //topics.push_back(F("settings/priority/grid/t3"));
    topics.push_back(F("settings/priority/grid/pr"));
    topics.push_back(F("settings/priority/grid/ssoc"));
    
    return topics;
}
