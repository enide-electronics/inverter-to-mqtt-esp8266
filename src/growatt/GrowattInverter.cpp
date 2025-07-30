/*
  GrowattInverter.cpp - Library for the ESP8266/ESP32 Arduino platform
  To read data from Growatt SPH and SPA inverters

  Based heavily on the growatt-esp8266 project at https://github.com/jkairys/growatt-esp8266

  Modified by JF enide.electronics (at) enide.net
  Licensed under GNU GPLv3
*/
#include "GrowattInverter.h"
#include "../GLog.h"
#include "GrowattTaskFactory.h"
#include "../Task.h"
#include "../ModbusUtils.h"


static uint8_t stateSequence[] = {0, 1, 3, 0, 1, 4, 0, 1, 3, 0, 1, 4, 2};
void GrowattInverter::incrementStateIdx() {
    currentStateIdx += 1;
    
    if (currentStateIdx >= (sizeof(stateSequence)/sizeof(uint8_t))) {
        currentStateIdx = 0;
    }   
}

void GrowattInverter::incrementModbusAddress() {
    currentModbusIdx += 1;
    if (currentModbusIdx >= this->slaveAddresses.size()) {
        currentModbusIdx = 0;
    }
}

float GrowattInverter::glueFloat(uint16_t w1, uint16_t w0) {
  unsigned long t;
  t = w1 << 16;
  t += w0;

  float f;
  f = t;
  f = f / 10;
  return f;
}

