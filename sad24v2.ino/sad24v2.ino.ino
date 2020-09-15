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

#define eeprom_mGlobalsStart sizeof(Connection)
#define eeprom_mOfflineParamsStart sizeof(Connection)+sizeof(Globals)
#define eeprom_mWorkerStart eeprom_mOfflineParamsStart + sizeof(offlineParams)


long long mCurrTime,vPrevTime2;

volatile bool mCanGoSleep   = true,
              mShouldReadSms = false;

unsigned long mCurrP;

    

workerInfo _water;
workerInfo _light;


/**********************************************************
   Геттеры и сеттеры.
 **********************************************************/


void setOfflineLight(int iLight, int iWater) {
  offlineParams _offlineParams;


  if (abs(iLight) > 199 || abs(iWater) > 199) {
    return;
  };

  EEPROM.get(eeprom_mOfflineParamsStart, _offlineParams);
  _offlineParams.tempUpLight1 = iLight * 100;
  _offlineParams.tempUpLight2 = iWater * 100;

  noInterrupts();
  EEPROM.put(eeprom_mOfflineParamsStart, _offlineParams);
  interrupts();
}

void setOfflineWater(int iLight, int iWater) {
  offlineParams _offlineParams;


if (abs(iLight) > 199 || abs(iWater) > 199) {
    return;
  };

  EEPROM.get(eeprom_mOfflineParamsStart, _offlineParams);
  _offlineParams.tempUpWater1 = iLight * 10;
  _offlineParams.tempUpWater2 = iWater * 10;

  noInterrupts();
  EEPROM.put(eeprom_mOfflineParamsStart, _offlineParams);
  interrupts();
}


unsigned long connectPeriod() {
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
  Connection _connection;
  EEPROM.get(0, _connection);  
  return strlen(_connection.sitePoint)>0 && strlen(_connection.apnPoint)>0;
}



int getDistance() {
    int duration, cm;  
    pinMode(TRIG_PIN, OUTPUT); 
    pinMode(ECHO_PIN, INPUT);    
    digitalWrite(TRIG_PIN, LOW); 
    delayMicroseconds(2); 
    digitalWrite(TRIG_PIN, HIGH); 
    delayMicroseconds(55); 
    digitalWrite(TRIG_PIN, LOW); 
    duration = pulseIn(ECHO_PIN, HIGH); 
    cm = round(duration / 58);    
    return cm;
}

void loadSensorInfo1() {
  unsigned long vCurrTime;
   
  {
    BMP085   dps = BMP085();
    dps.init(MODE_STANDARD, 17700, true);
    _sensorInfo.t2=dps.getTemperature2();
    _sensorInfo.p1=dps.getPressure2();

  };

  DHT dht(5, DHT22);
  dht.begin();

  /***********************************************************
   *    Датчик влажности тугой. Он может с первого           *
   *    раза данные не считать. Поэтому пытаемся             *
   *    его считать столько раз, сколько успеем за 5 секунд. *
   ***********************************************************/
  
  vCurrTime = millis();
  
  do {    
    _sensorInfo.t1 = dht.readTemperature2();
    _sensorInfo.h1 = dht.readHumidity2();
    delay(1000);
  } while ((isnan(_sensorInfo.h1) || _sensorInfo.h1==0 || isnan(_sensorInfo.t1)) && millis() - vCurrTime < 3000); //Почему-то иногда датчик выдает нулевые 

  _sensorInfo.distance=getDistance();
}


void showLong(long long iVal) {

 char buffer[25];
 sprintf_P(buffer, PSTR("%0ld"),iVal/1000000L);
 sprintf_P(buffer, PSTR("%0ld"),iVal%1000000L);
 Serial.print(buffer); 
 Serial.flush();
}



/**************************************************************
 *  Конец доп.методов                                         *
 **************************************************************/

/**************************************************************
   Методы работы с модемом
 **************************************************************/
