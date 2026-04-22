/*
  WiCMConfig.cpp - WifiManager configurations (Wifi and Parameters)
  
  Written by JF enide.electronics (at) enide.net
  Licensed under GNU GPLv3
*/
#include "WiCMConfig.h"
#include "GLog.h"
#include <ArduinoJson.h>
#include <FS.h>
#include <LittleFS.h>

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
#define MODBUS_ADDRS_K "modbus_addrs"
#define MODBUS_POLLING_K "modbus_poll_secs"
#define INVERTER_MODEL_K "inverter_model"
#define DARK_MODE_K "dark_mode"
#define TEMP_CTRL_ENABLED_K "tc_enabled"
#define TEMP_CTRL_TOPIC_K "tc_topic"
#define TEMP_CTRL_ON_K "tc_on"
#define TEMP_CTRL_OFF_K "tc_off"
#define TEMP_CTRL_TH_ON_K "tc_th_on"
#define TEMP_CTRL_TH_OFF_K "tc_th_off"
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
    if (LittleFS.exists(filename)) {
        LittleFS.remove(filename);
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
    this->modbusAddresses = {1};
    this->modbusPollingInSeconds = 5;
    this->inverterType = "none";
    this->darkMode = false;
    this->tempCtrlEnabled = false;
    this->tempCtrlTopic = "";
    this->tempCtrlPayloadOn = "ON";
    this->tempCtrlPayloadOff = "OFF";
    this->tempCtrlThresholdOn = 40.0f;
    this->tempCtrlThresholdOff = 35.0f;
}
WiCMParamConfig::~WiCMParamConfig(){};

void WiCMParamConfig::save() {
    //save the custom parameters to FS

    GLOG::println(F("WiCM: Saving config file"));

    JsonDocument json;

    json[DEVICE_NAME_K] = deviceName.c_str();
    json[SOFTAP_PASSWORD_K] = softApPassword.c_str();
    json[MQTT_SERVER_K] = mqttServer.c_str();
    json[MQTT_PORT_K] = mqttPort;
    mqttUsername.trim();
    json[MQTT_USERNAME_K] = mqttUsername.c_str();
    mqttPassword.trim();
    json[MQTT_PASSWORD_K] = mqttPassword.c_str();
    json[MQTT_TOPIC_K] = mqttBaseTopic.c_str();
    json[MODBUS_ADDRS_K] = modbusAddresses;
    json[MODBUS_POLLING_K] = modbusPollingInSeconds;
    json[INVERTER_MODEL_K] = inverterType.c_str();
    json[DARK_MODE_K] = darkMode;
    json[TEMP_CTRL_ENABLED_K] = tempCtrlEnabled;
    tempCtrlTopic.trim();
    json[TEMP_CTRL_TOPIC_K] = tempCtrlTopic.c_str();
    json[TEMP_CTRL_ON_K] = tempCtrlPayloadOn.c_str();
    json[TEMP_CTRL_OFF_K] = tempCtrlPayloadOff.c_str();
    json[TEMP_CTRL_TH_ON_K] = tempCtrlThresholdOn;
    json[TEMP_CTRL_TH_OFF_K] = tempCtrlThresholdOff;

    File configFile = LittleFS.open(F(PARAMS_FILE), "w");
    if (!configFile) {
        GLOG::println(F("WiCM: Save failed"));
    }

    serializeJson(json, configFile);
    configFile.close();

    //end save
}

