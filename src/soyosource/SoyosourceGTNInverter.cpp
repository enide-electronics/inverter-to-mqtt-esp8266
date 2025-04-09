/*
  SoyosourceGTNInverter.cpp

  For the Soyosource GTN1000/1200W Inverter models with display. The non-display one already has a wifi adapter.
  This code works by tapping into the LCD receive pin and interpreting the frames sent by the CPU.
  It will not send any commands back to the CPU.

  In the future it may send commands to the RS485 port to set the desired power output.
  
  Created by JF enide.electronics (at) enide.net
  Heavily based on https://github.com/Stefino76/soyosource-wifi-monitor/blob/main/soyosource-wifi-monitor.ino
  
  Check original project at https://github.com/Stefino76/soyosource-wifi-monitor
  
  Licensed under GNU GPLv3
*/

#include "SoyosourceGTNInverter.h"
#include "../GLog.h"
        
SoyosourceGTNInverter::SoyosourceGTNInverter(Stream *serial, bool shouldDeleteSerial): dataJson(256) {
    this->serial = serial;
    this->shouldDeleteSerial = shouldDeleteSerial;
}

SoyosourceGTNInverter::~SoyosourceGTNInverter() {
    if (this->shouldDeleteSerial) {
        delete serial;
    }
}

void SoyosourceGTNInverter::loop() {
  now = millis();

  //Lettura Seriale LCD
  if (this->serial->available()) {
    int byte = this->serial->read();  // Read the byte

    if (now - lastReadTime < 100 && byteIdx < 16) {
      data[byteIdx] = byte;
      byteIdx++;
    } else {
      data[0] = byte;
    
      if (byteIdx == 15) { //The entire message was readed
        saveToDataJson(data);
        isValid = true;
      }
    
      byteIdx = 1;
    } 
    
    lastReadTime = millis();
  }
}

void SoyosourceGTNInverter::read() {
    if (isValid) {
        String dataToSend;
        serializeJson(dataJson, dataToSend);
        GLOG::println(dataToSend);
    }
}
bool SoyosourceGTNInverter::isDataValid() {
    return isValid;
}
    
InverterData SoyosourceGTNInverter::getData(bool fullSet) {
    InverterData data;

    if (isValid) {
        uint16_t pw_req = dataJson["pw_req"];
        uint16_t mode = dataJson["mode"];
        uint16_t error = dataJson["error"] ;
        float bt_v = dataJson["bt_v"];
        float bt_a = dataJson["bt_a"];
        uint16_t ac_v = dataJson["ac_v"];
        float ac_hz = dataJson["ac_hz"] ;
        float temp = dataJson["temp"];
        float pw_out = bt_v * bt_a * 0.865; // as seen in the html.h file

        

        data.set("pw_req", pw_req);
        data.set("mode", mode);
        data.set("error", error);
        data.set("bt_v", bt_v);
        data.set("bt_a", bt_a);
        data.set("ac_v", ac_v);
        data.set("ac_hz", ac_hz);
        data.set("temp", temp);
        data.set("pw_out", pw_out);

        isValid = false;
    }
    
    return data;
}

void SoyosourceGTNInverter::setIncomingTopicData(const String &topic, const String &value) {

}

std::list<String> SoyosourceGTNInverter::getTopicsToSubscribe() {
    // nothing yet
    return std::list<String>();
}

void SoyosourceGTNInverter::saveToDataJson(int data[]) {
  dataJson["pw_req"] = static_cast<uint16_t>(data[1]) * 256 + data[2];
  dataJson["mode"] = data[3];
  dataJson["error"] = data[4];
  dataJson["bt_v"] = (static_cast<uint16_t>(data[5]) * 256 + data[6])*0.1;
  dataJson["bt_a"] = (static_cast<uint16_t>(data[7]) * 256 + data[8])*0.1;
  dataJson["ac_v"] = static_cast<uint16_t>(data[9]) * 256 + data[10];
  dataJson["ac_hz"] = (int)data[11]/2;
  dataJson["temp"] = ((static_cast<uint16_t>(data[12]) * 256 + data[13])-300)*0.1;
}

