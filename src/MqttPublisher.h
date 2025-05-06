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
#include "InverterData.h"

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
        
        void keepConnected();
        
    public:
        MqttPublisher(WiFiClient &espClient, const char *username, const char * password, const char *baseTopic, const char *server, int port = 1883);
        ~MqttPublisher();
       
        void publishData(InverterData &data);
        void publishTele();
        void publishOnline();
        
        void setClientId(String &clientId);
        void setCallback(void (*callback)(char* topic, byte* payload, unsigned int length));
        void addSubscription(const char *subtopic);

        void loop();
        bool isConnected();
};

#endif