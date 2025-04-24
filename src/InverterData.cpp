/*
  InverterData.cpp - Library for the ESP8266/ESP32 Arduino platform
  Inverter data

  Written by JF enide.electronics (at) enide.net
  Licensed under GNU GPLv3
*/

#include "InverterData.h"

InverterData::InverterData() {
    clear();
}

InverterData::~InverterData() {
    clear();
}

void InverterData::set(const char *name, float value) {
    snprintf (msg, MSG_BUFFER_SIZE, "%.1f", value);
    (*this)[String(name)] = String(msg);
}

void InverterData::set(const char *name, uint32_t value) {
    snprintf (msg, MSG_BUFFER_SIZE, "%ld", value);
    (*this)[String(name)] = String(msg);
}

void InverterData::set(const char *name, uint16_t value) {
    snprintf (msg, MSG_BUFFER_SIZE, "%d", value);
    (*this)[String(name)] = String(msg);
}

void InverterData::set(const char *name, uint8_t value) {
    snprintf (msg, MSG_BUFFER_SIZE, "%d", value);
    (*this)[String(name)] = String(msg);
}

void InverterData::set(const char *name, const char * value) {
    strncpy (msg, value, MSG_BUFFER_SIZE);
    msg[MSG_BUFFER_SIZE-1] = '\0';
    (*this)[String(name)] = String(msg);
}

void InverterData::set(const char *name, const String value) {
    (*this)[String(name)] = value;
}
