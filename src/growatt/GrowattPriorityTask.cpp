/*
  GrowattPriorityTask.cpp - Library header for the ESP8266/ESP32 Arduino platform
  Changes the inverter priority to Load First, Battery First or Grid First
  
  Written by JF enide.electronics (at) enide.net
  Licensed under GNU GPLv3
*/
#include <ArduinoJson.h>
#include "GrowattPriorityTask.h"
#include "../GLog.h"
#include "../ModbusUtils.h"

#define LOG_MSG "GrowattPriorityTask: set priority to "

GrowattPriorityTask::GrowattPriorityTask(ModbusMaster * node, const String &mqttValue) {
    this->node = node;
    this->mqttValue = mqttValue;
    this->mqttValue.trim();
}

GrowattPriorityTask::~GrowattPriorityTask() {
}

String GrowattPriorityTask::subtopic() {
    return F(TOPIC_SETTINGS_PRIORITY);
}

// so this doesn't work... register 1044 seems to be read-only like the documentation states
// uint8_t result = this->node->writeSingleRegister(1044, priority);
/*
 * https://community.home-assistant.io/t/esphome-modbus-growatt-shinewifi-s/369171/214
 * https://community.home-assistant.io/t/sniffing-rs232-comms-between-sph6000-and-a-shine-s/553421/4
 * Growatt does a big write.
 */
 
#define GRID_REG_START 1080
#define BAT_REG_START 1100
#define TIME_REG_LEN 3
bool GrowattPriorityTask::run() {
    String &priority = this->mqttValue;
    GLOG::print(String(F(LOG_MSG)) + priority);
    setSuccessful(false);

    if (priority == F(TOPIC_VALUE_PRIORITY_LOAD)) {
        if (!checkAndSetEnableBit(BAT_REG_START, 0)) {
            GLOG::println(String(F(LOG_MSG)) + priority + F(" failed, cannot read/write 1100...1102"));
            return false;
        }
        if (!checkAndSetEnableBit(GRID_REG_START, 0)) {
            GLOG::println(String(F(LOG_MSG)) + priority + F(" failed, cannot read/write 1080...1082"));
            return false;
        }
        setSuccessful(true);
    } else if (priority == F(TOPIC_VALUE_PRIORITY_BAT)) {
        if (!checkAndSetEnableBit(GRID_REG_START, 0)) {
            GLOG::println(String(F(LOG_MSG)) + priority + F(" failed, cannot read/write 1080...1082"));
            return false;
        }
        if (!checkAndSetEnableBit(BAT_REG_START, 1)) {
            GLOG::println(String(F(LOG_MSG)) + priority + F(" failed, cannot read/write 1100...1102"));
            return false;
        }
        setSuccessful(true);
    } else if (priority == F(TOPIC_VALUE_PRIORITY_GRID)) {
        if (!checkAndSetEnableBit(BAT_REG_START, 0)) {
            GLOG::println(String(F(LOG_MSG)) + priority + F(" failed, cannot read/write 1100...1102"));
            return false;
        }
        if (!checkAndSetEnableBit(GRID_REG_START, 1)) {
            GLOG::println(String(F(LOG_MSG)) + priority + F(" failed, cannot read/write 1080...1082"));
            return false;
        }
        setSuccessful(true);
    } else if (priority == F(TOPIC_VALUE_PRIORITY_STATUS)) {
        if (!readPriorityStatus()) {
            return false;
        }
        setSuccessful(true);
    } else {
        GLOG::println(String(F(LOG_MSG)) + priority + F(" failed, invalid value"));
        setSuccessful(false);
    }
    
    return isSuccessful();
}

bool GrowattPriorityTask::checkAndSetEnableBit(uint16_t baseAddr, uint8_t bitValue) {
    uint8_t result = this->node->readHoldingRegisters(baseAddr, TIME_REG_LEN);
    
    if (result == this->node->ku8MBSuccess) {
        for (int j = 0; j < TIME_REG_LEN; j++) {
            node->setTransmitBuffer(j, node->getResponseBuffer(j));
        }
        
        // dump to logs
        ModbusUtils::dumpRegisters(this->node, TIME_REG_LEN);
        
        // ensure 0 or 1
        bitValue &= 0x01;
        
        if (this->node->getResponseBuffer(2) != bitValue) {
            // don't upset the inverter
            delay(1000);
            
            // set bit
            this->node->setTransmitBuffer(2, bitValue);
                
            // write back to inverter
            result = this->node->writeMultipleRegisters(baseAddr, TIME_REG_LEN);
        }
    }
    
    // waits even if read fails
    delay(1000);
    return result == this->node->ku8MBSuccess;
}

void append(String &s, uint8_t v) {
    // leading zero
    if (v < 16) {
        s += '0';
    }
    
    // value
    s += String(v, DEC);
}

String toHHMM(uint16_t r) {

    String hhmm;
    
    uint8_t h = r >> 8;
    uint8_t m = r & 0xff;
    
    append(hhmm, h);
    hhmm += ':';
    append(hhmm, m);   
    
    return hhmm;
}