void parseSmsParamCommand(char* iCommand) {
  char vTmpStr[5],
       vCmd[10];
    mstr _mstr;
    Connection _connection;

   strcpy_P(vTmpStr, PSTR(":"));
  _mstr.entry(1, iCommand, vTmpStr, 10, vCmd);    
   EEPROM.get(0, _connection);   
    
  /**********************************
   * Меняем учетные данные к apn
   **********************************/
  
  strcpy_P(vTmpStr, PSTR("APN7"));

  if (_mstr.isEqual(vCmd, vTmpStr)) {  
    
    strcpy_P(vTmpStr, PSTR(":"));
    if (!_mstr.entry(2, iCommand, vTmpStr, 35, _connection.apnPoint)) {
      return;
    };

    if (!_mstr.entry(3, iCommand, vTmpStr, 11, _connection.apnLogin)) {
      return;
    };

    if (!_mstr.entry(4, iCommand, vTmpStr, 11, _connection.apnPass)) {
      return;
    };

#if IS_DEBUG>1
    Serial.print(F("APN: "));
    Serial.print(_connection.apnLogin);
    Serial.print(F(":"));
    Serial.print(_connection.apnPass);
    Serial.print(F("@"));
    Serial.println(_connection.apnPoint);
    Serial.flush();
#endif

    noInterrupts();
      EEPROM.put(0, _connection);
    interrupts();
  };

  strcpy_P(vTmpStr, PSTR("SITE"));
  if (_mstr.isEqual(vCmd, vTmpStr)) {

    strcpy_P(vTmpStr, PSTR(":"));

    if (!_mstr.entry(2, iCommand, vTmpStr, 11, _connection.siteLogin)) {
      return;
    };

    if (!_mstr.entry(3, iCommand, vTmpStr, 20, _connection.sitePass)) {
      return;
    };

    if (!_mstr.entry(4, iCommand, vTmpStr, sizeof(_connection.sitePoint), _connection.sitePoint)) {
      return;
    };

#if IS_DEBUG>1
    Serial.print(F("SITE: "));
    Serial.print(_connection.siteLogin);
    Serial.print(F(":"));
    Serial.print(_connection.sitePass);
    Serial.print(F("@"));
    Serial.println(_connection.sitePoint);
    Serial.flush();
#endif

    noInterrupts();
      EEPROM.put(0, _connection);
    interrupts();

  };

}

bool onSms(byte iSms, char* iCommand) {
  mstr _mstr;
  char vTmpStr[2];

  strcpy_P(vTmpStr, PSTR(":"));

  if (_mstr.numEntries(iCommand, vTmpStr)>3) {
      parseSmsParamCommand(iCommand);     
  };

  return true;
}

void readSms2() {
  
  gprs2 sim900(7, 8);  

  sim900.setOnSms(onSms);    

  #ifdef IS_DEBUG
      Serial.println(F("*SMS*"));
      Serial.flush();
  #endif   
  delay(500); //Не знаю почему, но без этой конструкции freemem показывает, что сожрано на 200байт больше памяти.
  sim900.readSms();
  #ifdef IS_DEBUG
      Serial.println(F("*ESMS*"));
      Serial.flush();
  #endif   
}

/**************************************************************
 * Пробуждаем модем, так как это не всегда                    *
 * может получиться с первого раза делаем несколько попыток.  *
 **************************************************************/
bool wk() {

  bool vStatus = false;
  byte vAttempt = 0;
    
  gprs2 sim900(7, 8);
   
  {
    Connection _connection;
    EEPROM.get(0, _connection);    
    sim900.setInternetSettings(_connection.apnPoint, _connection.apnLogin, _connection.apnPass);
  };

  sim900.wakeUp();  
  delay(10000);
    
  do {    
    vStatus = sim900.canWork();    
    delay(vAttempt * 3000);
    vAttempt++;
  } while (!vStatus && vAttempt < 3);

#if IS_DEBUG>1
  if (!vStatus) {
      char vError[20];
      sim900.getLastError(vError);
      Serial.print(F("WKE:"));
      Serial.println(vError);
      Serial.flush();
  };
#endif

  return vStatus;

}

/************************************************************
 * Отправляем модем в сон.                                  *
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
  #if IS_DEBUG>1
    Serial.println(F("*RST*"));
    Serial.flush();
  #endif
  sim900.hardRestart();
  delay(20000);
};
/**************************************************************************
    Конец методов работы с модемом
 *************************************************************************/
bool canGoSleep() {
   return mCanGoSleep && isDisabledLightRange() && isDisabledWaterRange();
}

