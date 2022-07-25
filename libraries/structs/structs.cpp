#include <Arduino.h>
#include <structs.h>
#include <EEPROM.h>



bool isDisabledLightRange() {
  offlineParams _offlineParams;
  EEPROM.get(eeprom_mOfflineParamsStart, _offlineParams);
  return abs(_offlineParams.tempUpLight1) == 19900 && abs(_offlineParams.tempUpLight2) == 19900;
}

bool isDisabledWaterRange() {
  offlineParams _offlineParams;
  EEPROM.get(eeprom_mOfflineParamsStart, _offlineParams);
  return abs(_offlineParams.tempUpWater1) == 1990 && abs(_offlineParams.tempUpWater2) == 1990;
}

void _setOffline(byte iDirection,int iLight, int iWater) {
  offlineParams _offlineParams;


  if (abs(iLight) > 199 || abs(iWater) > 199) {
    return;
  };

  EEPROM.get(eeprom_mOfflineParamsStart, _offlineParams);

  switch (iDirection) {
    case __LIGHT__:
      _offlineParams.tempUpLight1 = iLight * 100;
      _offlineParams.tempUpLight2 = iWater * 100;
    break;
    case __WATER__:
      _offlineParams.tempUpWater1 = iLight * 10;
      _offlineParams.tempUpWater2 = iWater * 10;
    break;
  };


  noInterrupts();
  EEPROM.put(eeprom_mOfflineParamsStart, _offlineParams);
  interrupts();

 };


