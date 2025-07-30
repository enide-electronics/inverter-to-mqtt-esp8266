/*
  WifiAndConfigManager.cpp - Library for the ESP8266/ESP32 Arduino platform
  SPIFFS based configuration

  Written by JF enide.electronics (at) enide.net
  Licensed under GNU GPLv3
*/

#include "WifiAndConfigManager.h"
#include "WiCMConfig.h"
#include "GLog.h"
#include <string>
#include <sstream>


static std::string vectorToCSV(const std::vector<int>& vec) {
    std::ostringstream oss;
    for (size_t i = 0; i < vec.size(); ++i) {
        oss << vec[i];
        if (i != vec.size() - 1) {
            oss << ",";
        }
    }
    return oss.str();
}

std::vector<int> csvToVector(const std::string& csv) {
    std::vector<int> result;
    std::istringstream iss(csv);
    std::string token;
    while (std::getline(iss, token, ',')) {
        result.push_back(String(token.c_str()).toInt());
    }
    return result;
}

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
    <option value="v_a_vmiii">Voltronic Axpert VMIII</option>
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
    saveWifiStaticIPRequired = false;
    saveParamsRequired = false;
    rebootRequired = false;
    wifiConnected = false;
    
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

    if (!SPIFFS.begin()) {
        GLOG::println(("WiCM: FS mount failed"));
        
        delay(1000);
        ESP.restart();
    }
}

void WifiAndConfigManager::saveParamConfigCallback() {
    GLOG::println(F("WiCM: Save PARAM config"));
    saveParamsRequired = true;
}

void WifiAndConfigManager::saveWifiConfigCallback() {
    GLOG::println(F("WiCM: Save WIFI config callback"));
    
    // do not try to read these fields outside this function, it will segfault
    String ip = getParam("ip");
    String gw = wm.server->arg("gw");
    String sn = wm.server->arg("sn");
    String dns = wm.server->arg("dns");

    if (ip != "" && gw != "" && sn != "" && dns != "") {
        GLOG::print(F("WiCM: STA IP: "));
        GLOG::println(ip);
        GLOG::print(F("WiCM: STA GW: "));
        GLOG::println(gw);
        GLOG::print(F("WiCM: STA SN: "));
        GLOG::println(sn);
        GLOG::print(F("WiCM: STADNS: "));
        GLOG::println(dns);
        
        bool ipOK = wifiCfg.ip.fromString(ip);
        bool gwOK = wifiCfg.gw.fromString(gw);
        bool snOK = wifiCfg.sn.fromString(sn);
        bool dnsOK = wifiCfg.dns.fromString(dns);

        if (!ipOK || !gwOK || !snOK || !dnsOK) {
            GLOG::printf("WiCM: Invalid static IP configuration\nWiCM: ipOK=%d, gwOK=%d, snOK=%d, dnsOK=%d\n", ipOK, gwOK, snOK, dnsOK);
        }

    } else {
        GLOG::println("WiCM: Enabling DHCP IP");
        wifiCfg.ip = IPAddress();
        wifiCfg.gw = IPAddress();
        wifiCfg.sn = IPAddress();;
        wifiCfg.dns = IPAddress();
    }

    saveWifiStaticIPRequired = true;
}

void WifiAndConfigManager::handleEraseAll() {
    doFactoryReset();

    wm.server->send(200, F("text/plain"), F("Done! Rebooting now, please wait a few seconds."));
    
    // needed to allow the response to be returned and the logs to be flushed
    delay(2000);
    
    ESP.restart();
}

void WifiAndConfigManager::doFactoryReset() {
    GLOG::println(F("WiCM: DELETE CONFIG"));
    paramsCfg.erase();

    GLOG::println(F("WiCM: DELETE STATIC WIFI CONFIG"));
    wifiCfg.erase();

    GLOG::println(F("WiCM: DELETE ESP WIFI CONFIG"));
    ESP.eraseConfig();

    GLOG::println("WiCM: FACTORY RESET DONE");
}

