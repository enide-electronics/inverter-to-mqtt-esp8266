/*
  StatusPage.cpp - Library for the ESP8266/ESP32 Arduino platform
  Renders a read-only status page styled like the WiFiManager portal pages.

  Written by JF enide.electronics (at) enide.net
  Licensed under GNU GPLv3
*/

#include "StatusPage.h"

#include <ESP8266WiFi.h>
#include <vector>
#include <algorithm>

// Logical groups used to pretty-print the inverter measurements on the status
// page. Keys not listed here (e.g. task results) are emitted in an "Other"
// section so nothing from the inverter is ever hidden.
struct StatusGroup {
    const char *title;
    std::vector<const char *> keys;
};

static const StatusGroup kStatusGroups[] = {
    {
        "PV input",
        {"Ppv1", "Vpv1", "Ipv1", "Ppv2", "Vpv2", "Ipv2", "Ppv", "Vpv"}
    },
    {
        "AC output",
        {"Pac", "Fac",
         "Vac",
         "Vac1", "Iac1", "Pac1",
         "Vac2", "Iac2", "Pac2",
         "Vac3", "Iac3", "Pac3",
         "PacMeter"}
    },
    {
        "Battery",
        {"Battery",
         "Vbat", "Ibat", "Pbat",
         "SoC", "Pdischarge", "Pcharge"}
    },
    {
        "Energy counters",
        {"Etoday", "Etotal", "Ttotal"}
    },
    {
        "Temperatures",
        {"Temp1", "Temp2", "Temp3", "TempHeatsink"}
    },
    {
        "State",
        {"status", "Mode", "ModeString", "Priority",
         "Derating", "DeratingMode",
         "OperationStatus", "OperationStatusId",
         "Error", "ErrorString", "ErrorBitmask",
         "MeterConnected"}
    },
};

static String htmlEscape(const String &input) {
    String out;
    out.reserve(input.length());
    for (size_t i = 0; i < input.length(); ++i) {
        char c = input[i];
        switch (c) {
            case '&':  out += F("&amp;"); break;
            case '<':  out += F("&lt;"); break;
            case '>':  out += F("&gt;"); break;
            case '"':  out += F("&quot;"); break;
            case '\'': out += F("&#39;"); break;
            default:   out += c; break;
        }
    }
    return out;
}

StatusPage::StatusPage() {
}

void StatusPage::mergeInverterData(const InverterData &data) {
    for (auto it = data.begin(); it != data.end(); ++it) {
        merged[it->first] = it->second;
    }
}

void StatusPage::clearInverterData() {
    merged.clear();
}

void StatusPage::setMqttConnectedProvider(BoolProvider fn) {
    mqttConnectedProvider = fn;
}

void StatusPage::setDarkModeProvider(BoolProvider fn) {
    darkModeProvider = fn;
}

void StatusPage::setDeviceNameProvider(StringProvider fn) {
    deviceNameProvider = fn;
}

void StatusPage::setInverterTypeProvider(StringProvider fn) {
    inverterTypeProvider = fn;
}

void StatusPage::setMqttServerProvider(StringProvider fn) {
    mqttServerProvider = fn;
}

void StatusPage::setMqttTopicProvider(StringProvider fn) {
    mqttTopicProvider = fn;
}

String StatusPage::formatUptime() const {
    unsigned long ms = millis();
    unsigned long secs = ms / 1000UL;
    unsigned long mins = secs / 60UL;
    unsigned long hours = mins / 60UL;
    unsigned long days = hours / 24UL;

    secs %= 60UL;
    mins %= 60UL;
    hours %= 24UL;

    char buf[48];
    if (days > 0) {
        snprintf(buf, sizeof(buf), "%lud %02luh %02lum %02lus", days, hours, mins, secs);
    } else if (hours > 0) {
        snprintf(buf, sizeof(buf), "%luh %02lum %02lus", hours, mins, secs);
    } else {
        snprintf(buf, sizeof(buf), "%lum %02lus", mins, secs);
    }
    return String(buf);
}

