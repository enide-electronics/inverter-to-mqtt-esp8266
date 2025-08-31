/*
  ModbusUtils.h - Library header for the ESP8266/ESP32 Arduino platform
  Utility functions that require modbus access
  
  Written by JF enide.electronics (at) enide.net
  Licensed under GNU GPLv3
*/
#ifndef MODBUS_UTILS_H
#define MODBUS_UTILS_H

#include <Arduino.h>
#include "GLog.h"
#include <ModbusMaster.h>

class ModbusUtils {
    public:
        // dumps the last response from modbus to the logs in hex format
        static void dumpRegisters(ModbusMaster * node, uint8_t length) {
            GLOG::print(", hex[");
            for (uint8_t i = 0; i < length; i++) {
                GLOG::print(String(node->getResponseBuffer(i), HEX) + ":");
            }
            GLOG::print("]");
        }

        static float glueFloat(uint16_t w1, uint16_t w0) {
            unsigned long t;
            t = w1 << 16;
            t += w0;

            float f;
            f = t;
            f = f / 10;
            return f;
        }
};
#endif