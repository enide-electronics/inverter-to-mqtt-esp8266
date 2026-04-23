/*
  StatusPage.h - Library header for the ESP8266/ESP32 Arduino platform
  Renders a read-only status page (uptime, MQTT connection, merged inverter data)
  styled like the WiFiManager portal pages.

  Written by JF enide.electronics (at) enide.net
  Licensed under GNU GPLv3
*/

#ifndef _STATUS_PAGE_H
#define _STATUS_PAGE_H

#include <Arduino.h>
#include <functional>
#include "InverterData.h"

class StatusPage {
    public:
        using BoolProvider = std::function<bool()>;
        using StringProvider = std::function<String()>;

        StatusPage();

        // Merge a (possibly partial) InverterData snapshot into the accumulated
        // state. Existing keys are overwritten with newer values; keys not
        // included in the snapshot stay untouched so that inverters that only
        // publish a subset on each read can still build up the full picture.
        void mergeInverterData(const InverterData &data);

        // Drop all accumulated inverter data. Call this when the active
        // inverter is swapped (e.g. after a config change).
        void clearInverterData();

        // Providers are sampled on every request so the page always shows live
        // values even when the underlying pointers (e.g. MqttPublisher) get
        // recreated on config changes.
        void setMqttConnectedProvider(BoolProvider fn);
        void setDarkModeProvider(BoolProvider fn);
        void setDeviceNameProvider(StringProvider fn);
        void setInverterTypeProvider(StringProvider fn);
        void setMqttServerProvider(StringProvider fn);
        void setMqttTopicProvider(StringProvider fn);

        // Renders the full HTML page body.
        String renderHTML() const;

        // Marker that WiFiManager's main menu can link to. Returns an HTML
        // snippet suitable for setCustomMenuHTML(). The storage is static so
        // the pointer stays valid for the lifetime of the program, which
        // WiFiManager requires.
        static const char *menuButtonHTML();

    private:
        InverterData merged;

        BoolProvider mqttConnectedProvider;
        BoolProvider darkModeProvider;
        StringProvider deviceNameProvider;
        StringProvider inverterTypeProvider;
        StringProvider mqttServerProvider;
        StringProvider mqttTopicProvider;

        String formatUptime() const;
        String renderStyle(bool dark) const;
        String renderInverterTable() const;
};

#endif
