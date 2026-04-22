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
    this->discoveryPublishedThisSession = false;
    this->discoveryPublishedThisBoot = false;
    
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
    client->publish((topic + "/tele/RSSI").c_str(), String(WiFi.RSSI()).c_str());
}

void MqttPublisher::publishOnline() {
    client->publish(LWT_TOPIC, "true", true);
}

bool MqttPublisher::publishFanCmdMsg(const char *absoluteTopic, const char *payload) {
    if (!client->connected()) {
        return false;
    }
    return client->publish(absoluteTopic, payload);
}

// Publishes an MQTT message whose payload may exceed the PubSubClient
// internal buffer. For small payloads, falls back to the ordinary
// publish() call; for larger payloads, uses the streaming
// beginPublish()/write()/endPublish() API so we do not need a giant
// pre-allocated buffer on the ESP8266.
bool MqttPublisher::publishLarge(const char *topic, const String &payload, bool retain) {
    if (!client->connected()) {
        return false;
    }

    const size_t bufferSize = client->getBufferSize();
    // PubSubClient needs a few bytes in the buffer for the MQTT header
    // (topic, length, ...), so leave a safety margin when deciding whether
    // to use the streamed API.
    const size_t headerMargin = 16 + strlen(topic);
    const size_t len = payload.length();

    if (len + headerMargin < bufferSize) {
        return client->publish(topic, (const uint8_t *) payload.c_str(), len, retain);
    }

    if (!client->beginPublish(topic, len, retain)) {
        return false;
    }

    const uint8_t *data = (const uint8_t *) payload.c_str();
    size_t remaining = len;
    while (remaining > 0) {
        // Write in chunks that fit within the PubSubClient buffer.
        size_t chunk = remaining;
        if (chunk > 256) {
            chunk = 256;
        }
        size_t written = client->write(data, chunk);
        if (written == 0) {
            // Connection died mid-write; endPublish() to free internal state.
            client->endPublish();
            return false;
        }
        data += written;
        remaining -= written;
    }

    return client->endPublish() != 0;
}

void MqttPublisher::scheduleDiscovery(const std::list<HaDiscoveryMessage> &messages) {
    this->pendingDiscovery = messages;
    this->discoveryPublishedThisSession = false;
    this->discoveryPublishedThisBoot = false;
}

void MqttPublisher::flushPendingDiscovery() {
    if (this->discoveryPublishedThisSession || this->pendingDiscovery.empty()) {
        return;
    }

    GLOG::println(String(F("MQTT: publishing ")) + this->pendingDiscovery.size() + F(" discovery messages"));

    bool allOk = true;
    for (const HaDiscoveryMessage &m : this->pendingDiscovery) {
        if (!publishLarge(m.topic.c_str(), m.payload, m.retain)) {
            GLOG::println(String(F("MQTT: discovery publish FAILED on ")) + m.topic);
            allOk = false;
            break;
        }
        GLOG::println(String(F("MQTT: discovery publish on topic ")) + m.topic);
        // Keep the PubSubClient happy while we push a bunch of retained
        // messages in a row.
        client->loop();
    }

    this->discoveryPublishedThisSession = allOk;

    if (allOk) {
        // Discovery messages are retained on the broker, so they survive
        // across disconnects: no need to keep ~5-8 KB of String payloads
        // on the heap for the rest of this boot. Freeing them avoids
        // fragmentation that otherwise breaks WiFiManager's /param page
        // rendering.
        this->discoveryPublishedThisBoot = true;
        this->pendingDiscovery.clear();
        GLOG::println(F("MQTT: discovery done, freed pending queue"));
    }
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
        clientId = this->topic + "-";
        clientId += String(ESP.getChipId(), HEX);
    
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

            // ... announce the device and its entities to Home Assistant
            // (only once per boot - the messages are retained, so they
            // survive broker disconnects and we avoid holding their
            // payloads on the heap for longer than strictly needed).
            if (!this->discoveryPublishedThisBoot) {
                this->discoveryPublishedThisSession = false;
                flushPendingDiscovery();
            }
      
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

    // If for some reason the discovery publish could not complete on the
    // last (re)connection (e.g. broker temporarily busy, transient failure
    // mid-stream), try again while the connection is alive.
    if (client->connected() && !this->discoveryPublishedThisSession) {
        flushPendingDiscovery();
    }
}

bool MqttPublisher::isConnected() {
    return client->connected();
}



