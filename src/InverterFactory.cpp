#include "InverterFactory.h"
#include "GlobalDefs.h"

#include <SoftwareSerial.h>

#include "growatt/GrowattInverter.h"
#include "growatt/MicInverter.h"
#include "growatt/MultiGrowattInverter.h"
#include "TestInverter.h"
#include "NoneInverter.h"
#include "soyosource/SoyosourceGTNInverter.h"
#include "voltronic/AxpertVMIII.h"
#include "GLog.h"

    
// factory interface implementations for SPH and MIC inverters
// note that in multi-inverter mode, inverters don't delete the serial port, ever
// it's up to the orchestrator class to delete it

class _SphFactory : public MultiGrowattInverterInnerFactory {
    virtual Inverter *createInverter(Stream *serial, int modbusAddress, bool enableRemoteCommands, bool isTL) {
        return new GrowattInverter(serial, false, modbusAddress, enableRemoteCommands, isTL);
    }
};

class _MicFactory : public MultiGrowattInverterInnerFactory {
    virtual Inverter *createInverter(Stream *serial, int modbusAddress, bool enableRemoteCommands, bool isTL) {
        return new MicInverter(serial, false, modbusAddress, isTL);
    }
};

static int getModbusAddress(const std::vector<int> modbusAddresses, int defaultAddr) {
    if (modbusAddresses.size() == 1) {
        return modbusAddresses[0];
    } else {
        return defaultAddr;
    }
}

static MultiGrowattInverter *createMultiGrowattInverter(MultiGrowattInverterInnerFactory *factory, const std::vector<int> modbusAddresses, bool isTL) {
#ifdef LARGE_ESP_BOARD
    #define PIN_RX D6
    #define PIN_TX D5
    SoftwareSerial *_softSerial = new SoftwareSerial(PIN_RX, PIN_TX);
    _softSerial->begin(9600);
    return new MultiGrowattInverter(_softSerial, true, modbusAddresses, true, isTL, factory);
#else
    Serial.begin(9600);
    return new MultiGrowattInverter(&Serial, false, modbusAddresses, true, isTL, factory);
#endif
}

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

static VoltronicAxpertVMIIIInverter *createVoltronicAxpertVMIIIInverter() {
#ifdef LARGE_ESP_BOARD
    #define PIN_RX D6
    #define PIN_TX D5
    SoftwareSerial *_softSerial = new SoftwareSerial(PIN_RX, PIN_TX);
    _softSerial->begin(2400);
    return new VoltronicAxpertVMIIIInverter(_softSerial, true);
#else
    Serial.begin(2400);
    return new VoltronicAxpertVMIIIInverter(&Serial, false);
#endif
}

static MicInverter *createMicInverter(int modbusAddress, bool isTL) {
#ifdef LARGE_ESP_BOARD
    #define PIN_RX D6
    #define PIN_TX D5
    SoftwareSerial *_softSerial = new SoftwareSerial(PIN_RX, PIN_TX);
    _softSerial->begin(9600);
    return new MicInverter(_softSerial, true, modbusAddress, isTL);
#else
    Serial.begin(9600);
    return new MicInverter(&Serial, false, modbusAddress, isTL);
#endif
}

// static method, caller is responsible for deleting the provided instance when no longer needed
Inverter *InverterFactory::createInverter(String type, const InverterParams params) {
    GLOG::println(String(F("FACT: inverter type ")) + type);
    
    if (type == "sph") {
        // remote control and single phase
        if (params.modbusAddresses.size() > 1) {
            return createMultiGrowattInverter(new _SphFactory(), params.modbusAddresses, false);
        } else {
            return createGrowattInverter(getModbusAddress(params.modbusAddresses, 1), true, false);
        }
    } else if (type == "sphtl") {
        // remote control and three phase
        if (params.modbusAddresses.size() > 1) {
            return createMultiGrowattInverter(new _SphFactory(), params.modbusAddresses, true);
        } else {
            return createGrowattInverter(getModbusAddress(params.modbusAddresses, 1), true, true);
        }
    } else if (type == "minxh") {
        // no remote control and single phase
        return createGrowattInverter(getModbusAddress(params.modbusAddresses, 1), false, false);
    } else if (type == "mic") {
        // no remote control, single phase
        if (params.modbusAddresses.size() > 1) {
            return createMultiGrowattInverter(new _MicFactory(), params.modbusAddresses, false);
        } else {
            return createMicInverter(getModbusAddress(params.modbusAddresses, 1), false);
        }
    } else if (type == "mictl") {
        // no remote control, three phase
        if (params.modbusAddresses.size() > 1) {
            return createMultiGrowattInverter(new _MicFactory(), params.modbusAddresses, true);
        } else {
            return createMicInverter(getModbusAddress(params.modbusAddresses, 1), true);
        }
    } else if (type == "test") {
        return new TestInverter();
    } else if (type == "gtn") {
        return createSoyosourceGTNInverter();
    } else if (type == "v_a_vmiii") {
        return createVoltronicAxpertVMIIIInverter();
    } else {
        return new NoneInverter();
    }
}
