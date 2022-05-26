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

    void makeCommunicationSession(long long mCurrTime,long long vPrevTime2,stdSensorInfoLoader& si,workerInfo& _water,workerInfo& _light);
    void checkCommunicationSession();
    unsigned long getConnectPeriod();
    void clearConfig();


};

#endif

