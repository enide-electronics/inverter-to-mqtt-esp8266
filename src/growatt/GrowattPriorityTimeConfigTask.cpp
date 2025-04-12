/*
  GrowattPriorityTimeConfigTask.cpp - Library header for the ESP8266/ESP32 Arduino platform
  This task handle setting time ranges for all the growatt sph priorities
  
  Written by JF enide.electronics (at) enide.net
  Licensed under GNU GPLv3
*/
#include "GrowattPriorityTimeConfigTask.h"
#include "GrowattPriorityTaskCommon.h"
#include <StringSplitter.h>

GrowattPriorityTimeConfigTask::GrowattPriorityTimeConfigTask(ModbusMaster * node, const String &priorityName, const String &timeName, const String &mqttPayload)
{
    this->node = node;
    this->priorityName = priorityName;
    this->timeName = timeName;
    this->mqttPayload = mqttPayload;
    this->mqttPayload.trim();
}

GrowattPriorityTimeConfigTask::~GrowattPriorityTimeConfigTask()
{
}

String GrowattPriorityTimeConfigTask::subtopic()
{
    // format should be similar to: "settings/priority/bat/t1"
    return String(F("settings/priority/")) + this->priorityName + "/" + this->timeName;
}

#define TIME_MODBUS_LEN 3

bool GrowattPriorityTimeConfigTask::run()
{
    GLOG::print("GrowattPriorityTimeConfigTask::run ");
    GLOG::print((subtopic() + " payload=").c_str());
    GLOG::println(mqttPayload.c_str());
    
    setSuccessful(false);
    
    uint16_t startAddress = getStartAddress();
    if (startAddress != 0xffff) {
        TimeRange tr;
        if (parseTimeRanges(&tr)) {
            // do a quick sanity check, start time should be before end time
            if (tr.startHour * 60 + tr.startMinute < tr.endHour * 60 + tr.endMinute) {
                uint8_t result = this->node->readHoldingRegisters(startAddress, TIME_MODBUS_LEN);
                if (result == this->node->ku8MBSuccess) {
                    // copy all 3 bytes (not really needed but since I don't set the enable bit yet, it wont't mess up that bit)
                    for (int j = 0; j < TIME_MODBUS_LEN; j++) {
                        node->setTransmitBuffer(j, node->getResponseBuffer(j));
                    }
                    
                    uint16_t startTime = tr.startHour;
                    startTime <<= 8;
                    startTime += tr.startMinute;
                    
                    uint16_t endTime = tr.endHour;
                    endTime <<= 8;
                    endTime += tr.endMinute;
                
                    // set the value we got from mqtt
                    this->node->setTransmitBuffer(0, startTime);
                    this->node->setTransmitBuffer(1, endTime);
                    // this->node->setTransmitBuffer(2, tr.enable);
                    
                    // don't upset the inverter
                    delay(1000);
                    
                    // write back to inverter
                    result = this->node->writeMultipleRegisters(startAddress, TIME_MODBUS_LEN);
                    
                    response().set((String(subtopic()) + F("/data")).c_str(), (String("addr=") + startAddress + 
                        " start=" + tr.startHour + ":" + tr.startMinute + 
                        " end=" + tr.endHour + ":" + tr.endMinute).c_str());
                    
                    setSuccessful(result == this->node->ku8MBSuccess);
                }
            }
        }
    }
    
    return isSuccessful();
}

uint16_t GrowattPriorityTimeConfigTask::getStartAddress()
{   
    if (this->timeName.length() == 2) {
        char tc = this->timeName.charAt(1);
        
        if (tc >= '1' && tc <= '3') {
            uint8_t tn = tc - '0';
            if (this->priorityName == F(TOPIC_VALUE_PRIORITY_GRID)) {
                return 1080 + 3 * (tn - 1);
            } else if (this->priorityName == F(TOPIC_VALUE_PRIORITY_BAT)) {
                return 1100 + 3 * (tn - 1);
            } else if (this->priorityName == F(TOPIC_VALUE_PRIORITY_LOAD)) {
                // return 1110 + 3 * (tn - 1);
                return 0xffff; // SPH does not support time ranges for load first
            }
        }
    }
    
    return 0xffff; // error
}

bool GrowattPriorityTimeConfigTask::parseTimeRanges(TimeRange *tr) 
{
    // 00:00 23:59

    auto ss = StringSplitter(this->mqttPayload, ' ', 2);

    // check if valid number of items
    if (ss.getItemCount() != 2) {
        return false;
    }

    String item;
    item = ss.getItemAtIndex(0);



    String startTime = ss.getItemAtIndex(0);
    String endTime = ss.getItemAtIndex(1);

    uint8_t sHour, sMinute;
    if (parseTimeString(startTime, &sHour, &sMinute)) {
        tr->startHour = sHour;
        tr->startMinute = sMinute;
    } else {
        return false;
    }

    uint8_t eHour, eMinute;
    if (parseTimeString(endTime, &eHour, &eMinute)) {
        tr->endHour = eHour;
        tr->endMinute = eMinute;
    } else {
        return false;
    }

    return true;
}

bool GrowattPriorityTimeConfigTask::parseTimeString(String hhmm, uint8_t *h, uint8_t *m) {
    StringSplitter ss = StringSplitter(hhmm, ':', 2);
    if (ss.getItemCount() == 2) {
        uint16_t hh = atoi(ss.getItemAtIndex(0).c_str());
        uint16_t mm = atoi(ss.getItemAtIndex(1).c_str());

        // check for stupid values
        if (hh > 23 || mm > 59) {
            return false;
        }

        // set them
        *h = hh;
        *m = mm;
        return true;
    } else {
        return false;
    }
}

