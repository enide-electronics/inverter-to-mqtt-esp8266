/*
  WifiAndConfigManager.cpp - Library for the ESP8266/ESP32 Arduino platform
  LittleFS based configuration

  Written by JF enide.electronics (at) enide.net
  Licensed under GNU GPLv3
*/

#include "WifiAndConfigManager.h"
#include "WiCMConfig.h"
#include "GLog.h"
#include <LittleFS.h>
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
    <option value="mic">Growatt MIC</option>
    <option value="mictl">Growatt MIC-TL</option>
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

// Dark mode head element: selectStyle + overrides that flip the portal to a
// dark palette. setCustomHeadElement() only stores the raw pointer, so this
// must live in RAM (.rodata/.data), not PROGMEM.
const char darkModeHeadElement[] =
  "<style>select{width:100%;border-radius:.3rem;font-size:1em;padding:5px;margin:5px 0;}</style>"
  "<style>"
  "body{background:#1e1e1e;color:#e6e6e6;}"
  "a,a:visited{color:#64b5f6;}"
  "input,select,textarea{background:#2a2a2a;color:#e6e6e6;border:1px solid #555;}"
  "input[type=checkbox]{filter:invert(1) hue-rotate(180deg);}"
  "button,.btn,input[type=submit],input[type=button]{background:#3a3a3a;color:#e6e6e6;border:1px solid #555;}"
  "hr{border-color:#444;}"
  ".msg{background:#2a2a2a;color:#e6e6e6;border-color:#555;}"
  "h1,h2,h3,h4,h5,h6{color:#e6e6e6;}"
  "fieldset{border-color:#444;}"
  "label{color:#e6e6e6;}"
  "</style>";

// The section-header strings below are passed directly to
// WiFiManagerParameter(const char*), which stores the raw pointer and later
// reads it with plain strlen()/memcpy() via String::operator=(const char*).
// Placing them in PROGMEM leads to unaligned IROM reads and LoadStoreError
// crashes inside WiFiManager::getParamOut(). Keep them in RAM (.rodata/.data),
// same as selectStyle above.
const char networkSectionHeaderStr[] = R"(
  <hr>
  <h3 style="margin-top:1em;">Network setup</h3>
  )";

const char uiSectionHeaderStr[] = R"(<hr><h3 style="margin-top:1em;">Appearance</h3>)";

const char mqttSectionHeaderStr[] = R"(<hr><h3 style="margin-top:1em;">MQTT setup</h3>)";

const char inverterSectionHeaderStr[] = R"(
  <hr>
  <h3 style="margin-top:1em;">Inverter setup</h3>
  )";

const char tempCtrlSectionHeaderStr[] = R"(
  <hr>
  <h3 style="margin-top:1em;">Temperature controller</h3>
  <p style="font-size:0.9em;margin:0 0 .5em 0;">
    Publishes an ON/OFF message to another device based on the inverter
    temperature (evaluated every few seconds; heartbeat sent once per minute).
  </p>
  )";

// The temperature controller "enabled" flag is submitted through a hidden text
// input ("tc_en_hidden") holding "0" or "1". A checkbox, with no name, updates
// the hidden input via JS when toggled, so the value is posted as part of the
// regular form submit.
const char tempCtrlEnabledCustomStr[] PROGMEM = R"(
  <label style="display:flex;align-items:center;gap:.5em;margin:.5em 0;">
    <input type="checkbox" id="tc_en_cb" onchange="document.getElementById('tc_en_hidden').value = this.checked ? '1' : '0'">
    <span>Enable temperature controller</span>
  </label>
  <script>
    document.getElementById('tc_en_cb').checked = (%s == "1");
    document.querySelector("[for='tc_en_hidden']").hidden = true;
    document.getElementById('tc_en_hidden').hidden = true;
  </script>
  )";

