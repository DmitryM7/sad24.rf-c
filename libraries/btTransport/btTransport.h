#ifndef btTransport_h
#define btTransport_h

#include <worker.h>



/********************************************
 * Адреса расположения управляющих структур *
 ********************************************/
#define eeprom_mWorkerStart eeprom_mOfflineParamsStart+sizeof(offlineParams)


class btTransport
{

  public:

    long long     makeCommunicationSession(long long mCurrTime,long long vPrevTime2,stdSensorInfoLoader& si,workerInfo& _water,workerInfo& _light);
    void          checkCommunicationSession();
    void          clearConfig();
    void          externalKeyPress(long long vCurrTime);
    unsigned long getConnectPeriod();
    long long     calcFirstConnectPeriod(long long vCurrTime);
    void          incConnectCount();

  private:

    bool _wasExternalKeyPress = false;


   //----------------------
};

#endif

