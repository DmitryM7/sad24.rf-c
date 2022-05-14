#ifndef stdTransport_h
#define stdTransport_h

class stdTransport
{

  public:

    unsigned long getConnectPeriod();
    void makeCommunicationSession(stdSensorInfoLoader& si);

  private:
     void parseSmsParamCommand(char* iCommand);
     void readSms2();
     void setConnectPeriod(int iSleepTime);
     bool isConnectInfoFull();
     void sl();
     void restartModem();
     void fillConnectInfo();
}

#endif

