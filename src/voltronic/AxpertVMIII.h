/*
  AxpertVMIII.h - The Voltronic Axpert VM III off-grid inverter

  Written by JF enide.electronics (at) enide.net
  Licensed under GNU GPLv3
*/

#ifndef AXPERT_VMIII_H
#define AXPERT_VMIII_H

#include <Arduino.h>
#include "VoltronicInverter.h"

class VoltronicAxpertVMIIIInverter : public VoltronicInverter {
    public:
        VoltronicAxpertVMIIIInverter(Stream *serial, bool shouldDeleteSerial);
        virtual ~VoltronicAxpertVMIIIInverter();

        virtual void read();
        virtual bool isDataValid();

        virtual InverterData getData(bool fullSet = false);

        virtual void setIncomingTopicData(const String &topic, const String &value);
        virtual std::list<String> getTopicsToSubscribe();
        virtual float getMaxTemperature();
        virtual std::list<HaDiscoveryMessage> getHomeAssistantDiscovery(const HaDiscoveryDevice &device);

    private:
        int state;
        float tempHeatsink;
        
        virtual void readRatedInformation(); // QPIRI
        virtual void readGeneralStatus();    // QPIGS
        virtual void readWarnings();         // QPIWS
        virtual void readMode();             // QMOD
};
#endif