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

#define SITE_POINT_SIZE 40


struct Connection { 
  char apnPoint[35];
  char apnLogin[11];
  char apnPass[11];
  char sitePoint[SITE_POINT_SIZE];
  char siteLogin[11];
  char sitePass[20];
};

struct Globals {
  char version[3];
  int  sleepTime;
  int  connectPeriod;
};

struct offlineParams {
  int  tempUpLight1;
  int  tempUpLight2;
  int  tempUpWater1;
  int  tempUpWater2;
};

#define eeprom_mGlobalsStart sizeof(Connection)
#define eeprom_mOfflineParamsStart sizeof(Connection)+sizeof(Globals)
#define eeprom_mWorkerStart eeprom_mOfflineParamsStart + sizeof(offlineParams)
#define BLINK_PIN 13

long long mCurrTime,vPrevTime2;

volatile bool mCanGoSleep   = true,
              mShouldReadSms = false;

int mCurrTempOut = 19900,
    mCurrTempIn  = 1990,   
    mCurrH;

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

void loadSensorInfo1(int &oT1, 
                      int &oH1, 
                     int &oT2, 
                     unsigned long &oP1) {
  unsigned long vCurrTime;

  BMP085   dps = BMP085();
  DHT dht(5, DHT22);

  dps.init(MODE_STANDARD, 17700, true);
  oT2=dps.getTemperature2();


  oP1=dps.getPressure2();

  dht.begin();

  /***********************************************************
   *    Датчик влажности тугой. Он может с первого           *
   *    раза данные не считать. Поэтому пытаемся             *
   *    его считать столько раз, сколько успеем за 5 секунд. *
   ***********************************************************/
  
  vCurrTime = millis();
  
  do {    
    oH1 = dht.readHumidity();
    oT1 = dht.readTemperature() * 100;
    delay(1000);
  } while ((isnan(oH1) || isnan(oT1)) && millis() - vCurrTime < 5000) ;

  oH1  = oH1 * 100;
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

#ifdef IS_DEBUG
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

    if (!_mstr.entry(4, iCommand, vTmpStr, SITE_POINT_SIZE, _connection.sitePoint)) {
      return;
    };

#ifdef IS_DEBUG
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
    vAttempt++;
  } while (!vStatus && vAttempt < 3);

#ifdef IS_DEBUG
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
  #ifdef IS_DEBUG
    Serial.println(F("*RST*"));
    Serial.flush();
  #endif
  sim900.hardRestart();
  delay(60000);
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
     sl();
     for (unsigned int vK = 0; vK < 37; vK++) {
       LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);    
     };      
  };


  sl();  //Первый цикл может быть пропущен поэтому необходимо модем отправить в сон.
  
  for (unsigned int vJ = 0; vJ < vSecondLoop; vJ++) {
        LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);    
  };     
  
 return vWaitTime; 

};


/*********************************************************
 * Метод отправляет информацию об измерения с датчиков.  *
 *********************************************************/

