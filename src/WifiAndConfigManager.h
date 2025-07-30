/*
  WifiAndConfigManager.h - Library header for the ESP8266/ESP32 Arduino platform
  SPIFFS based configuration

  Written by JF enide.electronics (at) enide.net
  Licensed under GNU GPLv3
*/

#ifndef WIFI_AND_CONFIG_MANAGER_H
#define WIFI_AND_CONFIG_MANAGER_H

#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <ArduinoJson.h>
#include "WiCMConfig.h"
#include <vector>


class WifiAndConfigManager {
    private:
        WiFiManager wm;
        
        // Setup params
        WiFiManagerParameter *deviceNameParam;
        WiFiManagerParameter *softApPasswordParam;
        WiFiManagerParameter *mqttServerParam;
        WiFiManagerParameter *mqttPortParam;
        WiFiManagerParameter *mqttUsernameParam;
        WiFiManagerParameter *mqttPasswordParam;
        WiFiManagerParameter *mqttBaseTopicParam;
        WiFiManagerParameter *modbusAddressParam;
        WiFiManagerParameter *modbusPollingInSecondsParam;
        
        char inverterModelCustomFieldBufferStr[800];
        WiFiManagerParameter *inverterModelCustomFieldParam;
        WiFiManagerParameter *inverterTypeCustomHidden;
        
        // setup vars
        WiCMParamConfig paramsCfg;

        // wifi params (Static IP & friends)
        WiCMWifiConfig wifiCfg;
        
        // Flags
        bool saveWifiStaticIPRequired;
        bool saveParamsRequired;
        bool rebootRequired;
        bool wifiConnected;
        
        void copyFromParamsToVars();
        void show();
        void saveParamConfigCallback();
        void saveWifiConfigCallback();
        void handleEraseAll();
        String getParam(String name);
        void _updateInverterTypeSelect();
        void _recycleParams();

    public:
        WifiAndConfigManager();

        void setupWifiAndConfig();

        String getDeviceName();
        String getMqttServer();
        int getMqttPort();
        String getMqttUsername();
        String getMqttPassword();
        String getMqttTopic();
        std::vector<int> getModbusAddresses();
        int getModbusPollingInSeconds();
        String getInverterType();

        WiFiManager & getWM();
        void loop();
        
        
        void doFactoryReset();
        bool checkforConfigChanges();
        bool isRestartRequired();
        bool isWifiConnected();
};

#endif
