#ifndef structs_h
#define structs_h
/************************************
 * Задаем параметры подключения УЗД * 
 ************************************/
#define ECHO_PIN 10
#define TRIG_PIN 3

/************************************************************
 * Максимальное кол-во символов в адресе управляющего сайта *
 ************************************************************/
#define __SITE_POINT_SIZE__ 40

/************************************************************
 * Интервал между запросами информации с датчиков.          *
 ************************************************************/
#define __MEASURE_PERIOD__ 180000

/************************************************************
 * Кол-во милисекунд которые ждем после включения модема    *
 ************************************************************/

#define __WAIT_MODEM_TIME__ 7000

struct ApnCon { 
  char apnPoint[35];
  char apnLogin[11];
  char apnPass[11];
};

struct SiteCon {
  char sitePoint[__SITE_POINT_SIZE__];
  char siteLogin[11];
  char sitePass[20];
};

struct Globals {
  char version[3];
  int  sleepTime;
  int  connectPeriod;
};

struct offlineParams {
  int  tempUpLight1;
  int  tempUpLight2;
  int  tempUpWater1;
  int  tempUpWater2;
};

struct sensorInfo {
             int t1=19900; 
             int h1; 
             int t2=1990; 
             unsigned long p1;
             int distance;
             unsigned long lastMeasure=0;
};

#define eeprom_mSiteCon sizeof(ApnCon)
#define eeprom_mGlobalsStart sizeof(ApnCon)+sizeof(SiteCon)
#define eeprom_mOfflineParamsStart eeprom_mGlobalsStart+sizeof(Globals)
 #define eeprom_mWorkerStart eeprom_mOfflineParamsStart + sizeof(offlineParams)
#endif