byte goSleep(byte iMode,long long iPrevTime) {
  unsigned long  vSleepTime   = 0, 
                 vPeriodSleep = 0,
                 vFirstLoop   = 0,
                 vSecondLoop  = 0;
                 

  long  vNextModem   = 0;
  long long vTimeStamp;

  byte vWaitTime=0;                 

 
   {
    worker _worker(eeprom_mWorkerStart);    
    vSleepTime   = _worker.getSleepTime();
    vTimeStamp   = _worker.getTimestamp();
   };
    
 
    
    vNextModem   = (long)(iPrevTime + connectPeriod() - vTimeStamp);
    
    /* Если по каким-то причинам было пропущено несколько подключений, то принудительно подключаемся */
    vNextModem    = max(0,vNextModem); 


    vSleepTime   = min(vNextModem, vSleepTime);
  
    vSleepTime   = (unsigned long) vSleepTime * iMode / 100;
 #ifdef IS_DEBUG
    Serial.print(F("Slp time * "));
    Serial.print(iMode);
    Serial.print(F("%="));
    Serial.print(vSleepTime);
    Serial.println(F(" (s)"));
    Serial.flush();
 #endif
    
    vPeriodSleep = (unsigned int)(vSleepTime / 8UL);        
    
    
    vFirstLoop   = (unsigned int)  vPeriodSleep / 37;
    vSecondLoop  = vPeriodSleep % 37;

    vWaitTime    = vSleepTime - vFirstLoop*37*8 - vSecondLoop * 8;
        
  for (unsigned int vJ = 0; vJ < vFirstLoop; vJ++) {      
     for (unsigned int vK = 0; vK < 37; vK++) {
       LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);    
     };      
  };

  
  for (unsigned int vJ = 0; vJ < vSecondLoop; vJ++) {
        LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);    
  };     
  
 return vWaitTime; 

};


/*********************************************************
 * Метод отправляет информацию об измерения с датчиков.  *
 *********************************************************/

bool updateRemoteMeasure(sensorInfo si) {
  char vParams[200],     
       vRes[40],
       sitePoint[__SITE_POINT_SIZE__];
  bool vResult;  



  gprs2 sim900(7, 8);

  {
    Connection _connection;      
    EEPROM.get(0, _connection);
    
    sprintf_P(vParams, PSTR("r=%s&s=%s&t1=%d&h1=%d&t2=%d&p1=%lu&w1=%lu&l1=%lu&d=%lu&ds=%d"), _connection.siteLogin, _connection.sitePass, si.t1, si.h1, si.t2, si.p1, _water.duration, _light.duration, millis(),si.distance);
    
    sim900.setInternetSettings(_connection.apnPoint, _connection.apnLogin, _connection.apnPass);
    strncpy(sitePoint,_connection.sitePoint,sizeof(sitePoint));
  };
  

  if (!sim900.canInternet()) {
    #if IS_DEBUG>1
      char vError[20];
      sim900.getLastError(vError);
      Serial.print(F("CI ER:"));
      Serial.println(vError);
      Serial.flush();
    #endif
    return false;
  };

  vResult = sim900.postUrl(sitePoint, vParams, vRes, sizeof(vRes));

  #ifdef IS_DEBUG
    Serial.print(F("MEAS "));
  #endif

  if (vResult) {
    /**************************************************************************
     *  Если исполнители закончили свою работу и модуль передал информацию,   *
     *  то тогда обнуляем время работы.                                       *
     **************************************************************************/

      #ifdef IS_DEBUG
        Serial.print(F(": "));
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
      Serial.print(F("ER: "));
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
        #if IS_DEBUG>2
            Serial.print(F("Sleep "));
            Serial.println(atoi(vParam1));
            Serial.flush();
        #endif
        setConnectPeriod(atoi(vParam1));
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
        setOfflineLight(atoi(vParam1), atoi(vParam2));
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
        setOfflineWater(atoi(vParam1), atoi(vParam2));
        return false;
      };
    };  

  return true;
};

/**********************************************************
 * Если требуется повторное соединение, то                *
 * функция возвращает True. Если подсоединения не         *
 * требуется, то возвращается False.                      *
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
 * Метод отправляет информацию о текущих координатах и    *
 * загружает новые параметры устройства.                  *
 **********************************************************/
