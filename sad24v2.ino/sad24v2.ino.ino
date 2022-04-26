#include <avr/wdt.h>
#include <BMP085.h>
#include <DHT.h>
#include <EEPROM.h>
#include <TimerOne.h>
#include <LowPower.h>
#include <MemoryFree.h>
#include <gprs2.h>
#include <mstr.h>
#include <worker.h>
#include <debug.h>
#include <structs.h>

sensorInfo _sensorInfo;


long long mCurrTime, vPrevTime2;

volatile bool mCanGoSleep   = true;              

unsigned long mCurrP, 
              connectPeriod;



workerInfo _water;
workerInfo _light;


void(* resetFunc) (void) = 0;

/**********************************************************
   Геттеры и сеттеры.
 **********************************************************/


void setOffline(byte iDirection,int iLight, int iWater) {
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
  }
  

  noInterrupts();
  EEPROM.put(eeprom_mOfflineParamsStart, _offlineParams);
  interrupts();
}



unsigned long getConnectPeriod() {
  Globals _globals;
  EEPROM.get(eeprom_mGlobalsStart, _globals);
  return _globals.connectPeriod * 60UL;
}

void setConnectPeriod(int iSleepTime) {

  if (iSleepTime < 5 || iSleepTime > 600) {
    return;
  };

  Globals _globals;
  EEPROM.get(eeprom_mGlobalsStart, _globals);
  _globals.connectPeriod = iSleepTime;

  noInterrupts();
  EEPROM.put(eeprom_mGlobalsStart, _globals);
  interrupts();

}

/***********************************************************
   Конец геттеров и сеттеров
 ***********************************************************/

/***********************************************************
   Дополнительные методы работы.
 ***********************************************************/

bool isConnectInfoFull() {
  ApnCon _ApnCon;
  SiteCon _siteCon;
  EEPROM.get(0, _ApnCon);
  EEPROM.get(eeprom_mSiteCon,_siteCon);
  return strlen(_siteCon.sitePoint) > 0 && strlen(_ApnCon.apnPoint) > 0;
}


int getMiddleDistance() {
   int vMeasurement[ __DISTANCE_COUNT__],
       vTemp=0;
  
  for (byte i=0; i< __DISTANCE_COUNT__;i++) {
    vMeasurement[i]=getDistance(); 
    delay(10); //Делаем задержку иначе датчик показывает не верные данные
  };  
  //Теперь сортируем массив по возрастанию. По идее массив должен быть почти упорядоченным, поэтому использую алгоритм вставками

  for (byte i=1;i<__DISTANCE_COUNT__;i++) {
    vTemp=vMeasurement[i];
    byte j=i;
    while (j>0 and vMeasurement[j-1] > vTemp) {
      vMeasurement[j]=vMeasurement[j-1];
      j=j-1;
    };
    vMeasurement[j]=vTemp;    
  };


  vTemp=0;
  //Теперь отбрасываем "выскакивающие" значения и находим среднее
  for (byte i=2; i< __DISTANCE_COUNT__-2;i++) {
    vTemp+=vMeasurement[i];
  };

  return round(vTemp / (__DISTANCE_COUNT__-4));
  
}
int getDistance() {
  unsigned int duration, cm;  
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(55);
  digitalWrite(TRIG_PIN, LOW);
  duration = pulseIn(ECHO_PIN, HIGH);
  cm      = round(duration / 58);
  return cm;
}

void loadSensorInfo1() {
  
  {
    BMP085   dps = BMP085();
    dps.init(MODE_STANDARD, 17700, true);
    _sensorInfo.t2 = dps.getTemperature2();
    _sensorInfo.p1 = dps.getPressure2();
  };


  {
    DHT dht(5, DHT22);
    unsigned long vCurrTime = millis();
    dht.begin();

     /***********************************************************
      *    Датчик влажности тугой. Он может с первого           *
      *    раза данные не считать. Поэтому пытаемся             *
      *    его считать столько раз, сколько успеем за 5 секунд. *
      ***********************************************************/
    do {
      _sensorInfo.t1 = dht.readTemperature2();
      _sensorInfo.h1 = dht.readHumidity2();        
    } while (_sensorInfo.t1==0 && _sensorInfo.h1==0 && millis() - vCurrTime < 3000);
    
  };

  
  _sensorInfo.distance = getMiddleDistance();  
}