void GrowattInverter::read() {

    // run tasks, if any
    if (incomingTasks.size() > 0) {
        runningTask = incomingTasks.front();
        incomingTasks.pop_front();
        
        GLOG::print(", TASK starting");
        
        runningTask->run();
        this->valid = true; // it's always true even if the task fails be cause we will always return a Ok/Fail message on the "task_topic"/result
        
        GLOG::print(", completed");
            
        return;
    }
    
    // read data
    GLOG::print(String(", step=") + stateSequence[currentStateIdx]);

    if (stateSequence[currentStateIdx] == 0) {
        uint8_t result1 = this->node->readInputRegisters(0, 12);
        if (result1 == this->node->ku8MBSuccess) {

            this->status = this->node->getResponseBuffer(0);

            // 2 PV inputs
            this->Vpv1 = this->glueFloat(0, this->node->getResponseBuffer(3));
            this->Ipv1 = this->glueFloat(0, this->node->getResponseBuffer(4)) / 10.0;
            this->Ppv1 = this->glueFloat(this->node->getResponseBuffer(5), this->node->getResponseBuffer(6));

            this->Vpv2 = this->glueFloat(0, this->node->getResponseBuffer(7));
            this->Ipv2 = this->glueFloat(0, this->node->getResponseBuffer(8)) / 10.0;
            this->Ppv2 = this->glueFloat(this->node->getResponseBuffer(9), this->node->getResponseBuffer(10));

            this->valid = true;
        } else {
            this->valid = false;
        }
        
    } else if (stateSequence[currentStateIdx] == 1) {
        // AC stuff, including the other phases for TL SPH inverters and energy produced
        // start reading at 35 and read up to 24 registers
        uint8_t result2 = this->node->readInputRegisters(35, 24);
        if (result2 == this->node->ku8MBSuccess) {
            this->Pac = this->glueFloat(this->node->getResponseBuffer(0), this->node->getResponseBuffer(1)); // 35, 36
            this->Fac = this->glueFloat(0, this->node->getResponseBuffer(2))/10; // 37

            this->Vac1 = this->glueFloat(0, this->node->getResponseBuffer(3)); // 38
            this->Iac1 = this->glueFloat(0, this->node->getResponseBuffer(4)); // 39
            this->Pac1 = this->glueFloat(this->node->getResponseBuffer(5), this->node->getResponseBuffer(6)); // 40, 41
            if (this->enableTL) {
                this->Vac2 = this->glueFloat(0, this->node->getResponseBuffer(7)); //42
                this->Iac2 = this->glueFloat(0, this->node->getResponseBuffer(8)); //43
                this->Pac2 = this->glueFloat(this->node->getResponseBuffer(9), this->node->getResponseBuffer(10)); //44, 45

                this->Vac3 = this->glueFloat(0, this->node->getResponseBuffer(11)); //46
                this->Iac3 = this->glueFloat(0, this->node->getResponseBuffer(12)); //47
                this->Pac3 = this->glueFloat(this->node->getResponseBuffer(13), this->node->getResponseBuffer(14)); //48, 49
            }
            this->Etoday = this->glueFloat(this->node->getResponseBuffer(18), this->node->getResponseBuffer(19)); //53, 54
            this->Etotal = this->glueFloat(this->node->getResponseBuffer(20), this->node->getResponseBuffer(21)); //55, 56
            this->Ttotal = this->glueFloat(this->node->getResponseBuffer(22), this->node->getResponseBuffer(23)); //57, 58
            
            this->valid = true;
        } else {
            this->valid = false;
        }
        
    } else  if (stateSequence[currentStateIdx] == 2) {
        // Temperatures, Battery and Priority (LoadFirst, BatFirst, GridFirst)
        // start reading at register 93 and read up to 30 registers
        uint8_t result3 = this->node->readInputRegisters(93, 30);
        if (result3 == this->node->ku8MBSuccess) {

            this->temp1 = this->glueFloat(0, this->node->getResponseBuffer(0)); //93
            this->temp2 = this->glueFloat(0, this->node->getResponseBuffer(1)); //94
            this->temp3 = this->glueFloat(0, this->node->getResponseBuffer(2)); //95
            
            this->deratingMode = this->node->getResponseBuffer(11); //104

            this->Priority = this->node->getResponseBuffer(25); //118
            this->BatteryType = this->node->getResponseBuffer(26); //119
            
            this->valid = true;
        } else {
            this->valid = false;
        }
        
    } else if (stateSequence[currentStateIdx] == 3) {
        // Battery status
        // start reading at register 1009 and read up to 6 registers
        uint8_t result4 = this->node->readInputRegisters(1009, 6);
        if (result4 == this->node->ku8MBSuccess) {
            // ModbusUtils::dumpRegisters(this->node, 6);
            this->Pdischarge = this->glueFloat(this->node->getResponseBuffer(0), this->node->getResponseBuffer(1)); //1009, 1010
            this->Pcharge = this->glueFloat(this->node->getResponseBuffer(2), this->node->getResponseBuffer(3)); //1011, 1012
            this->Vbat = this->glueFloat(0, this->node->getResponseBuffer(4)); //1013
            this->SOC = this->node->getResponseBuffer(5); // 1014
            
            this->valid = true;
        } else {
            this->valid = false;
        }
        
    } else {
        // EPS
        // EPS starts at register 1067 and is 15 registers long (see page 44)
        uint8_t result5 = this->node->readInputRegisters(1067, 15);
        if (result5 == this->node->ku8MBSuccess) {

            this->EpsFac = this->glueFloat(0, this->node->getResponseBuffer(0)) / 10.0; //1067

            this->EpsVac1 = this->glueFloat(0, this->node->getResponseBuffer(1)); //1068
            this->EpsIac1 = this->glueFloat(0, this->node->getResponseBuffer(2)); //1069
            this->EpsPac1 = this->glueFloat(this->node->getResponseBuffer(3), this->node->getResponseBuffer(4)); //1070, 1071

            if (this->enableTL) {
                this->EpsVac2 = this->glueFloat(0, this->node->getResponseBuffer(5)); //1072
                this->EpsIac2 = this->glueFloat(0, this->node->getResponseBuffer(6)); //1073
                this->EpsPac2 = this->glueFloat(this->node->getResponseBuffer(7), this->node->getResponseBuffer(8)); //1074, 1075

                this->EpsVac3 = this->glueFloat(0, this->node->getResponseBuffer(9)); //1076
                this->EpsIac3 = this->glueFloat(0, this->node->getResponseBuffer(10)); //1077
                this->EpsPac3 = this->glueFloat(this->node->getResponseBuffer(11), this->node->getResponseBuffer(12)); //1078, 1079
            }

            this->EpsLoadPercent = this->glueFloat(0, this->node->getResponseBuffer(13)); //1080
            this->EpsPF = this->glueFloat(0, this->node->getResponseBuffer(14)) / 100.0; //1081
            
            this->valid = true;
        } else {
            this->valid = false;
        }
        
    }
    
    lastUpdatedState = stateSequence[currentStateIdx];
    
    // iterate currentStateIdx over all inverters before changing to the next state
    // this will interleave data from multiple inverters over time and prevents inverters from being updated only after the entire state list has run for each of them
    
    incrementModbusAddress();
    this->node->begin(this->slaveAddresses[this->currentModbusIdx], *serial);

    if (this->currentModbusIdx == 0) {
        incrementStateIdx();
    }
}


