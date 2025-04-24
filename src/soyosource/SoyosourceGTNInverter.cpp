/*
  SoyosourceGTNInverter.cpp

  For the Soyosource GTN1000/1200W Inverter models with display. The non-display one already has a wifi adapter.
  This code works by tapping into the LCD receive pin and interpreting the frames sent by the CPU.
  It will not send any commands back to the CPU.

  In the future it may send commands to the RS485 port to set the desired power output.
  
  Created by JF enide.electronics (at) enide.net
  
  Originally based on https://github.com/Stefino76/soyosource-wifi-monitor
  And re-written with lots of ideas from https://github.com/syssi/esphome-soyosource-gtn-virtual-meter/blob/main/components/soyosource_display/soyosource_display.cpp
  
  Licensed under GNU GPLv3
*/

#include "SoyosourceGTNInverter.h"
#include "../GLog.h"

#define SOF_REQUEST 0x55
#define DISPLAY_PROTOCOL 0
#define WIFI_PROTOCOL 1

#define SOF_SOYO_RESPONSE 0xA6
#define SOF_MS51_RESPONSE 0x5A

#define SOF_SOYO_RESPONSE_LEN 15
#define SOF_MS51_RESPONSE_LEN 17

#define STATUS_COMMAND 0x01
#define SETTINGS_COMMAND 0x03

#define ERRORS_SIZE 8
static const char *const ERRORS[ERRORS_SIZE] = {
  "Reserved (Bit 1)",     // 0000 0001
  "DC voltage too low",   // 0000 0010
  "DC voltage too high",  // 0000 0100
  "AC voltage too high",  // 0000 1000
  "AC voltage too low",   // 0001 0000
  "Overheat",             // 0010 0000
  "Limiter connected",    // 0100 0000
  "Reserved (Bit 8)",     // 1000 0000
};

SoyosourceGTNInverter::SoyosourceGTNInverter(Stream *serial, bool shouldDeleteSerial) {
    this->serial = serial;
    this->shouldDeleteSerial = shouldDeleteSerial;
    this->lastReadMillis = millis();
    this->unknownFrameCounter = 0;
}

SoyosourceGTNInverter::~SoyosourceGTNInverter() {
    if (this->shouldDeleteSerial) {
        delete serial;
    }
}

void SoyosourceGTNInverter::loop() {
    uint32_t now = millis();

    if (now - this->lastReadMillis > 50) {
        this->rxBuffer.clear();
        this->lastReadMillis = now;
    }

    while (this->serial->available()) {
        uint8_t byte;
        byte = this->serial->read();

        if (this->parseSoyosourceDisplayByte(byte)) {
            this->lastReadMillis = now;
        } else {
            this->rxBuffer.clear();
        }
    }
}

static uint8_t chksum(const uint8_t data[], const uint8_t len) {
    uint8_t checksum = 0xFF;
  
    for (uint8_t i = 1; i < len; i++) {
        checksum = checksum - data[i];
    }

    return checksum;
}

bool SoyosourceGTNInverter::parseSoyosourceDisplayByte(uint8_t byte) {
    size_t at = this->rxBuffer.size();
    this->rxBuffer.push_back(byte);
    
    const uint8_t *raw = &this->rxBuffer[0];
    
    // Supported Soyosource responses
    //
    // Status request        >>> 0x55 0x01 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0xFE
    // Status response       <<< 0xA6 0x00 0x00 0xD1 0x02 0x00 0x00 0x00 0x00 0x00 0xFB 0x64 0x02 0x0D 0xBE
    //
    // Settings request      >>> 0x55 0x03 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0xFC
    // Settings response     <<< 0xA6 0x00 0x00 0xD3 0x02 0xD4 0x30 0x30 0x2D 0x00 0xFB 0x64 0x4B 0x06 0x19
    //
  
    uint16_t frame_len = SOF_SOYO_RESPONSE_LEN;
    uint8_t function_pos = 3;
    
    if (at == 0) {
        if (raw[0] != SOF_SOYO_RESPONSE && raw[0] != SOF_MS51_RESPONSE) {
            GLOG::printf("INVERTER: Invalid header: 0x%02X\n", raw[0]);

            // return false to reset buffer
            return false;
        }

        return true;
    }

    
    // Supported MS51 responses
    //
    // Status request        >>> 0x55 0x01 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0xFE
    // Status response       <<< 0x5A 0x01 0xD1 0x02 0x00 0x00 0x00 0x00 0x00 0xE7 0x32 0x00 0x00 0x00 0x00 0x19 0xF9
    //
    // Settings request     >>> 0x55 0x03 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0xFC
    // Settings response    <<< 0x5A 0x01 0xD3 0x02 0xD4 0x30 0x31 0x2F 0x00 0xE7 0x64 0x5A 0x00 0x06 0x37 0x5A 0x89
    //
    if (raw[0] == SOF_MS51_RESPONSE) {
        frame_len = SOF_MS51_RESPONSE_LEN;
        function_pos = 2;
    }

    // Bytes 0...14
    if (at < frame_len - 1)
        return true;

    // CRC received, now validate
    uint8_t function = raw[function_pos] & 0x0F;
    uint8_t computed_crc = chksum(raw, frame_len - 1);
    uint8_t remote_crc = raw[frame_len - 1];
    
    if (computed_crc != remote_crc) {
        GLOG::printf("INVERTER: CRC error: 0x%02X != 0x%02X\n", computed_crc, remote_crc);
        return false;
    }

    std::vector<uint8_t> data(this->rxBuffer.begin(), this->rxBuffer.begin() + frame_len);
    this->decodeFrameData(function, data);

    // return false to reset buffer
    return false;
}

