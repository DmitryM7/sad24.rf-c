#include <Arduino.h>
#include <stdSensorInfoLoader.h>
#include <gsmTransport.h>
#include <sim868.h>
#include <structs.h>
#include <EEPROM.h>
#include <mstr.h>


/******************************************
 *        PRIVATE FUNCTION                *
 * Функции нужны чтобы не морочиться      *
 * с созданием делегатов, которых сделать *
 * на этом языке весьма не тривиально     *
 ******************************************/

unsigned long _getConnectPeriod() {
  SiteCon _siteCon;
  EEPROM.get(eeprom_mSiteStart, _siteCon);
  return _siteCon.connectPeriod * 60UL;
}

void _setConnectPeriod(int iSleepTime) {

  if (iSleepTime < 5 || iSleepTime > 600) {
    return;
  };

  SiteCon _siteCon;

  EEPROM.get(eeprom_mSiteStart, _siteCon);

  _siteCon.connectPeriod = iSleepTime;

  noInterrupts();
  EEPROM.put(eeprom_mSiteStart, _siteCon);
  interrupts();

};

bool _beforeTaskUpdate(char* iStr) {
  char vDelimiter[2],
       vCommand[2],
       vParam1[20],
       vParam2[20];

  mstr _mstr;

  strcpy_P(vDelimiter, PSTR(";"));

  if (_mstr.numEntries(iStr, vDelimiter) < 2) {
    return false;
  };

  if (!_mstr.entry(1, iStr, vDelimiter, vCommand)) {
    return false;
  };

  if (strcmp_P(vCommand, PSTR("C")) == 0) {
    if (_mstr.entry(2, iStr, vDelimiter, 4, vParam1)) {
      _setConnectPeriod(atoi(vParam1));
      return false;
    };
  };

  if (strcmp_P(vCommand, PSTR("G")) == 0) {
    if (_mstr.entry(2, iStr, vDelimiter, 4, vParam1) && _mstr.entry(3, iStr, vDelimiter, 4, vParam2)) {
#if IS_DEBUG>2
      Serial.print(F("L off:"));
      Serial.print(vParam1);
      Serial.print(F("&"));
      Serial.println(vParam2);
      Serial.flush();
#endif
      _setOffline(__LIGHT__,atoi(vParam1), atoi(vParam2));
      return false;
    };
  };

  if (strcmp_P(vCommand, PSTR("J")) == 0) {
    if (_mstr.entry(2, iStr, vDelimiter, 4, vParam1) && _mstr.entry(3, iStr, vDelimiter, 4, vParam2)) {

       #if IS_DEBUG>2
                 Serial.print(F("W off:"));
                 Serial.print(vParam1);
                 Serial.print(F("&"));
                 Serial.println(vParam2);
                 Serial.flush();
       #endif

      _setOffline(__WATER__,atoi(vParam1), atoi(vParam2));
      return false;
    };
  };

  return true;
};

