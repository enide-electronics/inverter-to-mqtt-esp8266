/*
  GLog.cpp - Library for the ESP8266/ESP32 Arduino platform
  Global logging solution (used to prevent logging to the Serial interface)

  Written by JF enide.electronics (at) enide.net
  Licensed under GNU GPLv3
*/

#include "GLog.h"
#include "GlobalDefs.h"

Stream *GLOG::s = NULL;

void GLOG::println(unsigned char v) {
    if (GLOG::s) {
        GLOG::s->println(v);
    }
}

void GLOG::print(unsigned char v) {
    if (GLOG::s) {
        GLOG::s->print(v);
    }
}

void GLOG::println(char v) {
    if (GLOG::s) {
        GLOG::s->println(v);
    }
}

void GLOG::print(char v) {
    if (GLOG::s) {
        GLOG::s->print(v);
    }
}

void GLOG::println(int v) {
    if (GLOG::s) {
        GLOG::s->println(v);
    }
}

void GLOG::print(int v) {
    if (GLOG::s) {
        GLOG::s->print(v);
    }
}

void GLOG::println(const char * msg) {
    if (GLOG::s) {
        GLOG::s->println(msg);
    }
}

void GLOG::print(const char * msg) {
    if (GLOG::s) {
        GLOG::s->print(msg);
    }
}

void GLOG::println(const Printable &o) {
    if (GLOG::s) {
        GLOG::s->println(o);
    }
}

void GLOG::print(const Printable &o) {
    if (GLOG::s) {
        GLOG::s->print(o);
    }
}

void GLOG:: println(const String &o) {
    if (GLOG::s) {
        GLOG::s->println(o);
    }
}

void GLOG::print(const String &o) {
    if (GLOG::s) {
        GLOG::s->print(o);
    }
}

size_t GLOG::printf(const char *format, ...) {
    // based on Print.cpp
    if (GLOG::s) {
        va_list arg;
        va_start(arg, format);
        char temp[64];
        char* buffer = temp;
        size_t len = vsnprintf(temp, sizeof(temp), format, arg);
        va_end(arg);
        if (len > sizeof(temp) - 1) {
            buffer = new char[len + 1];
            if (!buffer) {
                return 0;
            }
            va_start(arg, format);
            vsnprintf(buffer, len + 1, format, arg);
            va_end(arg);
        }
        len = GLOG::s->write((const uint8_t*) buffer, len);
        if (buffer != temp) {
            delete[] buffer;
        }
        return len;
    }
}

void GLOG::logMqtt(char* topic, byte* payload, unsigned int length) {
    if (GLOG::s) {
        GLOG::s->print(F("MQTT: received ["));
        GLOG::s->print(topic);
        GLOG::s->print(F("]=["));
        for (unsigned int i = 0; i < length; i++) {
            GLOG::s->print((char)payload[i]);
        }
        GLOG::s->println(F("]"));
    }
}
    
void GLOG::setup() {
#ifdef LARGE_ESP_BOARD
    Serial.begin(115200);
    delay(10);
    GLOG::s = &Serial;
#else
    GLOG::s = NULL;
#endif
}

bool GLOG::isLogEnabled() {
    return GLOG::s != NULL;
}