String StatusPage::renderStyle(bool dark) const {
    // Minimal CSS mirroring the visual language of WiFiManager's HTTP_STYLE
    // (centered wrap, rounded pills, blue accents). The status page is
    // read-only so the only extra additions are table styles and status
    // badges.
    String s;
    s.reserve(1600);
    s += F(
        "<style>"
        ".c,body{text-align:center;font-family:verdana;}"
        "div{padding:5px;font-size:1em;margin:5px 0;box-sizing:border-box;}"
        ".wrap{text-align:left;display:inline-block;min-width:260px;max-width:560px;width:100%;}"
        "a{color:#000;font-weight:700;text-decoration:none;}"
        "a:hover{color:#1fa3ec;text-decoration:underline;}"
        "h1,h2,h3{margin:.4em 0;}"
        "hr{border:0;border-top:1px solid #ddd;margin:1em 0;}"
        "table.st{width:100%;border-collapse:collapse;margin:.2em 0 .8em 0;}"
        "table.st td{padding:.35em .5em;border-bottom:1px solid #eee;vertical-align:top;}"
        "table.st td.k{width:55%;color:#555;}"
        "table.st td.v{text-align:right;font-family:monospace;white-space:nowrap;}"
        ".badge{display:inline-block;padding:.15em .6em;border-radius:.8em;"
        "color:#fff;font-size:.85em;font-weight:700;}"
        ".badge.ok{background:#5cb85c;}"
        ".badge.ko{background:#dc3630;}"
        ".msg{padding:14px;margin:14px 0;border:1px solid #eee;"
        "border-left-width:5px;border-left-color:#1fa3ec;border-radius:.3rem;}"
        "button,input[type=button],input[type=submit]{cursor:pointer;border:0;"
        "background-color:#1fa3ec;color:#fff;line-height:2.4rem;font-size:1.2rem;"
        "width:100%;border-radius:.3rem;}"
        "form{margin:0;}"
        "</style>"
    );
    if (dark) {
        s += F(
            "<style>"
            "body{background:#1e1e1e;color:#e6e6e6;}"
            "a,a:visited{color:#64b5f6;}"
            "h1,h2,h3,h4,h5,h6{color:#e6e6e6;}"
            "hr{border-color:#444;}"
            "table.st td{border-bottom-color:#333;}"
            "table.st td.k{color:#bbb;}"
            ".msg{background:#2a2a2a;color:#e6e6e6;border-color:#555;"
            "border-left-color:#64b5f6;}"
            "button,input[type=button],input[type=submit]{background:#3a3a3a;"
            "color:#e6e6e6;border:1px solid #555;}"
            "</style>"
        );
    }
    return s;
}

static bool setContains(const std::vector<const char *> &v, const String &key) {
    for (auto *k : v) {
        if (key == k) return true;
    }
    return false;
}

String StatusPage::renderInverterTable() const {
    if (merged.empty()) {
        return F("<div class='msg'>No inverter data received yet.</div>");
    }

    String out;
    out.reserve(1024);

    // Track which keys have already been placed in a group so we can collect
    // the remainder in the "Other" section.
    std::vector<bool> consumed;
    consumed.assign(merged.size(), false);

    // Keep map key order stable for the "consumed" tracking.
    std::vector<const String *> keys;
    keys.reserve(merged.size());
    for (auto it = merged.begin(); it != merged.end(); ++it) {
        keys.push_back(&it->first);
    }

    for (const auto &group : kStatusGroups) {
        String rows;
        for (auto *groupKey : group.keys) {
            auto it = merged.find(String(groupKey));
            if (it == merged.end()) continue;

            rows += F("<tr><td class='k'>");
            rows += htmlEscape(it->first);
            rows += F("</td><td class='v'>");
            rows += htmlEscape(it->second);
            rows += F("</td></tr>");

            for (size_t i = 0; i < keys.size(); ++i) {
                if (*keys[i] == it->first) {
                    consumed[i] = true;
                    break;
                }
            }
        }

        if (rows.length() > 0) {
            out += F("<h3>");
            out += group.title;
            out += F("</h3><table class='st'>");
            out += rows;
            out += F("</table>");
        }
    }

    // Other / unknown keys
    String otherRows;
    for (size_t i = 0; i < keys.size(); ++i) {
        if (consumed[i]) continue;
        auto it = merged.find(*keys[i]);
        if (it == merged.end()) continue;

        otherRows += F("<tr><td class='k'>");
        otherRows += htmlEscape(it->first);
        otherRows += F("</td><td class='v'>");
        otherRows += htmlEscape(it->second);
        otherRows += F("</td></tr>");
    }
    if (otherRows.length() > 0) {
        out += F("<h3>Other</h3><table class='st'>");
        out += otherRows;
        out += F("</table>");
    }

    return out;
}