// Dark mode checkbox: mirrors the temperature controller pattern. A hidden
// text input ("dm_en_hidden") carries the "0"/"1" value in the form submit,
// and a no-name checkbox toggles it via JS.
const char darkModeEnabledCustomStr[] PROGMEM = R"(
  <label style="display:flex;align-items:center;gap:.5em;margin:.5em 0;">
    <input type="checkbox" id="dm_en_cb" onchange="document.getElementById('dm_en_hidden').value = this.checked ? '1' : '0'">
    <span>Enable dark mode</span>
  </label>
  <script>
    document.getElementById('dm_en_cb').checked = (%s == "1");
    document.querySelector("[for='dm_en_hidden']").hidden = true;
    document.getElementById('dm_en_hidden').hidden = true;
  </script>
  )";

WifiAndConfigManager::WifiAndConfigManager() {
    saveWifiStaticIPRequired = false;
    saveParamsRequired = false;
    rebootRequired = false;
    wifiConnected = false;
    statusPage = NULL;
    
    // config var web params
    networkSectionHeaderParam = NULL;
    deviceNameParam = NULL;
    softApPasswordParam = NULL;
    mqttSectionHeaderParam = NULL;
    mqttServerParam = NULL;
    mqttPortParam = NULL;
    mqttUsernameParam = NULL;
    mqttPasswordParam = NULL;
    mqttBaseTopicParam = NULL;
    inverterSectionHeaderParam = NULL;
    modbusAddressParam = NULL;
    modbusPollingInSecondsParam = NULL;
    inverterModelCustomFieldParam = NULL;
    inverterTypeCustomHidden = NULL;
    uiSectionHeaderParam = NULL;
    darkModeCustomParam = NULL;
    darkModeHidden = NULL;
    tempCtrlSectionHeaderParam = NULL;
    tempCtrlEnabledCustomParam = NULL;
    tempCtrlEnabledHidden = NULL;
    tempCtrlTopicParam = NULL;
    tempCtrlPayloadOnParam = NULL;
    tempCtrlPayloadOffParam = NULL;
    tempCtrlThresholdOnParam = NULL;
    tempCtrlThresholdOffParam = NULL;

    inverterTypeComboboxParamIdx = -1;
    tempCtrlCheckboxParamIdx = -1;
    darkModeCheckboxParamIdx = -1;

    if (!LittleFS.begin()) {
        GLOG::println(F("WiCM: FS mount failed"));
        
        delay(1000);
        ESP.restart();
    }
}

void WifiAndConfigManager::setStatusPage(StatusPage *sp) {
    statusPage = sp;
}

void WifiAndConfigManager::saveParamConfigCallback() {
    GLOG::println(F("WiCM: Save PARAM config"));
    saveParamsRequired = true;
}

void WifiAndConfigManager::saveWifiConfigCallback() {
    GLOG::println(F("WiCM: Save WIFI config callback"));
    
    // do not try to read these fields outside this function, it will segfault
    String ip = getParam(F("ip"));
    String gw = wm.server->arg(F("gw"));
    String sn = wm.server->arg(F("sn"));
    String dns = wm.server->arg(F("dns"));

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
            GLOG::printf_P(PSTR("WiCM: Invalid static IP configuration\nWiCM: ipOK=%d, gwOK=%d, snOK=%d, dnsOK=%d\n"), ipOK, gwOK, snOK, dnsOK);
        }

    } else {
        GLOG::println(F("WiCM: Enabling DHCP IP"));
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

    GLOG::println(F("WiCM: FACTORY RESET DONE"));
}

void WifiAndConfigManager::_updateInverterTypeSelect() {
    snprintf_P(inverterModelCustomFieldBufferStr, _IMCFBS_SIZE - 1, inverterTypeSelectStr, paramsCfg.inverterType.c_str());
    inverterModelCustomFieldBufferStr[_IMCFBS_SIZE - 1] = '\0';

    if (inverterModelCustomFieldParam != NULL) {
        delete inverterModelCustomFieldParam;
    }
    inverterModelCustomFieldParam = new WiFiManagerParameter(inverterModelCustomFieldBufferStr);
    if (inverterTypeComboboxParamIdx >= 0) {
        wm.getParameters()[inverterTypeComboboxParamIdx] = inverterModelCustomFieldParam;
    }
}

