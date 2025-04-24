/*
  InverterData.h - Library header for the ESP8266/ESP32 Arduino platform
  Inverter data
  
  Written by JF enide.electronics (at) enide.net
  Licensed under GNU GPLv3
*/

#ifndef _INVERTER_DATA_H
#define _INVERTER_DATA_H

#include <Arduino.h>
#include <map>

#define MSG_BUFFER_SIZE  (255)

class InverterData : public std::map<String, String> {   
    private:
        char msg[MSG_BUFFER_SIZE];
        
    public:
        InverterData();
        virtual ~InverterData();
        
        void set(const char *name, float value);

        void set(const char *name, uint32_t value);
        
        void set(const char *name, uint16_t value);

        void set(const char *name, uint8_t value);

        void set(const char *name, const char * value);
        
        void set(const char *name, const String value);
};

#endif