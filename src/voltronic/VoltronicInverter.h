/*
  VoltronicInverter.h - A base class to implement the communication with Voltronic (or clones) inverters

  Written by JF enide.electronics (at) enide.net
  Licensed under GNU GPLv3
*/

#ifndef VOLTRONIC_INVERTER_H
#define VOLTRONIC_INVERTER_H

#include <Arduino.h>
#include "../Inverter.h"

class VoltronicInverter : public Inverter {
    public:
        VoltronicInverter(Stream *serial, bool shouldDeleteSerial);
        virtual ~VoltronicInverter();

    private:
        bool shouldDeleteSerial;
    
    protected:
        Stream *serial;
        
        InverterData inverterData;
        bool isValid;

        virtual void readRatedInformation() = 0; // QPIRI, ^P007PIRI, etc. depending on inverter model
        virtual void readGeneralStatus() = 0; // QPIGS, ^P005GS, etc. depending on inverter model
        virtual void readWarnings() = 0; // QPIWS, and others...
        virtual void readMode() = 0; // QMOD only
        
        bool sendCommand(String cmd);
        uint16_t calcCRC(uint8_t *pin, uint8_t len);
        String recvResponse(uint16_t replysize);


};
#endif