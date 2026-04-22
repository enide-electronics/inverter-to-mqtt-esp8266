/*
  HaDiscovery.cpp - Implementation of the Tasmota / Home Assistant MQTT
  discovery helpers.

  Written by JF enide.electronics (at) enide.net
  Licensed under GNU GPLv3
*/

#include "HaDiscovery.h"

static String jsonEscape(const String &in) {
    String out;
    out.reserve(in.length() + 2);
    for (size_t i = 0; i < in.length(); i++) {
        char c = in[i];
        switch (c) {
            case '"':  out += F("\\\""); break;
            case '\\': out += F("\\\\"); break;
            case '\n': out += F("\\n"); break;
            case '\r': out += F("\\r"); break;
            case '\t': out += F("\\t"); break;
            default:
                if ((uint8_t) c < 0x20) {
                    char buf[8];
                    snprintf(buf, sizeof(buf), "\\u%04x", (uint8_t) c);
                    out += buf;
                } else {
                    out += c;
                }
        }
    }
    return out;
}

static String quotedOrNull(const char *s) {
    if (s == NULL) {
        return F("null");
    }
    return String(F("\"")) + jsonEscape(String(s)) + F("\"");
}

static String quoted(const String &s) {
    return String(F("\"")) + jsonEscape(s) + F("\"");
}

// Builds the Tasmota /config payload for the device.
// See https://tasmota.github.io/docs/Home-Assistant/ for details on the
// fields. We emit the subset recognised by the HA Tasmota integration;
// unused arrays default to a single "no relay / no switch / no button"
// entry so the integration does not create ghost switch/light entities.
static String buildTasmotaConfigPayload(const HaDiscoveryDevice &device) {
    String payload;
    payload.reserve(600);
    payload += F("{\"ip\":");
    payload += quoted(device.ipAddress);
    payload += F(",\"dn\":");
    payload += quoted(device.deviceName);
    payload += F(",\"fn\":[");
    payload += quoted(device.deviceName);
    payload += F(",null,null,null,null,null,null,null],\"hn\":");
    payload += quoted(device.hostname);
    payload += F(",\"mac\":");
    payload += quoted(device.mac);
    payload += F(",\"md\":");
    payload += quoted(device.model);
    payload += F(",\"ty\":0,\"if\":0,\"ofln\":");
    payload += quoted(device.availabilityOff);
    payload += F(",\"onln\":");
    payload += quoted(device.availabilityOn);
    payload += F(",\"state\":[\"OFF\",\"ON\",\"TOGGLE\",\"HOLD\"],\"sw\":");
    payload += quoted(device.softwareVersion);
    payload += F(",\"t\":");
    payload += quoted(device.baseTopic);
    // Non-standard full topic: our firmware publishes each value directly
    // under <baseTopic>/<name>, without the tele/stat/cmnd prefix. We keep
    // the Tasmota "ft" format for the device record, but the HA sensor
    // messages below point to the real state topics.
    payload += F(",\"ft\":\"%topic%/%prefix%/\"");
    payload += F(",\"tp\":[\"cmnd\",\"stat\",\"tele\"]");
    payload += F(",\"rl\":[0],\"swc\":[-1],\"swn\":[null],\"btn\":[0]");
    payload += F(",\"so\":{\"4\":0,\"11\":0,\"13\":0,\"17\":0,\"20\":0,\"30\":0,\"68\":0,\"73\":0,\"82\":0,\"114\":0,\"117\":0}");
    payload += F(",\"lk\":0,\"lt_st\":0,\"sho\":[0,0,0,0],\"ver\":1}");
    return payload;
}

// Builds the Tasmota /sensors payload. This is a loose interpretation of
// the protocol: Tasmota normally publishes a snapshot of tele/SENSOR here,
// but our firmware publishes each datum to its own topic. We therefore emit
// a template JSON listing every sensor the inverter exposes with its unit
// and the absolute MQTT topic where values are published. HA's Tasmota
// integration will not pick these up automatically; the per-sensor HA
// discovery messages emitted by appendHaSensors() cover that use case.
static String buildTasmotaSensorsPayload(
    const HaDiscoveryDevice &device,
    const HaSensorDescriptor *sensors,
    size_t sensorCount) {

    String payload;
    payload.reserve(128 + sensorCount * 96);
    payload += F("{\"sn\":{");

    bool first = true;
    for (size_t i = 0; i < sensorCount; i++) {
        const HaSensorDescriptor &s = sensors[i];
        if (!first) payload += F(",");
        first = false;

        payload += quoted(String(s.name));
        payload += F(":{\"state_topic\":");
        payload += quoted(device.baseTopic + "/" + s.name);
        if (s.unit != NULL) {
            payload += F(",\"unit_of_measurement\":");
            payload += quotedOrNull(s.unit);
        }
        if (s.deviceClass != NULL) {
            payload += F(",\"device_class\":");
            payload += quotedOrNull(s.deviceClass);
        }
        if (s.stateClass != NULL) {
            payload += F(",\"state_class\":");
            payload += quotedOrNull(s.stateClass);
        }
        payload += F("}");
    }

    payload += F("},\"ver\":1}");
    return payload;
}