/**************************************************************
    Конец доп.методов
 **************************************************************/

/**************************************************************
   Методы работы с модемом
 **************************************************************/
void parseSmsParamCommand(char* iCommand) {
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

    EEPROM.get(0, _ApnCon);

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
    EEPROM.put(0, _ApnCon);
    interrupts();
  };

  strcpy_P(vTmpStr, PSTR("SITE"));
  if (_mstr.isEqual(vCmd, vTmpStr)) {
    SiteCon _siteCon;
    EEPROM.get(eeprom_mSiteCon,_siteCon);

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
    EEPROM.put(eeprom_mSiteCon, _siteCon);
    interrupts();

  };

}

bool onSms(byte iSms, char* iCommand) {
  mstr _mstr;
  char vTmpStr[2];

  strcpy_P(vTmpStr, PSTR(":"));

  if (_mstr.numEntries(iCommand, vTmpStr) > 3) {
    parseSmsParamCommand(iCommand);
  };

  return true;
}

void readSms2() {

  gprs2 sim900(7, 8);

  sim900.setOnSms(onSms);

  delay(500); //Не знаю почему, но без этой конструкции freemem показывает, что сожрано на 200байт больше памяти.
  sim900.readSms();
}

/**************************************************************
   Пробуждаем модем, так как это не всегда
   может получиться с первого раза делаем несколько попыток.
 **************************************************************/
bool wk() {

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

}

bool doInternet() {
  gprs2 sim900(7, 8);
 
  {
    ApnCon _apnCon;
    EEPROM.get(0, _apnCon);
    sim900.setInternetSettings(_apnCon.apnPoint, _apnCon.apnLogin, _apnCon.apnPass);
  };
 
  if (!sim900.doInternet()) {
      return false;
  };

  return true;

}


/************************************************************
   Отправляем модем в сон.
 ************************************************************/
void sl() {
  gprs2 sim900(7, 8);
  sim900.sleep();
}
bool isDisabledLightRange() {
  offlineParams _offlineParams;
  EEPROM.get(eeprom_mOfflineParamsStart, _offlineParams);
  return abs(_offlineParams.tempUpLight1) == 19900 && abs(_offlineParams.tempUpLight2) == 19900;
}
bool isDisabledWaterRange() {
  offlineParams _offlineParams;
  EEPROM.get(eeprom_mOfflineParamsStart, _offlineParams);
  return abs(_offlineParams.tempUpWater1) == 1990 && abs(_offlineParams.tempUpWater2) == 1990;
}
void restartModem() {
  gprs2 sim900(7, 8);
  sim900.hardRestart();
  delay(__WAIT_MODEM_TIME__);
};
/**************************************************************************
    Конец методов работы с модемом
 *************************************************************************/
bool canGoSleep() {
  /********************
   * Уходим в сон, при следующих условиях:
   *   1. Оба исполнителя (вода, свет) выключены;
   *   2. Режим мониторинга окружающей температуры выключен
   *   3. Время передачи информации не менее 15 минут.
   */
  return mCanGoSleep && isDisabledLightRange() && isDisabledWaterRange() && connectPeriod >= 900; 
}