void WifiAndConfigManager::_updateTempCtrlCheckbox() {
    snprintf_P(tempCtrlEnabledBuffer, sizeof(tempCtrlEnabledBuffer), tempCtrlEnabledCustomStr,
             paramsCfg.tempCtrlEnabled ? "\"1\"" : "\"0\"");
    tempCtrlEnabledBuffer[sizeof(tempCtrlEnabledBuffer) - 1] = '\0';

    if (tempCtrlEnabledCustomParam != NULL) {
        delete tempCtrlEnabledCustomParam;
    }
    tempCtrlEnabledCustomParam = new WiFiManagerParameter(tempCtrlEnabledBuffer);
    if (tempCtrlCheckboxParamIdx >= 0) {
        wm.getParameters()[tempCtrlCheckboxParamIdx] = tempCtrlEnabledCustomParam;
    }
}

void WifiAndConfigManager::_updateDarkModeCheckbox() {
    snprintf_P(darkModeBuffer, sizeof(darkModeBuffer), darkModeEnabledCustomStr,
             paramsCfg.darkMode ? "\"1\"" : "\"0\"");
    darkModeBuffer[sizeof(darkModeBuffer) - 1] = '\0';

    if (darkModeCustomParam != NULL) {
        delete darkModeCustomParam;
    }
    darkModeCustomParam = new WiFiManagerParameter(darkModeBuffer);
    if (darkModeCheckboxParamIdx >= 0) {
        wm.getParameters()[darkModeCheckboxParamIdx] = darkModeCustomParam;
    }
}

void WifiAndConfigManager::_applyDarkModeHead() {
    // setCustomHeadElement stores the raw pointer; both strings are in
    // static storage so they remain valid for the lifetime of the portal.
    wm.setCustomHeadElement(paramsCfg.darkMode ? darkModeHeadElement : selectStyle);
}

void WifiAndConfigManager::_recycleParams() {
    if (networkSectionHeaderParam != NULL) delete networkSectionHeaderParam;
    if (deviceNameParam != NULL) delete deviceNameParam;
    if (softApPasswordParam != NULL) delete softApPasswordParam;
    if (mqttSectionHeaderParam != NULL) delete mqttSectionHeaderParam;
    if (mqttServerParam != NULL) delete mqttServerParam;
    if (mqttPortParam != NULL) delete mqttPortParam;
    if (mqttUsernameParam != NULL) delete mqttUsernameParam;
    if (mqttPasswordParam != NULL) delete mqttPasswordParam;
    if (mqttBaseTopicParam != NULL) delete mqttBaseTopicParam;
    if (inverterSectionHeaderParam != NULL) delete inverterSectionHeaderParam;
    if (modbusAddressParam != NULL) delete modbusAddressParam;
    if (modbusPollingInSecondsParam != NULL) delete modbusPollingInSecondsParam;
    if (inverterModelCustomFieldParam != NULL) delete inverterModelCustomFieldParam;
    if (inverterTypeCustomHidden != NULL) delete inverterTypeCustomHidden;
    if (uiSectionHeaderParam != NULL) delete uiSectionHeaderParam;
    if (darkModeCustomParam != NULL) delete darkModeCustomParam;
    if (darkModeHidden != NULL) delete darkModeHidden;
    if (tempCtrlSectionHeaderParam != NULL) delete tempCtrlSectionHeaderParam;
    if (tempCtrlEnabledCustomParam != NULL) delete tempCtrlEnabledCustomParam;
    if (tempCtrlEnabledHidden != NULL) delete tempCtrlEnabledHidden;
    if (tempCtrlTopicParam != NULL) delete tempCtrlTopicParam;
    if (tempCtrlPayloadOnParam != NULL) delete tempCtrlPayloadOnParam;
    if (tempCtrlPayloadOffParam != NULL) delete tempCtrlPayloadOffParam;
    if (tempCtrlThresholdOnParam != NULL) delete tempCtrlThresholdOnParam;
    if (tempCtrlThresholdOffParam != NULL) delete tempCtrlThresholdOffParam;
}