void _parseSmsParamCommand(char* iCommand) {
  char vTmpStr[5],
       vCmd[10];
  mstr _mstr;

  strcpy_P(vTmpStr, PSTR(":"));
  _mstr.entry(1, iCommand, vTmpStr, 10, vCmd);

  /**********************************
     Меняем учетные данные к apn
   **********************************/

  strcpy_P(vTmpStr, PSTR("APN7"));

  if (_mstr.isEqual(vCmd, vTmpStr)) {

    ApnCon _ApnCon;

    EEPROM.get(eeprom_mApnStart, _ApnCon);

    strcpy_P(vTmpStr, PSTR(":"));
    if (!_mstr.entry(2, iCommand, vTmpStr, 35, _ApnCon.apnPoint)) {
      return;
    };

    if (!_mstr.entry(3, iCommand, vTmpStr, 11, _ApnCon.apnLogin)) {
      return;
    };

    if (!_mstr.entry(4, iCommand, vTmpStr, 11, _ApnCon.apnPass)) {
      return;
    };

    #if IS_DEBUG>1
        Serial.print(F("A: "));
        Serial.print(_ApnCon.apnLogin);
        Serial.print(F(":"));
        Serial.print(_ApnCon.apnPass);
        Serial.print(F("@"));
        Serial.println(_ApnCon.apnPoint);
        Serial.flush();
    #endif

    noInterrupts();
    EEPROM.put(eeprom_mApnStart, _ApnCon);
    interrupts();
  };

  strcpy_P(vTmpStr, PSTR("SITE"));
  if (_mstr.isEqual(vCmd, vTmpStr)) {
    SiteCon _siteCon;
    EEPROM.get(eeprom_mSiteStart,_siteCon);

    strcpy_P(vTmpStr, PSTR(":"));

    if (!_mstr.entry(2, iCommand, vTmpStr, sizeof(_siteCon.siteLogin), _siteCon.siteLogin)) {
      return;
    };

    if (!_mstr.entry(3, iCommand, vTmpStr, sizeof(_siteCon.sitePass), _siteCon.sitePass)) {
      return;
    };

    if (!_mstr.entry(4, iCommand, vTmpStr, sizeof(_siteCon.sitePoint), _siteCon.sitePoint)) {
      return;
    };

    #if IS_DEBUG>1
      Serial.print(F("S: "));
      Serial.print(_siteCon.siteLogin);
      Serial.print(F(":"));
      Serial.print(_siteCon.sitePass);
      Serial.print(F("@"));
      Serial.println(_siteCon.sitePoint);
      Serial.flush();
    #endif

    noInterrupts();
    EEPROM.put(eeprom_mSiteStart, _siteCon);
    interrupts();

    };
  };



bool _onSms(byte iSms, char* iCommand) {
  mstr _mstr;
  char vTmpStr[2];

  strcpy_P(vTmpStr, PSTR(":"));

  if (_mstr.numEntries(iCommand, vTmpStr) > 3) {
    _parseSmsParamCommand(iCommand);
  };

  return true;
};

/********************************************
 *           END PRIVATE FUNCTION           *
 ********************************************/


void gsmTransport::readSms2() {

  sim868 modem(7, 8);

  modem.setOnSms(_onSms);

  delay(500); //Не знаю почему, но без этой конструкции freemem показывает, что сожрано на 200байт больше памяти.
  modem.readSms();
}

bool gsmTransport::wk() {

  bool vStatus = false;
  byte vAttempt = 1;

  sim868 modem(7, 8);

  modem.wakeUp();
  delay(__WAIT_MODEM_TIME__);

  do {
    vStatus = modem.canWork();
    delay(vAttempt * 3000);
    vAttempt++;
  } while (!vStatus && vAttempt < 4);

  if (!vStatus) {
    return false;
  };

  return true;

};

void gsmTransport::sl() {
  sim868 modem(7, 8);
  modem.sleep();
};

void gsmTransport::restartModem() {
     sim868 modem(7, 8);
     modem.hardRestart();
     delay(__WAIT_MODEM_TIME__);
};

void gsmTransport::fillConnectInfo() {
    while (!isConnectInfoFull()) {
      /*************
       * Нет данных для подключения к сайту
       *************/
           while (!wk()) {
            restartModem();
           };
           readSms2();
     };
};

bool gsmTransport::doInternet() {
  sim868 modem(7, 8);

  {
    ApnCon _apnCon;
    EEPROM.get(eeprom_mApnStart, _apnCon);
    modem.setInternetSettings(_apnCon.apnPoint, _apnCon.apnLogin, _apnCon.apnPass);
  };

  if (!modem.doInternet()) {
      return false;
  };

  return true;

};



bool gsmTransport::isConnectInfoFull() {
  ApnCon _ApnCon;
  SiteCon _siteCon;
  EEPROM.get(eeprom_mApnStart, _ApnCon);
  EEPROM.get(eeprom_mSiteStart,_siteCon);
  return strlen(_siteCon.sitePoint) > 0 && strlen(_ApnCon.apnPoint) > 0;
};