String StatusPage::renderHTML() const {
    bool dark = darkModeProvider ? darkModeProvider() : false;
    bool mqttUp = mqttConnectedProvider ? mqttConnectedProvider() : false;

    String deviceName = deviceNameProvider ? deviceNameProvider() : String();
    String inverterType = inverterTypeProvider ? inverterTypeProvider() : String();
    String mqttServer = mqttServerProvider ? mqttServerProvider() : String();
    String mqttTopic = mqttTopicProvider ? mqttTopicProvider() : String();

    String title = deviceName.length() > 0 ? deviceName : String(F("Status"));

    String page;
    page.reserve(3072);
    page += F("<!DOCTYPE html><html lang='en'><head>"
              "<meta name='format-detection' content='telephone=no'>"
              "<meta charset='UTF-8'>"
              "<meta name='viewport' content='width=device-width,initial-scale=1,user-scalable=no'/>"
              // Auto-refresh every 10s so the values stay close to live
              "<meta http-equiv='refresh' content='10'/>"
              "<title>");
    page += htmlEscape(title);
    page += F(" - Status</title>");
    page += renderStyle(dark);
    page += F("</head><body><div class='wrap'>");
    page += F("<h1>");
    page += htmlEscape(title);
    page += F("</h1><h3>Status</h3>");

    // --- System ---
    page += F("<h3>System</h3><table class='st'>");
    if (deviceName.length() > 0) {
        page += F("<tr><td class='k'>Device name</td><td class='v'>");
        page += htmlEscape(deviceName);
        page += F("</td></tr>");
    }
    page += F("<tr><td class='k'>Uptime</td><td class='v'>");
    page += formatUptime();
    page += F("</td></tr>");

    page += F("<tr><td class='k'>WiFi SSID</td><td class='v'>");
    page += htmlEscape(WiFi.SSID());
    page += F("</td></tr>");

    page += F("<tr><td class='k'>IP address</td><td class='v'>");
    page += WiFi.localIP().toString();
    page += F("</td></tr>");

    page += F("<tr><td class='k'>RSSI</td><td class='v'>");
    page += String(WiFi.RSSI());
    page += F(" dBm</td></tr>");

    page += F("<tr><td class='k'>MAC address</td><td class='v'>");
    page += WiFi.macAddress();
    page += F("</td></tr>");

    page += F("<tr><td class='k'>Free heap</td><td class='v'>");
    page += String(ESP.getFreeHeap());
    page += F(" bytes</td></tr>");
    page += F("</table>");

    // --- MQTT ---
    page += F("<h3>MQTT</h3><table class='st'>");
    page += F("<tr><td class='k'>Status</td><td class='v'>");
    if (mqttUp) {
        page += F("<span class='badge ok'>Connected</span>");
    } else {
        page += F("<span class='badge ko'>Disconnected</span>");
    }
    page += F("</td></tr>");
    if (mqttServer.length() > 0) {
        page += F("<tr><td class='k'>Server</td><td class='v'>");
        page += htmlEscape(mqttServer);
        page += F("</td></tr>");
    }
    if (mqttTopic.length() > 0) {
        page += F("<tr><td class='k'>Base topic</td><td class='v'>");
        page += htmlEscape(mqttTopic);
        page += F("</td></tr>");
    }
    page += F("</table>");

    // --- Inverter ---
    page += F("<h3>Inverter");
    if (inverterType.length() > 0) {
        page += F(" <small style='font-weight:normal;color:#888;'>(");
        page += htmlEscape(inverterType);
        page += F(")</small>");
    }
    page += F("</h3>");
    page += renderInverterTable();

    // --- Navigation ---
    page += F("<hr><form action='/' method='get'><button>Back</button></form>");

    page += F("</div></body></html>");
    return page;
}

const char *StatusPage::menuButtonHTML() {
    // WiFiManager stores the raw pointer, so this must live in program
    // storage. Using a plain static const char[] makes it reside in .rodata
    // for the entire program lifetime.
    static const char html[] =
        "<form action='/status' method='get'><button>Status</button></form><br/>\n";
    return html;
}
