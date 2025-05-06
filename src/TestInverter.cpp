#include "TestInverter.h"

TestInverter::TestInverter() {

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

    return data;
}

void TestInverter::setIncomingTopicData(const String &topic, const String &value) {

}

std::list<String> TestInverter::getTopicsToSubscribe() {
    return std::list<String>();
}