/**********************************************************
   Если требуется повторное соединение, то
   функция возвращает True. Если подсоединения не
   требуется, то возвращается False.
 **********************************************************/
bool gsmTransport::workWithRes(char* iRes) {
  char vTmpStr[2];
  mstr _mstr;

  worker _worker(eeprom_mWorkerStart);
  _worker.setBeforeTaskUpdate(_beforeTaskUpdate);



  strcpy_P(vTmpStr, PSTR("+"));
  if (_mstr.begins(iRes, vTmpStr)) {
    _mstr.trim(iRes, vTmpStr);
    return _worker.update(iRes);
  };

  return 0;

}

/**********************************************************
   Метод отправляет информацию о текущих координатах и
   загружает новые параметры устройства.
 **********************************************************/
bool gsmTransport::updateRemoteParams() {
  char vParams[100],
       sitePoint[__SITE_POINT_SIZE__];
  bool vShouldReconnect = true;
  sim868_coords coords;



  sim868 modem(7, 8);
    {
      SiteCon _siteCon;
      EEPROM.get(eeprom_mSiteStart, _siteCon);

       if (_needTransferGps) { 
          modem.getCoords(coords);
       };

      sprintf_P(vParams, PSTR("r=%s&s=%s&m=%ld&a=%ld&l=%ld&cc=%u"),
                               _siteCon.siteLogin,
                               _siteCon.sitePass,
                               _measurementId,
                               coords.a,
                               coords.l,
                               _connectCount);
      #if IS_DEBUG>0 
          Serial.println(vParams);
      #endif
      strncpy(sitePoint, _siteCon.sitePoint, sizeof(sitePoint));
    };



  while (vShouldReconnect) {
    char vRes[250];
    bool vResult;

    vResult = modem.postUrl(sitePoint, vParams, vRes, sizeof(vRes));
#ifdef IS_DEBUG
    Serial.print(F("P"));
    if (vResult) {
      Serial.print(F(":"));
      Serial.println(vRes);
    } else {
      char vError[20];
      modem.getLastError(vError);
      Serial.print(F("E:"));
      Serial.println(vError);
    };
    Serial.flush();
#endif

    if (!vResult) {
      return false;
    };

    vShouldReconnect = workWithRes(vRes);

  };

  return true;

}

long long gsmTransport::makeCommunicationSession(long long mCurrTime,long long vPrevTime2,stdSensorInfoLoader& si,workerInfo &_water,workerInfo &_light) {

  long int vD = (long)(mCurrTime - vPrevTime2);


  if (vD >= getConnectPeriod()) {

     /*** Начало блока работы с модемом ***/

      bool isModemWork = wk() && doInternet();


      if (!isModemWork) {
        restartModem();
        isModemWork = wk() && doInternet();
      };


      if (isModemWork) {
        byte vAttempt = 0;
        bool vStatus  = false;

       do {
          vStatus = updateRemoteMeasure(si,_water,_light);
          vAttempt++;
        } while (!vStatus && vAttempt < 3);


        if (vStatus) {
          //Если предыдущее подключение завершилось успешно, то пробуем передать другие данные.
          //Если же на предыдущем шаге не получилось, то идем спать.
          vStatus  = false;
          vAttempt = 0;

          incConnectCount(); //Увеличиваем счетчик подключений

         do {
            vStatus = updateRemoteParams();
            vAttempt++;
          } while (!vStatus && vAttempt < 3);
        };


      };


      /*** Конец блока работы с модемом ***/


     if (getConnectPeriod() >= __MODEM_SLEEP_PERIOD__) {  // При этом модем уводим спать только в том случае если интервал соединения больше определенного времени
         sl();
     };

     return mCurrTime;
  };

  return vPrevTime2;

};

