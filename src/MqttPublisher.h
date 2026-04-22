/*
  MqttPublisher.h - Library header for the ESP8266/ESP32 Arduino platform
  Publish Growatt SPH and SPA data to MQTT
  
  Written by JF enide.electronics (at) enide.net
  Licensed under GNU GPLv3
*/

#ifndef _MQTT_PUBLISHER_H
#define _MQTT_PUBLISHER_H

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <list>
#include "InverterData.h"
#include "HaDiscovery.h"

class MqttPublisher {
    private:
        PubSubClient *client;
        String serverIp;
        int portNumber;
        String username;
        String password;
        String topic;
        String clientId;
        std::vector<String> subscriptions;
        long lastReconnectAttemptMillis;
        std::list<HaDiscoveryMessage> pendingDiscovery;
        bool discoveryPublishedThisSession;
        bool discoveryPublishedThisBoot;
        
        void keepConnected();
        bool publishLarge(const char *topic, const String &payload, bool retain);
        void flushPendingDiscovery();
        
    public:
        MqttPublisher(WiFiClient &espClient, const char *username, const char * password, const char *baseTopic, const char *server, int port = 1883);
        ~MqttPublisher();
       
        void publishData(InverterData &data);
        void publishTele();
        void publishOnline();
        bool publishFanCmdMsg(const char *absoluteTopic, const char *payload);

        // Queues the Home Assistant / Tasmota discovery messages to be
        // published to the broker. They are actually sent after the next
        // successful MQTT (re)connection, as retained messages.
        void scheduleDiscovery(const std::list<HaDiscoveryMessage> &messages);
        
        void setClientId(String &clientId);
        void setCallback(void (*callback)(char* topic, byte* payload, unsigned int length));
        void addSubscription(const char *subtopic);

        void loop();
        bool isConnected();
};

#endif