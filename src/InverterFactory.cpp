#include "InverterFactory.h"
#include "GlobalDefs.h"

#include <SoftwareSerial.h>

#include "GrowattInverter.h"
#include "TestInverter.h"
#include "NoneInverter.h"
#include "soyosource/SoyosourceGTNInverter.h"
#include "GLog.h"

static GrowattInverter *createGrowattInverter(int modbusAddress, bool enableRemoteCommands, bool isTL) {
#ifdef LARGE_ESP_BOARD
    #define PIN_RX D6
    #define PIN_TX D5
    SoftwareSerial *_softSerial = new SoftwareSerial(PIN_RX, PIN_TX);
    _softSerial->begin(9600);
    return new GrowattInverter(_softSerial, true, modbusAddress, enableRemoteCommands, isTL);
#else
    Serial.begin(9600);
    return new GrowattInverter(&Serial, false, modbusAddress, enableRemoteCommands, isTL);
#endif
}

static SoyosourceGTNInverter *createSoyosourceGTNInverter() {
    #ifdef LARGE_ESP_BOARD
    #define PIN_RX D6
    #define PIN_TX D5
    SoftwareSerial *_softSerial = new SoftwareSerial(PIN_RX, PIN_TX);
    _softSerial->begin(9600);
    return new SoyosourceGTNInverter(_softSerial, true);
#else
    Serial.begin(9600);
    return new SoyosourceGTNInverter(&Serial, false);
#endif
}

// static method, caller is responsible for deleting the provided instance when no longer needed
Inverter *InverterFactory::createInverter(String type, InverterParams params) {
    GLOG::println(String(F("FACT: inverter type ")) + type);
    
    if (type == "sph") {
        // remote control and single phase
        return createGrowattInverter(params.modbusAddress, true, false);
    } else if (type == "sphtl") {
        // remote control and three phase
        return createGrowattInverter(params.modbusAddress, true, true);
    } else if (type == "minxh") {
        // no remote control and single phase
        return createGrowattInverter(params.modbusAddress, false, false);
    } else if (type == "test") {
        return new TestInverter();
    } else if (type == "gtn") {
        return createSoyosourceGTNInverter();
    } else {
        return new NoneInverter();
    }
}
