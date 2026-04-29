/*
  TemperatureController.cpp - Library for the ESP8266/ESP32 Arduino platform
  Controls an external device via MQTT based on the inverter temperature.

  Written by JF enide.electronics (at) enide.net
  Licensed under GNU GPLv3
*/

#include "TemperatureController.h"
#include "GLog.h"
#include <math.h>

// How often we re-read the inverter temperature and re-evaluate the desired
// state. Kept short so threshold crossings are reacted upon quickly.
#define TEMP_CTRL_EVAL_INTERVAL_MS   (5UL * 1000UL)
// How often we republish the current state even when it has not changed, so
// the broker always has a recent value (heartbeat).
#define TEMP_CTRL_HEARTBEAT_INTERVAL_MS (60UL * 1000UL)

TemperatureController::TemperatureController(Inverter *inverter,
                                             MqttPublisher *mqtt,
                                             bool enabled,
                                             const String &topic,
                                             const String &payloadOn,
                                             const String &payloadOff,
                                             float thresholdOn,
                                             float thresholdOff) {
    this->inverter = inverter;
    this->mqtt = mqtt;

    this->enabled = enabled;
    this->topic = topic;
    this->topic.trim();
    this->payloadOn = payloadOn;
    this->payloadOff = payloadOff;
    this->thresholdOn = thresholdOn;
    this->thresholdOff = thresholdOff;

    this->deviceOn = false;
    this->stateInitialized = false;

    // Schedule the first evaluation for one interval from now, to give the
    // inverter a chance to produce at least one valid reading.
    this->lastEvaluationAtMillis = millis();
    this->lastPublishAtMillis = 0;

    GLOG::printf("TCTRL: created enabled=%d topic=[%s] on=[%s] off=[%s] thOn=%.1f thOff=%.1f\n",
                 this->enabled ? 1 : 0, this->topic.c_str(), this->payloadOn.c_str(),
                 this->payloadOff.c_str(), this->thresholdOn, this->thresholdOff);
}

TemperatureController::~TemperatureController() {
}

bool TemperatureController::isEnabled() const {
    return this->enabled;
}

void TemperatureController::loop() {
    if (!this->enabled) {
        return;
    }
    if (this->inverter == NULL || this->mqtt == NULL) {
        return;
    }
    if (this->topic.length() == 0) {
        return;
    }

    unsigned long now = millis();
    if (now - this->lastEvaluationAtMillis < TEMP_CTRL_EVAL_INTERVAL_MS) {
        return;
    }
    this->lastEvaluationAtMillis = now;

    // Force a heartbeat publish if enough time has passed since the last
    // successful publish (or if we have never published at all).
    bool forceHeartbeat = (!this->stateInitialized) ||
                          (now - this->lastPublishAtMillis >= TEMP_CTRL_HEARTBEAT_INTERVAL_MS);

    this->evaluate(forceHeartbeat);
}

void TemperatureController::evaluate(bool forceHeartbeat) {
    float temperature = this->inverter->getMaxTemperature();
    if (isnan(temperature)) {
        GLOG::println(F("TCTRL: no valid temperature reading, skipping"));
        return;
    }

    bool previousDeviceOn = this->deviceOn;
    bool newDeviceOn = this->deviceOn;

    if (temperature >= this->thresholdOn) {
        newDeviceOn = true;
    } else if (temperature < this->thresholdOff) {
        newDeviceOn = false;
    }
    // else: keep the current state (hysteresis band)

    bool stateChanged = (newDeviceOn != previousDeviceOn);
    bool needsPublish = stateChanged || forceHeartbeat || !this->stateInitialized;

    this->deviceOn = newDeviceOn;

    if (!needsPublish) {
        GLOG::printf("TCTRL: temp=%.1f state=%s (unchanged)\n",
                     temperature, this->deviceOn ? "ON" : "OFF");
        return;
    }

    if (!this->mqtt->isConnected()) {
        GLOG::println(F("TCTRL: MQTT not connected, deferring publish"));
        return;
    }

    const String &payload = this->deviceOn ? this->payloadOn : this->payloadOff;
    bool ok = this->mqtt->publishFanCmdMsg(this->topic.c_str(), payload.c_str());
    if (ok) {
        this->stateInitialized = true;
        this->lastPublishAtMillis = millis();
        const char *reason = stateChanged ? "change" : "heartbeat";
        GLOG::printf("TCTRL: temp=%.1f published %s to [%s] (%s)\n",
                     temperature, payload.c_str(), this->topic.c_str(), reason);
    } else {
        GLOG::printf("TCTRL: temp=%.1f publish to [%s] FAILED\n",
                     temperature, this->topic.c_str());
    }
}
