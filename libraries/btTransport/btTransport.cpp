#include <Arduino.h>
#include <stdSensorInfoLoader.h>
#include <btTransport.h>
#include <gprs2.h>
#include <structs.h>
#include <EEPROM.h>
#include <mstr.h>


long long btTransport::makeCommunicationSession(long long mCurrTime,
                                           long long vPrevTime2,
                                           stdSensorInfoLoader& si,
                                           workerInfo &_water,
                                           workerInfo &_light) {


   _wasExternalKeyPress = false;
}

void btTransport::checkCommunicationSession() {

}

void btTransport::clearConfig() {

}

void btTransport::externalKeyPress(long long vCurrTime) {
  _wasExternalKeyPress = true;
}

unsigned long btTransport::getConnectPeriod() {

}

long long  btTransport::calcFirstConnectPeriod(long long vCurrTime) {
}