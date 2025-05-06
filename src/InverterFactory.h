/*
  InverterFactory.h - Library header for the ESP8266/ESP32 Arduino platform
  Factory to create inverter instances, based on the selected type
  
  Created by JF enide.electronics (at) enide.net
  Licensed under GNU GPLv3
*/

#ifndef _InverterFactory_h
#define _InverterFactory_h

#include <Arduino.h>
#include <Inverter.h>

struct InverterParams {
    int modbusAddress;
};

class InverterFactory {
    public: 
        static Inverter *createInverter(String type, InverterParams params);
};

#endif