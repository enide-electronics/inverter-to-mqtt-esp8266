/*
  AxpertVMIII.cpp - The Voltronic Axpert VM III off-grid inverter implementation

  Based on code from several project:
  - https://github.com/amishv/voltronic_ESP8266_MQTT/blob/main/src/communication.cpp
  - https://github.com/ned-kelly/docker-voltronic-homeassistant/blob/master/sources/inverter-cli/inverter.cpp

  Written by JF enide.electronics (at) enide.net
  Licensed under GNU GPLv3
*/

#include "AxpertVMIII.h"

VoltronicAxpertVMIIIInverter::VoltronicAxpertVMIIIInverter(Stream *serial, bool shouldDeleteSerial) 
    : VoltronicInverter(serial, shouldDeleteSerial) {
    state = 0;
        
}

VoltronicAxpertVMIIIInverter::~VoltronicAxpertVMIIIInverter() {
}

void VoltronicAxpertVMIIIInverter::read() {
    // switch state
    // 0: read QMOD
    // 1: send QPIRI
    // 2: read QPIGS
    // 3: read QPIWS
    isValid = false;

    switch (state) {
        case 0:
            state = 1;
            readMode();
            break;
        case 1:
            state = 2;
            readRatedInformation();
            break;
        case 2:
            state = 3;
            readGeneralStatus();
            break;
        case 3:
            state = 0;
            readWarnings();
            break;
        default:
            state = 0;
    }
}

bool VoltronicAxpertVMIIIInverter::isDataValid() {
    return isValid;
}


InverterData VoltronicAxpertVMIIIInverter::getData(bool fullSet) {
    return inverterData;
}


void VoltronicAxpertVMIIIInverter::setIncomingTopicData(const String &topic, const String &value) {
    // nothing for now
}

std::list<String> VoltronicAxpertVMIIIInverter::getTopicsToSubscribe() {
    std::list<String> topics;
    // add topic in the future if we want to remotely control the inverter
    return topics;
}


void VoltronicAxpertVMIIIInverter::readRatedInformation() {
    if (sendCommand("QPIRI")) {
        String response = recvResponse(98);
        inverterData.clear();

        if (response != "") {
            float grid_voltage_rating = 0.0;
            float grid_current_rating = 0.0;
            float out_voltage_rating = 0.0;
            float out_freq_rating = 0.0;
            float out_current_rating = 0.0;
            int out_va_rating = 0;
            int out_watt_rating = 0;
            float batt_rating = 0.0;
            float batt_recharge_voltage = 0.0;
            float batt_under_voltage = 0.0;
            float batt_bulk_voltage = 0.0;
            float batt_float_voltage = 0.0;
            int batt_type = 0;
            int max_grid_charge_current = 0;
            int max_charge_current = 0;
            int in_voltage_range = 0;
            int out_source_priority = 0;
            int charger_source_priority = 0;
            int machine_type = 0;
            int topology = 0;
            int out_mode = 0;
            float batt_redischarge_voltage = 0.0;
                
            sscanf(response.c_str(), "%f %f %f %f %f %d %d %f %f %f %f %f %d %d %d %d %d %d %*c %d %d %d %f", 
            &grid_voltage_rating, &grid_current_rating, &out_voltage_rating, &out_freq_rating, &out_current_rating, 
            &out_va_rating, &out_watt_rating, &batt_rating, &batt_recharge_voltage, &batt_under_voltage, 
            &batt_bulk_voltage, &batt_float_voltage, &batt_type, &max_grid_charge_current, &max_charge_current, 
            &in_voltage_range, &out_source_priority, &charger_source_priority, &machine_type, &topology, &out_mode, 
            &batt_redischarge_voltage);

            inverterData.set("VbatRecharge", batt_recharge_voltage);
            inverterData.set("VbatUnderVoltage", batt_under_voltage);
            inverterData.set("VbatBulkVoltage", batt_bulk_voltage);
            inverterData.set("VbatFloatVoltage", batt_float_voltage);
            inverterData.set("ImaxGridChargeCurrent", max_grid_charge_current);
            inverterData.set("ImaxChargeCurrent", max_charge_current);
            inverterData.set("PrioritySourceOut", out_source_priority);
            inverterData.set("PrioritySourceCharger", charger_source_priority);
            inverterData.set("VbatRedischargeVoltage", batt_redischarge_voltage);

            isValid = true;
        }
    }
}

