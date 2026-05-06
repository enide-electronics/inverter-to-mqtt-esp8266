/*
 Growatt SPH/SPA inverter data exporter
 Uses Modbus to communicate with the inverter's serial RS232 port
 Publishes data to a MQTT server

 This file is based on the "Basic ESP8266 MQTT example"

 To install the ESP8266 board, (using Arduino 1.6.4+):
  - Add the following 3rd party board manager under "File -> Preferences -> Additional Boards Manager URLs":
       http://arduino.esp8266.com/stable/package_esp8266com_index.json
  - Open the "Tools -> Board -> Board Manager" and click install for the ESP8266"
  - Select your ESP8266 board in "Tools -> Board"
    This version of the code runs fine on a Wemos D1 or NodeMCU v3 (ESP-12) board

 Also add the following Libraries:
  - ModbusMaster 2.0.1+
  - PubSubClient 2.8.0+

  Written by JF enide.electronics (at) enide.net
  Licensed under GNU GPLv3
*/

#include <stdio.h>

#include <Arduino.h>
#include <SoftwareSerial.h>

#include "GlobalDefs.h"
#include "Leds.h"
#include "WifiAndConfigManager.h"
#include "Inverter.h"
#include "InverterFactory.h"
#include "MqttPublisher.h"
#include "InverterData.h"
#include "StatusPage.h"
#include "TemperatureController.h"
#include "GLog.h"

/*
 * You can set the ESP8266 LED working mode by publishing a value to this topic
 * 0: LED always off
 * 1: LED always on
 * 2: LED blinks when reading data from the inverter and publishing it to MQTT (default)
 */
#define SETTINGS_LED_SUBTOPIC "settings/led"
#define MIN(a,b) (((a) < (b)) ? (a) : (b))

#ifdef LARGE_ESP_BOARD
#define BUTTON D2

#endif

WiFiClient espClient;
Leds leds;

// tasks last run at millis
unsigned long lastReportSentAtMillis = 0;
unsigned long lastTeleSentAtMillis = 0;
unsigned long lastWifiCheckAtMillis = 0;
bool areRemoteCommandsSupported = false;

// led status (0 = off, 1 = on, 2 = blink when publishing data)
uint8_t ledStatus = 2;
char mqttValueBuffer16[16];
uint8_t tasksRedLedCounter = 0;

Inverter *inverter = NULL;
MqttPublisher *mqtt = NULL;
TemperatureController *tempCtrl = NULL;
WifiAndConfigManager wcm;
StatusPage statusPage;

void setupTemperatureController() {
    tempCtrl = new TemperatureController(
        inverter,
        mqtt,
        wcm.getTempCtrlEnabled(),
        wcm.getTempCtrlTopic(),
        wcm.getTempCtrlPayloadOn(),
        wcm.getTempCtrlPayloadOff(),
        wcm.getTempCtrlThresholdOn(),
        wcm.getTempCtrlThresholdOff()
    );
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
    GLOG::logMqtt(topic, payload, length);

    String subTopic(topic);
    subTopic.replace(wcm.getMqttTopic() + "/", "");

    if (subTopic == SETTINGS_LED_SUBTOPIC) {
        // Switch on the LED if an 1 was received as first character
        char cLedStatus = (char)payload[0];
        if (cLedStatus == '1') {
            leds.lightUpDefault();   // Turn the LED on 
            ledStatus = 1;
        } else if (cLedStatus == '0') {
            leds.turnOffDefault();  // Turn the LED off
            ledStatus = 0;
        } else if (cLedStatus == '2') {
            leds.dimDefault();  // Dim the LED 
            ledStatus = 2;
        }
    } else {
        leds.lightUpRed(); // RED lights up
        tasksRedLedCounter++;
        
        int safeLength = MIN(length, 15);
        memcpy(mqttValueBuffer16, payload, safeLength);
        mqttValueBuffer16[safeLength] = '\0';
        
        String sPayload = String(mqttValueBuffer16);
        sPayload.trim();
        inverter->setIncomingTopicData(subTopic, sPayload);
    }
}

void setupInverter() {
    InverterParams p;
    p.modbusAddresses = wcm.getModbusAddresses();
    inverter = InverterFactory::createInverter(wcm.getInverterType(), p);
}

void setupMqtt(std::list<String> inverterSettingsTopics) {
    mqtt = new MqttPublisher(espClient, wcm.getMqttUsername().c_str(), wcm.getMqttPassword().c_str(), wcm.getMqttTopic().c_str(), wcm.getMqttServer().c_str(), wcm.getMqttPort());
    mqtt->setCallback(mqttCallback);
    mqtt->addSubscription(SETTINGS_LED_SUBTOPIC);
    
    for (std::list<String>::iterator it = inverterSettingsTopics.begin(); it != inverterSettingsTopics.end(); ++it) {
        mqtt->addSubscription((*it).c_str());
    }
    
}