byte goSleep(byte iMode, 
             long long iPrevTime
            ) {
  unsigned long  vSleepTime     = 0,
                 vCalcSleepTime = 0,
                 vPeriodSleep   = 0,
                 vCurrTime;

                 


  long  vNextModem   = 0;
  long long vTimeStamp;
  byte vDayOfWeek;

  {
    worker _worker(eeprom_mWorkerStart);
    vTimeStamp   = _worker.getTimestamp(vCurrTime,vDayOfWeek);
    vSleepTime   = _worker.getSleepTime(vDayOfWeek,vCurrTime);


  #ifdef IS_DEBUG_SLEEP
    Serial.print(F("M:"));
    Serial.print(vCurrTime);
    Serial.print(F(" S="));
    Serial.print(vSleepTime);
    Serial.print(F(" W="));
    Serial.print(vCurrTime+vSleepTime);
  #endif
    
  };

  vNextModem   = (long)(iPrevTime + connectPeriod - vTimeStamp);

  
  #ifdef IS_DEBUG_SLEEP
    Serial.print(F(" N="));
    Serial.println(vNextModem);
  #endif

  /***********************************************************************
   * Если по каким-то причинам было пропущено несколько подключений,     *
   * то принудительно подключаемся через 5 сек.                          *
   * ВНИМАНИЕ! 0 ставить нельзя, так как будет зависание.                *
   ***********************************************************************/
  vNextModem    = max(5, vNextModem);

  vSleepTime    = min(vNextModem, vSleepTime);

  if (vSleepTime<30) {
    return vSleepTime;    
  }

  vCalcSleepTime   = vSleepTime * iMode / 100U;

  /* Считаем кол-во циклов */
  vPeriodSleep = vCalcSleepTime / 8U;
  
  #ifdef IS_DEBUG_SLEEP
    Serial.print(F("S*"));
    Serial.print(iMode);
    Serial.print(F("%="));
    Serial.print(vCalcSleepTime);
    Serial.print(F("(s)"));
    Serial.print(F("P="));
    Serial.println(vPeriodSleep);
    Serial.flush();
  #endif

 

  for (unsigned int vJ = 0; vJ < vPeriodSleep; vJ++) {
      LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
  };

  return 0;

};


/*********************************************************
   Метод отправляет информацию об измерения с датчиков.
 *********************************************************/