void VoltronicAxpertVMIIIInverter::readGeneralStatus() {
    if (sendCommand("QPIGS")) {
        String response = recvResponse(110);
        inverterData.clear();

        if (response != "") {
            float voltage_grid = 0.0;
            float freq_grid = 0.0;
            float voltage_out = 0.0;
            float freq_out = 0.0;
            int load_va = 0;
            int load_watt = 0;
            int load_percent = 0;
            int voltage_bus = 0;
            float voltage_batt = 0.0;
            int batt_charge_current = 0;
            int batt_capacity = 0;
            int temp_heatsink = 0;
            float pv_input_current = 0.0;
            float pv_input_voltage = 0.0;
            float pv_input_watts = 0.0;
            float pv_input_watthour = 0.0;
            float load_watthour = 0.0;
            float scc_voltage = 0.0;
            int batt_discharge_current = 0;
            char device_status[9];
            memset(device_status, '\0', 9);

            sscanf(response.c_str(), "%f %f %f %f %d %d %d %d %f %d %d %d %f %f %f %d %s", 
            &voltage_grid, &freq_grid, &voltage_out, &freq_out, &load_va, &load_watt, &load_percent, 
            &voltage_bus, &voltage_batt, &batt_charge_current, &batt_capacity, &temp_heatsink, 
            &pv_input_current, &pv_input_voltage, &scc_voltage, &batt_discharge_current, device_status);

            inverterData.set("Vac", voltage_grid);
            inverterData.set("Fac", freq_grid);
            inverterData.set("VacOut", voltage_out);
            inverterData.set("FacOut", freq_out);
            inverterData.set("Vpv", pv_input_voltage);
            inverterData.set("Ipv", pv_input_current);
            inverterData.set("Ppv", pv_input_watts);
            inverterData.set("Epv", pv_input_watthour);
            inverterData.set("Vscc", scc_voltage);
            inverterData.set("LoadPercent", load_percent);
            inverterData.set("Pload", load_watt);
            inverterData.set("Eload", load_watthour);
            inverterData.set("PloadVA", load_va);
            inverterData.set("Vbus", voltage_bus);
            inverterData.set("TempHeatsink", temp_heatsink);
            inverterData.set("BatteryCapacity", batt_capacity);
            inverterData.set("Vbat", voltage_batt);
            inverterData.set("IbatCharge", batt_charge_current);
            inverterData.set("IbatDischarge", batt_discharge_current);
            inverterData.set("LoadStatusON", device_status[3]);
            inverterData.set("SCCchargeON", device_status[6]);
            inverterData.set("ACchargeON", device_status[7]);

            isValid = true;
        }
    }
}

void VoltronicAxpertVMIIIInverter::readWarnings() {
    if (sendCommand("QPIWS")) {
        String response = recvResponse(96);
        inverterData.clear();

        if (response != "") {
            inverterData.set("Warnings", response);
            isValid = true;
        }
    }
    
}

void VoltronicAxpertVMIIIInverter::readMode() {
    if (sendCommand("QMOD")) {
        String response = recvResponse(5);
        inverterData.clear();

        if (response != "") {
            uint8_t result;
            char mode = response.charAt(0);

            switch (mode) {
                case 'P': result = 1;   break;  // Power_On
                case 'S': result = 2;   break;  // Standby
                case 'L': result = 3;   break;  // Line
                case 'B': result = 4;   break;  // Battery
                case 'F': result = 5;   break;  // Fault
                case 'H': result = 6;   break;  // Power_Saving
                default:  result = 0;   break;  // Unknown
            }

            inverterData.set("InverterMode", result);
            isValid = true;
        }
    }
}
