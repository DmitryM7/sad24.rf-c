#ifndef structs_h
#define structs_h


/*******************************************************************
 * Интервал между запросами информации с датчиков. В секундах.     *
 *******************************************************************/
#define __MEASURE_PERIOD__ 180


/***************************************************************
 * Если интервал передачи данных не менее указанной константы, *
 * то модем уводим спать.                                      *
 ***************************************************************/
#define __MODEM_SLEEP_PERIOD__ 900


struct Globals {
  byte version;
};



#define __WATER_LIGHT__ 0
#define __WATER__ 1
#define __LIGHT__ 2

#define eeprom_mGlobalsStart 0
#define eeprom_mApnStart sizeof(Globals)
#define eeprom_mSiteStart eeprom_mApnStart+sizeof(ApnCon)
#define eeprom_mOfflineParamsStart eeprom_mSiteStart+sizeof(SiteCon)
#define eeprom_mWorkerStart eeprom_mOfflineParamsStart + sizeof(offlineParams)
#endif