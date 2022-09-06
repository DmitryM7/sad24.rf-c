#ifndef sim800_h
#define sim800_h
#include "SoftwareSerial.h"

const char OK_M[]    PROGMEM = "OK";
const char SPACE_M[] PROGMEM = " ";
const char COMMA_M[] PROGMEM = ",";
const char POINT_M[] PROGMEM = ".";

class sim800
{


  public:
    /********************************
     * For common use.              *
     ********************************/
    sim800(int pin1,int pin2);
    ~sim800();

    bool isPowerUp();	                   //Возвращает истину если устройство включено.
    bool hasNetwork();                     //Возвращает истину если устройство ловит сеть
    bool isConnect();                      //Возвращает истину если устройство зарегистрировано в сотовой сети

    bool hasGprsNetwork();                 //Возвращает истину если устройство подключено к GPRS сети
    bool gprsNetworkUp(bool iForce=false); // Запускает GPRS соединение
    bool gprsNetworkDown();

    bool hasGprs();                        //Возвращает истину если поднято GPRS соединение
    bool gprsUp(bool iForce=false);        // Поднимает GPRS соединение
    bool gprsDown();



    bool wakeUp();
    void sleep();
    bool softRestart();
    void hardRestart();

    void getLastError(char* oLastError);

    bool canWork();                        //Определяет может ли модем установить соединение, при этом пытается поднять несущую и соединение GPRS
    bool doInternet();

    void setInternetSettings(char* iApn,char* iLogin,char* iPass);

    bool postUrl(char* iUrl,char* iPar,char* oRes);
    bool postUrl(char* iUrl, char* iPar, char* oRes,unsigned int iResLength);


  protected:
    SoftwareSerial _modem;
    void _doCmd(char* iStr);
    void _doCmd(const __FlashStringHelper *iStr);
    bool _getAnswerWait(char* oRes,size_t iSize,char* iPattern,bool iSaveCRLF=false,unsigned long iTimeOut=1000);
    void _setLastError(unsigned int iErrorNum,char* iErrorText);
    uint8_t getLastErrorNum();
    void _sendTermCommand(bool iWaitOK=true);

  private:
    String _apn, _login, _pass,_lastError;
    uint8_t _lastErrorNum;


    bool  getAnswer3(char* oRes,size_t iSize);
    bool _getAnswer3(char* oRes,size_t iSize,bool saveCRLF,unsigned long iTimeOut=1000);
    bool _getAnswerWaitLong(char* oRes,size_t iSize,char* iNeedStr,unsigned long iTimeOut=1000);
    void _emptyBuffer(char* oBuff,size_t iSize);

    bool _setOnSmsMode();
    bool _clearSmsBody(char* iRes,unsigned int iSize);

    bool (*_onSmsRead)(byte iSms,char* oStr);
    bool (*_onBeforePostParams)(char* iStr);
    bool (*_onAfterPostParams)();

};

#endif
