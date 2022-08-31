#ifndef gsmTransport_h
#define gsmTransport_h

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
#define eeprom_mWorkerStart eeprom_mSiteStart+sizeof(SiteCon)

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

    long long     makeCommunicationSession(long long mCurrTime,long long vPrevTime2,stdSensorInfoLoader& si,workerInfo& _water,workerInfo& _light);
    void          checkCommunicationSession();
    void          clearConfig();
    void          externalKeyPress(long long vCurrTime);
    unsigned long getConnectPeriod();
    long long     calcFirstConnectPeriod(long long vCurrTime);
    void          needTransferGps();

 protected:
  void          incConnectCount();


  private:
     bool _needTransferGps = false;

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
     void setConnectPeriod(int iConnectPeriod);


     unsigned int _connectCount=0;
};

#endif