void WifiAndConfigManager::_updateInverterTypeSelect() {
    snprintf(inverterModelCustomFieldBufferStr, 799, inverterTypeSelectStr, paramsCfg.inverterType.c_str());
    inverterModelCustomFieldBufferStr[799] = '\0';

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

    wifiCfg.load();
    paramsCfg.load();
    show();

    wm.setCustomHeadElement(selectStyle);

    _recycleParams();

    // device params
    deviceNameParam = new WiFiManagerParameter("devicename", "Device Name", paramsCfg.deviceName.c_str(), 32);
    softApPasswordParam = new WiFiManagerParameter("wifipass", "SoftAP Password", paramsCfg.softApPassword.c_str(), 32);
    
    // MQTT params
    mqttServerParam = new WiFiManagerParameter("server", "MQTT server", paramsCfg.mqttServer.c_str(), 40);
    mqttPortParam = new WiFiManagerParameter("port", "MQTT port", String(paramsCfg.mqttPort).c_str(), 6);
    mqttUsernameParam = new WiFiManagerParameter("username", "MQTT username", String(paramsCfg.mqttUsername).c_str(), 32);
    mqttPasswordParam = new WiFiManagerParameter("password", "MQTT password", String(paramsCfg.mqttPassword).c_str(), 32);
    mqttBaseTopicParam = new WiFiManagerParameter("topic", "MQTT base topic", paramsCfg.mqttBaseTopic.c_str(), 24);
    
    // inverter params
    modbusAddressParam = new WiFiManagerParameter("modbus", "Inverter modbus address", vectorToCSV(paramsCfg.modbusAddresses).c_str(), 9); // at most 5 inverter IDs: a,b,c,d,e
    modbusPollingInSecondsParam = new WiFiManagerParameter("modbuspoll", "Inverter modbus polling (secs)", String(paramsCfg.modbusPollingInSeconds).c_str(), 3);
    _updateInverterTypeSelect();
    inverterTypeCustomHidden = new WiFiManagerParameter("im_key_custom", "Will be hidden", paramsCfg.inverterType.c_str(), 10);
    

    //set config callbacks
    wm.setSaveConfigCallback(std::bind(&WifiAndConfigManager::saveWifiConfigCallback, this));
    wm.setSaveParamsCallback(std::bind(&WifiAndConfigManager::saveParamConfigCallback, this));
    
    wm.setTitle("Inverter to MQTT ESP8266");
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
    WiFi.hostname(paramsCfg.deviceName.c_str());
    wm.setHostname(paramsCfg.deviceName.c_str());
    
    wm.setConfigPortalTimeout(60); // auto close configportal after n seconds
    wm.setAPClientCheck(true); // avoid timeout if client connected to softap
    wm.setShowInfoUpdate(false); // don't show OTA button on info page

    if (wifiCfg.isStaticIPConfigured()) {
        wm.setSTAStaticIPConfig(wifiCfg.ip, wifiCfg.gw, wifiCfg.sn, wifiCfg.dns);
    }
    
    // now connect with the wifi info previously stored
    bool res = wm.autoConnect(paramsCfg.deviceName.c_str(), paramsCfg.softApPassword.c_str());
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
    if (saveParamsRequired) {
        paramsCfg.save();
        saveParamsRequired = false;
    }

    if (saveWifiStaticIPRequired) {
        wifiCfg.save();
        saveWifiStaticIPRequired = false;
    }
}

