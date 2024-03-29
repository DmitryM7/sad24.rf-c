#ifndef gprs2_h
#define gprs2_h
#include "SoftwareSerial.h"

class gprs2
{

  //Revision:1418B05SIM800L24

  public:
    /********************************
     * For common use.              *
     ********************************/
    gprs2(int pin1,int pin2);
    ~gprs2();


    bool isPowerUp();	                   //���������� ������ ���� ���������� ��������.
    bool hasNetwork();                     //���������� ������ ���� ���������� ����� ����
    bool isConnect();                      //���������� ������ ���� ���������� ���������������� � ������� ����


    bool hasGprsNetwork();                 //���������� ������ ���� ���������� ���������� � GPRS ����    
    bool gprsNetworkUp(bool iForce=false); // ��������� GPRS ����������
    bool gprsNetworkDown();

    bool hasGprs();                        //���������� ������ ���� ������� GPRS ����������
    bool gprsUp(bool iForce=false);        // ��������� GPRS ����������
    bool gprsDown();


    bool canWork();                        //���������� ����� �� ����� ���������� ����������, ��� ���� �������� ������� ������� � ���������� GPRS
    bool doInternet();                        

    bool getCoords(char* iLongitude,char* iLatitdue,char* oRes,size_t iSize);

    void setInternetSettings(char* iApn,char* iLogin,char* iPass);
    bool postUrl(char* iUrl,char* iPar,char* oRes);
    bool postUrl(char* iUrl, char* iPar, char* oRes,unsigned int iResLength);


    void getSmsText(unsigned int iNum,char* oRes,unsigned int iSmsSize);
    bool sendSms(char* iPhone,char* iText);
    bool readSms(bool deleteAfterRead=true);

    bool setOnSms(bool (*iSmsEvent)(byte iSms, char* oStr));
    bool setBeforePostParams(bool (*ifunction)(char* iStr));
    bool setAfterPostParams(bool (*ifunction)());

    bool deleteAllSms();
    bool deleteSms(byte iSms);

    bool wakeUp();
    void sleep();
    bool softRestart();
    void hardRestart();

    void getLastError(char* oLastError);
    uint8_t getLastErrorNum();

  private:
    SoftwareSerial _modem;
    String _apn, _login, _pass,_lastError;
    uint8_t _lastErrorNum;
    bool  getAnswer3(char* oRes,size_t iSize);
    bool _getAnswer3(char* oRes,size_t iSize,bool saveCRLF,unsigned long iTimeOut=1000);
    bool _getAnswerWait(char* oRes,size_t iSize,char* iPattern,bool iSaveCRLF=false,unsigned long iTimeOut=1000);
    bool _getAnswerWaitLong(char* oRes,size_t iSize,char* iNeedStr,unsigned long iTimeOut=1000);
    void _setLastError(unsigned int iErrorNum,char* iErrorText);
    void _emptyBuffer(char* oBuff,size_t iSize);
    void _sendTermCommand(bool iWaitOK=true);
    bool _setSmsTextMode();
    bool _setOnSmsMode();

    bool (*_onSmsRead)(byte iSms,char* oStr);
    bool (*_onBeforePostParams)(char* iStr);
    bool (*_onAfterPostParams)();

    void _doCmd(char* iStr);
    void _doCmd(const __FlashStringHelper *iStr);        
    void _doCmd3(const __FlashStringHelper *iStr1,char* iStr2, const __FlashStringHelper *iStr3);        
    void _doCmd3(const __FlashStringHelper *iStr1,String iStr2, const __FlashStringHelper *iStr3);        
    bool _clearSmsBody(char* iRes,unsigned int iSize);
};

#endif
