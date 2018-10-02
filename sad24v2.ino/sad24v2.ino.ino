#define PERIOD_MODEM_SLEEP 180
#define WDT_ENABLE1

#include <avr/wdt.h>
#include <BMP085.h>
#include <DHT.h>
#include <gprs2.h>
#include <mstr.h>
#include <worker.h>
#include <EEPROM.h>
#include <TimerOne.h>
#include <LowPower.h>
#include <debug.h>
#include <MemoryFree.h>



struct Connection {
  char apnPoint[35];
  char apnLogin[11];
  char apnPass[11];
  char sitePoint[40];
  char siteLogin[11];
  char sitePass[20];
};

struct Globals {
  char version[3];
  int  sleepTime;
  int connectPeriod;
};

struct offlineParams {
  int  tempUpLight;
  int  tempUpWater;
};


unsigned int mOfflineParamsStart = sizeof(Connection) + sizeof(Globals);
unsigned int mWorkerStart = mOfflineParamsStart + sizeof(offlineParams);

long long vPrevTime2;

volatile bool mCanGoSleep=true;
bool isFirstRun = true;

int mCurrTempOut = 19900;
int mCurrTempIn  = -2000;

workerInfo _water;
workerInfo _light;



/**********************************************************
   Геттеры и сеттеры.
 **********************************************************/

int getCurrTempOut() {
  return mCurrTempOut;
}

int getCurrTempIn() {
  return mCurrTempIn;
}

int getTempUpLight() {
  offlineParams _offlineParams;
  EEPROM.get(mOfflineParamsStart, _offlineParams);
  return _offlineParams.tempUpLight;
};

int getTempUpWater() {
  offlineParams _offlineParams;
  EEPROM.get(mOfflineParamsStart, _offlineParams);
  return _offlineParams.tempUpWater;
};

void setTempOffline(int iLight, int iWater) {
  offlineParams _offlineParams;


  if (iLight < -99 || iLight > 99 || iWater < -99 || iWater > 99) {
    return;
  };

  EEPROM.get(mOfflineParamsStart, _offlineParams);
  _offlineParams.tempUpLight = iLight;
  _offlineParams.tempUpWater = iWater;

  noInterrupts();
  EEPROM.put(mOfflineParamsStart, _offlineParams);
  interrupts();

}

unsigned long connectPeriod() {
  Globals _globals;
  EEPROM.get(sizeof(Connection), _globals);
 
  return _globals.connectPeriod * 60UL;
}

void setConnectPeriod(int iSleepTime) {

  if (iSleepTime < 5 || iSleepTime > 600) {
    return;
  };

  Globals _globals;
  EEPROM.get(sizeof(Connection), _globals);
  _globals.connectPeriod = iSleepTime;

  noInterrupts();
  EEPROM.put(sizeof(Connection), _globals);
  interrupts();

}

/***********************************************************
   Конец геттеров и сеттеров
 ***********************************************************/

/***********************************************************
   Дополнительные методы работы.
 ***********************************************************/

void showDateTime() {
  worker _worker(mWorkerStart);
  _worker.showDateTime();
}

void loadSensorInfo1(int *oT1, int *oH1, int *oT2, long *oP1) {
  float    h1 = -100, t1 = 200;
  long     p1 = -100;
  unsigned long vCurrTime;
  int32_t  t2 = -100;

  Wire.begin();

  BMP085   dps = BMP085();
  DHT dht(2, DHT22);

  dps.init(MODE_STANDARD, 17700, true);
  dps.getTemperature(&t2);


  dps.getPressure(&p1);
  dht.begin();

  /***********************************************************
       Датчик влажности тугой. Он может с первого
       раза данные не считать. Поэтому пытаемся
       его считать столько раз, сколько успеем за 5 секунд.
   ***********************************************************/



  h1 = dht.readHumidity();
  t1 = dht.readTemperature();
  noInterrupts();
  vCurrTime = millis();
  interrupts();

  while ((isnan(h1) || isnan(t1)) && millis() - vCurrTime < 5000) {
    delay(1000);
    h1 = dht.readHumidity();
    t1 = dht.readTemperature();
  };


  /*****************************************************
     Если все таки мы не получили влажность и вторую
     температуру, то устанавливаем их значение в -1.
   *****************************************************/


  if (isnan(h1) || abs(h1) > 100) {
    h1 = -100;
  };

  if (isnan(t1) || abs(t1) > 999 ) {
    t1 = 0;
  };


  if (isnan(p1) || abs(p1) > 1000000 ) {
    p1 = -100;
  };


  mCurrTempOut = round(t1);
  mCurrTempIn  = round(t2 / 10);

  h1 = h1 * 100;
  t1 = t1 * 100;

  *oT1  = t1;
  *oP1  = p1;
  *oH1  = h1;
  *oT2  = t2;

}