void SoyosourceGTNInverter::decodeFrameData(const uint8_t &function, const std::vector<uint8_t> &data) {
    if (data.size() != SOF_SOYO_RESPONSE_LEN && data.size() != SOF_MS51_RESPONSE_LEN) {
        GLOG::println("INVERTER: Invalid frame size");
        isValid = buildErrorData(data);
        return;
    }

    uint8_t response_source = data[0];
    isValid = false;

    if (response_source == SOF_MS51_RESPONSE) {  
        switch (function) {
            case STATUS_COMMAND:
                isValid = this->extractMS51StatusData(data);
                break;
            case SETTINGS_COMMAND:
                GLOG::println("INVERTER: Ignoring settings frame");
                break;
            default:
                GLOG::printf("INVERTER: Ignoring MS51 frame, src=0x%02x, func=0x%02x\n", response_source, function);
        }
    } else if (response_source == SOF_SOYO_RESPONSE) {
        switch (function) {
            case STATUS_COMMAND:
                isValid = this->extractDisplayStatusData(data);
                break;
            case SETTINGS_COMMAND:
                GLOG::println("INVERTER: Ignoring settings frame");
                break;
            default:
                GLOG::printf("INVERTER: Ignoring frame, src=0x%02x, func=0x%02x\n", response_source, function);
        }
    } else {
        GLOG::printf("INVERTER: Ignoring unknwon frame, src=0x%02x, func=0x%02x\n", response_source, function);
        isValid = buildErrorData(data, response_source, function);
    }
}

void SoyosourceGTNInverter::read() {
    sendCommand(STATUS_COMMAND, DISPLAY_PROTOCOL);
}

bool SoyosourceGTNInverter::isDataValid() {
    return isValid;
}
    
InverterData SoyosourceGTNInverter::getData(bool fullSet) {
    if (isValid) {
        isValid = false;
        return inverterData;
    }
    
    return InverterData();
}

void SoyosourceGTNInverter::setIncomingTopicData(const String &topic, const String &value) {
}

std::list<String> SoyosourceGTNInverter::getTopicsToSubscribe() {
    // nothing yet
    return std::list<String>();
}

