#ifndef structs_h
#define structs_h

/************************************************************
 * ������������ ���-�� �������� � ������ ������������ ����� *
 ************************************************************/
#define __SITE_POINT_SIZE__ 40

/*******************************************************************
 * �������� ����� ��������� ���������� � ��������. � ��������.     *
 *******************************************************************/
#define __MEASURE_PERIOD__ 180

/************************************************************
 * ���-�� ���������� ������� ���� ����� ��������� ������    *
 ************************************************************/

#define __WAIT_MODEM_TIME__ 7000

/***************************************************************
 * ���� �������� �������� ������ �� ����� ��������� ���������, *
 * �� ����� ������ �����.                                      *
 ***************************************************************/
#define __MODEM_SLEEP_PERIOD__ 900

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


#define __WATER_LIGHT__ 0
#define __WATER__ 1
#define __LIGHT__ 2


#define eeprom_mSiteCon sizeof(ApnCon)
#define eeprom_mGlobalsStart sizeof(ApnCon)+sizeof(SiteCon)
#define eeprom_mOfflineParamsStart eeprom_mGlobalsStart+sizeof(Globals)
#define eeprom_mWorkerStart eeprom_mOfflineParamsStart + sizeof(offlineParams)
#endif