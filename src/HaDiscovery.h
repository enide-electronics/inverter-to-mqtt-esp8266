/*
  HaDiscovery.h - Home Assistant / Tasmota style MQTT discovery helpers

  The Tasmota device registration protocol publishes two retained messages
  per device at:
    - tasmota/discovery/<MAC>/config
    - tasmota/discovery/<MAC>/sensors

  Once the device is registered, individual entities (sensors, binary_sensors)
  can be added via the standard Home Assistant MQTT discovery mechanism
  (homeassistant/<component>/<object_id>/config), linking them back to the
  Tasmota-registered device through the mac "connections" array. That is the
  "supplemental custom discovery message" pattern recommended by Tasmota's
  Home Assistant documentation.

  This header exposes:
    - HaDiscoveryMessage: a plain topic/payload/retain tuple the publisher
      consumes.
    - HaSensorDescriptor: a static description of one data point published by
      an inverter (name, unit, device_class, state_class, ...).
    - HaDiscoveryBuilder: helpers that each Inverter uses to build its own
      Tasmota config/sensors and Home Assistant sensor/binary_sensor
      discovery messages.

  Written by JF enide.electronics (at) enide.net
  Licensed under GNU GPLv3
*/

#ifndef _HA_DISCOVERY_H
#define _HA_DISCOVERY_H

#include <Arduino.h>
#include <list>

struct HaDiscoveryMessage {
    String topic;
    String payload;
    bool retain;

    HaDiscoveryMessage() : retain(true) {}
    HaDiscoveryMessage(const String &t, const String &p, bool r = true)
        : topic(t), payload(p), retain(r) {}
};

// Describes one datum published by the inverter to <baseTopic>/<name>.
// All const char* fields may be nullptr to skip the corresponding JSON key.
struct HaSensorDescriptor {
    const char *name;          // subtopic published by the inverter (e.g. "Vpv1")
    const char *friendlyName;  // user facing name (e.g. "PV1 Voltage"), may be nullptr
    const char *unit;          // unit of measurement (e.g. "V"), may be nullptr
    const char *deviceClass;   // HA device_class (e.g. "voltage"), may be nullptr
    const char *stateClass;    // HA state_class (e.g. "measurement"), may be nullptr
    const char *icon;          // optional mdi icon (e.g. "mdi:solar-power")
};

// Describes the device the sensors belong to. One device maps to one
// tasmota/discovery/<MAC>/{config,sensors} pair in the Tasmota protocol and
// to one "device" block in HA discovery messages.
struct HaDiscoveryDevice {
    String mac;              // hex MAC address without colons (Tasmota id)
    String deviceName;       // friendly device name (Tasmota "dn")
    String hostname;         // hostname as reported by WiFi (Tasmota "hn")
    String model;            // inverter model (Tasmota "md")
    String manufacturer;     // optional, used in HA discovery device block
    String softwareVersion;  // firmware version (Tasmota "sw")
    String ipAddress;        // IP, used in Tasmota "ip" field
    String baseTopic;        // MQTT topic prefix where inverter data is published
                             // e.g. "growatt" or "growatt/22" in multi-inverter mode
    String availabilityTopic;// full topic for the online/LWT flag
    String availabilityOn;   // payload that means available (e.g. "true")
    String availabilityOff;  // payload that means unavailable (e.g. "false")
};

class HaDiscoveryBuilder {
    public:
        // Publishes on tasmota/discovery/<MAC>/config and /sensors.
        // Returns both messages concatenated to the list provided by the caller.
        static void appendTasmotaDevice(
            std::list<HaDiscoveryMessage> &out,
            const HaDiscoveryDevice &device,
            const HaSensorDescriptor *sensors,
            size_t sensorCount);

        // Appends one homeassistant/sensor/<uniqueId>/config message per sensor,
        // associating each with the Tasmota-registered device via its MAC.
        static void appendHaSensors(
            std::list<HaDiscoveryMessage> &out,
            const HaDiscoveryDevice &device,
            const HaSensorDescriptor *sensors,
            size_t sensorCount);

        // Convenience helper: produces both the Tasmota device registration
        // and the per-sensor HA discovery messages in one call.
        static void appendAll(
            std::list<HaDiscoveryMessage> &out,
            const HaDiscoveryDevice &device,
            const HaSensorDescriptor *sensors,
            size_t sensorCount);
};

#endif
