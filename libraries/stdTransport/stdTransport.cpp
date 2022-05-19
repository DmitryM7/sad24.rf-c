#include <Arduino.h>
#include <stdSensorInfoLoader.h>
#include <stdTransport.h>
#include <gprs2.h>
#include <structs.h>
#include <EEPROM.h>
#include <mstr.h>

bool stdTransport::wk() {

  bool vStatus = false;
  byte vAttempt = 0;

  gprs2 sim900(7, 8);

  sim900.wakeUp();
  delay(__WAIT_MODEM_TIME__);

  do {
    vStatus = sim900.canWork();
    delay(vAttempt * 3000);
    vAttempt++;
  } while (!vStatus && vAttempt < 3);

  if (!vStatus) {
    return false;
  };

  return true;

};

void stdTransport::sl() {
  gprs2 sim900(7, 8);
  sim900.sleep();
};

void stdTransport::restartModem() {
     gprs2 sim900(7, 8);
     sim900.hardRestart();
     delay(__WAIT_MODEM_TIME__);
};

void stdTransport::fillConnectInfo() {
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

bool stdTransport::doInternet() {
  gprs2 sim900(7, 8);

  {
    ApnCon _apnCon;
    EEPROM.get(eeprom_mApnStart, _apnCon);
    sim900.setInternetSettings(_apnCon.apnPoint, _apnCon.apnLogin, _apnCon.apnPass);
  };

  if (!sim900.doInternet()) {
      return false;
  };

  return true;

};


void stdTransport::parseSmsParamCommand(char* iCommand) {
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


bool stdTransport::isConnectInfoFull() {
  ApnCon _ApnCon;
  SiteCon _siteCon;
  EEPROM.get(eeprom_mApnStart, _ApnCon);
  EEPROM.get(eeprom_mSiteStart,_siteCon);
  return strlen(_siteCon.sitePoint) > 0 && strlen(_ApnCon.apnPoint) > 0;
};

unsigned long stdTransport::getConnectPeriod() {
  SiteCon _siteCon;
  EEPROM.get(eeprom_mSiteStart, _siteCon);
  return _siteCon.connectPeriod * 60UL;
}

void stdTransport::setConnectPeriod(int iSleepTime) {

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

/********** Это нужно убрать отсюда ************/

void stdTransport::setOffline(byte iDirection,int iLight, int iWater) {
  offlineParams _offlineParams;


  if (abs(iLight) > 199 || abs(iWater) > 199) {
    return;
  };

  EEPROM.get(eeprom_mOfflineParamsStart, _offlineParams);

  switch (iDirection) {
    case __LIGHT__:
      _offlineParams.tempUpLight1 = iLight * 100;
      _offlineParams.tempUpLight2 = iWater * 100;
    break;
    case __WATER__:
      _offlineParams.tempUpWater1 = iLight * 10;
      _offlineParams.tempUpWater2 = iWater * 10;
    break;
  };


  noInterrupts();
  EEPROM.put(eeprom_mOfflineParamsStart, _offlineParams);
  interrupts();

 };

/******** Конец, что нужно удалять ****/

bool stdTransport::beforeTaskUpdate(char* iStr) {
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
      setConnectPeriod(atoi(vParam1));
      connectPeriod=getConnectPeriod();

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
      setOffline(__LIGHT__,atoi(vParam1), atoi(vParam2));
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
      setOffline(__WATER__,atoi(vParam1), atoi(vParam2));
      return false;
    };
  };

  return true;
};




/**********************************************************
   Если требуется повторное соединение, то
   функция возвращает True. Если подсоединения не
   требуется, то возвращается False.
 **********************************************************/
bool stdTransport::workWithRes(char* iRes) {
  char vTmpStr[2];
  mstr _mstr;

  worker _worker(eeprom_mWorkerStart);
  _worker.setBeforeTaskUpdate(beforeTaskUpdate);



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
bool stdTransport::updateRemoteParams() {
  char vParams[70],
       sitePoint[__SITE_POINT_SIZE__];
  bool vShouldReconnect = true;


  gprs2 sim900(7, 8);
    {
      SiteCon _siteCon;
      EEPROM.get(eeprom_mSiteStart, _siteCon);
      sprintf_P(vParams, PSTR("r=%s&s=%s&m=c&l=%u&a=%u"), _siteCon.siteLogin, _siteCon.sitePass, 0, 0);
      strncpy(sitePoint, _siteCon.sitePoint, sizeof(sitePoint));
    };



  while (vShouldReconnect) {
    char vRes[250];
    bool vResult;

    vResult = sim900.postUrl(sitePoint, vParams, vRes, sizeof(vRes));
#ifdef IS_DEBUG
    Serial.print(F("P"));
    if (vResult) {
      Serial.print(F(":"));
      Serial.println(vRes);
    } else {
      char vError[20];
      sim900.getLastError(vError);
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

void stdTransport::makeCommunicationSession(long long mCurrTime,long long vPrevTime2,stdSensorInfoLoader& si,workerInfo &_water,workerInfo &_light) {

  long     int vD = (long)(mCurrTime - vPrevTime2);

  if (vD >= connectPeriod) {

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
          vStatus = updateRemoteParams();
          vAttempt++;
        } while (!vStatus && vAttempt < 3);


        if (vStatus) {
          //Если предыдущее подключение завершилось успешно, то пробуем передать другие данные.
          //Если же на предыдущем шаге не получилось, то идем спать.
          vStatus  = false;
          vAttempt = 0;
          do {
            vStatus = updateRemoteMeasure(si,_water,_light);
            vAttempt++;
          } while (!vStatus && vAttempt < 3);
        };


      };


      vPrevTime2 = mCurrTime;

      /*** Конец блока работы с модемом ***/


     if (connectPeriod >= __MODEM_SLEEP_PERIOD__) {  // При этом модем уводим спать только в том случае если интервал соединения больше определенного времени
         sl();
     };
  };

};

bool stdTransport::updateRemoteMeasure(stdSensorInfoLoader& si,workerInfo &_water,workerInfo &_light) {
  char vParams[200],
       vRes[100],
       sitePoint[__SITE_POINT_SIZE__];
  bool vResult;



  gprs2 sim900(7, 8);


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

    vResult = sim900.postUrl(sitePoint, vParams, vRes, sizeof(vRes));

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


  } else {
      #ifdef IS_DEBUG
        char vError[20];
        sim900.getLastError(vError);
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

void stdTransport::checkCommunicationSession() {

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


void stdTransport::readSms2() {

  gprs2 sim900(7, 8);

  sim900.setOnSms(onSms);

  delay(500); //Не знаю почему, но без этой конструкции freemem показывает, что сожрано на 200байт больше памяти.
  sim900.readSms();
}

bool stdTransport::onSms(byte iSms, char* iCommand) {
  mstr _mstr;
  char vTmpStr[2];

  strcpy_P(vTmpStr, PSTR(":"));

  if (_mstr.numEntries(iCommand, vTmpStr) > 3) {
    parseSmsParamCommand(iCommand);
  };

  return true;
};
