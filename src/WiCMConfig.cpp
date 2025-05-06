/*
  WiCMConfig.cpp - WifiManager configurations (Wifi and Parameters)
  
  Written by JF enide.electronics (at) enide.net
  Licensed under GNU GPLv3
*/
#include "WiCMConfig.h"
#include "GLog.h"
#include <ArduinoJson.h>
#include <FS.h>

// global
#define DEFAULT_TOPIC "inverter"
#define DEFAULT_SOFTAP_PASSWORD "12345678"
#define DEFAULT_DEVICE_NAME "inverter-to-mqtt-esp8266"

// parameter config
#define DEVICE_NAME_K "device_name"
#define SOFTAP_PASSWORD_K "web_password"
#define MQTT_SERVER_K "mqtt_server"
#define MQTT_PORT_K "mqtt_port"
#define MQTT_USERNAME_K "mqtt_username"
#define MQTT_PASSWORD_K "mqtt_password"
#define MQTT_TOPIC_K "mqtt_topic"
#define MODBUS_ADDR_K "modbus_addr"
#define MODBUS_POLLING_K "modbus_poll_secs"
#define INVERTER_MODEL_K "inverter_model"
#define PARAMS_FILE "/config.json"

// wifi config 
#define IP_K "ip"
#define GW_K "gw"
#define SN_K "sn"
#define DNS_K "dns"
#define STA_WIFI_PARAMS_FILE "/wificonfig.json"

// helpers
#define SHOW_JSON_FILE

static void eraseFile(const char *filename) {
    if (SPIFFS.exists(filename)) {
        SPIFFS.remove(filename);
    }
}

WiCMParamConfig::WiCMParamConfig() {
    this->deviceName = DEFAULT_DEVICE_NAME;
    this->softApPassword = DEFAULT_SOFTAP_PASSWORD;
    this->mqttServer = "localhost";
    this->mqttPort = 1883;
    this->mqttUsername = "";
    this->mqttPassword = "";
    this->mqttBaseTopic = DEFAULT_TOPIC;
    this->modbusAddress = 1;
    this->modbusPollingInSeconds = 5;
    this->inverterType = "none";
}
WiCMParamConfig::~WiCMParamConfig(){};

void WiCMParamConfig::save() {
    //save the custom parameters to FS

    GLOG::println(F("WiCM: Saving config file"));

    #if ARDUINOJSON_VERSION_MAJOR >= 6
        DynamicJsonDocument json(1024);
    #else
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.createObject();
    #endif

        json[DEVICE_NAME_K] = deviceName.c_str();
        json[SOFTAP_PASSWORD_K] = softApPassword.c_str();
        json[MQTT_SERVER_K] = mqttServer.c_str();
        json[MQTT_PORT_K] = mqttPort;
        mqttUsername.trim();
        json[MQTT_USERNAME_K] = mqttUsername.c_str();
        mqttPassword.trim();
        json[MQTT_PASSWORD_K] = mqttPassword.c_str();
        json[MQTT_TOPIC_K] = mqttBaseTopic.c_str();
        json[MODBUS_ADDR_K] = modbusAddress;
        json[MODBUS_POLLING_K] = modbusPollingInSeconds;
        json[INVERTER_MODEL_K] = inverterType.c_str();

        File configFile = SPIFFS.open(F(PARAMS_FILE), "w");
        if (!configFile) {
            GLOG::println(F("WiCM: Save failed"));
        }

    #if ARDUINOJSON_VERSION_MAJOR >= 6
        serializeJson(json, configFile);
    #else
        json.printTo(configFile);
    #endif
        configFile.close();

    //end save
}

