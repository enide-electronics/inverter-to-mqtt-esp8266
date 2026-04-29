/*
  Inverter.h - Library header for the ESP8266/ESP32 Arduino platform
  Inverter methods
  
  Modified by JF enide.electronics (at) enide.net
  Licensed under GNU GPLv3
*/

#ifndef _Inverter_h
#define _Inverter_h

#include <Arduino.h>
#include <list>
#include "InverterData.h"

class Inverter
{
    public:
        virtual ~Inverter(){}
        virtual void loop(){}     // if inverter code needs constant activity to do its magic
        virtual void read() = 0;  // periodically reads inverter data
        virtual bool isDataValid() = 0;
    
        virtual InverterData getData(bool fullSet = false) = 0;
        
        virtual void setIncomingTopicData(const String &topic, const String &value) = 0;
        virtual std::list<String> getTopicsToSubscribe() = 0;

        // Returns the current (maximum) inverter temperature in degrees Celsius.
        // Inverters with multiple temperature sensors must return the highest one.
        // Return NAN when no temperature data is available or the inverter has no
        // temperature sensor at all.
        virtual float getMaxTemperature() = 0;
};

#endif