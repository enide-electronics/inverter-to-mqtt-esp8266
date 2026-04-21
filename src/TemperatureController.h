/*
  TemperatureController.h - Library header for the ESP8266/ESP32 Arduino platform
  Controls an external device (e.g. a fan or a relay) via MQTT, based on the
  inverter's reported maximum temperature.

  Every minute the controller reads the inverter temperature; when it crosses
  the "turn on" threshold it publishes the ON payload, and when it falls below
  the "turn off" threshold it publishes the OFF payload. Between the two
  thresholds the current state is kept, providing hysteresis.

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
        // to evaluate the inverter temperature once per minute.
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

        // Evaluate the temperature reading and publish if needed.
        void evaluate();
};

#endif