void WifiAndConfigManager::setupWifiAndConfig() {

    wifiCfg.load();
    paramsCfg.load();
    show();

    _applyDarkModeHead();

    _recycleParams();

    // device params
    networkSectionHeaderParam = new WiFiManagerParameter(networkSectionHeaderStr);
    deviceNameParam = new WiFiManagerParameter("devicename", "Device Name", paramsCfg.deviceName.c_str(), 32);
    softApPasswordParam = new WiFiManagerParameter("wifipass", "SoftAP Password", paramsCfg.softApPassword.c_str(), 32);
    
    // MQTT params
    mqttSectionHeaderParam = new WiFiManagerParameter(mqttSectionHeaderStr);
    mqttServerParam = new WiFiManagerParameter("server", "MQTT server", paramsCfg.mqttServer.c_str(), 40);
    mqttPortParam = new WiFiManagerParameter("port", "MQTT port", String(paramsCfg.mqttPort).c_str(), 6);
    mqttUsernameParam = new WiFiManagerParameter("username", "MQTT username", String(paramsCfg.mqttUsername).c_str(), 32);
    mqttPasswordParam = new WiFiManagerParameter("password", "MQTT password", String(paramsCfg.mqttPassword).c_str(), 32);
    mqttBaseTopicParam = new WiFiManagerParameter("topic", "MQTT base topic", paramsCfg.mqttBaseTopic.c_str(), 24);
    
    // inverter params
    inverterSectionHeaderParam = new WiFiManagerParameter(inverterSectionHeaderStr);
    modbusAddressParam = new WiFiManagerParameter("modbus", "Inverter modbus address", vectorToCSV(paramsCfg.modbusAddresses).c_str(), 9); // at most 5 inverter IDs: a,b,c,d,e
    modbusPollingInSecondsParam = new WiFiManagerParameter("modbuspoll", "Inverter modbus polling (secs)", String(paramsCfg.modbusPollingInSeconds).c_str(), 3);
    inverterTypeComboboxParamIdx = -1;
    _updateInverterTypeSelect();
    inverterTypeCustomHidden = new WiFiManagerParameter("im_key_custom", "IT hidden", paramsCfg.inverterType.c_str(), 10);

    // UI appearance params (must be created before being assigned inside _updateDarkModeCheckbox)
    uiSectionHeaderParam = new WiFiManagerParameter(uiSectionHeaderStr);
    darkModeCheckboxParamIdx = -1;
    _updateDarkModeCheckbox();
    darkModeHidden = new WiFiManagerParameter("dm_en_hidden", "DME hidden", paramsCfg.darkMode ? "1" : "0", 2);

    // temperature controller params
    tempCtrlSectionHeaderParam = new WiFiManagerParameter(tempCtrlSectionHeaderStr);
    tempCtrlCheckboxParamIdx = -1;
    _updateTempCtrlCheckbox();
    tempCtrlEnabledHidden = new WiFiManagerParameter("tc_en_hidden", "TCE hidden", paramsCfg.tempCtrlEnabled ? "1" : "0", 2);
    tempCtrlTopicParam = new WiFiManagerParameter("tc_topic", "Target MQTT topic (e.g. fan/cmd)", paramsCfg.tempCtrlTopic.c_str(), 96);
    tempCtrlPayloadOnParam = new WiFiManagerParameter("tc_on", "Payload ON", paramsCfg.tempCtrlPayloadOn.c_str(), 24);
    tempCtrlPayloadOffParam = new WiFiManagerParameter("tc_off", "Payload OFF", paramsCfg.tempCtrlPayloadOff.c_str(), 24);
    tempCtrlThresholdOnParam = new WiFiManagerParameter("tc_th_on", "Turn ON above (&deg;C)", String(paramsCfg.tempCtrlThresholdOn, 1).c_str(), 6);
    tempCtrlThresholdOffParam = new WiFiManagerParameter("tc_th_off", "Turn OFF below (&deg;C)", String(paramsCfg.tempCtrlThresholdOff, 1).c_str(), 6);
    

    //set config callbacks
    wm.setSaveConfigCallback(std::bind(&WifiAndConfigManager::saveWifiConfigCallback, this));
    wm.setSaveParamsCallback(std::bind(&WifiAndConfigManager::saveParamConfigCallback, this));

    // Register /status on the WM webserver *before* WiFiManager registers its
    // own /status handler. ESP8266WebServer walks handlers in insertion order
    // and keeps the first match, so registering first here lets us replace
    // WM's (empty) /status with our own page. If no StatusPage was wired in,
    // this callback is a no-op and WM behaves exactly as before.
    wm.setWebServerCallback([this]() {
        if (statusPage == NULL || !wm.server) {
            return;
        }
        wm.server->on(String(F("/status")).c_str(), [this]() {
            String page = statusPage->renderHTML();
            wm.server->send(200, F("text/html"), page);
        });
    });

    wm.setTitle(F("Inverter to MQTT ESP8266"));

    // Add "custom" to the menu when a StatusPage is wired in; this slot is
    // filled by the HTML passed to setCustomMenuHTML() below and lets the
    // user jump to /status from the main portal page.
    std::vector<const char *> menu;
    if (statusPage != NULL) {
        menu = {"custom", "wifi", "param", "info", "sep", "restart", "exit"};
        wm.setCustomMenuHTML(StatusPage::menuButtonHTML());
    } else {
        menu = {"wifi", "param", "info", "sep", "restart", "exit"};
    }
    wm.setMenu(menu);

    // add device params
    wm.addParameter(networkSectionHeaderParam);
    wm.addParameter(deviceNameParam);
    wm.addParameter(softApPasswordParam);

    // add MQTT params
    wm.addParameter(mqttSectionHeaderParam);
    wm.addParameter(mqttServerParam);
    wm.addParameter(mqttPortParam);
    wm.addParameter(mqttUsernameParam);
    wm.addParameter(mqttPasswordParam);
    wm.addParameter(mqttBaseTopicParam);
    
    // add inverter params
    wm.addParameter(inverterSectionHeaderParam);
    wm.addParameter(inverterTypeCustomHidden); // Needs to be added before the javascript that hides it
    wm.addParameter(inverterModelCustomFieldParam);
    inverterTypeComboboxParamIdx = wm.getParametersCount() - 1;
    wm.addParameter(modbusAddressParam);
    wm.addParameter(modbusPollingInSecondsParam);

    // UI appearance params
    wm.addParameter(uiSectionHeaderParam);
    wm.addParameter(darkModeHidden);       // must be registered before the checkbox JS targets it
    wm.addParameter(darkModeCustomParam);
    darkModeCheckboxParamIdx = wm.getParametersCount() - 1;

    // temperature controller params
    wm.addParameter(tempCtrlSectionHeaderParam);
    wm.addParameter(tempCtrlEnabledHidden);       // must be registered before the checkbox JS targets it
    wm.addParameter(tempCtrlEnabledCustomParam);
    tempCtrlCheckboxParamIdx = wm.getParametersCount() - 1;
    wm.addParameter(tempCtrlTopicParam);
    wm.addParameter(tempCtrlPayloadOnParam);
    wm.addParameter(tempCtrlPayloadOffParam);
    wm.addParameter(tempCtrlThresholdOnParam);
    wm.addParameter(tempCtrlThresholdOffParam);

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
        GLOG::println(F("WiCM: Failed to connect to wifi, restarting..."));
        delay(1000);
        
        ESP.restart();
    } else {
        wifiConnected = WiFi.status() == WL_CONNECTED;
        wm.startWebPortal();
        wm.server->on((String(F("/eraseall")).c_str()), std::bind(&WifiAndConfigManager::handleEraseAll, this));
    }

    GLOG::println("");
    GLOG::println(F("WiCM: WiFi connected"));
    GLOG::print(F("WiCM: IP address: "));
    GLOG::println(WiFi.localIP());

    randomSeed(micros());

    copyFromParamsToVars();

    // now save it to the LittleFS file
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

    // UI appearance
    paramsCfg.darkMode = String(darkModeHidden->getValue()) == "1";

    // temperature controller values
    paramsCfg.tempCtrlEnabled = String(tempCtrlEnabledHidden->getValue()) == "1";
    paramsCfg.tempCtrlTopic = String(tempCtrlTopicParam->getValue());
    paramsCfg.tempCtrlTopic.trim();
    paramsCfg.tempCtrlPayloadOn = String(tempCtrlPayloadOnParam->getValue());
    paramsCfg.tempCtrlPayloadOff = String(tempCtrlPayloadOffParam->getValue());
    paramsCfg.tempCtrlThresholdOn = String(tempCtrlThresholdOnParam->getValue()).toFloat();
    paramsCfg.tempCtrlThresholdOff = String(tempCtrlThresholdOffParam->getValue()).toFloat();

    _updateInverterTypeSelect();
    _updateTempCtrlCheckbox();
    _updateDarkModeCheckbox();
    _applyDarkModeHead();
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
    
    GLOG::print(F("-> Modbus Addrs  : "));
    GLOG::println(vectorToCSV(paramsCfg.modbusAddresses).c_str());
    
    GLOG::print(F("-> Modbus Poll(s): "));
    GLOG::println(paramsCfg.modbusPollingInSeconds);

    GLOG::print(F("-> Inverter type: "));
    GLOG::println(paramsCfg.inverterType);

    GLOG::print(F("-> Dark mode     : "));
    GLOG::println(paramsCfg.darkMode ? F("yes") : F("no"));

    GLOG::print(F("-> TempCtrl on   : "));
    GLOG::println(paramsCfg.tempCtrlEnabled ? F("yes") : F("no"));
    GLOG::print(F("-> TempCtrl topic: "));
    GLOG::println(paramsCfg.tempCtrlTopic);
    GLOG::print(F("-> TempCtrl on>= : "));
    GLOG::println(String(paramsCfg.tempCtrlThresholdOn, 1));
    GLOG::print(F("-> TempCtrl off< : "));
    GLOG::println(String(paramsCfg.tempCtrlThresholdOff, 1));

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

