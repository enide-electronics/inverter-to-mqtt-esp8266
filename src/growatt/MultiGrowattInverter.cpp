/*
  MultiGrowattInverter.cpp - Multiple Growatt inverters on a RS485 bus

  Created by JF enide.electronics (at) enide.net
  Licensed under GNU GPLv3
*/
#include "MultiGrowattInverter.h"
#include "GrowattInverter.h"
#include "../GLog.h"
#include "GrowattTaskFactory.h"
#include "../Task.h"
#include "../ModbusUtils.h"

MultiGrowattInverter::MultiGrowattInverter(Stream *serial, bool shouldDeleteSerial, std::vector<int> slaveAddresses, bool enableRemoteCommands, bool enableThreePhases, MultiGrowattInverterInnerFactory *factory) {
    this->serial = serial;
    this->shouldDeleteSerial = shouldDeleteSerial;
    this->modbusAddrs.insert(this->modbusAddrs.end(), slaveAddresses.begin(), slaveAddresses.end());

    for (int modbusAddr : slaveAddresses) {
        Inverter *inverter = factory->createInverter(serial, modbusAddr, enableRemoteCommands, enableThreePhases);
        this->inverters[modbusAddr] = inverter;
    }

    // factory is no longer needed
    delete factory;
}

MultiGrowattInverter::~MultiGrowattInverter() {
    for (const auto & inverterEntry : inverters) {
        delete inverterEntry.second;
    }

    inverters.clear();
    modbusAddrs.clear();

    if (this->shouldDeleteSerial) {
        delete this->serial;
    }
}


void MultiGrowattInverter::read() {
    int modbusAddr = this->modbusAddrs[this->currentModbusIdx];

    GLOG::printf(" @ %d", modbusAddr);
    Inverter *inverter = this->inverters[modbusAddr];
    inverter->read();

    lastModbusIdx = this->currentModbusIdx;
    incrementModbusAddress();
}

bool MultiGrowattInverter::isDataValid() {
    int modbusAddr = this->modbusAddrs[this->lastModbusIdx];
    Inverter *inverter = this->inverters[modbusAddr];
    return inverter->isDataValid();
}

InverterData MultiGrowattInverter::getData(bool fullSet) {
    int modbusAddr = this->modbusAddrs[this->lastModbusIdx];
    Inverter *inverter = this->inverters[modbusAddr];
    
    InverterData data = inverter->getData(fullSet);

    InverterData dataWithAddrPrefix;
    for (const auto & entry : data) {
        dataWithAddrPrefix[String(modbusAddr) + "/" + entry.first] = entry.second;
    }

    return dataWithAddrPrefix;
}

void MultiGrowattInverter::setIncomingTopicData(const String &topic, const String &value) {
    // find prefix in topic
    // strip it from topic
    // get inveter by addr
    // call real method

    int modbusAddr = this->getAddrFromTopic(topic);
    if (modbusAddr < 0) {
        // error or not found
        return;
    }

    Inverter *inverter = this->inverters[modbusAddr];
    if (inverter == NULL) {
        // no inverter for this address
        return;
    }

    String topicWithoutPrefix = stripAddrFromTopic(topic);

    inverter->setIncomingTopicData(topicWithoutPrefix, value);
}

std::list<String> MultiGrowattInverter::getTopicsToSubscribe() {
    std::list<String> allTopics;

    for (auto inverterEntry : this->inverters) {
        int addr = inverterEntry.first;
        Inverter *inverter = inverterEntry.second;

        std::list<String> topics = inverter->getTopicsToSubscribe();

        // append the inverter address as topic prefix: growatt/22/settings/priority/bat/ac
        for (String topic : topics) {
            allTopics.push_back(String(addr, 10) + "/" + topic);
        }
    }

    return allTopics;
}

void MultiGrowattInverter::incrementModbusAddress() {
    currentModbusIdx += 1;
    if (currentModbusIdx >= this->modbusAddrs.size()) {
        currentModbusIdx = 0;
    }
}

int MultiGrowattInverter::getAddrFromTopic(const String & path) {
    int pos = path.indexOf('/'); // Find the position of the first '/'
    
    String sAddr;
    if (pos != -1) {
        sAddr = path.substring(0, pos);
    } else {
        // something is very wrong
        return -2;
    }

    int addr = sAddr.toInt();

    // less than zero is a valid number but we don't want those
    // zero means no valid number was found
    if (addr <= 0) {
        // no numeric prefix
        return -1;
    }

    return addr;
}

String MultiGrowattInverter::stripAddrFromTopic(const String & path) {
    int pos = path.indexOf('/'); // Find the position of the first '/'
    return path.substring(pos + 1);
}

