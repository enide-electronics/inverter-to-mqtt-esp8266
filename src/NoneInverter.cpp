#include "NoneInverter.h"

NoneInverter::NoneInverter() {

}

NoneInverter::~NoneInverter() {

}

void NoneInverter::read() {

}

bool NoneInverter::isDataValid() {
    return true;
}

    
InverterData NoneInverter::getData(bool fullSet) {
    return InverterData();
}

void NoneInverter::setIncomingTopicData(const String &topic, const String &value) {

}

std::list<String> NoneInverter::getTopicsToSubscribe() {
    return std::list<String>();
}