// Sanitises a name to be used as part of a MQTT topic / HA object id.
// Only [A-Za-z0-9_-] are kept, any other character becomes '_'.
static String sanitise(const String &in) {
    String out;
    out.reserve(in.length());
    for (size_t i = 0; i < in.length(); i++) {
        char c = in[i];
        if ((c >= 'A' && c <= 'Z') ||
            (c >= 'a' && c <= 'z') ||
            (c >= '0' && c <= '9') ||
            c == '_' || c == '-') {
            out += c;
        } else {
            out += '_';
        }
    }
    return out;
}

// Builds one homeassistant/sensor/<id>/config payload for a single sensor,
// linking it to the Tasmota-registered device through
// device.connections = [["mac", <MAC>]].
static String buildHaSensorConfigPayload(
    const HaDiscoveryDevice &device,
    const HaSensorDescriptor &sensor,
    const String &uniqueId) {

    String payload;
    payload.reserve(400);
    payload += F("{\"name\":");
    payload += quoted(sensor.friendlyName != NULL ? String(sensor.friendlyName) : String(sensor.name));
    payload += F(",\"state_topic\":");
    payload += quoted(device.baseTopic + "/" + sensor.name);
    payload += F(",\"availability_topic\":");
    payload += quoted(device.availabilityTopic);
    payload += F(",\"payload_available\":");
    payload += quoted(device.availabilityOn);
    payload += F(",\"payload_not_available\":");
    payload += quoted(device.availabilityOff);
    payload += F(",\"unique_id\":");
    payload += quoted(uniqueId);
    payload += F(",\"object_id\":");
    payload += quoted(uniqueId);

    if (sensor.unit != NULL) {
        payload += F(",\"unit_of_measurement\":");
        payload += quotedOrNull(sensor.unit);
    }
    if (sensor.deviceClass != NULL) {
        payload += F(",\"device_class\":");
        payload += quotedOrNull(sensor.deviceClass);
    }
    if (sensor.stateClass != NULL) {
        payload += F(",\"state_class\":");
        payload += quotedOrNull(sensor.stateClass);
    }
    if (sensor.icon != NULL) {
        payload += F(",\"icon\":");
        payload += quotedOrNull(sensor.icon);
    }

    // Device block: links to the same device registered by Tasmota config.
    payload += F(",\"device\":{\"identifiers\":[");
    payload += quoted(device.mac);
    payload += F("],\"connections\":[[\"mac\",");
    payload += quoted(device.mac);
    payload += F("]],\"name\":");
    payload += quoted(device.deviceName);
    payload += F(",\"model\":");
    payload += quoted(device.model);
    if (device.manufacturer.length() > 0) {
        payload += F(",\"manufacturer\":");
        payload += quoted(device.manufacturer);
    }
    payload += F(",\"sw_version\":");
    payload += quoted(device.softwareVersion);
    payload += F("}}");

    return payload;
}

void HaDiscoveryBuilder::appendTasmotaDevice(
    std::list<HaDiscoveryMessage> &out,
    const HaDiscoveryDevice &device,
    const HaSensorDescriptor *sensors,
    size_t sensorCount) {

    String topicBase = String(F("tasmota/discovery/")) + device.mac;

    out.push_back(HaDiscoveryMessage(
        topicBase + F("/config"),
        buildTasmotaConfigPayload(device),
        true));

    out.push_back(HaDiscoveryMessage(
        topicBase + F("/sensors"),
        buildTasmotaSensorsPayload(device, sensors, sensorCount),
        true));
}

void HaDiscoveryBuilder::appendHaSensors(
    std::list<HaDiscoveryMessage> &out,
    const HaDiscoveryDevice &device,
    const HaSensorDescriptor *sensors,
    size_t sensorCount) {

    String devId = sanitise(device.deviceName.length() > 0 ? device.deviceName : device.mac);

    for (size_t i = 0; i < sensorCount; i++) {
        const HaSensorDescriptor &s = sensors[i];
        String sensorId = sanitise(String(s.name));
        String uniqueId = devId + "_" + sensorId;

        String topic = String(F("homeassistant/sensor/"))
            + devId + "/" + sensorId + F("/config");

        out.push_back(HaDiscoveryMessage(
            topic,
            buildHaSensorConfigPayload(device, s, uniqueId),
            true));
    }
}

void HaDiscoveryBuilder::appendAll(
    std::list<HaDiscoveryMessage> &out,
    const HaDiscoveryDevice &device,
    const HaSensorDescriptor *sensors,
    size_t sensorCount) {

    appendTasmotaDevice(out, device, sensors, sensorCount);
    appendHaSensors(out, device, sensors, sensorCount);
}
