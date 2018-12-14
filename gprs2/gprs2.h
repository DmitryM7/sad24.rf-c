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


    bool isPowerUp();	                   //Возвращает истину если устройство включено.
    bool hasNetwork();                     //Возвращает истину если устройство ловит сеть
    bool isConnect();                      //Возвращает истину если устройство зарегистрировано в сотовой сети


    bool hasGprsNetwork();                 //Возвращает истину если устройство подключено к GPRS сети    
    bool gprsNetworkUp(bool iForce=false); // Запускает GPRS соединение
    bool gprsNetworkDown();

    bool hasGprs();                        //Возвращает истину если поднято GPRS соединение
    bool gprsUp(bool iForce=false);        // Поднимает GPRS соединение
    bool gprsDown();


    bool canWork();                        //Определяет может ли модем установить соединение, при этом пытается поднять несущую и соединение GPRS
    bool canInternet();                        

    bool getCoords(char* iLongitude,char* iLatitdue);

    void setInternetSettings(char* iApn,char* iLogin,char* iPass);
    bool postUrl(char* iUrl,char* iPar,char* oRes);
    bool postUrl(char* iUrl, char* iPar, char* oRes,unsigned int iResLength);


    bool saveOnSms();
    void getSmsText(unsigned int iNum,char* oRes,unsigned int iSmsSize);
    bool sendSms(char* iPhone,char* iText);
    bool readSms(bool deleteAfterRead=true);
    bool setOnSms(bool (*iSmsEvent)(byte iSms, char* oStr));
    bool deleteAllSms();
    bool deleteSms(byte iSms);

    bool wakeUp();
    void sleep();
    void softRestart();
    void hardRestart();

    void getLastError(char* oLastError);
    uint8_t getLastErrorNum();

  private:
    SoftwareSerial _modem;
    String _apn, _login, _pass,_lastError;
    uint8_t _lastErrorNum;
    bool   getAnswer3(char* oRes,size_t iSize);
    bool   _getAnswer3(char* oRes,size_t iSize,bool saveCRLF);
    bool   _getAnswer3(char* oRes,size_t iSize,bool saveCRLF,bool showAnswer);
    bool  _getAnswerWait(char* oRes,size_t iSize,char* iPattern,bool iSaveCRLF=false,bool iDebug=false);
    void _setLastError(unsigned int iErrorNum,char* iErrorText);
    void _emptyBuffer(char* oBuff,size_t iSize);
    void _sendTermCommand(bool iWaitOK=true);
    bool _setSmsTextMode();
    bool (*_onSmsRead)(byte iSms,char* oStr);

    void _doCmd(char* iStr);
    void _doCmd(const __FlashStringHelper *iStr);        
    void _doCmd3(const __FlashStringHelper *iStr1,char* iStr2, const __FlashStringHelper *iStr3);        
    void _doCmd3(const __FlashStringHelper *iStr1,String iStr2, const __FlashStringHelper *iStr3);        
    bool _clearSmsBody(char* iRes,unsigned int iSize);
};

#endif