void WiCMParamConfig::load() {
    //read configuration from FS json
    if (SPIFFS.exists(F(PARAMS_FILE))) {
        GLOG::println(F("WiCM: read config file"));
        File configFile = SPIFFS.open(F(PARAMS_FILE), "r");
        if (configFile) {
            GLOG::println(F("WiCM: open config file OK"));
            size_t size = configFile.size();

            // Allocate a buffer to store contents of the file.
            std::unique_ptr<char[]> buf(new char[size]);
            configFile.readBytes(buf.get(), size);

#if ARDUINOJSON_VERSION_MAJOR >= 6
            DynamicJsonDocument json(1024);
            auto deserializeError = deserializeJson(json, buf.get());
            
            if ( ! deserializeError ) {
#else
            DynamicJsonBuffer jsonBuffer;
            JsonObject& json = jsonBuffer.parseObject(buf.get());
            
            if (json.success()) {
#endif
                GLOG::println(F("WiCM: config json parsed"));
#ifdef SHOW_JSON_FILE
                String jsonStringified;
                serializeJson(json, jsonStringified);
                GLOG::println(String(F("WiCM: ")) + jsonStringified);
#endif

                if (json.containsKey(DEVICE_NAME_K)) {
                    deviceName = json[DEVICE_NAME_K].as<String>();
                } else {
                    deviceName = DEFAULT_DEVICE_NAME;
                }
                
                if (json.containsKey(SOFTAP_PASSWORD_K)) {
                    softApPassword = json[SOFTAP_PASSWORD_K].as<String>();
                } else {
                    softApPassword = DEFAULT_SOFTAP_PASSWORD;
                }
                
                if (json.containsKey(MQTT_SERVER_K)) {
                    mqttServer = json[MQTT_SERVER_K].as<String>();
                } else {
                    mqttServer = "";
                }

                if (json.containsKey(MQTT_PORT_K)) {
                    mqttPort = json[MQTT_PORT_K];
                } else {
                    mqttPort = 1883;
                }

                if (json.containsKey(MQTT_TOPIC_K)) {
                    mqttBaseTopic = json[MQTT_TOPIC_K].as<String>();
                } else {
                    mqttBaseTopic = DEFAULT_TOPIC;
                }
                
                if (json.containsKey(MQTT_USERNAME_K)) {
                    mqttUsername = json[MQTT_USERNAME_K].as<String>();
                } else {
                    mqttUsername = "";
                }
                
                if (json.containsKey(MQTT_PASSWORD_K)) {
                    mqttPassword = json[MQTT_PASSWORD_K].as<String>();
                } else {
                    mqttPassword = "";
                }

                if (json.containsKey(MODBUS_ADDR_K)) {
                    modbusAddress = json[MODBUS_ADDR_K];
                } else {
                    modbusAddress = 1;
                }
                
                if (json.containsKey(MODBUS_POLLING_K)) {
                    modbusPollingInSeconds = json[MODBUS_POLLING_K];
                } else {
                    modbusPollingInSeconds = 5;
                }

                if (json.containsKey(INVERTER_MODEL_K)) {
                    inverterType = json[INVERTER_MODEL_K].as<String>();
                    if (inverterType == "") {
                        inverterType = "none";
                    }
                } else {
                    inverterType = "none";
                }
            } else {
                GLOG::println(F("WiCM: config file parse error"));
            }
            configFile.close();
            GLOG::println(F("WiCM: read config file OK"));
        }
    } else {
        GLOG::println(F("WiCM: config file not found"));
    }

    //end read
}

void WiCMParamConfig::erase() {
    eraseFile(PARAMS_FILE);
}

WiCMWifiConfig::WiCMWifiConfig() {
}

WiCMWifiConfig::~WiCMWifiConfig() {
}

void WiCMWifiConfig::load() {
    if (SPIFFS.exists(F(STA_WIFI_PARAMS_FILE))) {
        GLOG::println(F("WiCM: read wifi file"));
        File networkFile = SPIFFS.open(F(STA_WIFI_PARAMS_FILE), "r");
        if (networkFile) {
            GLOG::println(F("WiCM: open wifi file OK"));
            size_t size = networkFile.size();

            // Allocate a buffer to store contents of the file.
            std::unique_ptr<char[]> buf(new char[size]);
            networkFile.readBytes(buf.get(), size);

#if ARDUINOJSON_VERSION_MAJOR >= 6
            DynamicJsonDocument json(1024);
            auto deserializeError = deserializeJson(json, buf.get());
            
            if ( ! deserializeError ) {
#else
            DynamicJsonBuffer jsonBuffer;
            JsonObject& json = jsonBuffer.parseObject(buf.get());
            
            if (json.success()) {
#endif
                GLOG::println(F("WiCM: wifi json parsed"));
#ifdef SHOW_JSON_FILE
                String jsonStringified;
                serializeJson(json, jsonStringified);
                GLOG::println(String(F("WiCM: ")) + jsonStringified);
#endif

                if (json.containsKey(IP_K)) {
                    ip.fromString(json[IP_K].as<String>());
                } else {
                    ip = IPAddress();
                }
                
                if (json.containsKey(GW_K)) {
                    gw.fromString(json[GW_K].as<String>());
                } else {
                    gw = IPAddress();
                }

                if (json.containsKey(SN_K)) {
                    sn.fromString(json[SN_K].as<String>());
                } else {
                    sn = IPAddress();
                }
                
                if (json.containsKey(DNS_K)) {
                    dns.fromString(json[DNS_K].as<String>());
                } else {
                    dns = IPAddress();
                }
                
            } else {
                GLOG::println(F("WiCM: wifi json parse error"));
            }
            networkFile.close();
            GLOG::println(F("WiCM: read wifi file OK"));
        }
    } else {
        GLOG::println(F("WiCM: wifi file not found"));
    }
}

void WiCMWifiConfig::save() {
    GLOG::println(F("WiCM: save wifi file"));

    #if ARDUINOJSON_VERSION_MAJOR >= 6
        DynamicJsonDocument json(1024);
    #else
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.createObject();
    #endif
        if (ip.isSet()) {
            json[IP_K] = ip.toString();
        }
        if (gw.isSet()) {
            json[GW_K] = gw.toString();
        }
        if (sn.isSet()) {
            json[SN_K] = sn.toString();
        }
        if (dns.isSet()) {
            json[DNS_K] = dns.toString();
        }

        File networkFile = SPIFFS.open(F(STA_WIFI_PARAMS_FILE), "w");
        if (!networkFile) {
            GLOG::println(F("WiCM: save wifi file failed"));
        }

    #if ARDUINOJSON_VERSION_MAJOR >= 6
        serializeJson(json, networkFile);
    #else
        json.printTo(networkFile);
    #endif
    networkFile.close();
    GLOG::println(F("WiCM: save wifi OK"));
}

void WiCMWifiConfig::erase() {
    eraseFile(STA_WIFI_PARAMS_FILE);
}

bool WiCMWifiConfig::isStaticIPConfigured() {
    return ip.isSet() && gw.isSet() && sn.isSet() && dns.isSet();
}
