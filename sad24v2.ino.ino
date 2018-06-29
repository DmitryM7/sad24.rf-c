#include <avr/wdt.h>
#include <BMP085.h>
#include <DHT.h>
#include <gprs2.h>
#include <mstr.h>
#include <worker.h>
#include <EEPROM.h>
#include <TimerOne.h>

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
  int  connectPeriod;
};

struct offlineParams {
  int  tempUpLight;
  int  tempUpWater;
};


unsigned int mOfflineParamsStart = sizeof(Connection) + sizeof(Globals);
unsigned int mWorkerStart = mOfflineParamsStart + sizeof(offlineParams);

int mCurrTempOut = 19900;
int mCurrTempIn  = -2000;

workerInfo _water;
workerInfo _light;


void loadSensorInfo1(int *oT1, int *oH1, int *oT2, long *oP1) {
  float    h1 = -100, t1 = 200;
  long int p1 = -100, vCurrTime;
  int32_t  t2 = -100;

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

  vCurrTime = millis();

  while ((isnan(h1) || isnan(t1)) && millis() - vCurrTime < 5000) {
    delay(1000);
    h1 = dht.readHumidity();
    t1 = dht.readTemperature();
    wdt_reset();
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

void showDateTime() {
  worker _worker(mWorkerStart);
  _worker.showDateTime();
}

void parseThreeParamCommand(char* iCommand) {
  char vTmpStr[10],
       vCmd[10];
  mstr _mstr;
  Connection _connection;
  strcpy_P(vTmpStr, PSTR(":"));
  Serial.println(F("Three param"));
  _mstr.entry(1, iCommand, vTmpStr, 10, vCmd);

  EEPROM.get(0, _connection);

  /**********************************
    Меняем учетные данные к apn
   **********************************/
  strcpy_P(vTmpStr, PSTR("APN7"));
  if (_mstr.isEqual(vCmd, vTmpStr)) {
    Serial.print(F("APN: "));

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

    Serial.print(_connection.apnLogin);
    Serial.print(F(":"));
    Serial.print(_connection.apnPass);
    Serial.print(F("@"));
    Serial.println(_connection.apnPoint);

    noInterrupts();
    EEPROM.put(0, _connection);
    interrupts();

    Serial.println(F("-"));
  };

  strcpy_P(vTmpStr, PSTR("SITE"));
  if (_mstr.isEqual(vCmd, vTmpStr)) {
    Serial.print(F("SITE: "));
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

    Serial.print(_connection.siteLogin);
    Serial.print(F(":"));
    Serial.print(_connection.sitePass);
    Serial.print(F("@"));
    Serial.println(_connection.sitePoint);

    noInterrupts();
    EEPROM.put(0, _connection);
    interrupts();

    Serial.println(F("-"));
  };
  
}

bool onSms(byte iSms, char* iCommand) {
  mstr _mstr;
  char vTmpStr[2];
  Serial.print(iSms);
  Serial.print(F(">"));
  Serial.println(iCommand);

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

  sim900.isReady();

  sim900.setOnSms(onSms);

  sim900.readSms(true);

}


bool setSleepTime(int iSleepTime) {
  Globals _globals;

  /**************************************************
   * Проверяю на разумность переданных значений.    *
   **************************************************/
  if (iSleepTime < 5 || iSleepTime > 600) {
    return false;
  };

  EEPROM.get(sizeof(Connection), _globals);
  _globals.sleepTime = iSleepTime;

  noInterrupts();
  EEPROM.put(sizeof(Connection), _globals);
  interrupts();


};
int  getSleepTime() {
  Globals _globals;
  EEPROM.get(sizeof(Connection), _globals);
  return _globals.sleepTime;
}



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



unsigned long int connectPeriod() {
  Globals _globals;
  EEPROM.get(sizeof(Connection), _globals);
  return _globals.connectPeriod * 60000;
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

bool doPostParams(char* iRes, unsigned int iSize) {
  char vL[11], vA[11], vParams[75], vError[20];
  bool vResult;

  Connection _connection;

  gprs2 sim900(7, 8);

  EEPROM.get(0, _connection);

  sim900.setInternetSettings(_connection.apnPoint, _connection.apnLogin, _connection.apnPass);

  sim900.getCoords(vL, vA);

  sprintf_P(vParams, PSTR("r=%s&s=%s&m=c&l=%s&a=%s"), _connection.siteLogin, _connection.sitePass, vL, vA);
  vResult = sim900.postUrl(_connection.sitePoint, vParams, iRes, iSize);
  Serial.print(F("Params "));

  if (vResult) {
    /**************************************************************************
     *                                                                        *
       Если исполнители закончили свою работу и модуль передал информацию,
       то тогда обнуляем время работы.
     *                                                                        *
     **************************************************************************/
    Serial.print(F("status: "));
    Serial.println(iRes);

  } else {
    sim900.getLastError(vError);
    Serial.print(F("error: "));
    Serial.println(vError);

  };

  return vResult;

};


bool updateRemoteMeasure(int t1,  int h1, int t2, long p1) {
  char vParams[200],
       vError[20],
       vRes[45];

  bool vResult;
  Connection _connection;

  gprs2 sim900(7, 8);

  EEPROM.get(0, _connection);


  sprintf_P(vParams, PSTR("r=%s&s=%s&t1=%d&h1=%d&t2=%d&p1=%ld&w1=%lu&l1=%lu&d=%lu"), _connection.siteLogin, _connection.sitePass, t1, h1, t2, p1, _water.duration, _light.duration, millis());

  sim900.setInternetSettings(_connection.apnPoint, _connection.apnLogin, _connection.apnPass);

  vResult = sim900.postUrl(_connection.sitePoint, vParams, vRes, sizeof(vRes));

  Serial.print(F("Measurement "));

  if (vResult) {
    /**************************************************************************
     *                                                                        *
       Если исполнители закончили свою работу и модуль передал информацию,
       то тогда обнуляем время работы.
     *                                                                        *
     **************************************************************************/
    Serial.print(F("status: "));
    Serial.println(vRes);

    
    if (!_water.isWork) {
      _water.duration = 0;
    };

    if (!_light.isWork) {
      _light.duration = 0;
    };
    

  } else {
    sim900.getLastError(vError);
    Serial.print(F("error: "));
    Serial.println(vError);

  };

  return vResult;
}



bool beforeTaskUpdate(char* iStr) {
  char vDelimiter = ';',
       vCommand[2],
       vParam1[20],
       vParam2[20];;
  mstr _mstr;



  if (_mstr.numEntries(iStr, vDelimiter) >= 2) {
    _mstr.entry(1, iStr, vDelimiter, vCommand);

    if (strcmp_P(vCommand, PSTR("C")) == 0) {
      if (_mstr.entry(2, iStr, vDelimiter, 4, vParam1)) {
        Serial.print(F("Sleep = "));
        Serial.println(atoi(vParam1));
        setConnectPeriod(atoi(vParam1));
        return false;
      };
    };

    if (strcmp_P(vCommand, PSTR("O")) == 0) {
      if (_mstr.entry(2, iStr, vDelimiter, 4, vParam1) && _mstr.entry(3, iStr, vDelimiter, 4, vParam2)) {
        Serial.print(F("Set temp offline:"));
        Serial.print(vParam1);
        Serial.print(F("&"));
        Serial.println(vParam2);
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

  worker _worker(mWorkerStart);
  _worker.setBeforeTaskUpdate(beforeTaskUpdate);
  mstr _mstr;

  strcpy_P(vTmpStr, PSTR("+"));
  if (_mstr.begins(iRes, vTmpStr)) {
    _mstr.trim(iRes, vTmpStr);
    return _worker.update(iRes);
  };

  return 0;

}

bool updateRemoteParams() {
  char vRes[150];
  byte vShouldReconnect = 1;

  while (vShouldReconnect == 1) {

    if (!doPostParams(vRes, sizeof(vRes))) {
      return false;
    };

    vShouldReconnect = workWithRes(vRes);
  };
  return true;

}



void doJob() {
  worker _worker(mWorkerStart);

  long secMidnight;
  byte currDayOfWeek;  
  byte executor;
  bool isWaterShouldWork = false, 
       isLightShouldWork = false;


  secMidnight   = _worker.getSecMidnight();
  currDayOfWeek = _worker.getDayOfWeek();

  isLightShouldWork = getCurrTempOut() <= getTempUpLight();
  isWaterShouldWork = getCurrTempIn()  >= getTempUpWater();  


  for (unsigned int vI = 0; vI < _worker.maxTaskCount; vI++) {
    executor = _worker.shouldTaskWork2(vI, secMidnight, currDayOfWeek);

    isLightShouldWork = isLightShouldWork || (bool)bitRead(executor,1);
    isWaterShouldWork = isWaterShouldWork || (bool)bitRead(executor,0);    

  };

  if (!isLightShouldWork && _light.isWork) {
    Serial.print(F("EL:"));
    Serial.println(secMidnight);
    _worker.stopLight();
    _light.isWork = false;
  };

  if (!isWaterShouldWork && _water.isWork) {
    Serial.print(F("EW:"));
    Serial.println(secMidnight);
    _worker.stopWater();
    _water.isWork = false;
  };

  if (isWaterShouldWork && !_water.isWork) {
    Serial.print(F("SW:"));
    Serial.println(secMidnight);
    _worker.startWater();
    _water.isWork = true;
    _water.startTime = secMidnight;
  };




  if (isLightShouldWork && !_light.isWork) {
    Serial.print(F("SL:"));
    Serial.println(secMidnight);
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
}

void Timer1_doJob(void) {
  wdt_reset();
  doJob();
}

void restartModem() {
  gprs2 sim900(7, 8);
  sim900.softRestart();
  delay(60000);
};



void setup() {

  Connection _connection;
  Globals _globals;

  worker _worker(mWorkerStart);

  


  pinMode(13, OUTPUT);
  digitalWrite(13, LOW);    // turn the LED off by making the voltage LOW

  _worker.stopWater();
  _worker.stopLight();

  Serial.begin(19200);
  Serial.println(F("Start"));
  Wire.begin();


  _water.isWork = false;
  _light.isWork = false;

  EEPROM.get(0, _connection);
  EEPROM.get(sizeof(Connection), _globals);


  if (strcmp_P(_globals.version, PSTR("INI")) != 0) {
  //  if (true) {
    Serial.println(F("SET"));

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

    // Задержка соединения по-умолчанию 15 минут
    _globals.connectPeriod = 15;

    _worker.setDateTime(17, 5, 9, 11, 0, 0);



    // Сохраняю настройки
    noInterrupts();
    EEPROM.put(0, _connection);
    EEPROM.put(sizeof(Connection), _globals);
    interrupts();

    setTempOffline(-99, 99);
    Serial.println(F("END SET"));
  };


  Timer1.initialize(3000000);
  Timer1.attachInterrupt(Timer1_doJob);

  wdt_enable (WDTO_8S);

}

void loop() {

  long p1;
  int t1, t2, h1;

  static bool          isFirstRun = true;
  static unsigned long vPrevTime1 = 0,
                       vPrevTime2 = 0;
  static int           vSim900ErrorCount = 0;


  unsigned long vCurrTime;


  vCurrTime = millis();


  /**************************************
      Проверяем СМС с паролем к сайту
   **************************************/

  if (vCurrTime - vPrevTime2 >= 60000 || isFirstRun) {
    digitalWrite(13, HIGH);    // turn the LED off by making the voltage LOW
    readSms();
    digitalWrite(13, LOW);    // turn the LED off by making the voltage LOW
    vPrevTime2 = millis();
  };

  /***************************************
     Выполняем основную работу.
   ***************************************/

  if (vCurrTime - vPrevTime1 >= connectPeriod() || isFirstRun) {                       

    //Timer1.stop();

    Serial.print(F("-Start: "));
    showDateTime();
    Serial.println(F(" -"));
    loadSensorInfo1(&t1, &h1, &t2, &p1);
    //Timer1.start();

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

    Serial.println(F("-Stop-"));
    Serial.flush();

    updateRemoteParams();

    if (updateRemoteMeasure(t1, h1, t2, p1)) {
      vSim900ErrorCount = 0;
    } else {
      vSim900ErrorCount += 1;
    };


    /***************************************************
     * Если при попытке отправить данные               *
     * возникло более 3х ошибок, то перезагружаем      *
     *  модем.                                         *
     ***************************************************/
    if (vSim900ErrorCount > 2) {
      restartModem();
      vSim900ErrorCount = 0;
    };


    vPrevTime1 = millis();

  };


  isFirstRun = false;
  delay(5000);
}
