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

#include <vector>
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

        uint32_t lastReadMillis;  
        std::vector<uint8_t> rxBuffer;
        uint32_t unknownFrameCounter;
        
        bool isValid;
        InverterData inverterData;

        bool parseSoyosourceDisplayByte(uint8_t byte);
        void decodeFrameData(const uint8_t &function, const std::vector<uint8_t> &data);
        
        bool extractDisplayStatusData(const std::vector<uint8_t> &data);
        bool extractMS51StatusData(const std::vector<uint8_t> &data);
        bool buildErrorData(const std::vector<uint8_t> &data, uint8_t response_source = 0, uint8_t function = 0);

        String displayModeToString(const uint8_t &operation_mode);
        String wifiModeToString(const uint8_t &operation_mode);
        String errorToString(const uint8_t &mask);
        void sendCommand(uint8_t function, uint8_t protocol);
};
#endif