bool updateRemoteParams() {  
  char vParams[70], 
       sitePoint[__SITE_POINT_SIZE__];
  bool vShouldReconnect = true;
  

  gprs2 sim900(7, 8);
  {
    char vL[11], vA[11];
    Connection _connection;
    
    EEPROM.get(0, _connection);
    sim900.setInternetSettings(_connection.apnPoint, _connection.apnLogin, _connection.apnPass); 
    strncpy(sitePoint,_connection.sitePoint,sizeof(sitePoint));

    if (!sim900.canInternet()) {
      #if IS_DEBUG>2
        char vError[20];
        sim900.getLastError(vError);
        Serial.print(F("CI ER:"));
        Serial.println(vError);
        Serial.flush();
      #endif
      return false;
    };

    sim900.getCoords(vL, vA,vParams,sizeof(vParams));    
    sprintf_P(vParams, PSTR("r=%s&s=%s&m=c&l=%s&a=%s"), _connection.siteLogin, _connection.sitePass, vL, vA);
  };
  



    while (vShouldReconnect) {      
      char vRes[250];
      bool vResult;
      
      vResult = sim900.postUrl(sitePoint, vParams, vRes, sizeof(vRes));
            #ifdef IS_DEBUG
              Serial.print(F("PARA"));
              if (vResult) {    
                Serial.print(F(" : "));
                Serial.println(vRes);
                Serial.flush();
              } else {
                char vError[20];
                sim900.getLastError(vError);
                Serial.print(F(" ER :"));
                Serial.println(vError);
                Serial.flush();    
              };
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
 
  mCurrTime=_worker.getTimestamp(vSecMidnight,vDayOfWeek);  


   /*******************************************************************
    * Если достигли минимального значения по температе,               *
    * то включаем устройство "Свет". При этом отмечаем, что включение *
    * произошло по границе.                                           *
    *******************************************************************/
    
   if (!isDisabledLightRange() && (_sensorInfo.t1 < _offlineParams.tempUpLight1 || _light.isEdge)) {   
     _light.isEdge     = true;
   };



   /******************************************************************
    * Если достигли мимального значения по внутренней температуре,   *
    * то включаем устройство "Вода". При этом отмечаем включение     *
    * по температуре. Делаю эту тему по заказу Юрца Маленького.      *
    ******************************************************************/
   if (!isDisabledWaterRange() && (_sensorInfo.t2 < _offlineParams.tempUpWater1 || _water.isEdge)) {
     _water.isEdge     = true;
   };  


  _light.isSchedule=false;
  _water.isSchedule=false;


  for (byte vI = 0; vI < _worker.maxTaskCount; vI++) {    
    byte executor = _worker.shouldTaskWork2(vI, vSecMidnight, vDayOfWeek);

    _light.isSchedule = _light.isSchedule || (bool)bitRead(executor, 1);
    _water.isSchedule = _water.isSchedule || (bool)bitRead(executor, 0);

  };

     
   if (_sensorInfo.t1 >= _offlineParams.tempUpLight2) { 
      _light.isEdge     = false;
   };

     
  if (_sensorInfo.t2 >= _offlineParams.tempUpWater2) {      
      _water.isEdge     = false;
   };

  #ifdef IS_DEBUG
     
     Serial.print(F("VSM "));
     Serial.print(vSecMidnight);
     Serial.print(F("   "));

     
     Serial.print(F("DOW "));
     Serial.print(vDayOfWeek);
     Serial.print(F("   "));

     Serial.print(F("Mem:"));
     Serial.print(freeMemory());  
     Serial.print(F("   "));

     Serial.print(F("MID:"));
     Serial.print(vSecMidnight);  
     Serial.print(F("   "));

     Serial.print(F("Mil:"));
     Serial.print(millis());  
     Serial.print(F("   "));
   
     Serial.print(F("T1:"));
     Serial.print(_sensorInfo.t1);     
     
     Serial.print(F("   T2:"));
     Serial.print(_sensorInfo.t2);

     Serial.print(F("   UPL1:"));
     Serial.print(_offlineParams.tempUpLight1);

     Serial.print(F("   UPW1:"));
     Serial.print(_offlineParams.tempUpWater1);

     Serial.print(F("   LEDG:"));
     Serial.println(_light.isEdge);
     //Serial.flush();
  #endif


  /************************************************************************
   *                        Останавливаем исполнителей                    *
   ************************************************************************/

  if (!_light.isSchedule && !_light.isEdge && _light.isWork) {

    #ifdef IS_DEBUG
      Serial.print(F("EL:"));
      Serial.println(vSecMidnight);
      //Serial.flush();
    #endif
    _worker.stopLight();
    _light.isWork = false;    
  };

  if (!_water.isSchedule && !_water.isEdge && _water.isWork) {

    #ifdef IS_DEBUG
      Serial.print(F("EW:"));
      Serial.println(vSecMidnight);
      //Serial.flush();
    #endif
    _worker.stopWater();
    _water.isWork = false;
  };


  /************************************************************************
   *                        Стартуем исполнителей                         *
   ************************************************************************/
   
  if ((_water.isSchedule || _water.isEdge) && !_water.isWork) {
    #ifdef IS_DEBUG
      Serial.print(F("SW:"));
      Serial.println(vSecMidnight);
      //Serial.flush();
    #endif
    _worker.startWater();
    _water.isWork = true;
    _water.startTime = vSecMidnight;
  };

  if ((_light.isSchedule || _light.isEdge) && !_light.isWork) {
    #ifdef IS_DEBUG
      Serial.print(F("SL:"));
      Serial.println(vSecMidnight);
      //Serial.flush();
    #endif
    _worker.startLight();
    _light.isWork = true;
    _light.startTime = vSecMidnight;
  };

   /**************************************************
    * Если исполнитель работает, то увеличиваем      *
    * время его работы.                              *
    **************************************************/

  if (_water.isWork) {
    /**********************************************************************
     *  Если время включения больше, чем текущее время                    *
     *  это означает, что либо мы перешагнули через                       *
     *  полуночь либо перевелись часы. Соответсвенно устанавливаем        *
     *  время включения в 0. Исходим из того, что считаем время работы    *
     *  исполнителя в текущих сутках.                                     *
     **********************************************************************/
    if (_water.startTime > vSecMidnight) {
      _water.startTime = 0;
    };

    
    _water.duration = vSecMidnight - _water.startTime;    
  };

  if (_light.isWork) {
    /**********************************************************************
     *  Если время включения больше, чем текущее время                    *
     *  это означает, что либо мы перешагнули через                       *
     *  полуночь либо перевелись часы. Соответсвенно устанавливаем        *
     *  время включения в 0. Исходим из того, что считаем время работы    *
     *  исполнителя в текущих сутках.                                     *
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
   * Пришло прерывание по нажатию на кнопку.    *
   * Обнуляем переменные соединения, выставляем *         
   * флаг считывания СМС сообщений.             *
   **********************************************/

  digitalWrite(LED_BUILTIN,HIGH); 

  Connection _connection;

  EEPROM.get(0, _connection);   

  strcpy_P(_connection.sitePoint, PSTR("\0"));    
  strcpy_P(_connection.apnPoint, PSTR("\0"));    
     
  #ifdef IS_DEBUG
    Serial.println(F("SMS|^"));
  #endif
  mShouldReadSms=true;

  noInterrupts();
  EEPROM.put(0, _connection);
  interrupts();

}

void setup() {

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
   // if (true) {
  
    #ifdef IS_DEBUG
      Serial.println(F("SET"));
      Serial.flush();
    #endif

    

    for (int vI = 0 ; vI < EEPROM.length() ; vI++) {
      EEPROM.write(vI, 0);
    };
  
    //Признак того, что инициализация выполнена    
    strcpy_P(_globals.version, PSTR("INI"));
    // Задержка соединения по-умолчанию 15 минут
    _globals.connectPeriod = 15;
    EEPROM.put(eeprom_mGlobalsStart, _globals);

   {
      Connection _connection;
      EEPROM.get(0, _connection);
      /*** SITE INIT ***/
      strcpy_P(_connection.sitePoint, PSTR("\0"));    
      strcpy_P(_connection.siteLogin, PSTR("\0"));    
      strcpy_P(_connection.sitePass, PSTR("\0"));
    
      /*** APN INIT ***/
     strcpy_P(_connection.apnPoint, PSTR("\0"));
     strcpy_P(_connection.apnLogin, PSTR("\0"));
     strcpy_P(_connection.apnPass, PSTR("\0"));
     EEPROM.put(0, _connection);
   };
    

   _worker.setDateTime(16, 9, 13, 18, 45, 0);

   //Устанавливаем умолчательные значения для автономного режима
   setOfflineLight(-199,199);
   setOfflineWater(-199,199);

    
  
    
 

  #ifdef IS_DEBUG
    Serial.println(F("END SET"));
    Serial.flush();
  #endif
  };

  /*********************************
   *  Инициализруем переменную. Если этого не сделать, 
   *  то возможно переполнение при первом старке.
   *  Дополнительно экономим
   *  память по глобальной переменной отображающей факт первого запуска.
   */
  {
    worker _worker(eeprom_mWorkerStart);
    vPrevTime2 = _worker.getTimestamp() - connectPeriod();
  };

  Wire.begin();

  loadSensorInfo1();
  
  Timer1.initialize(3000000);
  Timer1.attachInterrupt(Timer1_doJob);
  Timer1.start();

  
#ifdef WDT_ENABLE
  wdt_enable (WDTO_8S);
#endif
 
 attachInterrupt(0, pin_ISR, FALLING); 
}

void waitAndBlink() {
  digitalWrite(LED_BUILTIN,HIGH);
  delay(500);
  digitalWrite(LED_BUILTIN,LOW);
  delay(5000); 
}


void loop() 
{
   
    unsigned int vDistance;    
    waitAndBlink(); // Мигаем диодом, что живы
   
    long vD = (long)(mCurrTime-vPrevTime2);  // mCurrTime берем из прерывания по будильнику


    if (millis()-_sensorInfo.lastMeasure>__MEASURE_PERIOD__) {
      loadSensorInfo1();
      _sensorInfo.lastMeasure=millis();
       #ifdef IS_DEBUG
         Serial.print(F("T1:"));    Serial.print(_sensorInfo.t1);  Serial.print(F("  T2:")); Serial.print(_sensorInfo.t2); 
         Serial.print(F("  H:"));   Serial.print(_sensorInfo.h1);  Serial.print(F("  P:"));  Serial.print(_sensorInfo.p1); 
         Serial.print(F("  DIS:")); Serial.print(_sensorInfo.distance); 
         Serial.print(F("  W&L:")); Serial.print(_water.duration); Serial.print(F("&"));      
         Serial.println(_light.duration); 
         Serial.flush();
      #endif

    }          

  
    /***************************************************************
     * Была нажата кнопка считывания СМС сообщения, считываем      * 
     * до тех пор пока не будут заполнены ключевые параметры.      *
     * Если модем не готов, то будем его перезагружать его до тех  *
     * пор пока он не станет готовым.                              *
     **************************************************************/
    if (mShouldReadSms) {           

       while (!wk()) {                
        restartModem();
       };
              
       while (!isConnectInfoFull()) {
           readSms2();             
       };       
       digitalWrite(LED_BUILTIN,LOW); 
       mShouldReadSms=false;
    };

          

  if (vD >= connectPeriod()) { 

    /*** Начало блока работы с модемом ***/

    if (isConnectInfoFull()) {
      bool isModemWork = wk();  

      if (!isModemWork) {
        restartModem();
        isModemWork = wk();
      };

      if (isModemWork) {
        byte vAttempt = 0;
        bool vStatus  = false;

        do {
          vStatus = updateRemoteParams();         
          vAttempt++;
       } while (!vStatus && vAttempt < 3);

        
        vAttempt = 0;
        vStatus  = false;
        do {
          vStatus = updateRemoteMeasure(_sensorInfo);         
          vAttempt++;
        } while (!vStatus && vAttempt < 3);
      };            

      vPrevTime2 = mCurrTime;
      
      /*** Конец блока работы с модемом ***/
    } else {

     /****************************************************************
      * Если учетные данные пустые и не нажата кнопка приема СМС,    *
      * то начинаем все сначала.                                      *
      ****************************************************************/


      #ifdef IS_DEBUG
       Serial.println(F("NO MODEM CRED"));
      #endif;

      return;
    };
          
    
    sl();  //После того как модем передал данные уводим его в сон. Если это делать в основном теле функции, то отправлять будем 1 раз в секунду  
};

  
  if (!canGoSleep()) {
     return;
   };


 Timer1.stop();
 goSleep(50,vPrevTime2);
 goSleep(75,vPrevTime2);
 goSleep(90,vPrevTime2);
 
 /************************************************
  * Гарантировано даем включиться исполнителям.  *
  ************************************************/  
  {
    byte vDelay = goSleep(100,vPrevTime2);
    Timer1.start();
    delay(vDelay * 1000 + 2000);
  };     

 
 
}
