#ifndef stdTransport_h
#define stdTransport_h

#include <worker.h>

#define __WAIT_MODEM_TIME__ 7000

/************************************************************
 * Максимальное кол-во символов в адресе управляющего сайта *
 ************************************************************/
#define __SITE_POINT_SIZE__ 40

/********************************************
 * Адреса расположения управляющих структур *
 ********************************************/

#define eeprom_mGlobalsStart 0
#define eeprom_mApnStart sizeof(Globals)
#define eeprom_mSiteStart eeprom_mApnStart+sizeof(ApnCon)
#define eeprom_mOfflineParamsStart eeprom_mSiteStart+sizeof(SiteCon)
#define eeprom_mWorkerStart eeprom_mOfflineParamsStart + sizeof(offlineParams)

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


struct offlineParams {
  int  tempUpLight1;
  int  tempUpLight2;
  int  tempUpWater1;
  int  tempUpWater2;
};


class stdTransport
{

  public:

    unsigned long getConnectPeriod();
    void makeCommunicationSession(long long mCurrTime,long long vPrevTime2,stdSensorInfoLoader& si,workerInfo& _water,workerInfo& _light);
    void checkCommunicationSession();
    bool wk();
    void sl();
    void restartModem();
    void setOffline(byte iDirection,int iLight, int iWater); //todo Нужно переделать
    bool onSms(byte iSms, char* iCommand);
    void parseSmsParamCommand(char* iCommand);


  private:
     unsigned long connectPeriod;
     void readSms2();
     void setConnectPeriod(int iSleepTime);
     bool isConnectInfoFull();
     void fillConnectInfo();
     bool updateRemoteMeasure(stdSensorInfoLoader& si,workerInfo& _water,workerInfo& _light);
     bool workWithRes(char* iRes);
     bool updateRemoteParams();
     bool doInternet();
     bool beforeTaskUpdate(char* iStr);
};

#endif

