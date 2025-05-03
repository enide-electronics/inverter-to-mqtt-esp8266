/*
  VoltronicInverter.cpp - Common functions for voltronic inverters

  Based on code from:
  - https://github.com/amishv/voltronic_ESP8266_MQTT/blob/main/src/communication.cpp
  - https://github.com/ned-kelly/docker-voltronic-homeassistant/blob/master/sources/inverter-cli/inverter.cpp
  
  Written by JF enide.electronics (at) enide.net
  Licensed under GNU GPLv3
*/

#include "VoltronicInverter.h"
#include "../GLog.h"

VoltronicInverter::VoltronicInverter(Stream *serial, bool shouldDeleteSerial) {
    this->serial = serial;
    this->shouldDeleteSerial = shouldDeleteSerial;
}

VoltronicInverter::~VoltronicInverter() {
    if (shouldDeleteSerial) {
        delete serial;
    }
}


bool VoltronicInverter::sendCommand(String cmd) {
    uint8_t sendStr[30];
    uint8_t cmdLen = cmd.length();

    if (cmdLen > 29) {
        // prevent buffer overrun
        return false;
    }

    uint16_t cmdCrc = calcCRC((uint8_t *)cmd.c_str(), cmdLen);
    
    while (this->serial->available() > 0) {
        this->serial->read(); //arduino has no defined method to clear incomming buffer
    }

    memset(sendStr, 0, 30);
    sprintf((char *)sendStr, "%s%c%c\r", cmd.c_str(), (uint8_t)(cmdCrc >> 8), (uint8_t)(cmdCrc & 0xFF));

    GLOG::printf("\nINVERTER: sendCommand %2u bytes, cmd=\"%s\", CRC=0x%04x\n", strlen((char *)sendStr), cmd.c_str(), cmdCrc);
    
    return this->serial->write(sendStr, cmdLen + 2) > 0;
}

uint16_t VoltronicInverter::calcCRC(uint8_t *pin, uint8_t len) {
    uint16_t crc;
    uint8_t da;
    uint8_t *ptr;
    uint8_t bCRCHign;
    uint8_t bCRCLow;
    uint16_t crc_ta[16] = {
        0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7,
        0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef
    };
    ptr = pin;
    crc = 0;
    
    while (len-- != 0)
    {
        da = ((uint8_t)(crc >> 8)) >> 4;
        crc <<= 4;
        crc ^= crc_ta[da ^ (*ptr >> 4)];
        da = ((uint8_t)(crc >> 8)) >> 4;
        crc <<= 4;
        crc ^= crc_ta[da ^ (*ptr & 0x0f)];
        ptr++;
    }
    bCRCLow = crc;
    bCRCHign = (uint8_t)(crc >> 8);
    
    if (bCRCLow == 0x28 || bCRCLow == 0x0d || bCRCLow == 0x0a)
    {
        bCRCLow++;
    }
    
    if (bCRCHign == 0x28 || bCRCHign == 0x0d || bCRCHign == 0x0a)
    {
        bCRCHign++;
    }
    crc = ((uint16_t)bCRCHign) << 8;
    crc += bCRCLow;
    
    return (crc);
}

#define RECV_BUF_SIZE 256

String VoltronicInverter::recvResponse(uint16_t replysize) {
    uint16_t n = 0; // number of bytes received
    int b; // byte received
    char recvBuffer[RECV_BUF_SIZE];

    uint32_t startTimeMillis = millis();

    if (replysize >= RECV_BUF_SIZE) {
        return "";
    }

    do {
        if (this->serial->available() > 0) {
            b = this->serial->read();
            recvBuffer[n++] = b;
        }
    } while (n < replysize && n < RECV_BUF_SIZE && ((char) b) != '\r' && ((char) b) != '\n' && millis() - startTimeMillis < 5000);

    if (millis() - startTimeMillis >= 5000 || n != replysize) {
        return "";
    }

    recvBuffer[n] = '\0';

    // check first byte == (
    if (recvBuffer[0] != '(' || recvBuffer[replysize-1] != 0x0d) {
        GLOG::println("\nINVERTER: incorrect start/stop bytes.");
        GLOG::printf("INVERTER: Buffer: %s\n", recvBuffer);
        return "";
    }

    // check CRC
    uint16_t crc = calcCRC((uint8_t*)recvBuffer, replysize-3);
    if (recvBuffer[replysize-3]!=(crc>>8) || recvBuffer[replysize-2]!=(crc&0xff)) {
        return "";
    }

    return String(((const char *)recvBuffer) + 1); // ignore first byte
}