/*
  MicInverter.h - Library header for the ESP8266/ESP32 Arduino platform
  To read data from Growatt MIC inverters, using the old register map (v3.05)
  
  Based heavily on the growatt-esp8266 project at https://github.com/jkairys/growatt-esp8266
  
  Modified by JF enide.electronics (at) enide.net
  Licensed under GNU GPLv3
*/

#ifndef _MicInverter_h
#define _MicInverter_h

#include <Arduino.h>
#include <stdint.h>
#include <list>
#include <functional>
#include <ModbusMaster.h>
#include "../Task.h"

#include "../Inverter.h"

class MicInverter : public Inverter
{
    public:
        MicInverter(Stream *serial, bool shouldDeleteSerial, uint8_t slaveAddress, bool enableThreePhases);
        virtual ~MicInverter();
        virtual void read();
        virtual bool isDataValid();
    
        virtual InverterData getData(bool fullSet = false);
        virtual void setIncomingTopicData(const String &topic, const String &value);
        virtual std::list<String> getTopicsToSubscribe();

    private:
        Stream *serial;
        bool shouldDeleteSerial;
        bool enableTL;

        ModbusMaster *node;

        float Ppv; // W

        float Ppv1;
        float Vpv1;
        float Ipv1;

        float Ppv2;
        float Vpv2;
        float Ipv2;

        float Pac1; // VA
        float Vac1;
        float Iac1;

        float Pac2; // VA
        float Vac2;
        float Iac2;

        float Pac3; // VA
        float Vac3;
        float Iac3;
        
        float Fac; // Hz
        float Pac; // W
        
        float Etoday;
        float Etotal;
        float Ttotal;
        
        float tempInverter;
        float tempIPM;
        
        bool valid;

        uint8_t status;
};

#endif
