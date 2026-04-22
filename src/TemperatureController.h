/*
  TemperatureController.h - Library header for the ESP8266/ESP32 Arduino platform
  Controls an external device (e.g. a fan or a relay) via MQTT, based on the
  inverter's reported maximum temperature.

  The controller polls the inverter temperature frequently so it can react
  immediately when it crosses the "turn on" or "turn off" thresholds, but it
  only publishes an MQTT message when the desired ON/OFF state changes, to
  avoid flooding the broker. In addition, once per minute it republishes the
  current state as a heartbeat so the broker always has a recent value.
  Between the two thresholds the current state is kept, providing hysteresis.

  The instance binds to a specific Inverter + MqttPublisher at construction
  time, so a new controller must be created when either of those is replaced.

  Written by JF enide.electronics (at) enide.net
  Licensed under GNU GPLv3
*/

#ifndef _TEMPERATURE_CONTROLLER_H
#define _TEMPERATURE_CONTROLLER_H

#include <Arduino.h>
#include "Inverter.h"
#include "MqttPublisher.h"

class TemperatureController {
    public:
        TemperatureController(Inverter *inverter,
                              MqttPublisher *mqtt,
                              bool enabled,
                              const String &topic,
                              const String &payloadOn,
                              const String &payloadOff,
                              float thresholdOn,
                              float thresholdOff);
        ~TemperatureController();

        // Should be called from the main loop. It internally rate-limits itself
        // to evaluate the inverter temperature every few seconds and to
        // heartbeat-publish the current state once per minute.
        void loop();

        bool isEnabled() const;

    private:
        Inverter *inverter;
        MqttPublisher *mqtt;

        bool enabled;
        String topic;
        String payloadOn;
        String payloadOff;
        float thresholdOn;
        float thresholdOff;

        bool deviceOn;            // currently known desired state
        bool stateInitialized;    // true after we publish for the first time
        unsigned long lastEvaluationAtMillis;
        unsigned long lastPublishAtMillis;

        // Evaluate the temperature reading and publish if needed. When
        // forceHeartbeat is true the current state is published even if it
        // has not changed, so the broker gets a periodic refresh.
        void evaluate(bool forceHeartbeat);
};

#endif
