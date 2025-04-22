/*
  SoyosourceGTNInverter.h - Library header for the ESP8266/ESP32 Arduino platform
  Soyosource GTN1000/1200W Inverter model with display
  It works by tapping into the LCD receive pin.
  It will not send any commands back to the CPU.
  
  Created by JF enide.electronics (at) enide.net
  Heavily based on https://github.com/Stefino76/soyosource-wifi-monitor/blob/main/soyosource-wifi-monitor.ino

  Check original project at https://github.com/Stefino76/soyosource-wifi-monitor

  Licensed under GNU GPLv3
*/

#ifndef _SOYOSOURCE_GTN_INVERTER_H
#define _SOYOSOURCE_GTN_INVERTER_H

#include <ArduinoJson.h>
#include "../Inverter.h"

class SoyosourceGTNInverter : public Inverter {
    public:
        SoyosourceGTNInverter(Stream *serial, bool shouldDeleteSerial);
        virtual ~SoyosourceGTNInverter();
        virtual void loop();
        virtual void read();
        virtual bool isDataValid();
    
        virtual InverterData getData(bool fullSet = false);
        virtual void setIncomingTopicData(const String &topic, const String &value);
        virtual std::list<String> getTopicsToSubscribe();
    private:
        Stream *serial;
        bool shouldDeleteSerial;
        DynamicJsonDocument dataJson;
        uint32_t now;
        uint32_t lastReadTime = 0;  
        int data[16];
        int byteIdx = 0;
        bool isValid;

        void saveToDataJson();
        bool isValidMessage();
};
#endif