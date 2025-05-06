/*
  VirtualLimiter.h - Periodically sends the requested power to the inverter
  
  GPIO0 / D3 : TX
  GPIO5 / D1 : RX

  Message bytes: 36, 86, 0, 33, 0, 0, 128, 8
                                Ph Pl      Checksum (264-Ph-Pl)

  Based on : https://github.com/ChrisHomewood/MQTT_to_Soyosource-Inverter_RS485/blob/main/output_control_MQTT_Soyosource_wifikit32.ino
  Written by JF enide.electronics (at) enide.net
  Licensed under GNU GPLv3
*/
#include "VirtualLimiter.h"
#include "../GlobalDefs.h"
#include "../GLog.h"


#ifdef LARGE_ESP_BOARD
#define VIRTUAL_METER_TX_PIN D3
#define VIRTUAL_METER_RX_PIN D1
VirtualLimiter::VirtualLimiter() : rs485Port(VIRTUAL_METER_RX_PIN, VIRTUAL_METER_TX_PIN) {
    rs485Port.begin(4800);
    lastSentAt = millis();
    demandPower = 0;

    messageBuffer[0] = 0x24; // 36
    messageBuffer[1] = 0x56; // 86
    messageBuffer[2] = 0x00; // 0
    messageBuffer[3] = 0x21; // 33
    messageBuffer[4] = 0;    // p high
    messageBuffer[5] = 0;    // p low
    messageBuffer[6] = 0x80; // 128
    messageBuffer[7] = 0x08; // chksum
}
#else
VirtualLimiter::VirtualLimiter() {
}
#endif

VirtualLimiter::~VirtualLimiter() { 
}

void VirtualLimiter::loop() {
#ifdef LARGE_ESP_BOARD
    uint32_t now = millis();

    if (now - lastSentAt > 300) {
        lastSentAt = now;
        
        uint8_t pHigh = demandPower >> 8;
        uint8_t pLow = demandPower & 0xFF;
        messageBuffer[4] = pHigh;
        messageBuffer[5] = pLow;
        
        // The checksum calculation found all around the web, and seen below, is incorrect
        // The inverter will ignore messages with incorrect checksum and resume operation in PV Mode (not PV Limit)
        // If you want to test and see the problem by yourself:
        // - the power value to something between 256W and 263W
        // - wait a couple of seconds and the inverter will be in PV Mode again (not PV Limit)
        //
        // int chksum = 264 - pHigh - pLow;
        // if (chksum >= 256) chksum = 8;

        uint8_t chksum = 264 - pHigh - pLow;
        messageBuffer[7] = chksum & 0xFF; // 0xFF is not needed, this is already an 8 bit variable
        
        rs485Port.write(messageBuffer, 8);
        GLOG::printf("METR: demand=%d, ph=0x%02x, pl=0x%02x, chksum=0x%02x\n", demandPower, pHigh, pLow, chksum);
    }
#endif
}

void VirtualLimiter::updateDemand(uint16_t demandPower) {
#ifdef LARGE_ESP_BOARD
    this->demandPower = demandPower;
#endif
}
