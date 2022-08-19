#include <avr/wdt.h>
#include <stdSensorInfoLoader.h>
#include <EEPROM.h>
#include <TimerOne.h>
#include <LowPower.h>
#include <MemoryFree.h>
#include <gsmTransport.h>
#include <worker.h>
#include <debug.h>
#include <structs.h>





long long mCurrTime, mPrevTime2;

volatile bool mCanGoSleep   = true;              


stdSensorInfoLoader _sensorInfo;
gsmTransport        _stdTransport;

workerInfo _water;
workerInfo _light;


void(* resetFunc) (void) = 0;



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
  bool vRes;

  vRes=mCanGoSleep && isDisabledLightRange() && isDisabledWaterRange() && _stdTransport.getConnectPeriod() >= 900;
  
  return vRes; 
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

  
  {
     vNextModem   = (long)(iPrevTime + _stdTransport.getConnectPeriod() - vTimeStamp);
  }

  
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

  if (!isDisabledLightRange() && (_sensorInfo.getT1() < _offlineParams.tempUpLight1 || _light.isEdge)) {
    _light.isEdge     = true;
  };



  /******************************************************************
     Если достигли мимального значения по внутренней температуре,
     то включаем устройство "Вода". При этом отмечаем включение
     по температуре. Делаю эту тему по заказу Юрца Маленького.
   ******************************************************************/
  if (!isDisabledWaterRange() && (_sensorInfo.getT2() < _offlineParams.tempUpWater1 || _water.isEdge)) {
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
  if (_sensorInfo.getT1() >= _offlineParams.tempUpLight2 || isDisabledLightRange()) {  
    _light.isEdge     = false;
  };


  if (_sensorInfo.getT2() >= _offlineParams.tempUpWater2 || isDisabledWaterRange()) { 
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

  _stdTransport.externalKeyPress(mCurrTime);
  resetFunc();
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


  if (_globals.version!=21) {
  
    
    //Признак того, что инициализация выполнена
    _globals.version = 21;
    EEPROM.put(eeprom_mGlobalsStart, _globals);

    ;    

    //Устанавливаем умолчательные значения для автономного режима     
    _setOffline(__WATER__,-199, 199);
    _setOffline(__LIGHT__,-199, 199);

   _stdTransport.clearConfig();
         /**************************************************************************
          *   Инициализруем переменную. Если этого не сделать,                     *
          *   то возможно переполнение при первом старке.                          *
          *   Дополнительно экономим                                               *
          *   память по глобальной переменной отображающей факт первого запуска.   *
          **************************************************************************/       
    
    
    _worker.setDateTime(16, 9, 13, 18, 45, 0); //Устанавливаем часы в эпоху Дарьи

  };
  
  mPrevTime2 = _stdTransport.calcFirstConnectPeriod(_worker.getTimestamp());             
  
  
  _sensorInfo.loadSensorInfo(_worker.getTimestamp()); 

  Timer1.initialize(7000000);
  Timer1.attachInterrupt(Timer1_doJob);
  Timer1.start();

  #ifdef WDT_ENABLE
    wdt_enable (WDTO_8S);
  #endif

  attachInterrupt(0, pin_ISR, FALLING);  

}


void loop()
{
  
   long int vTimeAfterLastMeasure = (long) (mCurrTime - _sensorInfo.getLastMeasure()); // mCurrTime берем из прерывания по будильнику

     

   /*****************************************************************
    * Замеряем пар-ры окружающей среды в заданный интервал времени  *
    *****************************************************************/
   if (vTimeAfterLastMeasure > __MEASURE_PERIOD__) {

       Timer1.stop(); //Отключаем таймер, так как в функции loadSensorInfo есть критичный участок кода
       _sensorInfo.loadSensorInfo(mCurrTime);
       Timer1.start();     
            
   };

   /*****************************************
    * Обмениваемся информацией              *
    *****************************************/

     _stdTransport.checkCommunicationSession();
   
     mPrevTime2 = _stdTransport.makeCommunicationSession(mCurrTime,
                                                         mPrevTime2,
                                                         _sensorInfo,
                                                         _water,
                                                         _light); 
   
 
   if (!canGoSleep()) {
      return;
   };


  Timer1.stop();

   {
      byte waitTime=0;
 
       do {
          waitTime=goSleep(50, mPrevTime2);
       } while (waitTime==0);
 
       Timer1.start();
       delay(waitTime * 1000 + 2000);
 
   };
}
