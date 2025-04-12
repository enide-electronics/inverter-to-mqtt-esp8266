/*
  MqttPublisher.cpp - Library for the ESP8266/ESP32 Arduino platform
  Publish Growatt SPH and SPA data to MQTT
  
  Written by JF enide.electronics (at) enide.net
  Licensed under GNU GPLv3
*/

#include "MqttPublisher.h"
#include "GLog.h"
#include "uptime_formatter.h"

#define LWT_TOPIC ((this->topic + "/online").c_str())
        
MqttPublisher::MqttPublisher(WiFiClient &espClient, const char *username, const char * password, const char *baseTopic, const char *server, int port) {
    this->serverIp = server;
    this->portNumber = port;
    this->username = username;
    this->password = password;
    this->client = new PubSubClient(espClient);
    this->client->setBufferSize(768);   // 768 should be enough for the JSON payloads
    this->client->setServer(serverIp.c_str(), portNumber);  
    this->lastReconnectAttemptMillis = 0;
    
    this->topic = baseTopic;
    this->clientId = "unknown";
}

MqttPublisher::~MqttPublisher() {
    delete this->client;
}
       
void MqttPublisher::publishData(InverterData &data) {
    for (std::map<String, String>::iterator it = data.begin(); it != data.end(); ++it) {
        const String & name = it->first;
        const String & value = it->second;
        
        client->publish((topic + String("/") + name).c_str(), value.c_str());
    }
}

void MqttPublisher::publishTele() {
    client->publish((topic + "/tele/IP").c_str(), WiFi.localIP().toString().c_str());
    client->publish((topic + "/tele/ClientID").c_str(), clientId.c_str());
    client->publish((topic + "/tele/Uptime").c_str(), uptime_formatter::getUptime().c_str());
}

void MqttPublisher::publishOnline() {
    client->publish(LWT_TOPIC, "true", true);
}

void MqttPublisher::setClientId(String &clientId) {
    this->clientId = clientId;
}


void MqttPublisher::setCallback(void (*callback)(char* topic, byte* payload, unsigned int length)) {
    client->setCallback(callback);
}

void MqttPublisher::addSubscription(const char *subtopic) {
    String fullTopic = this->topic + "/" + subtopic;
    
    GLOG::println(String(F("MQTT: subscribe [")) + fullTopic + "]");
    subscriptions.push_back(fullTopic);
}

void MqttPublisher::keepConnected() {
    // Don't loop here, do it on the main loop
    if (!client->connected() && millis() - lastReconnectAttemptMillis > 5000L) {
        lastReconnectAttemptMillis = millis();

        // Create a random client ID
        clientId = "growatt-";
        clientId += String(random(0xffff), HEX);
    
        // Attempt to connect
        bool success;
        if (username.length() == 0 && password.length() == 0) {
            GLOG::print(String(F("MQTT: attempting connection to ")) + this->serverIp + F("..."));
            success = client->connect(clientId.c_str(), LWT_TOPIC, 1, true, "false");
        } else {
            GLOG::print(String(F("MQTT: attempting connection to ")) + this->serverIp + F(" with username '") + username + F("' and password with ") + password.length() + F(" chars..."));
            success = client->connect(clientId.c_str(), username.c_str(), password.c_str(), LWT_TOPIC, 1, true, "false");

        }
        if (success) {
            GLOG::println(F("connected"));
            
            // Once connected, publish an announcement...
            publishTele();
            publishOnline();
      
            // ... and resubscribe
            for (String s : subscriptions) {
                client->subscribe(s.c_str());
            }
        } else {
            GLOG::print(F("failed, rc="));
            GLOG::print(client->state());
            GLOG::println(F(" retry in 5 seconds"));
        }
    }
}

void MqttPublisher::loop() {
    keepConnected();
    client->loop();
}

bool MqttPublisher::isConnected() {
    return client->connected();
}