unsigned long getSecMidnight() {
  worker _worker(mWorkerStart);
  return _worker.getSecMidnight();
}

long long getTimestamp() {
  worker _worker(mWorkerStart);
  return _worker.getTimestamp();
}




/**************************************************************
   Конец доп.методов
 **************************************************************/

/**************************************************************
   Методы работы с модемом
 **************************************************************/
void parseThreeParamCommand(char* iCommand) {
  char vTmpStr[5],
       vCmd[10];
  mstr _mstr;
  Connection _connection;
  strcpy_P(vTmpStr, PSTR(":"));


  _mstr.entry(1, iCommand, vTmpStr, 10, vCmd);

  EEPROM.get(0, _connection);

  /**********************************
    Меняем учетные данные к apn
   **********************************/
  strcpy_P(vTmpStr, PSTR("APN7"));
  if (_mstr.isEqual(vCmd, vTmpStr)) {
#ifdef IS_DEBUG
    Serial.print(F(">APN: "));
#endif

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

#ifdef IS_DEBUG
    Serial.print(F(">SITE: "));
#endif
    strcpy_P(vTmpStr, PSTR(":"));

    if (!_mstr.entry(2, iCommand, vTmpStr, 11, _connection.siteLogin)) {
      return;
    };

    if (!_mstr.entry(3, iCommand, vTmpStr, 20, _connection.sitePass)) {
      return;
    };

    if (!_mstr.entry(4, iCommand, vTmpStr, 40, _connection.sitePoint)) {
      return;
    };

#ifdef IS_DEBUG
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

  switch (_mstr.numEntries(iCommand, vTmpStr)) {
    case 4:
      parseThreeParamCommand(iCommand);
      break;
  };

  return true;
}

void readSms() {
  gprs2 sim900(7, 8);

  sim900.setOnSms(onSms);
  sim900.readSms();
}

bool wk() {
  Connection _connection;
  bool vStatus;
  byte vAttempt = 0;
    
  gprs2 sim900(7, 8);

  EEPROM.get(0, _connection);
  
  sim900.setInternetSettings(_connection.apnPoint, _connection.apnLogin, _connection.apnPass);

  do {
    vStatus = sim900.wakeUp() && sim900.canWork();
    delay(vAttempt * 5000);
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

void sl() {
  gprs2 sim900(7, 8);
  sim900.sleep();
}

void restartModem() {
  gprs2 sim900(7, 8);
  #ifdef IS_DEBUG
    Serial.println(F("*REST*"));
    Serial.flush();
  #endif
  sim900.softRestart();
  delay(60000);
};
/**************************************************************************
    Конец методов работы с модемом
 *************************************************************************/


byte goSleep(byte iMode,long long iPrevTime) {
  unsigned long  vSleepTime   = 0, 
                 vPeriodSleep = 0,
                 vFirstLoop   = 0,
                 vSecondLoop  = 0;

           long  vNextModem   = 0;

  byte vWaitTime=0;                 

 
   {
    worker _worker(mWorkerStart);    
    vSleepTime   = _worker.getSleepTime();
   };
    
 

    /* connectPeriod() < getTimestamp() - iPrevTime */
    vNextModem   = (long)(iPrevTime + connectPeriod() - getTimestamp());
#ifdef IS_DEBUG_1
    Serial.print(F("vNextModem1:"));
    Serial.println(vNextModem);
#endif
    /* Если по каким-то причинам было пропущено несколько подключений, то принудительно подключаемся */
    vNextModem    = max(0,vNextModem); 

#ifdef IS_DEBUG_1

    Serial.print(F("vNextModem2:"));
    Serial.println(vNextModem);

    vSleepTime   = min(vNextModem, vSleepTime);
    Serial.print(F("Sleep time:"));
    Serial.println(vSleepTime);
    
    vSleepTime   = (unsigned long) vSleepTime * iMode / 100;
    Serial.print(F("Sleep time * "));
    Serial.print(iMode);
    Serial.print(F("%:"));
    Serial.println(vSleepTime);

    /****************************************************
     * Если перекатываемся через полночь,               *
     * то время следующего старта будет меньше,         *
     * чем текущее время.                               *
     ****************************************************/
    vPeriodSleep = (unsigned int)(vSleepTime / 8UL);        
    
    
    vFirstLoop   = (unsigned int)  vPeriodSleep / 37;
    vSecondLoop  = vPeriodSleep % 37;

    vWaitTime    = vSleepTime - vFirstLoop*37*8 - vSecondLoop * 8;


    Serial.print(F("Sleep period:"));
    Serial.println(vPeriodSleep);

    Serial.print(F("First Loop: "));
    Serial.println(vFirstLoop);

    Serial.print(F("Second Loop: "));
    Serial.println(vSecondLoop);

    Serial.print(F("vWaitTime: "));
    Serial.println(vWaitTime);
    Serial.flush();

#endif
        
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
   
 #ifdef IS_DEBUG
   Serial.print(F(". wait-2. "));
   Serial.println(getSecMidnight());
   Serial.flush();
 #endif 

 return vWaitTime; 

};

 

bool doPostParams(char* iRes, unsigned int iSize) {
  char vParams[70];
  bool vResult;
  Connection _connection;
  

  gprs2 sim900(7, 8);


  EEPROM.get(0, _connection);
  sim900.setInternetSettings(_connection.apnPoint, _connection.apnLogin, _connection.apnPass);


  if (!sim900.canInternet()) {
    char vError[20];
    sim900.getLastError(vError);
    #ifdef IS_DEBUG
    Serial.print(F("CInet error: "));
    Serial.println(vError);
    #endif
    return false;
  };

{
  char vL[11], vA[11];
    
  sim900.getCoords(vL, vA);
  sprintf_P(vParams, PSTR("r=%s&s=%s&m=c&l=%s&a=%s"), _connection.siteLogin, _connection.sitePass, vL, vA);
};


  vResult = sim900.postUrl(_connection.sitePoint, vParams, iRes, iSize);
#ifdef IS_DEBUG
  Serial.print(F("Params "));
#endif

  if (vResult) {
    /**************************************************************************
     *                                                                        *
       Если исполнители закончили свою работу и модуль передал информацию,
       то тогда обнуляем время работы.
     *                                                                        *
     **************************************************************************/
#ifdef IS_DEBUG
    Serial.print(F("status: "));
    Serial.println(iRes);
    Serial.flush();
#endif
  } else {
    char vError[20];
    sim900.getLastError(vError);

    #ifdef IS_DEBUG
      Serial.print(F("error: "));
      Serial.println(vError);
      Serial.flush();
    #endif

  };


  return vResult;

};

bool updateRemoteMeasure(int t1,  int h1, int t2, long p1) {
  char vParams[200],     
       vRes[35],
       sitePoint[40];
  bool vResult;  



  gprs2 sim900(7, 8);

  {
    Connection _connection;      
    EEPROM.get(0, _connection);
    sprintf_P(vParams, PSTR("r=%s&s=%s&t1=%d&h1=%d&t2=%d&p1=%ld&w1=%lu&l1=%lu&d=%lu"), _connection.siteLogin, _connection.sitePass, t1, h1, t2, p1, _water.duration, _light.duration, millis());
    sim900.setInternetSettings(_connection.apnPoint, _connection.apnLogin, _connection.apnPass);
    strncpy(sitePoint,_connection.sitePoint,40);
  };
  

  if (!sim900.canInternet()) {
    char vError[20];
    sim900.getLastError(vError);
    #ifdef IS_DEBUG
      Serial.print(F("CInet error: "));
      Serial.println(vError);
      Serial.flush();
    #endif
    return false;
  };

  vResult = sim900.postUrl(sitePoint, vParams, vRes, sizeof(vRes));

#ifdef IS_DEBUG
  Serial.print(F("Measurement "));
#endif

  if (vResult) {
    /**************************************************************************
     *                                                                        *
       Если исполнители закончили свою работу и модуль передал информацию,
       то тогда обнуляем время работы.
     *                                                                        *
     **************************************************************************/
#ifdef IS_DEBUG
    Serial.print(F("status: "));
    Serial.println(vRes);
    Serial.flush();
#endif


    if (!_water.isWork) {
      noInterrupts();
      _water.duration = 0;
      interrupts();
    };

    if (!_light.isWork) {
      noInterrupts();
      _light.duration = 0;
      interrupts();
    };


  } else {
    char vError[20];
    sim900.getLastError(vError);
#ifdef IS_DEBUG
    Serial.print(F("error: "));
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
  if (_mstr.numEntries(iStr, vDelimiter) > 1) {

    if (!_mstr.entry(1, iStr, vDelimiter, vCommand)) {
#ifdef IS_DEBUG
      Serial.println(F("E-N410"));
      Serial.flush();
#endif
      return false;
    };

    if (strcmp_P(vCommand, PSTR("C")) == 0) {
      if (_mstr.entry(2, iStr, vDelimiter, 4, vParam1)) {
#ifdef IS_DEBUG
        Serial.print(F("Sleep = "));
        Serial.println(atoi(vParam1));
        Serial.flush();
#endif
        setConnectPeriod(atoi(vParam1));
        return false;
      };
    };

    if (strcmp_P(vCommand, PSTR("O")) == 0) {
      if (_mstr.entry(2, iStr, vDelimiter, 4, vParam1) && _mstr.entry(3, iStr, vDelimiter, 4, vParam2)) {
#ifdef IS_DEBUG
        Serial.print(F("Set temp offline:"));
        Serial.print(vParam1);
        Serial.print(F("&"));
        Serial.println(vParam2);
        Serial.flush();
#endif
        setTempOffline(atoi(vParam1), atoi(vParam2));
        return false;
      };
    };

  };

  return true;
};

/**********************************************************
   Если требуется повторное соединение, то
   функция возвращает 1. Если подсоединения не
   требуется, то возвращается 0.
 **********************************************************/
byte workWithRes(char* iRes) {
  char vTmpStr[2];  
  mstr _mstr;

  worker _worker(mWorkerStart);
 _worker.setBeforeTaskUpdate(beforeTaskUpdate);
  
  

  strcpy_P(vTmpStr, PSTR("+"));
  if (_mstr.begins(iRes, vTmpStr)) {
    _mstr.trim(iRes, vTmpStr);
    return _worker.update(iRes);
  };

  return 0;

}

bool updateRemoteParams() {
  char vRes[250];
  byte vShouldReconnect = 1;


  while (vShouldReconnect == 1) {

    if (!doPostParams(vRes, sizeof(vRes))) {
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

  worker _worker(mWorkerStart);
  unsigned long secMidnight=0;
  bool isWaterShouldWork = false,
       isLightShouldWork = false;

  secMidnight=_worker.getSecMidnight();

    #ifdef IS_DEBUG
    Serial.print(F("Mem:"));
    Serial.print(freeMemory());
    Serial.print(F(" & "));
    Serial.println(secMidnight);
    Serial.flush();
  #endif

     
  isLightShouldWork = getCurrTempOut() <= getTempUpLight();
  isWaterShouldWork = getCurrTempIn()  >= getTempUpWater();

  for (byte vI = 0; vI < _worker.maxTaskCount; vI++) {    
    byte executor = _worker.shouldTaskWork2(vI, secMidnight);

    isLightShouldWork = isLightShouldWork || (bool)bitRead(executor, 1);
    isWaterShouldWork = isWaterShouldWork || (bool)bitRead(executor, 0);

  };

  

  if (!isLightShouldWork && _light.isWork) {
#ifdef IS_DEBUG
    Serial.print(F("EL:"));
    Serial.println(secMidnight);
    Serial.flush();
#endif
    _worker.stopLight();
    _light.isWork = false;    
  };

  if (!isWaterShouldWork && _water.isWork) {
#ifdef IS_DEBUG
    Serial.print(F("EW:"));
    Serial.println(secMidnight);
    Serial.flush();
#endif
    _worker.stopWater();
    _water.isWork = false;
  };

  if (isWaterShouldWork && !_water.isWork) {
#ifdef IS_DEBUG
    Serial.print(F("SW:"));
    Serial.println(secMidnight);
    Serial.flush();
#endif
    _worker.startWater();
    _water.isWork = true;
    _water.startTime = secMidnight;
  };




  if (isLightShouldWork && !_light.isWork) {
#ifdef IS_DEBUG
    Serial.print(F("SL:"));
    Serial.println(secMidnight);
    Serial.flush();
#endif
    _worker.startLight();
    _light.isWork = true;
    _light.startTime = secMidnight;
  };



  /**************************************************
     Если исполнитель работает, то увеличиваем
     время его работы.
   **************************************************/

  if (_water.isWork) {
    /************************************************
       Если время включения больше, чем текущее время
       это означает, что либо мы перешагнули через
       полуночь либо перевелись часы. Соответсвенно устанавливаем
       время включения в 0. Исходим из того, что считаем время работы
       исполнителя в текущих сутках.
    */
    if (_water.startTime > secMidnight) {
      _water.startTime = 0;
    };
    _water.duration = secMidnight - _water.startTime;
  };

  if (_light.isWork) {
    /************************************************
       Если время включения больше, чем текущее время
       это означает, что либо мы перешагнули через
       полуночь либо перевелись часы. Соответсвенно устанавливаем
       время включения в 0. Исходим из того, что считаем время работы
       исполнителя в текущих сутках.
    */
    if (_light.startTime > secMidnight) {
      _light.startTime = 0;
    };
    _light.duration = secMidnight - _light.startTime;
  };

  mCanGoSleep = !(_light.isWork || _water.isWork);
}


void setup() {

  #ifdef IS_DEBUG_1
  Serial.begin(19200);
#endif  

    

  Connection _connection;
  Globals _globals;

  worker _worker(mWorkerStart);
 



  pinMode(4, OUTPUT);
  pinMode(13, OUTPUT);
  digitalWrite(13, LOW);    // turn the LED off by making the voltage LOW

  _worker.stopWater();
  _worker.stopLight();
  



  _water.isWork = false;
  _light.isWork = false;

  EEPROM.get(0, _connection);
  EEPROM.get(sizeof(Connection), _globals);


  if (strcmp_P(_globals.version, PSTR("INI")) != 0) {
  //  if (true) {
#ifdef IS_DEBUG
    Serial.println(F("SET"));
#endif

    for (int vI = 0 ; vI < EEPROM.length() ; vI++) {
      EEPROM.write(vI, 0);
    };

    //Признак того, что инициализация выполнена
    strcpy_P(_globals.version, PSTR("INI"));

    // Site POINT
    strcpy_P(_connection.sitePoint, PSTR("xn--24-6kcq7d.xn--p1ai/ri/sa"));

    // Site LOGIN
    strcpy_P(_connection.siteLogin, PSTR("guest"));

    // Site PASS
    strcpy_P(_connection.sitePass, PSTR("guest"));


    strcpy_P(_connection.apnPoint, PSTR("\0"));
    strcpy_P(_connection.apnLogin, PSTR("\0"));
    strcpy_P(_connection.apnPass, PSTR("\0"));

    // Задержка соединения по-умолчанию 15 минут
    _globals.connectPeriod = 15;

    _worker.setDateTime(16, 9, 13, 18, 45, 0);



    // Сохраняю настройки
    noInterrupts();
    EEPROM.put(0, _connection);
    EEPROM.put(sizeof(Connection), _globals);
    interrupts();

    setTempOffline(-99, 99);
#ifdef IS_DEBUG
    Serial.println(F("END SET"));
#endif
  };

  Timer1.initialize(3000000);
  Timer1.attachInterrupt(Timer1_doJob);
  Timer1.start();

 /*
  * Отладка ошибки 23.09.28
  * 
  */
    vPrevTime2 = 66900;
  _worker.setDateTime(18, 9, 23, 23, 33, 0);
  isFirstRun = false;
 
#ifdef WDT_ENABLE
  wdt_enable (WDTO_8S);
#endif

}

void showLong(long long iVal) {

 char buffer[25];
 sprintf(buffer, "%0ld", iVal/1000000L);
 Serial.print(buffer);  
 sprintf(buffer, "%0ld", iVal%1000000L);
 Serial.println(buffer); 

}

void loop() 
{
  bool isModemWork = false;   



   delay(10000); //Обязательно оставить, иначе слишком быстро дергаются часы и из-за этого через некоторое время сбрасываются
  long long vCurrTime=getTimestamp();
  long vD = (long)(vCurrTime-vPrevTime2);
   #ifdef IS_DEBUG_1
  Serial.print(F("vCurrTime:"));
  showLong(vCurrTime);
  Serial.print(F("vPrevTime2:"));
  showLong(vPrevTime2);

  Serial.print(F("vD:"));
  Serial.println(vD);

  #endif
  
  if (vD >= connectPeriod() || isFirstRun) {

digitalWrite(13, HIGH);    // turn the LED off by making the voltage LOW
  
   #ifdef IS_DEBUG_1
    Serial.println(F("***"));
    showDateTime();
    Serial.println();    
    Serial.flush();
   #endif

    isModemWork = wk();
   

if (isModemWork) {
 byte vAttempt = 0;
 bool vStatus  = false;
 readSms();

       do {
         vStatus = updateRemoteParams();
         
         if (!vStatus && vAttempt==1) {
          restartModem();
         };
         
         vAttempt++;
      } while (!vStatus && vAttempt < 3);
} else {
  Serial.println(F("*Modem not work"));
  Serial.flush();
}


if (isModemWork) {
  long p1;
  int t1, t2, h1;
  byte vAttempt = 0;
  bool vStatus  = false;
  loadSensorInfo1(&t1, &h1, &t2, &p1);

  #ifdef IS_DEBUG
    Serial.print(F("T_out:"));
    Serial.println(t1);

    Serial.print(F("T_in:"));
    Serial.println(t2);

    Serial.print(F("H:"));
    Serial.println(h1);

    Serial.print(F("P:"));
    Serial.println(p1);


    Serial.print(F("W&L:"));
    Serial.print(_water.duration);
    Serial.print(F("&"));
    Serial.println(_light.duration);


    Serial.print(F("Off.mode: "));
    Serial.print(getTempUpLight());
    Serial.print(F(" & "));
    Serial.println(getTempUpWater());

    Serial.print(F("Con.per.: "));
    Serial.println(connectPeriod());
    
    Serial.flush();
  #endif
 

       do {
        vStatus = updateRemoteMeasure(t1, h1, t2, p1);

        if (!vStatus && vAttempt==1) {
          restartModem();
         };
         
        vAttempt++;
      } while (!vStatus && vAttempt < 3);

} else {
  Serial.println(F("*Modem not work"));
  Serial.flush();;
}
     
    vPrevTime2 = getTimestamp();
    sl();  //После того как модем передал данные уводим его в сон. Если это делать в основном теле функции, то отправлять будем 1 раз в секунду
  
digitalWrite(13, LOW);
};

 
  isFirstRun = false;

   {
   bool vCanGoSleep;
   noInterrupts();
   vCanGoSleep=mCanGoSleep;
   interrupts();

   if (!vCanGoSleep) {
     return;
   };
 };

 Timer1.stop();
 goSleep(50,vPrevTime2);
 goSleep(75,vPrevTime2);
 goSleep(90,vPrevTime2);
/**************************************************
  * Гарантировано даем включиться исполнителям.
  * 
  **************************************************/  
  {
    byte vDelay = goSleep(100,vPrevTime2);
    Timer1.start();
    delay(vDelay * 1000 + 2000);
  };

  
 
}