void setupLogger() {
    GLOG::setup();
    wcm.getWM().setDebugOutput(GLOG::isLogEnabled());
}

void setupStatusPage() {
    // Providers capture the *address* of the globals so they keep working
    // even when mqtt/inverter get deleted and re-created on config changes.
    statusPage.setMqttConnectedProvider([]() {
        return mqtt != NULL && mqtt->isConnected();
    });
    statusPage.setDarkModeProvider([]() { return wcm.getDarkMode(); });
    statusPage.setDeviceNameProvider([]() { return wcm.getDeviceName(); });
    statusPage.setInverterTypeProvider([]() { return wcm.getInverterType(); });
    statusPage.setMqttServerProvider([]() {
        return wcm.getMqttServer() + ":" + String(wcm.getMqttPort());
    });
    statusPage.setMqttTopicProvider([]() { return wcm.getMqttTopic(); });

    wcm.setStatusPage(&statusPage);
}

void applyNewConfiguration() {
    delay(1000);
    
    GLOG::println(F("LOOP: New config, deleting objects"));
    
    // delete old objects
    delete tempCtrl;
    delete mqtt;
    delete inverter;
    espClient.stop();

    // Values belonged to the previous inverter instance; drop them so the
    // status page doesn't show stale readings for an inverter that is no
    // longer active.
    statusPage.clearInverterData();
    
    GLOG::println(F("LOOP: New config, creating objects"));
    
    // set them up again
    setupInverter();
    auto topics = inverter->getTopicsToSubscribe();
    setupMqtt(topics);
    areRemoteCommandsSupported = topics.size() > 0;

    setupTemperatureController();
}

bool isFactoryResetRequested() {
#ifdef LARGE_ESP_BOARD
    static unsigned long pressStart = 0;
    unsigned long now = millis();

    if (BUTTON == LOW) {               // button is pressed, start counting
        if (pressStart == 0) pressStart = now;
        if (now - pressStart >= 30000UL) {
            return true;
        }
    } else {                           // button released
        pressStart = 0;
    }
#endif

    return false;
}

void setup() {
#ifdef LARGE_ESP_BOARD
    pinMode(BUTTON, INPUT_PULLUP);
#endif
    setupLogger();
    setupStatusPage();
    wcm.setupWifiAndConfig();
    setupInverter();
    auto topics = inverter->getTopicsToSubscribe();
    setupMqtt(topics);
    areRemoteCommandsSupported = topics.size() > 0;

    setupTemperatureController();
}

void loop() {
    wcm.loop();

    if (isFactoryResetRequested()) {
        GLOG::println(F("LOOP: Factory reset!"));
        wcm.doFactoryReset();
        delay(1000);
        ESP.restart();
    }

    // handle config changes
    if (wcm.checkforConfigChanges()) {
        if (wcm.isRestartRequired()) {
            GLOG::println(F("LOOP: New config, RESTARTING!"));
            delay(1000);
            ESP.restart();
        } else {
            applyNewConfiguration();
        }
    }
    
    mqtt->loop();
    inverter->loop();
    tempCtrl->loop();

    unsigned long now = millis();

    // inverter report
    if (mqtt->isConnected() && now - lastReportSentAtMillis > wcm.getModbusPollingInSeconds() * (unsigned)1000) {
        if (ledStatus == 2) leds.lightUpDefault(); // Turn the LED on
        GLOG::print(F("LOOP: Polling inverter"));
        inverter->read();

        if (inverter->isDataValid()) {
            GLOG::print(F(", publishing"));
            InverterData data = inverter->getData();
            mqtt->publishData(data);
            // Inverters may return partial data on each read. Merging into
            // the status page lets the /status view build up the full set of
            // measurements over successive polls.
            statusPage.mergeInverterData(data);
            GLOG::println(F(", done!"));
        } else {
            GLOG::println(F(", failed!"));
        }

        lastReportSentAtMillis = now;
        
        if (ledStatus == 2) leds.dimDefault(); // Turn the LED off
        if (tasksRedLedCounter > 0) tasksRedLedCounter--;
        if (areRemoteCommandsSupported) {
            if (tasksRedLedCounter == 0) {
                leds.dimRed(); // Dim RED led
            }
        } else {
            leds.turnOffRed();
        }
    }

    // inverter tele report
    if (mqtt->isConnected() && now - lastTeleSentAtMillis > 60000) {
        GLOG::println(F("LOOP: Publishing telemetry"));
        mqtt->publishTele();

        lastTeleSentAtMillis = now;
    }

    if (mqtt->isConnected() && now - lastWifiCheckAtMillis > 5000) {
        if (!wcm.isWifiConnected()) {
            leds.turnOffDefault(); // LED should recover once Wifi reconnects
        }
    
        lastWifiCheckAtMillis = now;
    }
}
