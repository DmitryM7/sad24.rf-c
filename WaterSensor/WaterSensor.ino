#include <avr/wdt.h>
#include <debug.h>
#include <worker.h>
#include <mstr.h>
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
         
volatile int NbTopsFan; //measuring the rising edges of the signal
volatile unsigned long int mPrevFlow = 0;
volatile bool mCanEnablePump=true;
volatile unsigned long int mWirePumpOn=0,mWirePumpOff=0;
bool isWater = false;  
unsigned long int mLastWireOffSend=0,mLastWireOnnSend=0;

workerInfo _water;
workerInfo _light;



#define HAS_ULTRASONIC_SENSOR 0
#define LEVEL_WATER_TOP 75        // Уровень при котором нужно включить насос для откачки. Другими словами, если вода ближе, чем указанная здесь цфира, то включаем насосо.
#define LEVEL_WATER_MIDDLE 175    // Уровень при котором хорошо работать. Медианное значение воды к которому автоматика будет стремиться привести состояние воды.
#define LEVEL_WATER_BOTTOM 250    // Уровень при котором нельзя вклчюать насосо.


#define ECHO_PIN   10           // ECHO  pin УЗ-сенсора
#define TRIG_PIN   3            // TRIG  pin УЗ-сенсора
#define TRIG_DELAY 40           // DELAY задержка перед опроса сигнала
  
#define WIRE_SERVER 1
#define WIRE_ADDRESS 7
#define WIRE_REPEAT_TIMEOUT 3000
#define WORD_LENGTH 30

#if WIRE_SERVER>0
  #define CRITICAL_PREASURE 2.9        // Критическое давление при котором устройсво отключится без учета  потока.
  #define MAX_PREASURE 2.15          // Давление при котором устройство отключится если нет потока
  #define MIN_PREASURE 2.10          // В случае если давление в системе упадет меньше указанного, то устройство вклчючит насос
  #define FLOW_DIFF 40              // Время в течение которого устройство опеделяет отсутствие потока.
#else
  #define CRITICAL_PREASURE 3.5        // Критическое давление при котором устройсво отключится без учета  потока.
  #define MAX_PREASURE 2.2          // Давление при котором устройство отключится если нет потока
  #define MIN_PREASURE 1.9          // В случае если давление в системе упадет меньше указанного, то устройство вклчючит насос
  #define FLOW_DIFF 50              // Время в течение которого устройство опеделяет отсутствие потока.
#endif

worker _worker(0);    

 
void rpm ()     //This is the function that the interupt calls 
{ 
  //NbTopsFan++;  //This function measures the rising and falling edge of the hall effect sensors signal
  mPrevFlow = millis();
} 
// The setup() method runs once, when the sketch starts

int getDistance() {
    int duration, cm;     
    digitalWrite(TRIG_PIN, LOW); 
    delayMicroseconds(2); 
    digitalWrite(TRIG_PIN, HIGH); 
    delayMicroseconds(TRIG_DELAY); 
    digitalWrite(TRIG_PIN, LOW); 
    duration = pulseIn(ECHO_PIN, HIGH); 
    cm = duration / 58;
    return cm;
}

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

  float mCurrPreasure, 
        mDistance     = getDistance();
   unsigned long  mCurrFlowTimeDiff=getPrevFlowDiff();

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
  * Нет потока больше заданного интервала - отключаем воду.    *
  *************************************************************/

 if (mCurrPreasure >= MAX_PREASURE && (mCurrFlowTimeDiff = getPrevFlowDiff()) >= FLOW_DIFF && isWater) {

  #ifdef IS_DEBUG
    Serial.print(F("Max preasure: ")); Serial.print(mCurrPreasure); Serial.print(F(" and no flow: ")); Serial.print(mCurrFlowTimeDiff); Serial.println(F(" - power off."));  Serial.flush();
   #endif
    _worker.stopWater();

    #if WIRE_SERVER==0
    if (millis() - mLastWireOffSend > WIRE_REPEAT_TIMEOUT) {
      char wWord[WORD_LENGTH];
      sprintf_P(wWord, PSTR("OFF|p:%lu|f:%lu|nf"),(unsigned long)(mCurrPreasure*100),mCurrFlowTimeDiff);
      Wire.beginTransmission(WIRE_ADDRESS);
      Wire.write(wWord);
      Wire.endTransmission();  
      mLastWireOffSend=millis();
      #ifdef IS_DEBUG
        Serial.print(F("Wire:")); Serial.println(wWord); Serial.flush();
      #endif
    }
    #endif
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

   #if WIRE_SERVER==0
    if (millis() - mLastWireOffSend > WIRE_REPEAT_TIMEOUT) {
      char wWord[WORD_LENGTH];
      sprintf_P(wWord, PSTR("OFF|p:%lu|f:%lu|cp"),(unsigned long)(mCurrPreasure*100),999999);
      Wire.beginTransmission(WIRE_ADDRESS);
      Wire.write(wWord);
      Wire.endTransmission();  

      mLastWireOffSend=millis();
      
      #ifdef IS_DEBUG
        Serial.print(F("Wire:")); Serial.println(wWord); Serial.flush();
      #endif
    }
    #endif


   _worker.stopWater();
    isWater = false;
    mCanEnablePump=false;
  };

  /**************************************************   
   * Проверка №4.                                  *
   * Давление упало. При этом насос не работает.    *
   * Включаем его.                                  *
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

  /**************************************************************
   * Если у ведомого включена вода, то сообщаем об этом мастеру 
   **************************************************************/

   #if WIRE_SERVER==0
    if (isWater) {
      char wWord[WORD_LENGTH];
      sprintf_P(wWord, PSTR("ONN|p:%lu|f:%lu"),(unsigned long)(mCurrPreasure*100),mCurrFlowTimeDiff);    
      Wire.beginTransmission(WIRE_ADDRESS);
      Wire.write(wWord);
      Wire.endTransmission();  
      mLastWireOnnSend=millis();
      #ifdef IS_DEBUG
        Serial.print(F("Wire:")); Serial.println(wWord); Serial.flush();
      #endif   
    };
   #endif

  wdt_reset();

}

void receiveEvent(int howMany) {
 char vAnswer[WORD_LENGTH];
 char vNeedStr[4];
 byte vCount=0;
 mstr _mstr;

 _mstr.emptyBuffer(vAnswer,sizeof(vAnswer));

 
 while (Wire.available()) {
  char c = Wire.read();
     if (vCount<sizeof(vAnswer)) {
        vAnswer[vCount]=c;
        vCount++;
     };
 };

 #if IS_DEBUG>0
   Serial.print(F("Wire rec: ")); Serial.println(vAnswer); Serial.flush();
 #endif

 strcpy_P(vNeedStr, PSTR("ONN"));

 if (_mstr.indexOf(vAnswer,vNeedStr)>-1) {
   mWirePumpOn=millis();
 } else {
   mWirePumpOff=millis();
 };
}
