/*
  WiCMConfig.h - WifiManager configurations (Wifi and Parameters)
  
  Written by JF enide.electronics (at) enide.net
  Licensed under GNU GPLv3
*/

#ifndef WICM_CONFIG_H
#define WICM_CONFIG_H

#include <Arduino.h>
#include <vector>
#include <IPAddress.h>

// Setup vars
class WiCMParamConfig {
    public:
        String filename;

        String deviceName;
        String softApPassword;
        String mqttServer;
        int mqttPort;
        String mqttUsername;
        String mqttPassword;
        String mqttBaseTopic;
        std::vector<int> modbusAddresses;
        int modbusPollingInSeconds;
        String inverterType;
        
        WiCMParamConfig();
        virtual ~WiCMParamConfig();

        void save();
        void load();
        void erase();
};

// WiFi params (Static IP & friends)
class WiCMWifiConfig {
    public:
        IPAddress ip;
        IPAddress gw;
        IPAddress sn;
        IPAddress dns;

        WiCMWifiConfig();
        virtual ~WiCMWifiConfig();

        void save();
        void load();
        void erase();

        bool isStaticIPConfigured();
};

#endif