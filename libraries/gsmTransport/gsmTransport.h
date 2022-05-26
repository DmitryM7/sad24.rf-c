#ifndef btTransport_h
#define btTransport_h

#include <worker.h>

#define __WAIT_MODEM_TIME__ 7000

/************************************************************
 * Максимальное кол-во символов в адресе управляющего сайта *
 ************************************************************/
#define __SITE_POINT_SIZE__ 40

/********************************************
 * Адреса расположения управляющих структур *
 ********************************************/
#define eeprom_mApnStart eeprom_mOfflineParamsStart+sizeof(offlineParams)
#define eeprom_mSiteStart eeprom_mApnStart+sizeof(ApnCon)
#define eeprom_mWorkerStart eeprom_mSiteStart+sizeof(offlineParams)

struct ApnCon {
  char apnPoint[35];
  char apnLogin[11];
  char apnPass[11];
};

struct SiteCon {
  char sitePoint[__SITE_POINT_SIZE__];
  char siteLogin[11];
  char sitePass[20];
  int  connectPeriod;
};




class gsmTransport
{

  public:

    void makeCommunicationSession(long long mCurrTime,long long vPrevTime2,stdSensorInfoLoader& si,workerInfo& _water,workerInfo& _light);
    void checkCommunicationSession();
    void clearConfig();
    unsigned long getConnectPeriod();


  private:
     unsigned long connectPeriod;
     void readSms2();
     bool isConnectInfoFull();
     void fillConnectInfo();
     bool updateRemoteMeasure(stdSensorInfoLoader& si,workerInfo& _water,workerInfo& _light);
     bool workWithRes(char* iRes);
     bool updateRemoteParams();
     bool doInternet();
     bool wk();
     void sl();
     void restartModem();
     bool onSms(byte iSms, char* iCommand);
     void setOffline(byte iDirection,int iLight, int iWater);
     void clearSite();
     void clearApn();
};

#endif
