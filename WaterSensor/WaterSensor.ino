#include <avr/wdt.h>
#include <debug.h>
#include <worker.h>
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
bool isWater = false;  

workerInfo _water;
workerInfo _light;


#define CRITICAL_PREASURE 3       // Критическое давление при котором устройсво отключится без учета  потока.
#define MAX_PREASURE 2.5          // Давление при котором устройство отключится если нет потока
#define MIN_PREASURE 2.1          // В случае если давление в системе упадет меньше указанного, то устройство вклчючит насос
#define FLOW_DIFF 50              // Время в течение которого устройство опеделяет отсутствие потока.

#define HAS_ULTRASONIC_SENSOR 0
#define LEVEL_WATER_TOP 75        // Уровень при котором нужно включить насос для откачки. Другими словами, если вода ближе, чем указанная здесь цфира, то включаем насосо.
#define LEVEL_WATER_MIDDLE 175    // Уровень при котором хорошо работать. Медианное значение воды к которому автоматика будет стремиться привести состояние воды.
#define LEVEL_WATER_BOTTOM 250    // Уровень при котором нельзя вклчюать насосо.


#define ECHO_PIN   10           // ECHO  pin УЗ-сенсора
#define TRIG_PIN   3            // TRIG  pin УЗ-сенсора
#define TRIG_DELAY 50           // DELAY задержка перед опроса сигнала

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

  wdt_enable (WDTO_8S);
  
} 
void loop ()    
{

  float mCurrPreasure = getCurrPreasure(), 
        mDistance     = getDistance(),
        mCurrFlowTimeDiff;

  #if IS_DEBUG>1
      //Serial.print(mCurrPreasure); Serial.print(F(" and flow ")); Serial.print(getPrevFlowDiff()); Serial.println(); Serial.flush();
  #endif 

 /******************************************************************
  * Проверка №1.                                                  *
  * Проверяем, что уровень допустим для работы насоса. Если вода   *
  * ушла дальше, то не разрешаем включать насос. При этом включаем *
  * запрещающий инидкатор.                                         *
  ******************************************************************/

  #if HAS_ULTRASONIC_SENSOR>0

   if (mDistance>LEVEL_WATER_BOTTOM) {
      _worker.stopWater();
      _worker.startLight();
      isWater=false;
      return;
    };

    _worker.stopLight(); // Отключаем запрешающий индикатор.
  #endif
 
 /**************************************************************
  * Проверка №2.                                              *
  * Нет потока больше заданного интервала - отключаем воду.    *
  *************************************************************/

 if ((mCurrPreasure = getCurrPreasure()) >= MAX_PREASURE && (mCurrFlowTimeDiff = getPrevFlowDiff()) >= FLOW_DIFF && isWater) {

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
  if ((mCurrPreasure = getCurrPreasure()) >= CRITICAL_PREASURE) {

   #ifdef IS_DEBUG
     Serial.print(F("!!! Critical preasure: ")); Serial.print(mCurrPreasure); Serial.print(F("- power off !!!")); Serial.print(F("FLOW: ")); Serial.println(getPrevFlowDiff()); Serial.flush();
   #endif

    _worker.stopWater();
    isWater = false;
  };

  /**************************************************   
   * Проверка №4.                                  *
   * Давление упало. При этом насос не работает.    *
   * Включаем его.                                  *
   **************************************************/

  if ((mCurrPreasure = getCurrPreasure()) < MIN_PREASURE && !isWater) {

   #ifdef IS_DEBUG
    Serial.print(F("Min preasure: ")); Serial.print(mCurrPreasure); Serial.println(F(" - power on.")); Serial.flush();
   #endif

    _worker.startWater();
    isWater = true;
  };

  wdt_reset();

}