bool SoyosourceGTNInverter::extractDisplayStatusData(const std::vector<uint8_t> &data) {
    auto soyosource_get_16bit = [&](size_t i) -> uint16_t {
        return (uint16_t(data[i + 0]) << 8) | (uint16_t(data[i + 1]) << 0);
    };

    // Byte Len  Payload                Content              Coeff.      Unit        Example value
    // 0     1   0xA6                   Header
    // 1     2   0x00 0x84              Output Power         1.0         W           132 W
    inverterData.set("PacMeter", soyosource_get_16bit(1));
    
    // 3     1   0x91                   Operation mode (High nibble), Frame function (Low nibble)
    //                                                                0x01: Status frame
    uint8_t rawOperationMode = data[3] >> 4;
    inverterData.set("Mode", rawOperationMode);
    inverterData.set("ModeString", displayModeToString(rawOperationMode));

    // 4     1   0x40                   Error and status bitmask
    inverterData.set("Error", data[4]);
    uint8_t raw_status_bitmask = data[4] & ~(1 << 6);
    inverterData.set("MeterConnected", ((bool) (data[4] & (1 << 6))) ? "yes" : "no");
    inverterData.set("OperationStatusId", (uint8_t) ((raw_status_bitmask == 0x00) ? 0 : 2));
    inverterData.set("OperationStatus", (raw_status_bitmask == 0x00) ? "Normal" : "Standby");
    inverterData.set("ErrorBitmask", (float) raw_status_bitmask);
    inverterData.set("ErrorString", errorToString(raw_status_bitmask));
    
    // 5     2   0x01 0xC5              Battery voltage
    float vbat = soyosource_get_16bit(5) * 0.1f;

    // 7     2   0x00 0xDB              Battery current
    float ibat = soyosource_get_16bit(7) * 0.1f;
    float pbat = vbat * ibat;
    inverterData.set("Vbat", vbat);
    inverterData.set("Ibat", ibat);
    inverterData.set("Pbat", pbat);
    
    // Replicate the behaviour of the display firmware to avoid confusion
    // We are using a constant efficiency of 87% like the display firmware (empirically discovered)
    // See https://github.com/syssi/esphome-soyosource-gtn-virtual-meter/issues/184#issuecomment-2264960366
    inverterData.set("Pac", pbat * 0.86956f);

    // 9     2   0x00 0xF7              Grid voltage
    inverterData.set("Vac", soyosource_get_16bit(9));

    // 11     1   0x63                   Grid frequency
    inverterData.set("Fac", data[11] / 2.0f);
    
    // 12    2   0x02 0xBC              Temperature
    inverterData.set("Temp", (soyosource_get_16bit(12) - 300) * 0.1f);

    return true;
}

bool SoyosourceGTNInverter::extractMS51StatusData(const std::vector<uint8_t> &data) {
    auto soyosource_get_16bit = [&](size_t i) -> uint16_t {
        return (uint16_t(data[i + 0]) << 8) | (uint16_t(data[i + 1]) << 0);
    };

    GLOG::printf("INVERTER: Status frame (MS51, %d bytes) received\n", data.size());

    if (soyosource_get_16bit(8) == 0x0000 && data[15] == 0x00) {
        GLOG::println("INVERTER: Ignoring empty MS51 status");
        return false;
    }

    // Soyosource status: 0xA6 0x02 0xEA 0x91 0x40 0x01 0xC5 0x00 0x32 0x00 0xF7 0x64 0x02 0x12 0xDB
    // MS5 status:             0x5A 0x01 0x91 0x40 0x01 0xC5 0x00 0x32 0x00 0xF7 0x32 0x00 0xCA 0x00 0x00 0x17 0x2B

    // Byte Len  Payload                Content              Coeff.      Unit        Example value
    // 0     1   0x5A                   Header
    // 1     1   0x01                   Unknown always 1
    // 2     1   0x91                   Operation mode (High nibble), Frame function (Low nibble)
    uint8_t raw_operation_mode = data[2] >> 4;
    inverterData.set("Mode", raw_operation_mode);
    inverterData.set("ModeString", this->wifiModeToString(raw_operation_mode));

    // 3     1   0x40                   Error and status bitmask
    uint8_t raw_status_bitmask = data[3] & ~(1 << 6);
    inverterData.set("MeterConnected", ((bool) (data[3] & (1 << 6)) ? "yes" : "no"));
    inverterData.set("OperationStatusId", (uint8_t) ((raw_status_bitmask == 0x00) ? 0 : 2));
    inverterData.set("OperationStatus", (raw_status_bitmask == 0x00) ? "Normal" : "Standby");
    inverterData.set("ErrorBitmask", (float) raw_status_bitmask);
    inverterData.set("ErrorString", this->errorToString(raw_status_bitmask));

    // 4     2   0x01 0xC5              Battery voltage
    float vbat = soyosource_get_16bit(4) * 0.1f;
    inverterData.set("Vbat", vbat);

    // 6     2   0x00 0x32              Battery current
    float ibat = soyosource_get_16bit(6) * 0.1f;
    float pbat = vbat * ibat;
    inverterData.set("Ibat", ibat);
    inverterData.set("Pbat", pbat);
      
    // 8     2   0x00 0xF7              Grid voltage
    inverterData.set("Vac", soyosource_get_16bit(8) * 1.0f);

    // 10    1   0x32                   Grid frequency       1.0         Hz          50 Hz
    inverterData.set("Fac", data[10]);

    // 11    2   0x00 0xCA              Output Power         1.0         W           202 W
    inverterData.set("PacMeter", soyosource_get_16bit(11) * 1.0f);
    inverterData.set("Pac", pbat * 0.86956f);

    // 13    2   0x00 0x00              Total energy         0.1         kWh         00.0 kWh
    // When this value reaches 6500, it only delivers a sawtooth resetting to 6477 on each hit of 6500.
    // The expected wrap around on 6553.5 to 0 doesn't happen.
    inverterData.set("Etotal", soyosource_get_16bit(13) * 0.1f);

    // 15    1   0x17                   Temperature          1.0         °C          23 °C
    inverterData.set("Temp", data[15]);

    return true;
}
bool SoyosourceGTNInverter::buildErrorData(const std::vector<uint8_t> &data, uint8_t response_source, uint8_t function) {
    inverterData.clear();

    inverterData.set("BadFrameCount", ++this->unknownFrameCounter);
    
    if (response_source != 0) {
        inverterData.set("BadSource", response_source);
    }
    
    if (function != 0) {
        inverterData.set("BadFunction", function);
    }
    return true;
}