bool gsmTransport::updateRemoteMeasure(stdSensorInfoLoader& si,
                                       workerInfo &_water,
                                       workerInfo &_light) {
  char vParams[200],
       vRes[100],
       sitePoint[__SITE_POINT_SIZE__];
  bool vResult;



  sim868 modem(7, 8);




    {
      SiteCon _siteCon;
      EEPROM.get(eeprom_mSiteStart, _siteCon);

      sprintf_P(vParams,
                PSTR("r=%s&s=%s&t1=%d&h1=%d&t2=%d&p1=%ld&w1=%lu&l1=%lu&d=%lu&ds=%d"),
                _siteCon.siteLogin,
                _siteCon.sitePass,
                si.getT1(),
                si.getH1(),
                si.getT2(),
                si.getP1(),
                _water.duration,
                _light.duration,
                millis(),
                si.getDistance()
               );

       strncpy(sitePoint, _siteCon.sitePoint, sizeof(sitePoint));
    };

    _measurementId = 0;

    vResult = modem.postUrl(sitePoint, vParams, vRes, sizeof(vRes));

#ifdef IS_DEBUG
  Serial.print(F("M"));
#endif

  if (vResult) {
    /**************************************************************************
        Если исполнители закончили свою работу и модуль передал информацию,
        то тогда обнуляем время работы.
     **************************************************************************/

#ifdef IS_DEBUG
    Serial.print(F(":"));
    Serial.println(vRes);
    Serial.flush();
#endif


    if (!_water.isWork) {
      _water.duration = 0;
    };

    if (!_light.isWork) {
      _light.duration = 0;
    };

    _measurementId=atol(vRes);


  } else {
      #ifdef IS_DEBUG
        char vError[20];
        modem.getLastError(vError);
        Serial.print(F("E:"));
        Serial.println(vError);
        Serial.flush();
     #endif
  };
  return vResult;
};

/*********************************************************
   Метод отправляет информацию об измерения с датчиков.
 *********************************************************/

void gsmTransport::checkCommunicationSession() {

   if (!isConnectInfoFull()) {

     /***************************************************************
      * Реквизиты доступа пустые. Переходим в режим ожидания СМС    *
      * с пар-ми доступ.                                            *
      * Если модем не готов, то будем его перезагружать его до тех  *
      * пор пока он не станет готовым.                              *
      ****************************************************************/
     digitalWrite(LED_BUILTIN, HIGH);
     fillConnectInfo();
     digitalWrite(LED_BUILTIN, LOW);
  };
};



unsigned long gsmTransport::getConnectPeriod() {
  return _getConnectPeriod();
}

void gsmTransport::setOffline(byte iDirection,int iLight, int iWater) {
  _setOffline(iDirection,iLight,iWater);
}

void gsmTransport::clearApn() {
      ApnCon _apnCon;
      /*** APN INIT ***/
      strcpy_P(_apnCon.apnPoint, PSTR("\0"));
      strcpy_P(_apnCon.apnLogin, PSTR("\0"));
      strcpy_P(_apnCon.apnPass, PSTR("\0"));
      EEPROM.put(eeprom_mApnStart, _apnCon);
}

void gsmTransport::clearSite() {
      SiteCon _siteCon;
      // Очищаем инфо о подключении
      strcpy_P(_siteCon.sitePoint, PSTR("\0"));
      strcpy_P(_siteCon.siteLogin, PSTR("\0"));
      strcpy_P(_siteCon.sitePass, PSTR("\0"));
      // Устанавливаем интервал подключения
     _siteCon.connectPeriod=15;

      EEPROM.put(eeprom_mSiteStart, _siteCon);
}

void gsmTransport::clearConfig() {
   clearApn();
   clearSite();
}

long long gsmTransport::calcFirstConnectPeriod(long long vCurrTime) {
  return vCurrTime - getConnectPeriod();
}

void gsmTransport::setConnectPeriod(int iSleepTime) {
  _setConnectPeriod(iSleepTime);
};

void gsmTransport::externalKeyPress(long long vCurrTime) {
   clearConfig();
}

void gsmTransport::incConnectCount() {
  _connectCount++;
}
void gsmTransport::toogleTransferGps() {
  _needTransferGps=!_needTransferGps;
}