bool updateRemoteMeasure(sensorInfo si) {
  char vParams[200],
       vRes[100],
       sitePoint[__SITE_POINT_SIZE__];
  bool vResult;



  gprs2 sim900(7, 8);

  
    {
      SiteCon _siteCon;
      EEPROM.get(eeprom_mSiteCon, _siteCon);

      sprintf_P(vParams, PSTR("r=%s&s=%s&t1=%d&h1=%d&t2=%d&p1=%ld&w1=%lu&l1=%lu&d=%lu&ds=%d"), _siteCon.siteLogin,
              _siteCon.sitePass,
              si.t1,
              si.h1,
              si.t2,
              si.p1,
              _water.duration,
              _light.duration,
              millis(),
              si.distance
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
}

bool beforeTaskUpdate(char* iStr) {
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
bool workWithRes(char* iRes) {
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
bool updateRemoteParams() {
  char vParams[70],
       sitePoint[__SITE_POINT_SIZE__];
  bool vShouldReconnect = true;


  gprs2 sim900(7, 8);
    {
      SiteCon _siteCon;  
      EEPROM.get(eeprom_mSiteCon, _siteCon);
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

void Timer1_doJob(void) {

#ifdef WDT_ENABLE
  wdt_reset();
#endif

  
  worker _worker(eeprom_mWorkerStart);
  offlineParams _offlineParams;
  unsigned long vSecMidnight = 0;
  byte vDayOfWeek;

  EEPROM.get(eeprom_mOfflineParamsStart, _offlineParams);

  mCurrTime = _worker.getTimestamp(vSecMidnight, vDayOfWeek);


  /*******************************************************************
     Если достигли минимального значения по температе,
     то включаем устройство "Свет". При этом отмечаем, что включение
     произошло по границе.
   *******************************************************************/

  if (!isDisabledLightRange() && (_sensorInfo.t1 < _offlineParams.tempUpLight1 || _light.isEdge)) {
    _light.isEdge     = true;
  };



  /******************************************************************
     Если достигли мимального значения по внутренней температуре,
     то включаем устройство "Вода". При этом отмечаем включение
     по температуре. Делаю эту тему по заказу Юрца Маленького.
   ******************************************************************/
  if (!isDisabledWaterRange() && (_sensorInfo.t2 < _offlineParams.tempUpWater1 || _water.isEdge)) {
    _water.isEdge     = true;
  };


  _light.isSchedule = false;
  _water.isSchedule = false;


  for (byte vI = 0; vI < _worker.maxTaskCount; vI++) {
    byte executor = _worker.shouldTaskWork2(vI, vSecMidnight, vDayOfWeek);

    _light.isSchedule = _light.isSchedule || (bool)bitRead(executor, 1);
    _water.isSchedule = _water.isSchedule || (bool)bitRead(executor, 0);

  };


 /********************
  * Отключаем исполнителя по границе в двух случаях: 
  *  1. достигли граничной температуры, 
  *  2. режим мониторинга температуры отключен
  ********************/
  if (_sensorInfo.t1 >= _offlineParams.tempUpLight2 || isDisabledLightRange()) {  
    _light.isEdge     = false;
  };


  if (_sensorInfo.t2 >= _offlineParams.tempUpWater2 || isDisabledWaterRange()) { 
    _water.isEdge     = false;
  };

 #ifdef IS_DEBUG

  Serial.print(F("VSM:"));
  Serial.print(vSecMidnight);
  Serial.print(F("   "));


  Serial.print(F("DOW:"));
  Serial.print(vDayOfWeek);
  Serial.print(F("   "));

  Serial.print(F("Mem:"));
  Serial.println(freeMemory());

 #endif


  /************************************************************************
                            Останавливаем исполнителей
   ************************************************************************/

  if (!_light.isSchedule && !_light.isEdge && _light.isWork) {

#if IS_DEBUG>3
    Serial.print(F("EL:"));
    Serial.println(vSecMidnight);
#endif
    _worker.stopLight();
    _light.isWork = false;
  };

  if (!_water.isSchedule && !_water.isEdge && _water.isWork) {

#if IS_DEBUG>3
    Serial.print(F("EW:"));
    Serial.println(vSecMidnight);
#endif
    _worker.stopWater();
    _water.isWork = false;
  };


  /************************************************************************
                            Стартуем исполнителей
   ************************************************************************/

  if ((_water.isSchedule || _water.isEdge) && !_water.isWork) {
#if IS_DEBUG>3
    Serial.print(F("SW:"));
    Serial.println(vSecMidnight);
#endif
    _worker.startWater();
    _water.isWork = true;
    _water.startTime = vSecMidnight;
  };

  if ((_light.isSchedule || _light.isEdge) && !_light.isWork) {

#if IS_DEBUG>3
    Serial.print(F("SL:"));
    Serial.println(vSecMidnight);
#endif
    _worker.startLight();
    _light.isWork = true;
    _light.startTime = vSecMidnight;
  };

  /**************************************************
     Если исполнитель работает, то увеличиваем
     время его работы.
   **************************************************/

  if (_water.isWork) {
    /**********************************************************************
        Если время включения больше, чем текущее время
        это означает, что либо мы перешагнули через
        полуночь либо перевелись часы. Соответсвенно устанавливаем
        время включения в 0. Исходим из того, что считаем время работы
        исполнителя в текущих сутках.
     **********************************************************************/
    if (_water.startTime > vSecMidnight) {
      _water.startTime = 0;
    };


    _water.duration = vSecMidnight - _water.startTime;
  };

  if (_light.isWork) {
    /**********************************************************************
        Если время включения больше, чем текущее время
        это означает, что либо мы перешагнули через
        полуночь либо перевелись часы. Соответсвенно устанавливаем
        время включения в 0. Исходим из того, что считаем время работы
        исполнителя в текущих сутках.
     **********************************************************************/
    if (_light.startTime > vSecMidnight) {
      _light.startTime = 0;
    };
    _light.duration = vSecMidnight - _light.startTime;
  };


  mCanGoSleep = !(_light.isWork || _water.isWork);
  
}

void pin_ISR () {
  /**********************************************
     Пришло прерывание по нажатию на кнопку.
     Обнуляем переменные соединения, выставляем
     флаг считывания СМС сообщений.
   **********************************************/  
  
  clearApn();

  clearSite();  

  #ifdef IS_DEBUG
    Serial.println(F("^"));
    Serial.flush();
  #endif  
  resetFunc();
}

void clearApn() {
      ApnCon _apnCon;
      EEPROM.get(0, _apnCon);      
      /*** APN INIT ***/
      strcpy_P(_apnCon.apnPoint, PSTR("\0"));
      strcpy_P(_apnCon.apnLogin, PSTR("\0"));
      strcpy_P(_apnCon.apnPass, PSTR("\0"));
      EEPROM.put(0, _apnCon);
}

void clearSite() {
      SiteCon _siteCon;
      EEPROM.get(eeprom_mSiteCon, _siteCon);      
      /*** SITE INIT ***/
      strcpy_P(_siteCon.sitePoint, PSTR("\0"));
      strcpy_P(_siteCon.siteLogin, PSTR("\0"));
      strcpy_P(_siteCon.sitePass, PSTR("\0"));
      EEPROM.put(eeprom_mSiteCon, _siteCon);
}



void setup() {

  Wire.begin(); // ВАЖНО!!! ЭТА КОМАНДА ДОЛЖНА БЫТЬ ДО ПЕРВОЙ РАБОТЫ С ЧАСАМИ

  #ifdef IS_DEBUG
    Serial.begin(19200);
  #endif  
  

  Globals _globals;
  worker _worker(eeprom_mWorkerStart);

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW

  _worker.stopWater();
  _worker.stopLight();


  EEPROM.get(eeprom_mGlobalsStart, _globals);


  if (strcmp_P(_globals.version, PSTR("INI")) != 0) {

    //Признак того, что инициализация выполнена
    strcpy_P(_globals.version, PSTR("INI"));

    // Задержка соединения по-умолчанию 15 минут
    _globals.connectPeriod = 15;
    EEPROM.put(eeprom_mGlobalsStart, _globals);

    clearApn();
    clearSite();    

    //Устанавливаем умолчательные значения для автономного режима
    setOffline(__WATER__,-199, 199);
    setOffline(__LIGHT__,-199, 199);
    
    _worker.setDateTime(16, 9, 13, 18, 45, 0);

  };
  
   connectPeriod=getConnectPeriod();
  /**************************************************************************
   *   Инициализруем переменную. Если этого не сделать,                     *
   *   то возможно переполнение при первом старке.                          *
   *   Дополнительно экономим                                               *
   *   память по глобальной переменной отображающей факт первого запуска.   *
   **************************************************************************/
  {
    worker _worker(eeprom_mWorkerStart);
    vPrevTime2 = _worker.getTimestamp() - connectPeriod;
  };
  

  loadSensorInfo1();

  Timer1.initialize(7000000);
  Timer1.attachInterrupt(Timer1_doJob);
  Timer1.start();

  #ifdef WDT_ENABLE
    wdt_enable (WDTO_8S);
  #endif

  attachInterrupt(0, pin_ISR, FALLING);  

}


void fillConnectInfo() {
    while (!isConnectInfoFull()) {
      /*************
       * Нет данных для подключения к сайту
       *************/           
           while (!wk()) {      
            restartModem();
           };

           Timer1.stop();
            readSms2();
           Timer1.start();           
     };       
}

void loop()
{
  unsigned int vDistance;
  long     int vD = (long)(mCurrTime - vPrevTime2),
  
      vTimeAfterLastMeasure = (long) (mCurrTime - _sensorInfo.lastMeasure); // mCurrTime берем из прерывания по будильнику

   /**************************************************
    * Этот блок должен быть выше остальных.          *
    * При нажатии на кнопку сброса пар-в индикация   *
    * должна сразу отображать этот режим.            *
    **************************************************/
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
     return; 
  };
  

      if (vTimeAfterLastMeasure > __MEASURE_PERIOD__) {

       Timer1.stop(); //Отключаем таймер, так как в функции loadSensorInfo1 есть критичный участок кода
       loadSensorInfo1();
       _sensorInfo.lastMeasure = mCurrTime;
       Timer1.start();     
            
     };



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
            vStatus = updateRemoteMeasure(_sensorInfo);
            vAttempt++;
          } while (!vStatus && vAttempt < 3);
        };


      };


      vPrevTime2 = mCurrTime;

      /*** Конец блока работы с модемом ***/    


     if (connectPeriod >= __MODEM_SLEEP_PERIOD__) {  // При этом модем уводим спать только в том случае если интервал соединения больше определенного времени
         sl();  //После того как модем передал данные уводим его в сон. Если это делать в основном теле функции, то отправлять будем 1 раз в секунду
     };
  };

 if (!canGoSleep()) {
    return;
  };


  Timer1.stop();
{
  byte waitTime=0;
 
     do {
        waitTime=goSleep(50, vPrevTime2);
     } while (waitTime==0);
 
     Timer1.start();
     delay(waitTime * 1000 + 2000);
 
}   

}
