#ifndef gprs2_h
#define gprs2_h
#include "SoftwareSerial.h"

class gprs2
{

  public:
    /********************************
     * For common use.              *
     ********************************/
    gprs2(int pin1,int pin2);
    bool postUrl(char* iUrl,char* iPar,char* oRes);
    bool postUrl(char* iUrl, char* iPar, char* oRes,unsigned int iResLength);
    void setInternetSettings(char* vApn,char* vLogin,char* vPass);
    bool powerUp();
    bool powerDown();
    bool gprsUp();
    bool hasGprs();
    bool isGprsUp();
    bool isPowerUp();
    bool isConnect();
    bool canDoPostUrl();
    void getLastError(char* oLastError);
    uint8_t getLastErrorNum();
    bool saveOnSms();
    void getSmsText(unsigned int iNum,char* oRes,int iSmsSize);
    bool deleteAllSms();
    bool deleteSms(byte iSms);
    void doCmd(const __FlashStringHelper *iStr,char* oRes,int iSize);
    void doCmd(char* iStr,char* oRes,int iSize);
    bool isReady();
    int getSmsCount();
    bool sendSms(char* iPhone,char* iText);
    void readSms(bool deleteAfterRead);
    void readSms();
    bool setOnSms(bool (*_mSmsEvent)(byte iSms, char* oStr));
    bool hasNetwork(byte iWaitAttempt);
    void softRestart();
    void getCoords(char* iLongitude,char* iLatitdue);

  private:
    SoftwareSerial _modem;
    char  _apn[25];
    char  _login[20];
    char  _pass[20];
    char  _lastError[20]; 
    uint8_t _lastErrorNum;
    bool   getAnswer3(char* oRes,unsigned int iSize);
    bool   _getAnswer3(char* oRes,unsigned int iSize,bool saveCRLF);
    bool   _getAnswer3(char* oRes,unsigned int iSize,bool saveCRLF,bool showAnswer);
    void _setLastError(unsigned int iErrorNum,char* iErrorText);
    void _emptyBuffer(char* oBuff,int iSize);
    void _sendTermCommand();
    bool _setSmsTextMode();
    int _freeRam();
    void _getSmsBody(char* vRes,char* vCommand);
    bool (*_onSmsRead)(byte iSms,char* oStr);
    void _doCmd(char* iStr);
    void _doCmd(const __FlashStringHelper *iStr);        
    void _setSizeParamError(unsigned int iErrorNum);
};

#endif
