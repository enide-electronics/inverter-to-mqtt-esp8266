/*
  GrowattInverter.cpp - Library for the ESP8266/ESP32 Arduino platform
  To read data from Growatt SPH and SPA inverters

  Based heavily on the growatt-esp8266 project at https://github.com/jkairys/growatt-esp8266

  Modified by JF enide.electronics (at) enide.net
  Licensed under GNU GPLv3
*/
#include "MicInverter.h"
#include "../GLog.h"
#include "../ModbusUtils.h"

MicInverter::MicInverter(Stream *serial, bool shouldDeleteSerial, uint8_t slaveAddress, bool enableThreePhases) {
    this->serial = serial;
    this->shouldDeleteSerial = shouldDeleteSerial;
    this->enableTL = enableThreePhases;

    this->node = new ModbusMaster();
    this->node->begin(slaveAddress, *serial);

    this->valid = false;

    this->status = 0;
    
    this->Ppv = 0.0;

    this->Ppv1 = 0.0;
    this->Vpv1 = 0.0;
    this->Ipv1 = 0.0;

    this->Ppv2 = 0.0;
    this->Vpv2 = 0.0;
    this->Ipv2 = 0.0;

    this->Pac1 = 0.0;
    this->Vac1 = 0.0;
    this->Iac1 = 0.0;

    this->Pac2 = 0.0;
    this->Vac2 = 0.0;
    this->Iac2 = 0.0;

    this->Pac3 = 0.0;
    this->Vac3 = 0.0;
    this->Iac3 = 0.0;

    this->Fac = 0.0;
    this->Pac = 0.0;

    this->Etoday = 0.0;
    this->Etotal = 0.0;
    this->Ttotal = 0.0;

    this->tempInverter = 0.0;
    this->tempIPM = 0.0;
}

MicInverter::~MicInverter() {
    delete this->node;

    if (this->shouldDeleteSerial) {
        delete this->serial;
    }
}

void MicInverter::read() {
    uint8_t result = this->node->readInputRegisters(0, 42);
    if (result == this->node->ku8MBSuccess) {
        
        this->valid = true;
        this->status = this->node->getResponseBuffer(0);

        this->Ppv = ModbusUtils::glueFloat(this->node->getResponseBuffer(1), this->node->getResponseBuffer(2));

        
        this->Vpv1 = ModbusUtils::glueFloat(0, this->node->getResponseBuffer(3));
        this->Ipv1 = ModbusUtils::glueFloat(0, this->node->getResponseBuffer(4));
        this->Ppv1 = ModbusUtils::glueFloat(this->node->getResponseBuffer(5), this->node->getResponseBuffer(6));

        this->Vpv2 = ModbusUtils::glueFloat(0, this->node->getResponseBuffer(7));
        this->Ipv2 = ModbusUtils::glueFloat(0, this->node->getResponseBuffer(8));
        this->Ppv2 = ModbusUtils::glueFloat(this->node->getResponseBuffer(9), this->node->getResponseBuffer(10));


        this->Pac = ModbusUtils::glueFloat(this->node->getResponseBuffer(11), this->node->getResponseBuffer(12));
        this->Fac = ModbusUtils::glueFloat(0, this->node->getResponseBuffer(13)) / 10.0;

        this->Vac1 = ModbusUtils::glueFloat(0, this->node->getResponseBuffer(14));
        this->Iac1 = ModbusUtils::glueFloat(0, this->node->getResponseBuffer(15));
        this->Pac1 = ModbusUtils::glueFloat(this->node->getResponseBuffer(16), this->node->getResponseBuffer(17));

        if (this->enableTL) {
            this->Vac2 = ModbusUtils::glueFloat(0, this->node->getResponseBuffer(18));
            this->Iac2 = ModbusUtils::glueFloat(0, this->node->getResponseBuffer(19));
            this->Pac2 = ModbusUtils::glueFloat(this->node->getResponseBuffer(20), this->node->getResponseBuffer(21));

            this->Vac3 = ModbusUtils::glueFloat(0, this->node->getResponseBuffer(22));
            this->Iac3 = ModbusUtils::glueFloat(0, this->node->getResponseBuffer(23));
            this->Pac3 = ModbusUtils::glueFloat(this->node->getResponseBuffer(24), this->node->getResponseBuffer(25));
        }

        this->Etoday = ModbusUtils::glueFloat(this->node->getResponseBuffer(26), this->node->getResponseBuffer(27));
        this->Etotal = ModbusUtils::glueFloat(this->node->getResponseBuffer(28), this->node->getResponseBuffer(29));
        this->Ttotal = ModbusUtils::glueFloat(this->node->getResponseBuffer(30), this->node->getResponseBuffer(31));

        this->tempInverter = ModbusUtils::glueFloat(0, this->node->getResponseBuffer(32));
        this->tempIPM = ModbusUtils::glueFloat(0, this->node->getResponseBuffer(41));
    } else {
        this->valid = false;
    }
}

bool MicInverter::isDataValid() {
    return this->valid;
}


InverterData MicInverter::getData(bool ignored) {
    InverterData data;
    
    data.set("status", this->status);

    data.set("Ppv", this->Ppv);    

    data.set("Ppv1", this->Ppv1);    
    data.set("Vpv1", this->Vpv1);
    data.set("Ipv1", this->Ipv1);
    
    data.set("Ppv2", this->Ppv2);   
    data.set("Vpv2", this->Vpv2);
    data.set("Ipv2", this->Ipv2);
    
    data.set("Pac", this->Pac);
    data.set("Fac", this->Fac);
    
    data.set("Vac1", this->Vac1);
    data.set("Iac1", this->Iac1);
    data.set("Pac1", this->Pac1);
    
    if (this->enableTL) {
        data.set("Vac2", this->Vac2);
        data.set("Iac2", this->Iac2);
        data.set("Pac2", this->Pac2);
        
        data.set("Vac3", this->Vac3);
        data.set("Iac3", this->Iac3);
        data.set("Pac3", this->Pac3);
    }

    data.set("Etoday", this->Etoday);
    data.set("Etotal", this->Etotal);
    data.set("Ttotal", this->Ttotal);

    data.set("Temp1", this->tempInverter);
    data.set("Temp2", this->tempIPM);
    
    return data;
}

void MicInverter::setIncomingTopicData(const String &topic, const String &value) {
    GLOG::println(F("INVERTER: no remote control: task rejected"));
}

std::list<String> MicInverter::getTopicsToSubscribe() {
    // no subscriptions, no commands
    return std::list<String>();
}
