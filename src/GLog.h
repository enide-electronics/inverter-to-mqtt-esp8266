/*
  GLog.h - Library header for the ESP8266/ESP32 Arduino platform
  Global logging solution (used to prevent logging to the Serial interface)

  Written by JF enide.electronics (at) enide.net
  Licensed under GNU GPLv3
*/

#ifndef G_LOG_H
#define G_LOG_H

#include <Arduino.h>

class GLOG {
    public:
        static void println(unsigned char v);
        static void print(unsigned char v);
        static void println(char v);
        static void print(char v);
        static void println(int v);
        static void print(int v);
        static void println(const char * msg);
        static void print(const char * msg);
        static void println(const Printable &o);
        static void print(const Printable &o);
        static void println(const String &o);
        static void print(const String &o);
        static size_t printf(const char *format, ...);
        static void logMqtt(char* topic, byte* payload, unsigned int length);
        
        static void setup();
        static bool isLogEnabled();
        
    private:
        static Stream *s;
};

#endif
