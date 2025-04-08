/*
  WifiAndConfigManager.cpp - Library for the ESP8266/ESP32 Arduino platform
  SPIFFS based configuration

  Written by JF enide.electronics (at) enide.net
  Licensed under GNU GPLv3
*/

#include "WifiAndConfigManager.h"
#include <FS.h>
#include "GLog.h"

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

#define DEFAULT_TOPIC "inverter"
#define DEFAULT_SOFTAP_PASSWORD "12345678"
#define DEFAULT_DEVICE_NAME "growatt-adapter-esp8266"

/*
  Future work on WiFiManager: To automatically exit from the paramsave page back to root, after saving!
  WiFiManager: strings_en.h
  change
    const char HTTP_PARAMSAVED[]       PROGMEM = "<div class='msg S'>Saved<br/></div>";
  to
    const char HTTP_PARAMSAVED[]       PROGMEM = "<meta http-equiv='refresh' content='5;url=/' /><div class='msg S'>Saved<br/></div>";
*/

const char inverterTypeSelectStr[] PROGMEM = R"(
  <label for='inverter_model'>Inverter model</label>
  <select name="inverterModel" id="inverter_model" onchange="document.getElementById('im_key_custom').value = this.value">
    <option value="sph">Growatt SPH</option>
    <option value="sphtl">Growatt SPH-TL</option>
    <option value="minxh">Growatt MIN-XH</option>
    <option value="gtn">Soyosource GTN (display)</option>
    <option value="test">Test</option>
    <option value="none">None</option>
  </select>
  <script>
    document.getElementById('inverter_model').value = "%s";
    document.querySelector("[for='im_key_custom']").hidden = true;
    document.getElementById('im_key_custom').hidden = true;
  </script>
  )";

// do not place in PROGMEM because wm keeps the address of the const char * which then is volatile
  const char selectStyle[] = "<style>select{width:100%;border-radius:.3rem;background:white;font-size:1em;padding:5px;margin:5px 0;}</style>";

WifiAndConfigManager::WifiAndConfigManager() {
    saveRequired = false;
    rebootRequired = false;
    wifiConnected = false;

    // config vars
    deviceName = DEFAULT_DEVICE_NAME;
    softApPassword = DEFAULT_SOFTAP_PASSWORD;
    mqttServer = "localhost";
    mqttPort = 1883;
    mqttUsername = "";
    mqttPassword = "";
    mqttBaseTopic = DEFAULT_TOPIC;
    modbusAddress = 1;
    modbusPollingInSeconds = 5;
    inverterType = "none";
    
    // config var web params
    deviceNameParam = NULL;
    softApPasswordParam = NULL;
    mqttServerParam = NULL;
    mqttPortParam = NULL;
    mqttUsernameParam = NULL;
    mqttPasswordParam = NULL;
    mqttBaseTopicParam = NULL;
    modbusAddressParam = NULL;
    modbusPollingInSecondsParam = NULL;
    inverterModelCustomFieldParam = NULL;
    inverterTypeCustomHidden = NULL;
}

void WifiAndConfigManager::saveConfigCallback() {
    GLOG::println(F("WiCM: Save config"));
    saveRequired = true;
}

void WifiAndConfigManager::handleEraseAll() {
    doFactoryReset();

    wm.server->send(200, F("text/plain"), F("Done! Rebooting now, please wait a few seconds."));
    
    // needed to allow the response to be returned and the logs to be flushed
    delay(2000);
    
    ESP.restart();
}

void WifiAndConfigManager::doFactoryReset() {
    GLOG::println(F("WiCM: DELETE SPIFFS CONFIG"));
    
    if (SPIFFS.exists(F("/config.json"))) {
        SPIFFS.remove(F("/config.json"));
    }

    GLOG::println(F("WiCM: DELETE WIFI CONFIG"));
    ESP.eraseConfig();

    GLOG::println("WiCM: FACTORY RESET DONE");
}

void WifiAndConfigManager::_updateInverterTypeSelect() {
snprintf(inverterModelCustomFieldBufferStr, 699, inverterTypeSelectStr, inverterType.c_str());
    inverterModelCustomFieldBufferStr[699] = '\0';

    inverterModelCustomFieldParam = new WiFiManagerParameter(inverterModelCustomFieldBufferStr);
}

