/*
  MultiGrowattInverter.h - Multiple Growatt inverters on a RS485 bus
  
  By JF enide.electronics (at) enide.net
  Licensed under GNU GPLv3
*/

#ifndef _MultiGrowattInverter_h
#define _MultiGrowattInverter_h

#include <Arduino.h>
#include <stdint.h>
#include <list>
#include <functional>
#include <map>
#include <ModbusMaster.h>
#include "../Task.h"
#include "GrowattInverter.h"

#include "../Inverter.h"

class MultiGrowattInverterInnerFactory {
    public:
        virtual Inverter *createInverter(Stream *serial, int modbusAddress, bool enableRemoteCommands, bool isTL) = 0;
};

class MultiGrowattInverter : public Inverter
{
    public:
        MultiGrowattInverter(Stream *serial, bool shouldDeleteSerial, std::vector<int> slaveAddresses, bool enableRemoteCommands, bool enableThreePhases, MultiGrowattInverterInnerFactory *factory);
        virtual ~MultiGrowattInverter();
        virtual void read();
        virtual bool isDataValid();
    
        virtual InverterData getData(bool fullSet = false);
        virtual void setIncomingTopicData(const String &topic, const String &value);
        virtual std::list<String> getTopicsToSubscribe();

    private:
        void incrementModbusAddress();
        int getAddrFromTopic(const String & path);
        String stripAddrFromTopic(const String & path);
        
        Stream *serial;
        bool shouldDeleteSerial;
        std::vector<int> modbusAddrs;
        std::map<int,Inverter*> inverters;

        int currentModbusIdx;
        int lastModbusIdx;

};

#endif
