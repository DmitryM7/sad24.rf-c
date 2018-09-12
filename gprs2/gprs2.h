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
    bool canInternet();                        

    bool getCoords(char* iLongitude,char* iLatitdue);

    void setInternetSettings(char* iApn,char* iLogin,char* iPass);
    bool postUrl(char* iUrl,char* iPar,char* oRes);
    bool postUrl(char* iUrl, char* iPar, char* oRes,unsigned int iResLength);


    bool saveOnSms();
    int getSmsCount();
    void getSmsText(unsigned int iNum,char* oRes,unsigned int iSmsSize);
    bool sendSms(char* iPhone,char* iText);
    bool readSms(bool deleteAfterRead=true);
    bool setOnSms(bool (*_mSmsEvent)(byte iSms, char* oStr));
    bool deleteAllSms();
    bool deleteSms(byte iSms);

    bool wakeUp();
    void sleep();
    void softRestart();

    void getLastError(char* oLastError);
    uint8_t getLastErrorNum();

  private:
    SoftwareSerial _modem;
//    char  _apn[35], _login[11], _pass[11];
    String _apn, _login, _pass,_lastError;
//    char  _lastError[20]; 
    uint8_t _lastErrorNum;
    bool   getAnswer3(char* oRes,size_t iSize);
    bool   _getAnswer3(char* oRes,size_t iSize,bool saveCRLF);
    bool   _getAnswer3(char* oRes,size_t iSize,bool saveCRLF,bool showAnswer);
    bool  _getAnswerWait(char* oRes,size_t iSize,char* iPattern,bool iSaveCRLF=false,bool iDebug=false);
    void _setLastError(unsigned int iErrorNum,char* iErrorText);
    void _emptyBuffer(char* oBuff,size_t iSize);
    void _sendTermCommand(bool iWaitOK=true);
    bool _setSmsTextMode();
    void _getSmsBody(char* vRes,char* oBody,unsigned int iMaxSmsBodyLength);
    bool (*_onSmsRead)(byte iSms,char* oStr);

    void _doCmd(char* iStr);
    void _doCmd(const __FlashStringHelper *iStr);        
    void _doCmd3(const __FlashStringHelper *iStr1,char* iStr2, const __FlashStringHelper *iStr3);        
    void _doCmd3(const __FlashStringHelper *iStr1,String iStr2, const __FlashStringHelper *iStr3);        
};

#endif