bool WifiAndConfigManager::getDarkMode() {
    return paramsCfg.darkMode;
}

bool WifiAndConfigManager::getTempCtrlEnabled() {
    return paramsCfg.tempCtrlEnabled;
}

String WifiAndConfigManager::getTempCtrlTopic() {
    return paramsCfg.tempCtrlTopic;
}

String WifiAndConfigManager::getTempCtrlPayloadOn() {
    return paramsCfg.tempCtrlPayloadOn;
}

String WifiAndConfigManager::getTempCtrlPayloadOff() {
    return paramsCfg.tempCtrlPayloadOff;
}

float WifiAndConfigManager::getTempCtrlThresholdOn() {
    return paramsCfg.tempCtrlThresholdOn;
}

float WifiAndConfigManager::getTempCtrlThresholdOff() {
    return paramsCfg.tempCtrlThresholdOff;
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
            GLOG::println(F("WiCM: Failed to connect to wifi, restarting..."));
            delay(1000);
            
            ESP.restart();
        }
        
        if (!isWifiConnected()) {
            GLOG::printf_P(PSTR("WiCM: Failed to connect to wifi, retry %d"), connectRetries);
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
        GLOG::println(String(F("WiCM: WiFi ")) + String(wifiConnectedNow ? F("") : F("dis")) + F("connected"));
        wifiConnected = wifiConnectedNow;
    }
    
    return wifiConnected;
}
