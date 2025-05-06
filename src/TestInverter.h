/*
  TestInverter.h - Library header for the ESP8266/ESP32 Arduino platform
  An inverter that sends values
  
  Created by JF enide.electronics (at) enide.net
  Licensed under GNU GPLv3
*/

#ifndef _TestInverter_h
#define _TestInverter_h

#include <Arduino.h>
#include <stdint.h>
#include <list>
#include "Inverter.h"

class TestInverter : public Inverter
{
    public:
        TestInverter();
        virtual ~TestInverter();
        virtual void read();
        virtual bool isDataValid();
    
        virtual InverterData getData(bool fullSet = false);
        virtual void setIncomingTopicData(const String &topic, const String &value);
        virtual std::list<String> getTopicsToSubscribe();
};

#endif
