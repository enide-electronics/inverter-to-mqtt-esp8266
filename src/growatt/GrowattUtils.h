/*
  GrowattUtils.h - Library header for the ESP8266/ESP32 Arduino platform
  Shared helpers for Growatt tasks
  
  Written by JF enide.electronics (at) enide.net
  Licensed under GNU GPLv3
*/
#ifndef GROWATT_UTILS_H
#define GROWATT_UTILS_H

#include <Arduino.h>
#include <stdint.h>

namespace GrowattUtils {
    // Appends the hex representation of v to s (zero-padded to 2 chars minimum),
    // optionally followed by a ':' separator.
    void appendHex(String &s, uint16_t v, bool appendColon);
}

#endif
