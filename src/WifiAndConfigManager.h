/*
  WifiAndConfigManager.h - Library header for the ESP8266/ESP32 Arduino platform
  LittleFS based configuration

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

#define _IMCFBS_SIZE 890


class WifiAndConfigManager {
    private:
        WiFiManager wm;
        
        // Setup params
        WiFiManagerParameter *networkSectionHeaderParam;
        WiFiManagerParameter *deviceNameParam;
        WiFiManagerParameter *softApPasswordParam;
        WiFiManagerParameter *mqttSectionHeaderParam;
        WiFiManagerParameter *mqttServerParam;
        WiFiManagerParameter *mqttPortParam;
        WiFiManagerParameter *mqttUsernameParam;
        WiFiManagerParameter *mqttPasswordParam;
        WiFiManagerParameter *mqttBaseTopicParam;
        WiFiManagerParameter *inverterSectionHeaderParam;
        WiFiManagerParameter *modbusAddressParam;
        WiFiManagerParameter *modbusPollingInSecondsParam;
        
        char inverterModelCustomFieldBufferStr[_IMCFBS_SIZE];
        WiFiManagerParameter *inverterModelCustomFieldParam;
        WiFiManagerParameter *inverterTypeCustomHidden;

        // Temperature controller params
        WiFiManagerParameter *tempCtrlSectionHeaderParam;
        WiFiManagerParameter *tempCtrlEnabledCustomParam;
        WiFiManagerParameter *tempCtrlEnabledHidden;
        WiFiManagerParameter *tempCtrlTopicParam;
        WiFiManagerParameter *tempCtrlPayloadOnParam;
        WiFiManagerParameter *tempCtrlPayloadOffParam;
        WiFiManagerParameter *tempCtrlThresholdOnParam;
        WiFiManagerParameter *tempCtrlThresholdOffParam;
        char tempCtrlEnabledBuffer[512];

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
        void _updateTempCtrlCheckbox();
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

        bool getTempCtrlEnabled();
        String getTempCtrlTopic();
        String getTempCtrlPayloadOn();
        String getTempCtrlPayloadOff();
        float getTempCtrlThresholdOn();
        float getTempCtrlThresholdOff();

        WiFiManager & getWM();
        void loop();
        
        
        void doFactoryReset();
        bool checkforConfigChanges();
        bool isRestartRequired();
        bool isWifiConnected();
};

#endif