void WifiAndConfigManager::_recycleParams() {
    if (deviceNameParam != NULL) delete deviceNameParam;
    if (softApPasswordParam != NULL) delete softApPasswordParam;
    if (mqttServerParam != NULL) delete mqttServerParam;
    if (mqttPortParam != NULL) delete mqttPortParam;
    if (mqttUsernameParam != NULL) delete mqttUsernameParam;
    if (mqttPasswordParam != NULL) delete mqttPasswordParam;
    if (mqttBaseTopicParam != NULL) delete mqttBaseTopicParam;
    if (modbusAddressParam != NULL) delete modbusAddressParam;
    if (modbusPollingInSecondsParam != NULL) delete modbusPollingInSecondsParam;
    if (inverterModelCustomFieldParam != NULL) delete inverterModelCustomFieldParam;
    if (inverterTypeCustomHidden != NULL) delete inverterTypeCustomHidden;
}

void WifiAndConfigManager::setupWifiAndConfig() {

    load();
    show();

    wm.setCustomHeadElement(selectStyle);

    _recycleParams();

    // device params
    deviceNameParam = new WiFiManagerParameter("devicename", "Device Name", deviceName.c_str(), 32);
    softApPasswordParam = new WiFiManagerParameter("wifipass", "SoftAP Password", softApPassword.c_str(), 32);
    
    // MQTT params
    mqttServerParam = new WiFiManagerParameter("server", "MQTT server", mqttServer.c_str(), 40);
    mqttPortParam = new WiFiManagerParameter("port", "MQTT port", String(mqttPort).c_str(), 6);
    mqttUsernameParam = new WiFiManagerParameter("username", "MQTT username", String(mqttUsername).c_str(), 32);
    mqttPasswordParam = new WiFiManagerParameter("password", "MQTT password", String(mqttPassword).c_str(), 32);
    mqttBaseTopicParam = new WiFiManagerParameter("topic", "MQTT base topic", mqttBaseTopic.c_str(), 24);
    
    // inverter params
    modbusAddressParam = new WiFiManagerParameter("modbus", "Inverter modbus address", String(modbusAddress).c_str(), 3);
    modbusPollingInSecondsParam = new WiFiManagerParameter("modbuspoll", "Inverter modbus polling (secs)", String(modbusPollingInSeconds).c_str(), 3);
    _updateInverterTypeSelect();
    inverterTypeCustomHidden = new WiFiManagerParameter("im_key_custom", "Will be hidden", inverterType.c_str(), 10);
    

    //set config callbacks
    wm.setSaveConfigCallback(std::bind(&WifiAndConfigManager::saveConfigCallback, this));
    wm.setSaveParamsCallback(std::bind(&WifiAndConfigManager::saveConfigCallback, this));
    
    wm.setTitle("Growatt Adapter ESP8266");
    std::vector<const char *> menu = {"wifi", "param", "info", "sep", "restart", "exit"};
    wm.setMenu(menu);

    // add device params
    wm.addParameter(deviceNameParam);
    wm.addParameter(softApPasswordParam);

    // add MQTT params
    wm.addParameter(mqttServerParam);
    wm.addParameter(mqttPortParam);
    wm.addParameter(mqttUsernameParam);
    wm.addParameter(mqttPasswordParam);
    wm.addParameter(mqttBaseTopicParam);
    
    // add inverter params
    wm.addParameter(inverterTypeCustomHidden); // Needs to be added before the javascript that hides it
    wm.addParameter(inverterModelCustomFieldParam);
    wm.addParameter(modbusAddressParam);
    wm.addParameter(modbusPollingInSecondsParam);

    // make static ip fields visible in Wifi menu
    wm.setShowStaticFields(true);
    wm.setShowDnsFields(true);

    WiFi.mode(WIFI_STA);
    WiFi.hostname(deviceName.c_str());
    wm.setHostname(deviceName.c_str());
    
    wm.setConfigPortalTimeout(60); // auto close configportal after n seconds
    wm.setAPClientCheck(true); // avoid timeout if client connected to softap
    wm.setShowInfoUpdate(false); // don't show OTA button on info page
    
    // now connect with the wifi info previously stored
    bool res = wm.autoConnect(deviceName.c_str(), softApPassword.c_str());
    if (!res) {
        GLOG::println("WiCM: Failed to connect to wifi, restarting...");
        delay(1000);
        
        ESP.restart();
    } else {
        wifiConnected = WiFi.status() == WL_CONNECTED;
        wm.startWebPortal();
        wm.server->on((String(FPSTR("/eraseall")).c_str()), std::bind(&WifiAndConfigManager::handleEraseAll, this));
    }

    GLOG::println("");
    GLOG::println(F("WiCM: WiFi connected"));
    GLOG::print(F("WiCM: IP address: "));
    GLOG::println(WiFi.localIP());

    randomSeed(micros());

    copyFromParamsToVars();

    // now save it to the SPIFFS file
    if (saveRequired) {
        save();
        saveRequired = false;
    }
}