GrowattInverter::GrowattInverter(Stream *serial, bool shouldDeleteSerial, std::vector<int> slaveAddresses, bool enableRemoteCommands, bool enableThreePhases) {
    this->serial = serial;
    this->shouldDeleteSerial = shouldDeleteSerial;
    this->enableRemoteCommands = enableRemoteCommands;
    this->enableTL = enableThreePhases;

    this->currentStateIdx = 0;
    this->lastUpdatedState = 0;
    this->slaveAddresses = slaveAddresses;
    this->currentModbusIdx = 0;
    if (this->slaveAddresses.size() == 0) {
        this->slaveAddresses = {1};
    }
    this->node = new ModbusMaster();
    this->node->begin(this->slaveAddresses[this->currentModbusIdx], *serial);

    this->valid = false;
    this->status = 0;
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

    this->temp1 = 0.0;
    this->temp2 = 0.0;
    this->temp3 = 0.0;

    this->deratingMode = 0;
    this->Priority = 0;
    this->BatteryType = 0;

    this->Pdischarge = 0.0;
    this->Pcharge = 0.0;
    this->Vbat = 0.0;
    this->SOC = 0;

    this->EpsFac = 0;

    this->EpsPac1 = 0.0;
    this->EpsVac1 = 0.0;
    this->EpsIac1 = 0.0;

    this->EpsPac2 = 0.0;
    this->EpsVac2 = 0.0;
    this->EpsIac2 = 0.0;

    this->EpsPac3 = 0.0;
    this->EpsVac3 = 0.0;
    this->EpsIac3 = 0.0;

    this->EpsLoadPercent = 0.0;
    this->EpsPF = 0.0;
}

GrowattInverter::~GrowattInverter() {
    delete this->node;

    if (this->shouldDeleteSerial) {
        delete this->serial;
    }
}

bool GrowattInverter::isDataValid() {
    return this->valid;
}

