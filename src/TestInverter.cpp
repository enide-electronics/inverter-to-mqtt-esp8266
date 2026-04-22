#include "TestInverter.h"

TestInverter::TestInverter() {
    this->temperature = NAN;
}

TestInverter::~TestInverter() {

}

void TestInverter::read() {

}

bool TestInverter::isDataValid() {
    return true;
}

    
InverterData TestInverter::getData(bool fullSet) {
    InverterData data;

    
    float ten = 10.0;

    data.set("status", (uint8_t) 0);

    float v = random(1000, 1500) / ten;
    float i = random(5, 80) / ten;
    float p = v * i;
    data.set("Ppv1", p);    
    data.set("Vpv1", v);
    data.set("Ipv1", i);
    
    p *= 0.98; // losses in convertion! :-)
    v = random(2180, 2460) / ten; // assuming 230Vac country
    i = p / v;
    data.set("Vac1", v);
    data.set("Iac1", i);
    data.set("Pac1", p);
    
    data.set("Pac", p);
    data.set("Fac", (float) 50.0);

    // Simulate a plausible inverter temperature between 25 .. 75 degC
    this->temperature = random(250, 750) / 10.0f;
    data.set("Temp", this->temperature);

    return data;
}

void TestInverter::setIncomingTopicData(const String &topic, const String &value) {

}

std::list<String> TestInverter::getTopicsToSubscribe() {
    return std::list<String>();
}

float TestInverter::getMaxTemperature() {
    return this->temperature;
}

static const HaSensorDescriptor TEST_SENSORS[] = {
    // name,    friendly,                   unit,   device_class,  state_class,   icon
    { "status", "Status",                   NULL,   NULL,          NULL,          "mdi:test-tube" },
    { "Ppv1",   "PV1 Power",                "W",    "power",       "measurement", NULL },
    { "Vpv1",   "PV1 Voltage",              "V",    "voltage",     "measurement", NULL },
    { "Ipv1",   "PV1 Current",              "A",    "current",     "measurement", NULL },
    { "Vac1",   "Grid Voltage L1",          "V",    "voltage",     "measurement", NULL },
    { "Iac1",   "Grid Current L1",          "A",    "current",     "measurement", NULL },
    { "Pac1",   "Grid Apparent Power L1",   "VA",   "apparent_power", "measurement", NULL },
    { "Pac",    "Grid Active Power",        "W",    "power",       "measurement", NULL },
    { "Fac",    "Grid Frequency",           "Hz",   "frequency",   "measurement", NULL },
    { "Temp",   "Inverter Temperature",     "\xC2\xB0""C", "temperature", "measurement", NULL },
};

std::list<HaDiscoveryMessage> TestInverter::getHomeAssistantDiscovery(const HaDiscoveryDevice &device) {
    std::list<HaDiscoveryMessage> out;

    HaDiscoveryDevice d = device;
    if (d.model.length() == 0) {
        d.model = F("Test Inverter");
    }
    if (d.manufacturer.length() == 0) {
        d.manufacturer = F("enide.net");
    }

    const size_t count = sizeof(TEST_SENSORS) / sizeof(TEST_SENSORS[0]);
    HaDiscoveryBuilder::appendAll(out, d, TEST_SENSORS, count);

    return out;
}