void WifiAndConfigManager::load() {
    //read configuration from FS json
    GLOG::println(F("WiCM: Mounting FS..."));

    if (SPIFFS.begin()) {
        GLOG::println("WiCM: FS mount OK");
        if (SPIFFS.exists(F("/config.json"))) {
            //file exists, reading and loading
            GLOG::println(F("WiCM: Reading config file"));
            File configFile = SPIFFS.open(F("/config.json"), "r");
            if (configFile) {
                GLOG::println(F("WiCM: Opened config file"));
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
                    GLOG::println(F("\nparsed json"));

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
                    GLOG::println(F("failed to parse"));
                }
                configFile.close();
            }
        }
    } else {
        GLOG::println(("WiCM: FS mount failed"));
    }
    //end read

}

void WifiAndConfigManager::save() {
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

    File configFile = SPIFFS.open(F("/config.json"), "w");
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

void WifiAndConfigManager::copyFromParamsToVars() {
    // copy values back to our variables
    deviceName = String(deviceNameParam->getValue());
    softApPassword = String(softApPasswordParam->getValue());
    mqttServer = String(mqttServerParam->getValue());
    mqttPort = String(mqttPortParam->getValue()).toInt();
    mqttUsername = String(mqttUsernameParam->getValue());
    mqttUsername.trim();
    mqttPassword = String(mqttPasswordParam->getValue());
    mqttPassword.trim();
    mqttBaseTopic = String(mqttBaseTopicParam->getValue());
    modbusAddress = String(modbusAddressParam->getValue()).toInt();
    modbusPollingInSeconds = String(modbusPollingInSecondsParam->getValue()).toInt();
    inverterType = String(inverterTypeCustomHidden->getValue());

    _updateInverterTypeSelect();
}

String WifiAndConfigManager::getParam(String name){
    //read parameter from server, for custom hmtl input
    String value;
    if(wm.server->hasArg(name)) {
        value = wm.server->arg(name);
    }
    return value;
}

void WifiAndConfigManager::show() {
    GLOG::println(F("---------------------------"));
    GLOG::print(F("Device name   : "));
    GLOG::println(deviceName);
    
    GLOG::print(F("SoftAP pass   : "));
    GLOG::println(softApPassword);
    
    GLOG::print(F("Mqtt server   : "));
    GLOG::println(mqttServer);
 
    GLOG::print(F("Mqtt port     : "));
    GLOG::println(mqttPort);
    
    GLOG::print(F("Mqtt Username : "));
    GLOG::println(mqttUsername);
    
    GLOG::print(F("Mqtt Password : "));
    GLOG::println(mqttPassword);
    
    GLOG::print(F("Mqtt Topic    : "));
    GLOG::println(mqttBaseTopic);
    
    GLOG::print(F("Modbus Address: "));
    GLOG::println(modbusAddress);
    
    GLOG::print(F("Modbus Poll(s): "));
    GLOG::println(modbusPollingInSeconds);

    GLOG::print(F("Inverter type: "));
    GLOG::println(inverterType);
}

String WifiAndConfigManager::getDeviceName() {
    return deviceName;
}

String WifiAndConfigManager::getMqttServer() {
    return mqttServer;
}

int WifiAndConfigManager::getMqttPort() {
    return mqttPort;
}

String WifiAndConfigManager::getMqttUsername() {
    return mqttUsername;
}

String WifiAndConfigManager::getMqttPassword() {
    return mqttPassword;
}

String WifiAndConfigManager::getMqttTopic() {
    return mqttBaseTopic;
}

int WifiAndConfigManager::getModbusAddress() {
    return modbusAddress;
}

int WifiAndConfigManager::getModbusPollingInSeconds() {
    return modbusPollingInSeconds;
}

String WifiAndConfigManager::getInverterType() {
    return inverterType;
}

WiFiManager & WifiAndConfigManager::getWM() {
    return wm;
}

bool WifiAndConfigManager::checkforConfigChanges() {
    if (saveRequired) {
        
        String newDeviceName = String(deviceNameParam->getValue());
        if (newDeviceName != deviceName) {
            GLOG::println(String(F("WiCM: New device name : ")) + newDeviceName);
            rebootRequired = true;
        }
        
        copyFromParamsToVars();
        
        save();
        saveRequired = false;
        
        show();
        
        return true;
    } else {
        return false;
    }
}

bool WifiAndConfigManager::isRestartRequired() {
    return rebootRequired;
}

bool WifiAndConfigManager::isWifiConnected() {
    bool wifiConnectedNow = WiFi.status() == WL_CONNECTED;
    
    if (wifiConnected != wifiConnectedNow) {
        GLOG::println(String(F("WiCM: WiFi")) + String(wifiConnectedNow ? F("") : F("dis")) + String("connected"));
        wifiConnected = wifiConnectedNow;
    }
    
    return wifiConnected;
}