InverterData GrowattInverter::getData(bool fullSet) {
    InverterData data;
    
    // handle task data
    if (runningTask != NULL) {
        // return task data
        if (runningTask->isSuccessful()) {
            InverterData respData = runningTask->response();
            data.insert(respData.begin(), respData.end());
        }
        // append task result
        data.set((runningTask->subtopic()+"/result").c_str(), runningTask->isSuccessful() ? "Ok" : "Fail");
        
        delete runningTask;
        runningTask = NULL;
        
        this->valid = false;
        
        return data;
    }
    
    // handle read data
    if (lastUpdatedState == 0 || fullSet) {
        data.set("status", this->status);
      
        data.set("Ppv1", this->Ppv1);    
        data.set("Vpv1", this->Vpv1);
        data.set("Ipv1", this->Ipv1);
      
        data.set("Ppv2", this->Ppv2);   
        data.set("Vpv2", this->Vpv2);
        data.set("Ipv2", this->Ipv2);
    }

    if (lastUpdatedState == 1 || fullSet) {
        data.set("Vac1", this->Vac1);
        data.set("Iac1", this->Iac1);
        data.set("Pac1", this->Pac1);
        
        data.set("Pac", this->Pac);
        data.set("Fac", this->Fac);
        
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
    } 

    if (lastUpdatedState == 2 || fullSet) {
        data.set("Temp1", this->temp1);
        data.set("Temp2", this->temp2);
        data.set("Temp3", this->temp3);
        
        data.set("DeratingMode", this->deratingMode);
        
        switch(this->deratingMode) {
            case 0:
                data.set("Derating", "None");
            break;
            case 1:
                data.set("Derating", "PV");
            break;
            case 2:
                data.set("Derating", "*");
            break;
            case 3:
                data.set("Derating", "Vac");
            break;
            case 4:
                data.set("Derating", "Fac");
            break;
            case 5:
                data.set("Derating", "Tboost");
            break;
            case 6:
                data.set("Derating", "Tinv");
            break;
            case 7:
                data.set("Derating", "Control");
            break;
            case 8:
                data.set("Derating", "*");
            break;
            case 9:
                data.set("Derating", "OverBackByTime");
            break;
            default:
                data.set("Derating", "Unknown");
        }
      
        switch (this->Priority) {
            case 0:
                data.set("Priority", "Load");
            break;
            case 1:
                data.set("Priority", "Bat");
            break;
            case 2:
                data.set("Priority", "Grid");
            break;
            default:
                data.set("Priority", (String("Unknown ") + this->Priority).c_str());
        }

        // Battery
        switch (this->BatteryType) {
            case 0:
                data.set("Battery", "LeadAcid");
            break;
            
            case 1:
                data.set("Battery", "Lithium");
            break;
            default:
                data.set("Battery", (String("Unknown type ") + this->BatteryType).c_str());
        }
    }

    if (lastUpdatedState == 3 || fullSet) {
        data.set("Pdischarge", this->Pdischarge);
        data.set("Pcharge", this->Pcharge);
        data.set("Vbat", this->Vbat);
        data.set("SOC", this->SOC);
    }

    if (lastUpdatedState == 4 || fullSet) {
        // EPS
        data.set("EpsFac", this->EpsFac);

        data.set("EpsPac1", this->EpsPac1);
        data.set("EpsVac1", this->EpsVac1);
        data.set("EpsIac1", this->EpsIac1);

        if (this->enableTL) {
            data.set("EpsPac2", this->EpsPac2);
            data.set("EpsVac2", this->EpsVac2);
            data.set("EpsIac2", this->EpsIac2);
            
            data.set("EpsPac3", this->EpsPac3);
            data.set("EpsVac3", this->EpsVac3);
            data.set("EpsIac3", this->EpsIac3);
        }

        data.set("EpsLoadPercent", this->EpsLoadPercent);
        data.set("EpsPF", this->EpsPF);
    }
    
    return data;
}



void GrowattInverter::setIncomingTopicData(const String &topic, const String &value)
{
    if (incomingTasks.size() > 3) {
        GLOG::println(F("INVERTER: tasks queue full: task rejected"));
        return;
    }
    
    Task* task = GrowattTaskFactory::create(this->node, topic, value);
    if (task != NULL) {
        incomingTasks.push_back(task);
        GLOG::println(String(F("INVERTER: accepted task topic=[")) + topic + F("], value=[") + value + F("]"));
    } else {
        GLOG::println(String(F("INVERTER: unknown task topic=[")) + topic + F("], value=[") + value + F("]"));
    }
    
}

std::list<String> GrowattInverter::getTopicsToSubscribe()
{
    if (this->enableRemoteCommands) {
        return GrowattTaskFactory::registeredSubtopics();
    } else {
        // no subscriptions, no commands
        return std::list<String>();
    }
}
