/*
  NoneInverter.h - Library header for the ESP8266/ESP32 Arduino platform
  Null/None inverter that doesn't do anything
  
  Created by JF enide.electronics (at) enide.net
  Licensed under GNU GPLv3
*/

#ifndef _NoneInverter_h
#define _NoneInverter_h

#include <Arduino.h>
#include <stdint.h>
#include <list>
#include "Inverter.h"

class NoneInverter : public Inverter
{
    public:
        NoneInverter();
        virtual ~NoneInverter();
        virtual void read();
        virtual bool isDataValid();
    
        virtual InverterData getData(bool fullSet = false);
        virtual void setIncomingTopicData(const String &topic, const String &value);
        virtual std::list<String> getTopicsToSubscribe();
};

#endif