void WiCMParamConfig::load() {
    //read configuration from FS json
    if (LittleFS.exists(F(PARAMS_FILE))) {
        GLOG::println(F("WiCM: read config file"));
        File configFile = LittleFS.open(F(PARAMS_FILE), "r");
        if (configFile) {
            GLOG::println(F("WiCM: open config file OK"));
            size_t size = configFile.size();

            // Allocate a buffer to store contents of the file.
            std::unique_ptr<char[]> buf(new char[size]);
            configFile.readBytes(buf.get(), size);

            JsonDocument json;
            auto deserializeError = deserializeJson(json, buf.get());

            if ( ! deserializeError ) {
                GLOG::println(F("WiCM: config json parsed"));
#ifdef SHOW_JSON_FILE
                String jsonStringified;
                serializeJson(json, jsonStringified);
                GLOG::println(String(F("WiCM: ")) + jsonStringified);
#endif

                deviceName = json[DEVICE_NAME_K] | DEFAULT_DEVICE_NAME;
                softApPassword = json[SOFTAP_PASSWORD_K] | DEFAULT_SOFTAP_PASSWORD;
                mqttServer = json[MQTT_SERVER_K] | "";
                mqttPort = json[MQTT_PORT_K] | 1883;
                mqttBaseTopic = json[MQTT_TOPIC_K] | DEFAULT_TOPIC;
                mqttUsername = json[MQTT_USERNAME_K] | "";
                mqttPassword = json[MQTT_PASSWORD_K] | "";

                if (json[MODBUS_ADDRS_K].is<JsonArrayConst>()) {
                    modbusAddresses.clear();
                    for (int i : json[MODBUS_ADDRS_K].as<JsonArrayConst>()) {
                        modbusAddresses.push_back(i);
                    }
                } else {
                    modbusAddresses = {1};
                }

                modbusPollingInSeconds = json[MODBUS_POLLING_K] | 5;

                inverterType = json[INVERTER_MODEL_K] | "none";
                if (inverterType == "") {
                    inverterType = "none";
                }

                darkMode = json[DARK_MODE_K] | false;

                tempCtrlEnabled = json[TEMP_CTRL_ENABLED_K] | false;
                tempCtrlTopic = json[TEMP_CTRL_TOPIC_K] | "";
                tempCtrlPayloadOn = json[TEMP_CTRL_ON_K] | "ON";
                tempCtrlPayloadOff = json[TEMP_CTRL_OFF_K] | "OFF";
                tempCtrlThresholdOn = json[TEMP_CTRL_TH_ON_K] | 40.0f;
                tempCtrlThresholdOff = json[TEMP_CTRL_TH_OFF_K] | 35.0f;
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
    if (LittleFS.exists(F(STA_WIFI_PARAMS_FILE))) {
        GLOG::println(F("WiCM: read wifi file"));
        File networkFile = LittleFS.open(F(STA_WIFI_PARAMS_FILE), "r");
        if (networkFile) {
            GLOG::println(F("WiCM: open wifi file OK"));
            size_t size = networkFile.size();

            // Allocate a buffer to store contents of the file.
            std::unique_ptr<char[]> buf(new char[size]);
            networkFile.readBytes(buf.get(), size);

            JsonDocument json;
            auto deserializeError = deserializeJson(json, buf.get());

            if ( ! deserializeError ) {
                GLOG::println(F("WiCM: wifi json parsed"));
#ifdef SHOW_JSON_FILE
                String jsonStringified;
                serializeJson(json, jsonStringified);
                GLOG::println(String(F("WiCM: ")) + jsonStringified);
#endif

                if (json[IP_K].is<const char*>()) {
                    ip.fromString(json[IP_K].as<String>());
                } else {
                    ip = IPAddress();
                }

                if (json[GW_K].is<const char*>()) {
                    gw.fromString(json[GW_K].as<String>());
                } else {
                    gw = IPAddress();
                }

                if (json[SN_K].is<const char*>()) {
                    sn.fromString(json[SN_K].as<String>());
                } else {
                    sn = IPAddress();
                }

                if (json[DNS_K].is<const char*>()) {
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

void WiCMWifiConfig::save() const {
    GLOG::println(F("WiCM: save wifi file"));

    JsonDocument json;
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

    File networkFile = LittleFS.open(F(STA_WIFI_PARAMS_FILE), "w");
    if (!networkFile) {
        GLOG::println(F("WiCM: save wifi file failed"));
    }

    serializeJson(json, networkFile);
    networkFile.close();
    GLOG::println(F("WiCM: save wifi OK"));
}

void WiCMWifiConfig::erase() {
    eraseFile(STA_WIFI_PARAMS_FILE);
}

bool WiCMWifiConfig::isStaticIPConfigured() const {
    return ip.isSet() && gw.isSet() && sn.isSet() && dns.isSet();
}

/*
 ArduinoJSON vector converter
 Source: https://arduinojson.org/v6/how-to/create-converters-for-stl-containers/
*/
namespace ArduinoJson {
template <typename T>
struct Converter<std::vector<T> > {
  static void toJson(const std::vector<T>& src, JsonVariant dst) {
    JsonArray array = dst.to<JsonArray>();
    for (T item : src)
      array.add(item);
  }

  static std::vector<T> fromJson(JsonVariantConst src) {
    std::vector<T> dst;
    for (T item : src.as<JsonArrayConst>())
      dst.push_back(item);
    return dst;
  }

  static bool checkJson(JsonVariantConst src) {
    JsonArrayConst array = src;
    bool result = array;
    for (JsonVariantConst item : array)
      result &= item.is<T>();
    return result;
  }
};
}  // namespace ArduinoJson, previously ARDUINOJSON_NAMESPACE