const char * toEnableString(uint16_t r) {
    return r > 0 ? "on" : "off";
}

/*
  Priority Holding Regiser Map
  
  1070 grid power rate
  1071 grid ssoc
  ...
  1080 grid start time 1
  1081 grid stop time 1
  1082 grid enable 1
  1083 grid start time 2
  1084 grid stop time 2
  1085 grid enable 2
  1086 grid start time 3
  1087 grid stop time 3
  1088 grid enable 3
  ...
  1090 bat power rate
  1091 bat ssoc
  1092 bat ac charger enable
  ...
  1100 bat start time 1
  1101 bat stop time 1
  1102 bat enable 1
  1103 bat start time 2
  1104 bat stop time 2
  1105 bat enable 2
  1106 bat start time 3
  1107 bat stop time 3
  1108 bat enable 3
  ...
  Next ones are for SPA inverter ONLY
  1110 load start time 1
  1111 load stop time 1
  1112 load enable 1
  1113 load start time 2
  1114 load stop time 2
  1115 load enable 2
  1116 load start time 3
  1117 load stop time 3
  1118 load enable 3
 */
bool GrowattPriorityTask::readPriorityStatus() {
    uint8_t result = this->node->readHoldingRegisters(1070, 49); // [1070..1118]
    
    if (result == this->node->ku8MBSuccess) {
#if ARDUINOJSON_VERSION_MAJOR >= 6
        DynamicJsonDocument json(640);
#else
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.createObject();
#endif
        // grid
        json["grid"]["pr"]   = node->getResponseBuffer(0); // 1070
        json["grid"]["ssoc"] = node->getResponseBuffer(1); // 1071
        
        json["grid"]["t1"]   = toHHMM(node->getResponseBuffer(10)) + " " + toHHMM(node->getResponseBuffer(11)); // 1080, 1081
        json["grid"]["t1_enable"] = toEnableString(node->getResponseBuffer(12));                                // 1082
        json["grid"]["t2"]   = toHHMM(node->getResponseBuffer(13)) + " " + toHHMM(node->getResponseBuffer(14)); // 1083, 1084
        json["grid"]["t2_enable"] = toEnableString(node->getResponseBuffer(15));                                // 1085
        json["grid"]["t3"]   = toHHMM(node->getResponseBuffer(16)) + " " + toHHMM(node->getResponseBuffer(17)); // 1086, 1087
        json["grid"]["t3_enable"] = toEnableString(node->getResponseBuffer(18));                                // 1088
        
        // bat
        json["bat"]["pr"]   = node->getResponseBuffer(20); // 1090
        json["bat"]["ssoc"] = node->getResponseBuffer(21); // 1091
        json["bat"]["ac"]   = toEnableString(node->getResponseBuffer(22)); // 1092
        
        json["bat"]["t1"]   = toHHMM(node->getResponseBuffer(30)) + " " + toHHMM(node->getResponseBuffer(31)); // 1100, 1101
        json["bat"]["t1_enable"] = toEnableString(node->getResponseBuffer(32));                                // 1102
        json["bat"]["t2"]   = toHHMM(node->getResponseBuffer(33)) + " " + toHHMM(node->getResponseBuffer(34)); // 1103, 1104
        json["bat"]["t2_enable"] = toEnableString(node->getResponseBuffer(35));                                // 1105
        json["bat"]["t3"]   = toHHMM(node->getResponseBuffer(36)) + " " + toHHMM(node->getResponseBuffer(37)); // 1106, 1107
        json["bat"]["t3_enable"] = toEnableString(node->getResponseBuffer(38));                                          // 1108

        // load (SPA ONLY, ignore for SPH)
        json["load"]["t1"]   = toHHMM(node->getResponseBuffer(40)) + " " + toHHMM(node->getResponseBuffer(41)); // 1110, 1111
        json["load"]["t1_enable"] = toEnableString(node->getResponseBuffer(42));                                // 1112
        json["load"]["t2"]   = toHHMM(node->getResponseBuffer(43)) + " " + toHHMM(node->getResponseBuffer(44)); // 1113, 1114
        json["load"]["t2_enable"] = toEnableString(node->getResponseBuffer(45));                                // 1115
        json["load"]["t3"]   = toHHMM(node->getResponseBuffer(46)) + " " + toHHMM(node->getResponseBuffer(47)); // 1116, 1117
        json["load"]["t3_enable"] = toEnableString(node->getResponseBuffer(48));                                // 1118

        String jsonResponse;
#if ARDUINOJSON_VERSION_MAJOR >= 6
        serializeJson(json, jsonResponse);
#else
        json.printTo(jsonResponse);
#endif
        GLOG::println(String(" ok, json=") + jsonResponse);
        response().set((String(F(TOPIC_SETTINGS_PRIORITY)) + F("/data")).c_str(), jsonResponse); // setting as string
        return true;
    } else {
        GLOG::println(String(" failed with code ") + String(result, DEC) + F(", cannot read 1070...1118"));
        return false;
    }
}