void WifiAndConfigManager::copyFromParamsToVars() {
    // copy values back to our variables
    paramsCfg.deviceName = String(deviceNameParam->getValue());
    paramsCfg.softApPassword = String(softApPasswordParam->getValue());
    
    paramsCfg.mqttServer = String(mqttServerParam->getValue());
    paramsCfg.mqttPort = String(mqttPortParam->getValue()).toInt();
    paramsCfg.mqttUsername = String(mqttUsernameParam->getValue());
    paramsCfg.mqttUsername.trim();
    paramsCfg.mqttPassword = String(mqttPasswordParam->getValue());
    paramsCfg.mqttPassword.trim();
    paramsCfg.mqttBaseTopic = String(mqttBaseTopicParam->getValue());
    
    paramsCfg.modbusAddresses = csvToVector(modbusAddressParam->getValue());
    paramsCfg.modbusPollingInSeconds = String(modbusPollingInSecondsParam->getValue()).toInt();
    paramsCfg.inverterType = String(inverterTypeCustomHidden->getValue());

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
    GLOG::print(F("-> IP            : "));
    if (wifiCfg.ip.isSet()) {
        GLOG::println(wifiCfg.ip.toString());    
    } else {
        GLOG::println(F("<not set>"));    
    }
    GLOG::print(F("-> GW            : "));
    if (wifiCfg.gw.isSet()) {
        GLOG::println(wifiCfg.gw.toString());    
    } else {
        GLOG::println(F("<not set>"));    
    }
    GLOG::print(F("-> SN            : "));
    if (wifiCfg.sn.isSet()) {
        GLOG::println(wifiCfg.sn.toString());    
    } else {
        GLOG::println(F("<not set>"));    
    }
    GLOG::print(F("-> DNS           : "));
    if (wifiCfg.dns.isSet()) {
        GLOG::println(wifiCfg.dns.toString());    
    } else {
        GLOG::println(F("<not set>"));    
    }

    GLOG::println(F("---------------------------"));
    GLOG::print(F("-> Device name   : "));
    GLOG::println(paramsCfg.deviceName);
    
    GLOG::print(F("-> SoftAP pass   : "));
    GLOG::println(paramsCfg.softApPassword);
    
    GLOG::print(F("-> Mqtt server   : "));
    GLOG::println(paramsCfg.mqttServer);
 
    GLOG::print(F("-> Mqtt port     : "));
    GLOG::println(paramsCfg.mqttPort);
    
    GLOG::print(F("-> Mqtt Username : "));
    GLOG::println(paramsCfg.mqttUsername);
    
    GLOG::print(F("-> Mqtt Password : "));
    GLOG::println(paramsCfg.mqttPassword);
    
    GLOG::print(F("-> Mqtt Topic    : "));
    GLOG::println(paramsCfg.mqttBaseTopic);
    
    GLOG::print(F("-> Modbus Addresses: "));
    GLOG::println(vectorToCSV(paramsCfg.modbusAddresses).c_str());
    
    GLOG::print(F("-> Modbus Poll(s): "));
    GLOG::println(paramsCfg.modbusPollingInSeconds);

    GLOG::print(F("-> Inverter type: "));
    GLOG::println(paramsCfg.inverterType);
    GLOG::println(F("---------------------------"));
}

String WifiAndConfigManager::getDeviceName() {
    return paramsCfg.deviceName;
}

String WifiAndConfigManager::getMqttServer() {
    return paramsCfg.mqttServer;
}

int WifiAndConfigManager::getMqttPort() {
    return paramsCfg.mqttPort;
}

String WifiAndConfigManager::getMqttUsername() {
    return paramsCfg.mqttUsername;
}

String WifiAndConfigManager::getMqttPassword() {
    return paramsCfg.mqttPassword;
}

String WifiAndConfigManager::getMqttTopic() {
    return paramsCfg.mqttBaseTopic;
}

std::vector<int> WifiAndConfigManager::getModbusAddresses() {
    return paramsCfg.modbusAddresses;
}

int WifiAndConfigManager::getModbusPollingInSeconds() {
    return paramsCfg.modbusPollingInSeconds;
}

String WifiAndConfigManager::getInverterType() {
    return paramsCfg.inverterType;
}

WiFiManager & WifiAndConfigManager::getWM() {
    return wm;
}

void WifiAndConfigManager::loop() {
    int connectRetries = 0;
    wm.process();
    
    // try to handle reconnections when esp core misbehaves
    while (!isWifiConnected()) {
        bool connected = wm.autoConnect(paramsCfg.deviceName.c_str(), paramsCfg.softApPassword.c_str());

        if (!connected && ++connectRetries > 5) {
            GLOG::println("WiCM: Failed to connect to wifi, restarting...");
            delay(1000);
            
            ESP.restart();
        }
        
        if (!isWifiConnected()) {
            GLOG::printf("WiCM: Failed to connect to wifi, retry %d", connectRetries);
        }

        delay(1000);
    }
}

bool WifiAndConfigManager::checkforConfigChanges() {
    if (saveWifiStaticIPRequired) {
        wifiCfg.save();
        saveWifiStaticIPRequired = false;
    }

    if (saveParamsRequired) {
        String newDeviceName = String(deviceNameParam->getValue());
        if (newDeviceName != paramsCfg.deviceName) {
            GLOG::println(String(F("WiCM: New device name : ")) + newDeviceName);
            rebootRequired = true;
        }
        
        copyFromParamsToVars();
        
        paramsCfg.save();
        saveParamsRequired = false;
        
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
        GLOG::println(String(F("WiCM: WiFi ")) + String(wifiConnectedNow ? F("") : F("dis")) + String("connected"));
        wifiConnected = wifiConnectedNow;
    }
    
    return wifiConnected;
}
