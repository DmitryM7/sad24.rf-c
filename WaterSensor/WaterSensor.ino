#include <avr/wdt.h>
#include <debug.h>
#include <worker.h>
#include <mstr.h>
#include <stdSensorInfoLoader.h>
/************************************************************************************************
 * Пример кода для посчета количества протекующей жидкости                                      *
 * int Calc;                               
 * NbTopsFan = 0;   //Set NbTops to 0 ready for calculations                                    *
 * 
 * sei();      //Enables interrupts                                                              *
 * delay (1000);   //Wait 1 second                                                               *
 * cli();      //Disable interrupts                                                              *
 * Calc = (NbTopsFan * 60 / 5.5); //(Pulse frequency x 60) / 5.5Q, = flow rate in L/hour         *
 * Serial.print (Calc, DEC); //Prints the number calculated above                                
 * *
 * Serial.print (" L/hour\r\n"); //Prints "L/hour" and returns a  new line                       *
 ************************************************************************************************/
         
volatile unsigned      int NbTopsFan; //measuring the rising edges of the signal
volatile unsigned long int mPrevFlow = 0,
                           mWirePumpOn=0,
                           mWirePumpOff=0;;
volatile bool              mCanEnablePump=true,
                           isWater = false;

workerInfo _water;
workerInfo _light;
stdSensorInfoLoader mStdSensorInfo;



#define HAS_ULTRASONIC_SENSOR 0
#define LEVEL_WATER_TOP 75        // Уровень при котором нужно включить насос для откачки. Другими словами, если вода ближе, чем указанная здесь цфира, то включаем насосо.
#define LEVEL_WATER_MIDDLE 175    // Уровень при котором хорошо работать. Медианное значение воды к которому автоматика будет стремиться привести состояние воды.
#define LEVEL_WATER_BOTTOM 250    // Уровень при котором нельзя вклчюать насосо.

  

#define CRITICAL_PREASURE 2.7        // Критическое давление при котором устройсво отключится без учета  потока.
#define MAX_PREASURE 2.6          // Давление при котором устройство отключится если нет потока
#define MIN_PREASURE 2.6           // В случае если давление в системе упадет меньше указанного, то устройство вклчючит насос
#define FLOW_DIFF 15            // Время в течение которого устройство опеделяет отсутствие потока.

worker _worker(0);    

 
void rpm ()     //This is the function that the interupt calls 
{ 
  //NbTopsFan++;  //This function measures the rising and falling edge of the hall effect sensors signal
  mPrevFlow = millis();
} 
// The setup() method runs once, when the sketch starts


float getCurrPreasure() {
  unsigned int v;
  float ret;
  v  = analogRead(A0);

  ret = 2 * (float)v * 5 / 1024;

  //Округляем до 100тых
  ret=floor(ret * 100) / 100;
  
  return ret < 0 ? 0 : ret;  
}

unsigned long int getPrevFlowDiff() {
  return millis() - mPrevFlow;
}


void setup() 
{ 
  #ifdef IS_DEBUG
    Serial.begin(19200); 
    Serial.println(F("*** Start device ***")); Serial.flush();
  #endif

  pinMode(LED_BUILTIN, OUTPUT);
  
  pinMode(3, INPUT); 
  attachInterrupt(1, rpm, RISING); 
  
  pinMode(A0, INPUT);
  _worker.stopWater();
  _worker.stopLight();

  

  #if WIRE_SERVER==0
    Wire.begin();
  #else
    Wire.begin(WIRE_ADDRESS);
    Wire.onReceive(receiveEvent);
  #endif

  pinMode(LED_BUILTIN, OUTPUT);

  wdt_enable (WDTO_8S);
  
} 
void loop ()    
{
  float mCurrPreasure;

  mStdSensorInfo.loadSensorInfo(millis());

  
  unsigned long  mCurrFlowTimeDiff = getPrevFlowDiff();
  unsigned int mDistance           = mStdSensorInfo.getDistance();

  digitalWrite(LED_BUILTIN,HIGH);
  delay(500);
  digitalWrite(LED_BUILTIN,LOW);

  mCurrPreasure = getCurrPreasure();

  #if IS_DEBUG>1
      Serial.print(mCurrPreasure); Serial.print(F(" and flow ")); Serial.print(mCurrFlowTimeDiff); Serial.println(); Serial.flush();
  #endif 
  
 #if WIRE_SERVER>0                  

    if (!isWater) {
        //Если давление позволяет, то включаем насос
        if (mCurrPreasure  < (CRITICAL_PREASURE - 0.02)) { //Как временное решение делаю поправку на 0.02, т.к. датчик скачет в этом диапазоне  

          if(mWirePumpOn > mWirePumpOff && millis() - mWirePumpOn < (WIRE_REPEAT_TIMEOUT + 1000)) {
            #if IS_DEBUG>0
              Serial.println(F("ONN by WIRE")); Serial.flush();
            #endif
           _worker.startWater();
          }; 
       };

       //Если получили сигнал на отключение, то отключаем насос, или не получили сигнал на включение. Видимо ведомый погиб.
       if(mWirePumpOff > mWirePumpOn || millis() - mWirePumpOn > (WIRE_REPEAT_TIMEOUT + 1000)) {
        #if IS_DEBUG>0
          Serial.println(F("OFF by WIRE")); Serial.flush();
        #endif
        _worker.stopWater();
      };            
    };

 #endif

  
  #if HAS_ULTRASONIC_SENSOR>0

   if (mDistance>LEVEL_WATER_BOTTOM) {
    #ifdef IS_DEBUG
     Serial.println(F("Lower water level. Pump disable.")); Serial.flush();
    #endif
    mCanEnablePump=false
   } else {
    mCanEnablePump=true;
   };

  #endif

 
 /**************************************************************
  * Проверка №2.                                              *
  * Нет потока больше заданного интервала при  достижении нужного давления, то отключаем воду.    *
  *************************************************************/

 if (mCurrPreasure >= MAX_PREASURE && (mCurrFlowTimeDiff = getPrevFlowDiff()) >= FLOW_DIFF && isWater) {

  #ifdef IS_DEBUG
    Serial.print(F("Max preasure: ")); Serial.print(mCurrPreasure); Serial.print(F(" and no flow: ")); Serial.print(mCurrFlowTimeDiff); Serial.println(F(" - power off."));  Serial.flush();
   #endif
    _worker.stopWater();
    
    isWater = false;
  };

 /********************************************************
  * Проверка №3.                                        *
  * Достигли критического значения по давлению.          *
  * Отключаем насос вне зависимости от других параметров *
  ********************************************************/
  if (mCurrPreasure >= CRITICAL_PREASURE) {

   #ifdef IS_DEBUG
     Serial.print(F("!!! Critical preasure: ")); Serial.print(mCurrPreasure); Serial.print(F("- power off !!!")); Serial.print(F("FLOW: ")); Serial.println(getPrevFlowDiff()); Serial.flush();
   #endif   


   _worker.stopWater();
    isWater = false;
    mCanEnablePump=false;
  };

  /**************************************************   
   * Проверка №4.                                  *
   * Давление упало. При этом насос не работает.    *
   * Включаем его.  ! Нужно сделать защиту от открыва шланга.                                *
   **************************************************/

  if ((mCurrPreasure = getCurrPreasure()) < MIN_PREASURE) {

    if (!isWater) {
      #ifdef IS_DEBUG
        Serial.print(F("Min preasure: ")); Serial.print(mCurrPreasure); Serial.println(F(" - power on.")); Serial.flush();
      #endif   
       _worker.startWater();
       isWater = true;
    };
   
  };

  wdt_reset();

}
