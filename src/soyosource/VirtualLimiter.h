/*
  VirtualLimiter.h - Periodically sends the requested power to the inverter
  
  GPIO0 / D3 : TX
  GPIO5 / D1 : RX

  Written by JF enide.electronics (at) enide.net
  Licensed under GNU GPLv3
*/

#include <Arduino.h>
#include <SoftwareSerial.h>

class VirtualLimiter {
    public:
        VirtualLimiter();
        virtual ~VirtualLimiter();

        void loop();
        void updateDemand(uint16_t demandPower);
    private:
        uint32_t lastSentAt;
        uint16_t demandPower;
        uint8_t messageBuffer[8];
        SoftwareSerial rs485Port;
};