String SoyosourceGTNInverter::displayModeToString(const uint8_t &operation_mode) {
    // 0x01: 0001 BatCP Mode + Operation
    // 0x02: 0010 PV Mode + Operation
    // 0x05: 0101 BatCP Mode + Standby
    // 0x06: 0110 PV Mode + Standby
    // 0x08: 1000 Bat Limit + Operation
    // 0x09: 1001 Bat Limit + (BatCP Mode) + Operation
    // 0x0C: 1100 Bat limit + Standby
    // 0x0D: 1101 Bat Limit + (BatCP Mode) + Standby
    //       ||||
    //       |||BatCP Mode bit
    //       |||
    //       ||PV Mode bit
    //       ||
    //       |Standby bit
    //       |
    //       Bat Limit bit
    //
    switch (operation_mode) {
        case 0x01:
        case 0x05:
            return "Battery Constant Power";
        case 0x02:
        case 0x06:
            return "PV";
        case 0x08:
        case 0x09:
        case 0x0C:
        case 0x0D:
            return "Battery Limit";
    }

    return String("Unknown 0x") + String(operation_mode, HEX);
}

String SoyosourceGTNInverter::wifiModeToString(const uint8_t &operation_mode) {
    // 0x01: 0001   Battery
    // 0x05: 0101   Battery + Standby
    //
    // 0x02: 0010   PV
    // 0x06: 0110   PV + Standby
    //
    // 0x09: 1001   Battery + Limiter
    // 0x0D: 1101   Battery + Limiter + Standby
    //
    // 0x0A: 1010   PV + Limiter
    // 0x0E: 1110   PV + Limiter + Standby
    //       ||||
    //       |||Battery mode bit
    //       |||
    //       ||PV mode bit
    //       ||
    //       |Standby bit
    //       |
    //       Limiter bit
    //
    switch (operation_mode) {
        case 0x01:
        case 0x05:
            return "Battery Constant Power";
        case 0x02:
        case 0x06:
            return "PV";
        case 0x09:
        case 0x0D:
            return "Battery Limit";
        case 0x0A:
        case 0x0E:
            return "PV Limit";
    }

  return String("Unknown 0x") + String(operation_mode, HEX);
}
  
String SoyosourceGTNInverter::errorToString(const uint8_t &mask) {
    std::string values = "";
    if (mask) {
        for (int i = 0; i < ERRORS_SIZE; i++) {
            if (mask & (1 << i)) {
                values.append(ERRORS[i]);
                values.append(";");
            }
        }
        
        if (!values.empty()) {
            values.pop_back();
        }
    }
    
    return String(values.c_str());
}

void SoyosourceGTNInverter::sendCommand(uint8_t function, uint8_t protocol) {
    uint8_t frame[12];
    uint8_t len;
    
    if (protocol == DISPLAY_PROTOCOL) {
        len = 6;
        frame[0] = SOF_REQUEST;
        frame[1] = function;
        frame[2] = 0x00;
        frame[3] = 0x00;
        frame[4] = 0x00;
        frame[5] = chksum(frame, 5);
    } else {
        len = 12;
        frame[0] = SOF_REQUEST;
        frame[1] = function;
        frame[2] = 0x00;
        frame[3] = 0x00;
        frame[4] = 0x00;
        frame[5] = 0x00;
        frame[6] = 0x00;
        frame[7] = 0x00;
        frame[8] = 0x00;
        frame[9] = 0x00;
        frame[10] = 0x00;
        frame[11] = chksum(frame, 11);
    }

    this->serial->write(frame, len);
    this->serial->flush();
}