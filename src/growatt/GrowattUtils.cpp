/*
  GrowattUtils.cpp - Library source for the ESP8266/ESP32 Arduino platform
  Shared helpers for Growatt tasks
  
  Written by JF enide.electronics (at) enide.net
  Licensed under GNU GPLv3
*/
#include "GrowattUtils.h"

namespace GrowattUtils {
    void appendHex(String &s, uint16_t v, bool appendColon) {
        // leading zero
        if (v < 16) {
            s += '0';
        }
        
        // value
        s += String(v, HEX);
        
        // separator
        if (appendColon) {
            s += ':';
        }
    }
}
