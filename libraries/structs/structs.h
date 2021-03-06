#ifndef structs_h
#define structs_h
/************************************
 * ������ ��������� ����������� ��� * 
 ************************************/
#define ECHO_PIN 10
#define TRIG_PIN 3

/************************************************************
 * ������������ ���-�� �������� � ������ ������������ ����� *
 ************************************************************/
#define __SITE_POINT_SIZE__ 40

/************************************************************
 * �������� ����� ��������� ���������� � ��������.          *
 ************************************************************/
#define __MEASURE_PERIOD__ 180000

/************************************************************
 * ���-�� ���������� ������� ���� ����� ��������� ������    *
 ************************************************************/

#define __WAIT_MODEM_TIME__ 7000

struct Connection { 
  char apnPoint[35];
  char apnLogin[11];
  char apnPass[11];
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

#define eeprom_mGlobalsStart sizeof(Connection)
#define eeprom_mOfflineParamsStart sizeof(Connection)+sizeof(Globals)
#define eeprom_mWorkerStart eeprom_mOfflineParamsStart + sizeof(offlineParams)
#endif