bool updateRemoteMeasure(int t1,  int h1, int t2, long unsigned int p1) {
  char vParams[200],     
       vRes[40],
       sitePoint[SITE_POINT_SIZE];
  bool vResult;  



  gprs2 sim900(7, 8);

  {
    Connection _connection;      
    EEPROM.get(0, _connection);
    
    sprintf_P(vParams, PSTR("r=%s&s=%s&t1=%d&h1=%d&t2=%d&p1=%lu&w1=%lu&l1=%lu&d=%lu"), _connection.siteLogin, _connection.sitePass, t1, h1, t2, p1, _water.duration, _light.duration, millis());
    
    sim900.setInternetSettings(_connection.apnPoint, _connection.apnLogin, _connection.apnPass);
    strncpy(sitePoint,_connection.sitePoint,sizeof(sitePoint));
  };
  

  if (!sim900.canInternet()) {
    #ifdef IS_DEBUG
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
        #ifdef IS_DEBUG
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
        #ifdef IS_DEBUG
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
        #ifdef IS_DEBUG
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
       sitePoint[SITE_POINT_SIZE];
  bool vShouldReconnect = true;
  

  gprs2 sim900(7, 8);
  {
    char vL[11], vA[11];
    Connection _connection;
    
    EEPROM.get(0, _connection);
    sim900.setInternetSettings(_connection.apnPoint, _connection.apnLogin, _connection.apnPass); 
    strncpy(sitePoint,_connection.sitePoint,sizeof(sitePoint));

    if (!sim900.canInternet()) {
      #ifdef IS_DEBUG
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
  bool isWaterShouldWork = false,
       isLightShouldWork = false;
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
    
   if (!isDisabledLightRange() && (mCurrTempOut < _offlineParams.tempUpLight1 || _light.isEdge)) {
     isLightShouldWork = true;
     _light.isEdge     = true;
   };



   /******************************************************************
    * Если достигли мимального значения по внутренней температуре,   *
    * то включаем устройство "Вода". При этом отмечаем включение     *
    * по температуре. Делаю эту тему по заказу Юрца Маленького.      *
    ******************************************************************/
   if (!isDisabledWaterRange() && (mCurrTempIn < _offlineParams.tempUpWater1 || _water.isEdge)) {
     isWaterShouldWork = true;
     _water.isEdge     = true;
   };  


  for (byte vI = 0; vI < _worker.maxTaskCount; vI++) {    
    byte executor = _worker.shouldTaskWork2(vI, vSecMidnight, vDayOfWeek);

    isLightShouldWork = isLightShouldWork || (bool)bitRead(executor, 1);
    isWaterShouldWork = isWaterShouldWork || (bool)bitRead(executor, 0);

  };

     
   if (mCurrTempOut >= _offlineParams.tempUpLight2) {
      isLightShouldWork = false;
      _light.isEdge     = false;
   };

     
  if (mCurrTempIn >= _offlineParams.tempUpWater2) {
      isWaterShouldWork = false;
      _water.isEdge     = false;
   };

  #ifdef IS_DEBUG
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
     Serial.print(mCurrTempOut);     
     Serial.print(F("   T2:"));
     Serial.println(mCurrTempIn);
     //Serial.flush();
  #endif


  /************************************************************************
   *                        Останавливаем исполнителей                    *
   ************************************************************************/

  if (!isLightShouldWork && _light.isWork) {

    #ifdef IS_DEBUG
      Serial.print(F("EL:"));
      Serial.println(vSecMidnight);
      //Serial.flush();
    #endif
    _worker.stopLight();
    _light.isWork = false;    
  };

  if (!isWaterShouldWork && _water.isWork) {

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
   
  if (isWaterShouldWork && !_water.isWork) {
    #ifdef IS_DEBUG
      Serial.print(F("SW:"));
      Serial.println(vSecMidnight);
      //Serial.flush();
    #endif
    _worker.startWater();
    _water.isWork = true;
    _water.startTime = vSecMidnight;
  };

  if (isLightShouldWork && !_light.isWork) {
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

 digitalWrite(BLINK_PIN,HIGH); 

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

  Connection _connection;
  Globals _globals;
  worker _worker(eeprom_mWorkerStart);
   
  pinMode(BLINK_PIN, OUTPUT);
  digitalWrite(BLINK_PIN, LOW);    // turn the LED off by making the voltage LOW

  _worker.stopWater();
  _worker.stopLight();
  
  EEPROM.get(0, _connection);
  EEPROM.get(eeprom_mGlobalsStart, _globals);


  if (strcmp_P(_globals.version, PSTR("INI")) != 0) {
  //  if (true) {
    #ifdef IS_DEBUG
      Serial.println(F("SET"));
      Serial.flush();
    #endif

    for (int vI = 0 ; vI < EEPROM.length() ; vI++) {
      EEPROM.write(vI, 0);
    };

    //Признак того, что инициализация выполнена
    strcpy_P(_globals.version, PSTR("INI"));
    /*** SITE INIT ***/
    strcpy_P(_connection.sitePoint, PSTR("\0"));    
    strcpy_P(_connection.siteLogin, PSTR("\0"));    
    strcpy_P(_connection.sitePass, PSTR("\0"));
    
    /*** APN INIT ***/
    strcpy_P(_connection.apnPoint, PSTR("\0"));
    strcpy_P(_connection.apnLogin, PSTR("\0"));
    strcpy_P(_connection.apnPass, PSTR("\0"));

    // Задержка соединения по-умолчанию 15 минут
    _globals.connectPeriod = 15;

    _worker.setDateTime(16, 9, 13, 18, 45, 0);

    // Сохраняю настройки
    noInterrupts();
    EEPROM.put(0, _connection);
    EEPROM.put(eeprom_mGlobalsStart, _globals);
    interrupts();

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
  
  Timer1.initialize(3000000);
  Timer1.attachInterrupt(Timer1_doJob);
  Timer1.start();

  
#ifdef WDT_ENABLE
  wdt_enable (WDTO_8S);
#endif
 
 attachInterrupt(0, pin_ISR, FALLING); 
}

void waitAndBlink() {
  digitalWrite(BLINK_PIN,HIGH);
  delay(500);
  digitalWrite(BLINK_PIN,LOW);
  delay(5000); 
}


void loop() 
{
    
  waitAndBlink(); // Мигаем диодом, что живы
   
    long vD = (long)(mCurrTime-vPrevTime2);  // mCurrTime берем из прерывания по будильнику
      
    loadSensorInfo1(mCurrTempOut,mCurrH, mCurrTempIn, mCurrP);

    #ifdef IS_DEBUG
     Serial.print(F("T1:")); Serial.print(mCurrTempOut); Serial.print(F("  T2:")); Serial.print(mCurrTempIn); Serial.print(F("  H:")); Serial.print(mCurrH); Serial.print(F("  P:")); Serial.print(mCurrP); Serial.print(F("  W&L:"));     Serial.print(_water.duration); Serial.print(F("&")); Serial.println(_light.duration); Serial.flush();
   #endif

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
       digitalWrite(BLINK_PIN,LOW); 
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
          vStatus = updateRemoteMeasure(mCurrTempOut, mCurrH, mCurrTempIn, mCurrP);         
          vAttempt++;
        } while (!vStatus && vAttempt < 3);
      };            
      
      
      /*** Конец блока работы с модемом ***/
    };
          
    vPrevTime2 = mCurrTime;
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
