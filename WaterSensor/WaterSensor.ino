#include <avr/wdt.h>
#include <debug.h>
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


#define CRITICAL_PREASURE 3.5   // Критическое давление при котором устройсво отключится без учета  потока.
#define MAX_PREASURE 2.5        // Давление при котором устройство отключится если нет потока
#define MIN_PREASURE 2.1        // В случае если давление в системе упадет меньше указанного, то устройство вклчючит насос
#define FLOW_DIFF 50         // Время в течение которого устройство опеделяет отсутствие потока.

 
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
  return ret < 0 ? 0 : ret;  
}

unsigned long int getPrevFlowDiff() {
  return millis() - mPrevFlow;
}

void confPump() {
  pinMode(11, OUTPUT);
  digitalWrite(11,LOW);
}
void disablePump() {  
  digitalWrite(11,LOW);
}

void enablePump() {
  digitalWrite(11,HIGH);
}


void setup() 
{ 
  #ifdef IS_DEBUG
    Serial.begin(19200); 
    Serial.println(F("*** Start device ***")); Serial.flush();
  #endif
  
  pinMode(3, INPUT); 
  attachInterrupt(1, rpm, RISING); 
  
  pinMode(A0, INPUT);

  confPump();

  wdt_enable (WDTO_8S);
  
} 
void loop ()    
{

  float mCurrPreasure = getCurrPreasure(), mCurrFlowTimeDiff;

  #ifdef IS_DEBUG
      //Serial.print(mCurrPreasure); Serial.print(F(" and flow ")); Serial.print(getPrevFlowDiff()); Serial.println(); Serial.flush();
  #endif 
 
 /**************************************************************
  * Проверка №1.                                              *
  * Нет потока больше заданного интервала - отключаем воду.    *
  *************************************************************/

 if ((mCurrPreasure = getCurrPreasure()) >= MAX_PREASURE && (mCurrFlowTimeDiff = getPrevFlowDiff()) >= FLOW_DIFF && isWater) {

  #ifdef IS_DEBUG
    Serial.print(F("Max preasure: ")); Serial.print(mCurrPreasure); Serial.print(F(" and no flow: ")); Serial.print(mCurrFlowTimeDiff); Serial.println(F(" - power off."));  Serial.flush();
   #endif
    disablePump();
    isWater = false;
  };

 /********************************************************
  * Проверка №2.                                        *
  * Достигли критического значения по давлению.          *
  * Отключаем насос вне зависимости от других параметров *
  ********************************************************/
  if ((mCurrPreasure = getCurrPreasure()) >= CRITICAL_PREASURE) {

   #ifdef IS_DEBUG
     Serial.print(F("!!! Critical preasure: ")); Serial.print(mCurrPreasure); Serial.print(F("- power off !!!")); Serial.print(F("FLOW: ")); Serial.println(getPrevFlowDiff()); Serial.flush();
   #endif

    disablePump();
    isWater = false;
  };

  /**************************************************   * Проверка №3.                                  *
   * Давление упало. При этом насос не работает.    *
   * Включаем его.                                  *
   **************************************************/

  if ((mCurrPreasure = getCurrPreasure()) < MIN_PREASURE && !isWater) {

   #ifdef IS_DEBUG
    Serial.print(F("Min preasure: ")); Serial.print(mCurrPreasure); Serial.println(F(" - power on.")); Serial.flush();
   #endif

    enablePump();
    isWater = true;
  };

 wdt_reset();

}
